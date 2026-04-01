#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "logic.h"
#include "misc.h"

#define PEDANTIC_MAX_CREATURES 128

enum ActorState {
  CS_FDIRMASK = 0xf,
  CS_SLIDETOKEN = 0x10,
  CS_REVERSE = 0x20,
  CS_PUSHED = 0x40,
  CS_TELEPORTED = 0x80
};

enum CollisionCheckFlags {
  CMM_RELEASING = 0x0001,
  CMM_CLEARANIMATIONS = 0x0002,
  CMM_STARTMOVEMENT = 0x0004,
  CMM_PUSHBLOCKS = 0x0008,
  CMM_PUSHBLOCKSNOW = 0x0010,
};

static inline TileID Level_get_terrain(Level const* self, Position pos) {
  return self->map[pos].top.id;
}
static inline void Level_set_terrain(Level* self, Position pos, TileID tile) {
  self->map[pos].top.id = tile;
}

enum TerrainState {
  /**
   * Is there a non-Chip, non-animation actor on this cell?
   */
  FS_CLAIMED = 0x40,
  /**
   * Is there an animation on this cell?
   */
  FS_ANIMATED = 0x20,
  /**
   * Was there ever a trap on this cell?
   * Not equivalent to checking if TileID is a trap since pedantic recessed
   * walls can overwrite terrain
   */
  FS_HAD_TRAP = 0x01,
  /**
   * Was there ever a teleport on this cell?
   * Not equivalent to checking if TileID is a teleport since pedantic recessed
   * walls can overwrite terrain
   */
  FS_HAD_TELEPORT = 0x02,
};

static inline void Level_cell_add_claim(Level* self, Position pos) {
  self->map[pos].top.state |= FS_CLAIMED;
}
static inline void Level_cell_remove_claim(Level* self, Position pos) {
  self->map[pos].top.state &= ~FS_CLAIMED;
}
static inline bool Level_cell_has_claim(Level const* self, Position pos) {
  return self->map[pos].top.state & FS_CLAIMED;
}
static inline void Level_cell_add_animation(Level* self, Position pos) {
  self->map[pos].top.state |= FS_ANIMATED;
}
static inline void Level_cell_remove_animation(Level* self, Position pos) {
  self->map[pos].top.state &= ~FS_ANIMATED;
}
static inline bool Level_cell_has_animation(Level const* self, Position pos) {
  return self->map[pos].top.state & FS_ANIMATED;
}
static inline void Level_cell_add_trap_presence(Level* self, Position pos) {
  self->map[pos].top.state |= FS_HAD_TRAP;
}
static inline bool Level_cell_ever_had_trap(Level const* self, Position pos) {
  return self->map[pos].top.state & FS_HAD_TRAP;
}
static inline void Level_cell_add_teleport_presence(Level* self, Position pos) {
  self->map[pos].top.state |= FS_HAD_TELEPORT;
}
static inline bool Level_cell_ever_had_teleport(Level const* self,
                                                Position pos) {
  return self->map[pos].top.state & FS_HAD_TELEPORT;
}
static inline Actor* Level_get_chip(Level* self) {
  return &self->actors[0];
}
static inline Actor* Level_get_last_actor(Level* self) {
  return &self->actors[self->actors_n - 1];
}
static inline bool Level_in_endgame(Level const* self) {
  return self->lx_state.endgame_timer > 0;
}
static void Level_start_endgame(Level* self) {
  self->lx_state.endgame_timer = 13;
  self->timer_offset = 1;
}
static inline bool Actor_is_moving(Actor const* self) {
  return self->move_cooldown > 0;
}
static uint8_t Level_lynx_rng(Level* self) {
  uint8_t n = (self->lx_state.prng1 >> 2) - self->lx_state.prng1;
  if (!(self->lx_state.prng1 & 0x02))
    n -= 1;
  self->lx_state.prng1 =
      (self->lx_state.prng1 >> 1) | (self->lx_state.prng2 & 0x80);
  self->lx_state.prng2 = (self->lx_state.prng2 << 1) | (n & 0x01);
  return self->lx_state.prng1 ^ self->lx_state.prng2;
}

static void Level_stop_terrain_sfx(Level* level) {
  Level_stop_sfx(level, SND_SKATING_FORWARD);
  Level_stop_sfx(level, SND_SKATING_TURN);
  Level_stop_sfx(level, SND_FIREWALKING);
  Level_stop_sfx(level, SND_WATERWALKING);
  Level_stop_sfx(level, SND_ICEWALKING);
  Level_stop_sfx(level, SND_SLIDEWALKING);
  Level_stop_sfx(level, SND_SLIDING);
}

static bool lynx_init_level(Level* self) {
  memset(self->actors, 0, sizeof(Actor) * MAX_CREATURES);
  self->actors_n = 0;
  Actor* chip = NULL;
  if (self->lx_state.pedantic_mode && self->status_flags & SF_BAD_TILES) {
    self->status_flags |= SF_INVALID;
  }
  for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
    MapCell* cell = &self->map[pos];
    // Convert MS tiles into Lynx-comptabile subtitutes
    if (cell->top.id == TILE_BLOCK_STATIC) {
      cell->top.id = TileID_actor_with_dir(ACTOR_BLOCK, DIRECTION_NORTH);
    }
    if (cell->bottom.id == TILE_BLOCK_STATIC) {
      cell->bottom.id = TileID_actor_with_dir(ACTOR_BLOCK, DIRECTION_NORTH);
    }
    if (TileID_is_ms_special(cell->top.id)) {
      cell->top.id = TILE_WALL;
      if (self->lx_state.pedantic_mode) {
        self->status_flags |= SF_INVALID;
      }
    }
    if (TileID_is_ms_special(cell->bottom.id)) {
      cell->bottom.id = TILE_WALL;
      if (self->lx_state.pedantic_mode) {
        self->status_flags |= SF_INVALID;
      }
    }
    // Detect MS-style buried tiles
    if (cell->bottom.id != TILE_FLOOR && (!TileID_is_terrain(cell->bottom.id) ||
                                     TileID_is_terrain(cell->top.id))) {
      // warn("level %d: invalid \"buried\" tile at (%d %d)",
      //      num, pos % CXGRID, pos / CXGRID);
      self->status_flags |= SF_INVALID;
    }
    // Create actors
    if (TileID_is_actor(cell->top.id)) {
      Actor* actor = &self->actors[self->actors_n];
      self->actors_n += 1;
      actor->pos = pos;
      actor->id = TileID_actor_get_id(cell->top.id);
      actor->direction = TileID_actor_get_dir(cell->top.id);
      if (self->lx_state.pedantic_mode && actor->id == ACTOR_BLOCK &&
          TileID_is_ice(cell->bottom.id)) {
        actor->direction = DIRECTION_NIL;
      }
      if (actor->id == ACTOR_CHIP) {
        if (chip) {
          // warn("level %d: multiple Chips on the map!", num);
          self->status_flags |= SF_INVALID;
        }
        chip = actor;
        actor->direction = DIRECTION_SOUTH;
      } else {
        Level_cell_add_claim(self, actor->pos);
      }
      cell->top.id = cell->bottom.id;
      cell->bottom.id = TILE_FLOOR;
    }
    // These tiles don't exist in Lynx Lynx, so they are technically invalid
    if (self->lx_state.pedantic_mode &&
        (cell->top.id == TILE_THIN_WALL_NORTH || cell->top.id == TILE_THIN_WALL_WEST)) {
      self->status_flags |= SF_INVALID;
    }
    if (cell->top.id == TILE_TRAP) {
      Level_cell_add_trap_presence(self, pos);
    }
    if (cell->top.id == TILE_TELEPORT) {
      Level_cell_add_teleport_presence(self, pos);
    }
  }
  if (!chip) {
    // warn("level %d: Chip isn't on the map!", num);
    self->status_flags |= SF_INVALID;
    chip = &self->actors[self->actors_n];
    self->actors_n += 1;
    chip->pos = 0;
    chip->hidden = true;
  }
  // Swap Chip to be the first actor
  if (chip) {
    Actor* first_actor = &self->actors[0];
    Actor temp;
    temp = *first_actor;
    *first_actor = *chip;
    *chip = temp;
    chip = first_actor;
  }
  // TODO: Validate trap and cloner lists
  memset(self->player_boots, 0, sizeof(self->player_boots));
  memset(self->player_keys, 0, sizeof(self->player_keys));
  self->lx_state = (LxState){
      .pedantic_mode = self->lx_state.pedantic_mode,
      .chip_stuck = self->lx_state.pedantic_mode &&
                    chip->pos != POSITION_NULL &&
                    TileID_is_ice(Level_get_terrain(self, chip->pos)),
      .chip_predicted_pos = POSITION_NULL,
      .to_place_wall_pos = POSITION_NULL
  };

  return !(self->status_flags & SF_INVALID);
}

static void Actor_remove(Actor* self, Level* level, TileID animation_type) {
  if (self->id != ACTOR_CHIP) {
    Level_cell_remove_claim(level, self->pos);
  }
  if (self->state & CS_PUSHED) {
    Level_stop_sfx(level, SND_BLOCK_MOVING);
  }
  self->id = animation_type;
  self->animation_frame =
      ((level->current_tick + level->init_step_parity) & 1) ? 12 : 11;
  self->animation_frame -= 1;
  self->hidden = false;
  self->state = 0;
  self->move_decision = DIRECTION_NIL;
  // If this actor just started moving, put it back in the cell it came from
  if (self->move_cooldown == 8) {
    self->pos = Position_neighbor(self->pos, Direction_back(self->direction));
    self->move_cooldown = 0;
  }
  Level_cell_add_animation(level, self->pos);
}

static void Level_remove_chip(Level* self, ChipStatus reason, Actor* also) {
  Actor* chip = Level_get_chip(self);
  switch (reason) {
    case CHIP_DROWNED:
      Level_add_sfx(self, SND_WATER_SPLASH);
      Actor_remove(chip, self, ANIM_WATER);
      break;
    case CHIP_BOMBED:
      Level_add_sfx(self, SND_BOMB_EXPLODES);
      Actor_remove(chip, self, ANIM_BOMB);
      break;
    case CHIP_OUTOFTIME:
      Actor_remove(chip, self, ANIM_ENTITY);
      break;
    case CHIP_BURNED:
      Level_add_sfx(self, SND_CHIP_LOSES);
      Actor_remove(chip, self, ANIM_ENTITY);
      break;
    case CHIP_COLLIDED:
      Level_add_sfx(self, SND_CHIP_LOSES);
      Actor_remove(chip, self, ANIM_ENTITY);
      if (also && also != chip) {
        Actor_remove(also, self, ANIM_ENTITY);
      }
      break;
  }
  Level_stop_terrain_sfx(self);
  Level_start_endgame(self);
}

static void Actor_erase_animation(Actor* self, Level* level) {
  self->hidden = true;
  Level_cell_remove_animation(level, self->pos);
  if (self == Level_get_last_actor(level)) {
    self->id = TILE_NOTHING;
    level->actors_n -= 1;
  }
}

static void Actor_set_forced_move(Actor* self, Direction dir) {
  self->state &= ~CS_FDIRMASK;
  self->state |= dir;
}

static Direction Actor_get_forced_move(Actor const* self) {
  return self->state & CS_FDIRMASK;
}

static Direction Slide_get_forced_direction(TileID self,
                                            Level* level,
                                            bool advance_rff) {
  switch (self) {
    case TILE_FORCE_FLOOR_NORTH:
      return DIRECTION_NORTH;
    case TILE_FORCE_FLOOR_WEST:
      return DIRECTION_WEST;
    case TILE_FORCE_FLOOR_SOUTH:
      return DIRECTION_SOUTH;
    case TILE_FORCE_FLOOR_EAST:
      return DIRECTION_EAST;
    case TILE_FORCE_FLOOR_RANDOM:
      if (advance_rff) {
        level->rff_dir = Direction_right(level->rff_dir);
      }
      return level->rff_dir;
    default:
      return DIRECTION_NIL;
  }
}

static Direction get_ice_wall_turn_dir(TileID floor, Direction dir) {
  switch (floor) {
    case TILE_ICE_CORNER_NORTH_EAST:
      return dir == DIRECTION_SOUTH  ? DIRECTION_EAST
             : dir == DIRECTION_WEST ? DIRECTION_NORTH
                                     : dir;
    case TILE_ICE_CORNER_SOUTH_WEST:
      return dir == DIRECTION_NORTH  ? DIRECTION_WEST
             : dir == DIRECTION_EAST ? DIRECTION_SOUTH
                                     : dir;
    case TILE_ICE_CORNER_NORTH_WEST:
      return dir == DIRECTION_SOUTH  ? DIRECTION_WEST
             : dir == DIRECTION_EAST ? DIRECTION_NORTH
                                     : dir;
    case TILE_ICE_CORNER_SOUTH_EAST:
      return dir == DIRECTION_NORTH  ? DIRECTION_EAST
             : dir == DIRECTION_WEST ? DIRECTION_SOUTH
                                     : dir;
    default:
      return dir;
  }
}

static Direction Actor_calculate_forced_move(Actor* self, Level* level, bool advance_rff) {
  if (level->current_tick == 0)
    return DIRECTION_NIL;
  TileID terrain = Level_get_terrain(level, self->pos);
  if (TileID_is_ice(terrain)) {
    if (self->id == ACTOR_CHIP &&
        (Level_player_has_item(level, TILE_BOOTS_ICE) || level->lx_state.chip_stuck))
      return DIRECTION_NIL;
    if (self->direction == DIRECTION_NIL)
      return DIRECTION_NIL;
    return self->direction;
  } else if (TileID_is_force_floor(terrain)) {
    if (self->id == ACTOR_CHIP && Level_player_has_item(level, TILE_BOOTS_FORCE_FLOOR))
      return DIRECTION_NIL;
    // FF overrides are now handled separately
    return Slide_get_forced_direction(terrain, level, advance_rff);
  } else if (self->state & CS_TELEPORTED) {
    self->state &= ~CS_TELEPORTED;
    return self->direction;
  }
  return DIRECTION_NIL;
}

static Direction TileID_get_exit_impeding_directions(TileID self) {
  switch (self) {
    case TILE_THIN_WALL_NORTH:
      return DIRECTION_NORTH;
    case TILE_THIN_WALL_WEST:
      return DIRECTION_WEST;
    case TILE_THIN_WALL_SOUTH:
      return DIRECTION_SOUTH;
    case TILE_THIN_WALL_EAST:
      return DIRECTION_EAST;
    case TILE_THIN_WALL_SOUTH_EAST:
      return DIRECTION_SOUTH | DIRECTION_EAST;
    // NOTE: Haha gotcha: contrary to what one might assume IceWall_ direction means the directions that *don't* have walls.
    // So, IceWall_Northwest actually has walls on the south and east edges of its tile. Great!
    case TILE_ICE_CORNER_NORTH_WEST:
      return DIRECTION_SOUTH | DIRECTION_EAST;
    case TILE_ICE_CORNER_NORTH_EAST:
      return DIRECTION_SOUTH | DIRECTION_WEST;
    case TILE_ICE_CORNER_SOUTH_WEST:
      return DIRECTION_NORTH | DIRECTION_EAST;
    case TILE_ICE_CORNER_SOUTH_EAST:
      return DIRECTION_NORTH | DIRECTION_WEST;
    default:
      return DIRECTION_NIL;
  };
}

static bool TileID_impedes_actor(TileID self,
                                 Level const* level,
                                 Actor const* actor,
                                 Direction dir) {
  switch (self) {
    case TILE_WALL:
    case TILE_INVISIBLE_WALL:
    case TILE_TOGGLE_DOOR_CLOSED:
    case TILE_CLONE_MACHINE:
    case TILE_BLOCK_STATIC:
    case TILE_DROWNED_CHIP:
    case TILE_BURNED_CHIP:
    case TILE_EXITED_CHIP:
    case TILE_EXIT_ANIM_1:
    case TILE_EXIT_ANIM_2:
    case TILE_OVERLAY_BUFFER:
    case TILE_UNUSED_1:
    case TILE_UNUSED_2:
    case TILE_ICE_BLOCK:
      return true;
    case TILE_GRAVEL:
      return actor->id != ACTOR_CHIP && actor->id != ACTOR_BLOCK;
    case TILE_DIRT:
    case TILE_THIEF:
    case TILE_HINT:
    case TILE_HIDDEN_WALL:
    case TILE_BLUE_WALL_REAL:
    case TILE_BLUE_WALL_FAKE:
    case TILE_POPUP_WALL:
    case TILE_EXIT:
    case TILE_IC_CHIP:
    case TILE_KEY_YELLOW:
    case TILE_KEY_GREEN:
    case TILE_BOOTS_FORCE_FLOOR:
    case TILE_BOOTS_ICE:
    case TILE_BOOTS_WATER:
    case TILE_BOOTS_FIRE:
      return actor->id != ACTOR_CHIP;
    case TILE_SOCKET:
      return actor->id != ACTOR_CHIP || level->chips_left > 0;
    case TILE_DOOR_RED:
    case TILE_DOOR_BLUE:
    case TILE_DOOR_GREEN:
    case TILE_DOOR_YELLOW:
      return actor->id != ACTOR_CHIP || !Level_player_has_item(level, self);
    case TILE_FIRE:
      return actor->id != ACTOR_CHIP && actor->id != ACTOR_BLOCK && actor->id != ACTOR_FIREBALL;
    case TILE_ICE_CORNER_NORTH_WEST:
    case TILE_THIN_WALL_SOUTH_EAST:
      return dir & (DIRECTION_NORTH | DIRECTION_WEST);
    case TILE_ICE_CORNER_NORTH_EAST:
      return dir & (DIRECTION_NORTH | DIRECTION_EAST);
    case TILE_ICE_CORNER_SOUTH_WEST:
      return dir & (DIRECTION_SOUTH | DIRECTION_WEST);
    case TILE_ICE_CORNER_SOUTH_EAST:
      return dir & (DIRECTION_SOUTH | DIRECTION_EAST);
    case TILE_THIN_WALL_NORTH:
      return dir == DIRECTION_SOUTH;
    case TILE_THIN_WALL_EAST:
      return dir == DIRECTION_WEST;
    case TILE_THIN_WALL_SOUTH:
      return dir == DIRECTION_NORTH;
    case TILE_THIN_WALL_WEST:
      return dir == DIRECTION_EAST;
    default:
      return false;
  }
}

enum FindActorFlags {
  FA_NO_CHIP = 0x01,
  FA_ANIMS = 0x02,
};

static Actor* Level_find_actor(Level* self, Position pos, uint8_t flags) {
  uint32_t i = 0;
  if (flags & FA_NO_CHIP) {
    i += 1;
  }
  for (; i < self->actors_n; i++) {
    Actor* actor = &self->actors[i];
    if (actor->pos == pos && !actor->hidden &&
        ((bool)(flags & FA_ANIMS) == TileID_is_animation(actor->id))) {
      return actor;
    }
  }
  return NULL;
}

static bool Actor_can_be_pushed(Actor* self,
                                Level* level,
                                Direction dir,
                                uint8_t flags);

static bool Actor_check_collision(Actor const* self,
                                  Level* level,
                                  Direction dir,
                                  uint8_t flags) {
  assert(self != NULL);
  assert(dir != DIRECTION_NIL);
  if (self->move_cooldown)
    return false;
  // Exit collision check
  TileID this_terrain = Level_get_terrain(level, self->pos);
  Direction exit_dirs_blocked =
      TileID_get_exit_impeding_directions(this_terrain);
  if (exit_dirs_blocked & dir)
    return false;
  if ((this_terrain == TILE_TRAP || this_terrain == TILE_CLONE_MACHINE) &&
      !(flags & CMM_RELEASING))
    return false;
  // Can't go backwards on force floors
  if (TileID_is_force_floor(this_terrain) &&
      !(self->id == ACTOR_CHIP && Level_player_has_item(level, TILE_BOOTS_FORCE_FLOOR)) &&
      Slide_get_forced_direction(this_terrain, level, false) ==
          Direction_back(dir))
    return false;
  int8_t x = self->pos % MAP_WIDTH;
  int8_t y = self->pos / MAP_HEIGHT;
  // Can't just use Position_neighbor since that'd wrap when x is 31 and we're
  // going right
  y += dir == DIRECTION_NORTH ? -1 : dir == DIRECTION_SOUTH ? +1 : 0;
  x += dir == DIRECTION_WEST ? -1 : dir == DIRECTION_EAST ? +1 : 0;
  if (x < 0 || x >= MAP_WIDTH)
    return false;
  if (y < 0 || y >= MAP_HEIGHT) {
    if (level->lx_state.pedantic_mode && (flags & CMM_STARTMOVEMENT)) {
      level->lx_state.map_breached = true;
      // warn("map breach in pedantic mode at (%d %d)", x, y);
    }
    return false;
  }
  Position target_pos = x + y * MAP_WIDTH;
  // Check terrain
  TileID new_terrain = Level_get_terrain(level, target_pos);
  if (new_terrain == TILE_TOGGLE_DOOR_CLOSED || new_terrain == TILE_TOGGLE_DOOR_OPEN) {
    new_terrain ^= level->lx_state.toggle_walls_xor;
  }
  if (TileID_impedes_actor(new_terrain, level, self, dir))
    return false;
  // Check actor
  if (Level_cell_has_animation(level, target_pos)) {
    if (self->id == ACTOR_CHIP)
      return false;
    if (flags & CMM_CLEARANIMATIONS) {
      Actor* anim = Level_find_actor(level, target_pos, FA_ANIMS);
      Actor_erase_animation(anim, level);
    }
  }
  if (Level_cell_has_claim(level, target_pos)) {
    if (self->id != ACTOR_CHIP)
      return false;
    Actor* other = Level_find_actor(level, target_pos, FA_NO_CHIP);
    if (other && other->id == ACTOR_BLOCK) {
      if (!Actor_can_be_pushed(other, level, dir, flags & ~CMM_RELEASING))
        return false;
    }
  }
  // These tiles turn into real walls, but these checks have to happen after the
  // `TileID` and push checks we want blocks to be able to be pushed off these tiles
  if (self->id == ACTOR_CHIP && (new_terrain == TILE_HIDDEN_WALL || new_terrain == TILE_BLUE_WALL_FAKE)) {
    if (flags & CMM_STARTMOVEMENT) {
      level->map[target_pos].top.id = TILE_WALL;
    }
    return false;
  }
  return true;
}

static TriRes Actor_start_moving_to(Actor* self, Level* level, bool releasing) {
  assert(!Actor_is_moving(self));
  Direction move_dir;
  if (self->move_decision) {
    move_dir = self->move_decision;
  } else if (Actor_get_forced_move(self)) {
    move_dir = Actor_get_forced_move(self);
  } else {
    return TRIRES_NOTHING;
  }
  assert(!Direction_is_diagonal(move_dir));
  self->direction = move_dir;

  TileID from_terrain = Level_get_terrain(level, self->pos);

  if (self->id == ACTOR_CHIP && !Level_player_has_item(level, TILE_BOOTS_FORCE_FLOOR)) {
    if (TileID_is_force_floor(from_terrain) && self->move_decision == DIRECTION_NIL) {
      self->state |= CS_SLIDETOKEN;
    } else if (!TileID_is_ice(from_terrain) ||
               Level_player_has_item(level, TILE_BOOTS_ICE)) {
      self->state &= ~CS_SLIDETOKEN;
    }
  }
  if (!Actor_check_collision(self, level, move_dir,
                             CMM_PUSHBLOCKSNOW | CMM_CLEARANIMATIONS |
                                 CMM_STARTMOVEMENT |
                                 (releasing ? CMM_RELEASING : 0))) {
    // Show player bonks and play the SFX if we haven't bonk already
    if (self->id == ACTOR_CHIP) {
      if (!level->lx_state.chip_bonked) {
        level->lx_state.chip_bonked = true;
        Level_add_sfx(level, SND_CANT_MOVE);
      }
      level->lx_state.chip_pushing = true;
    }
    // If we bonked while on ice, turn around
    if (TileID_is_ice(from_terrain) &&
        !(self->id == ACTOR_CHIP && Level_player_has_item(level, TILE_BOOTS_ICE))) {
      self->direction =
          get_ice_wall_turn_dir(from_terrain, Direction_back(self->direction));
    }
    return TRIRES_NOTHING;
  }

  if (level->lx_state.map_breached && (Level_get_chip(level)->id == ACTOR_CHIP)) {
    Level_remove_chip(level, CHIP_COLLIDED, self);
    return TRIRES_DIED;
  }
  assert(releasing ||
         !(from_terrain == TILE_CLONE_MACHINE || from_terrain == TILE_TRAP));

  if (self->id != ACTOR_CHIP) {
    // Remove the claim on the location we're about to leav
    Level_cell_remove_claim(level, self->pos);
    // NOTE: If it looks like Chip will *try* to move into out cell (and we're
    // about to leave), mark ourselves as the actor Chip is colliding with,
    // which we will use when Chip will eventually try to move, see a few lines
    // down
    if (self->id != ACTOR_BLOCK && self->pos == level->lx_state.chip_predicted_pos) {
      level->lx_state.chip_colliding_actor = self;
    }
  }
  // NOTE: When there's apparently a monster that just left the cell we're
  // trying to enter, kill outselves as if we collided into them
  if (self->id == ACTOR_CHIP && level->lx_state.chip_colliding_actor &&
      !level->lx_state.chip_colliding_actor->hidden) {
    // ???
    level->lx_state.chip_colliding_actor->move_cooldown = 8;
    Level_remove_chip(level, CHIP_COLLIDED,
                      level->lx_state.chip_colliding_actor);
    return TRIRES_DIED;
  }
  self->pos = Position_neighbor(self->pos, move_dir);
  assert(0 <= self->pos && self->pos < MAP_WIDTH * MAP_HEIGHT);
  // Why `+=` instead of `=`? Can cooldown be negative?
  self->move_cooldown += 8;

  if (self->id != ACTOR_CHIP) {
    // Add the claim for the new location we're in
    Level_cell_add_claim(level, self->pos);
    // If we're now at Chip's cell, kill 'em
    Actor* chip = Level_get_chip(level);
    if (self->pos == chip->pos && !chip->hidden) {
      Level_remove_chip(level, CHIP_COLLIDED, self);
      return TRIRES_DIED;
    }
  } else {
    level->lx_state.chip_bonked = false;
    // If we entered an actor's cell, kill ourselves
    Actor* monster = Level_find_actor(level, self->pos, FA_NO_CHIP);
    if (monster) {
      Level_remove_chip(level, CHIP_COLLIDED, monster);
      return TRIRES_DIED;
    }
  }

  // If *any* block is pushed, make Chip show the pushing animation
  if (self->state & CS_PUSHED) {
    level->lx_state.chip_pushing = true;
    Level_add_sfx(level, SND_BLOCK_MOVING);
  }
  return TRIRES_SUCCESS;
}

static Position Level_find_connected_cell(Level const* self,
                                          Position from_pos,
                                          TileID target_id,
                                          ConnList* list) {
  // In pedantic mode, connections can only be in reading order, so search
  // manually instead of relying on the list
  if (self->lx_state.pedantic_mode) {
    for (uint16_t offset = 1; offset < MAP_WIDTH * MAP_HEIGHT; offset += 1) {
      Position searched_pos = (from_pos + offset) % (MAP_WIDTH * MAP_HEIGHT);
      TileID terrain = Level_get_terrain(self, searched_pos);
      if (terrain == target_id) {
        return searched_pos;
      }
    }
    return POSITION_NULL;
  }
  // In the usual mode, scan the conn list
  for (size_t idx = 0; idx < list->length; idx += 1) {
    TileConn conn = list->items[idx];
    if (conn.from == from_pos)
      return conn.to;
  }
  return POSITION_NULL;
}

static TriRes Actor_advance_movement(Actor* self, Level* level, bool releasing);

static Actor* Actor_new(Level* level) {
  Actor* actor = &level->actors[0]; // Set intentionally in the event there's no creatures beyond Chip
  // The +1 after the loop will then set it to be the next after him

  for (uint32_t i = 1; i < level->actors_n; i++) {
    actor = &level->actors[i];
    if (actor->hidden)
      return actor;
  }
  actor += 1;

  size_t actors_used = actor - level->actors;
  if (actors_used >= MAX_CREATURES) {
    warn("Ran out of room in the creatures array!");
    return NULL;
  }
  if (level->lx_state.pedantic_mode && actors_used >= PEDANTIC_MAX_CREATURES)
    return NULL;
  actor->hidden = true;
  level->actors_n += 1;
  return actor;
}

static bool Level_activate_cloner(Level* self, Position pos) {
  assert(pos != POSITION_NULL);
  if (pos >= MAP_WIDTH * MAP_HEIGHT) {
    warn("Off-map cloning attempted: (%d %d)", pos % MAP_WIDTH,
         pos / MAP_WIDTH);
    return false;
  }
  if (Level_get_terrain(self, pos) != TILE_CLONE_MACHINE) {
    warn("Red button not connected to a clone machine at (%d %d)",
         pos % MAP_WIDTH, pos / MAP_WIDTH);
    return false;
  }
  Actor* actor = Level_find_actor(self, pos, 0);
  if (!actor)
    return false;
  Actor* clone = Actor_new(self);

  // This can only happen if we ran out of actors. Whoops?
  if (!clone)
    return Actor_advance_movement(actor, self, true) != TRIRES_NOTHING;

  // Actually clone the actor
  *clone = *actor;

  if (Actor_advance_movement(actor, self, true) != TRIRES_SUCCESS) {
    // We failed to exit, don't actually show the clone, then
    clone->hidden = true;
    return false;
  }
  return true;
}

static void Level_turn_tanks(Level* self) {
  for (uint32_t i = 1; i < self->actors_n; i++) {
    Actor* actor = &self->actors[i];
    if (actor->hidden || actor->id != ACTOR_TANK)
      continue;
    TileID terrain = Level_get_terrain(self, actor->pos);
    if (terrain == TILE_CLONE_MACHINE || TileID_is_ice(terrain))
      continue;
    actor->state ^= CS_REVERSE;
  }
}

static TriRes Actor_enter_tile(Actor* self, Level* level, bool pedantic_idle) {
  assert(!(pedantic_idle && !level->lx_state.pedantic_mode));
  if (TileID_is_animation(self->id))
    return true;
  assert(self->move_cooldown <= 0);

  TileID terrain = Level_get_terrain(level, self->pos);

  if (self->id == ACTOR_CHIP && level->lx_state.to_place_wall_pos != POSITION_NULL)
    return true;

  if (self->id == ACTOR_CHIP) {
    if (level->lx_state.to_place_wall_pos != POSITION_NULL)
      return true;
    if (!Level_player_has_item(level, TILE_BOOTS_ICE)) {
      self->direction = get_ice_wall_turn_dir(terrain, self->direction);
    }
  } else {
    if (!pedantic_idle) {
      self->direction = get_ice_wall_turn_dir(terrain, self->direction);
    }
  }

  switch (terrain) {
    case TILE_WATER:
      if (self->id == ACTOR_GLIDER ||
          (self->id == ACTOR_CHIP && Level_player_has_item(level, TILE_BOOTS_WATER))) {
        // We survive, yay!
        break;
      } else {
        // Drown
        if (self->id == ACTOR_BLOCK) {
          Level_set_terrain(level, self->pos, TILE_DIRT);
        }
        if (self->id == ACTOR_CHIP) {
          Level_remove_chip(level, CHIP_DROWNED, NULL);
        } else {
          Actor_remove(self, level, ANIM_WATER);
        }
        return TRIRES_DIED;
      }
    case TILE_FIRE:
      if (pedantic_idle)
        break;
      if (self->id == ACTOR_CHIP && !Level_player_has_item(level, TILE_BOOTS_FIRE)) {
        Level_remove_chip(level, CHIP_BURNED, NULL);
        return TRIRES_DIED;
      }
      break;
    case TILE_DIRT:
    case TILE_BLUE_WALL_REAL:
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      if (self->id == ACTOR_CHIP) {
        // Only play the SFX if we're Chip
        Level_add_sfx(level, SND_TILE_EMPTIED);
      }
      break;
    case TILE_POPUP_WALL:
      if (self->id == ACTOR_CHIP) {
        Level_set_terrain(level, self->pos, TILE_WALL);
        Level_add_sfx(level, SND_WALL_CREATED);
      }
      break;
    case TILE_DOOR_RED:
    case TILE_DOOR_GREEN:
    case TILE_DOOR_BLUE:
    case TILE_DOOR_YELLOW: {
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      if (self->id == ACTOR_CHIP) {
        uint8_t* item_ptr = Level_player_item_ptr(level, terrain);
        if (terrain != TILE_DOOR_GREEN && *item_ptr > 0) {
          *item_ptr -= 1;
        }
        Level_add_sfx(level, SND_DOOR_OPENED);
      }
      break;
    }
    case TILE_KEY_BLUE:
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      [[fallthrough]];
    case TILE_KEY_RED:
    case TILE_KEY_GREEN:
    case TILE_KEY_YELLOW: {
      if (self->id != ACTOR_CHIP)
        break;
      Level_add_sfx(level, SND_ITEM_COLLECTED);
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      uint8_t* item_ptr = Level_player_item_ptr(level, terrain);
      if (*item_ptr == 255) {
        *item_ptr = 0;
      } else {
        *item_ptr += 1;
      }
      break;
    }
    case TILE_BOOTS_ICE:
    case TILE_BOOTS_FORCE_FLOOR:
    case TILE_BOOTS_FIRE:
    case TILE_BOOTS_WATER: {
      if (self->id != ACTOR_CHIP)
        break;
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      uint8_t* item_ptr = Level_player_item_ptr(level, terrain);
      *item_ptr = 1;
      Level_add_sfx(level, SND_ITEM_COLLECTED);
      break;
    }
    case TILE_THIEF:
      if (self->id != ACTOR_CHIP)
        break;
      *Level_player_item_ptr(level, TILE_BOOTS_ICE) = 0;
      *Level_player_item_ptr(level, TILE_BOOTS_FORCE_FLOOR) = 0;
      *Level_player_item_ptr(level, TILE_BOOTS_FIRE) = 0;
      *Level_player_item_ptr(level, TILE_BOOTS_WATER) = 0;
      Level_add_sfx(level, SND_BOOTS_STOLEN);
      break;
    case TILE_IC_CHIP:
      if (pedantic_idle || self->id != ACTOR_CHIP)
        break;
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      if (level->chips_left > 0) {
        level->chips_left -= 1;
      }
      Level_add_sfx(level, SND_IC_COLLECTED);
      break;
    case TILE_SOCKET:
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      if (self->id == ACTOR_CHIP) {
        Level_add_sfx(level, SND_SOCKET_OPENED);
      }
      break;
    case TILE_BOMB:
      if (pedantic_idle)
        break;
      Level_set_terrain(level, self->pos, TILE_FLOOR);
      if (self->id == ACTOR_CHIP) {
        Level_remove_chip(level, CHIP_BOMBED, NULL);
      } else {
        Level_add_sfx(level, SND_BOMB_EXPLODES);
        Actor_remove(self, level, ANIM_BOMB);
      }
      return TRIRES_DIED;
    case TILE_TRAP:
      if (!pedantic_idle) {
        Level_add_sfx(level, SND_TRAP_ENTERED);
      }
      break;
    case TILE_BUTTON_TANK:
      if (!pedantic_idle) {
        Level_turn_tanks(level);
        Level_add_sfx(level, SND_BUTTON_PUSHED);
      }
      break;
    case TILE_BUTTON_TOGGLE:
      if (!pedantic_idle) {
        level->lx_state.toggle_walls_xor ^= TILE_TOGGLE_DOOR_OPEN ^ TILE_TOGGLE_DOOR_CLOSED;
        Level_add_sfx(level, SND_BUTTON_PUSHED);
      }
      break;
    case TILE_BUTTON_CLONE:
      if (!pedantic_idle) {
        Position connected_cell_pos = Level_find_connected_cell(
            level, self->pos, TILE_CLONE_MACHINE, &level->cloner_connections);
        if (connected_cell_pos != POSITION_NULL) {
          bool clone_success = Level_activate_cloner(level, connected_cell_pos);
          if (clone_success) {
            Level_add_sfx(level, SND_BUTTON_PUSHED);
          }
        }
      }
      break;
    case TILE_BUTTON_TRAP:
      if (!pedantic_idle) {
        Level_add_sfx(level, SND_BUTTON_PUSHED);
      }
    break;
    case TILE_EXIT:
      if (self->id != ACTOR_CHIP)
        break;
      self->hidden = true;
      level->level_complete = true;
      Level_add_sfx(level, SND_CHIP_WINS);
      break;
    default:
      break;
  }
  return TRIRES_SUCCESS;
}

/**
 * Returns `true` if the actor has still cooldown to go
 */
static bool Actor_reduce_cooldown(Actor* self, Level const* level) {
  if (TileID_is_animation(self->id))
    return true;
  assert(self->move_cooldown > 0);

  if (self->id == ACTOR_CHIP && level->lx_state.chip_stuck)
    return true;

  uint8_t speed = 2;
  if (self->id == ACTOR_BLOB) {
    speed /= 2;
  }
  TileID terrain = Level_get_terrain(level, self->pos);
  if (TileID_is_force_floor(terrain) &&
      !(self->id == ACTOR_CHIP && Level_player_has_item(level, TILE_BOOTS_FORCE_FLOOR))) {
    speed *= 2;
  }
  if (TileID_is_ice(terrain) &&
      !(self->id == ACTOR_CHIP && Level_player_has_item(level, TILE_BOOTS_ICE))) {
    speed *= 2;
  }
  self->move_cooldown -= speed;
  self->animation_frame = self->move_cooldown / 2;
  return Actor_is_moving(self);
}

static TriRes Actor_advance_movement(Actor* self,
                                     Level* level,
                                     bool releasing) {
  if (TileID_is_animation(self->id))
    return TRIRES_SUCCESS;
  Direction previous_releasing_dir = DIRECTION_NIL;

  // If we aren't currently moving, start right now!
  if (!Actor_is_moving(self)) {
    if (releasing) {
      assert(self->direction != DIRECTION_NIL);
      previous_releasing_dir = self->move_decision;
      self->move_decision = self->direction;
    }
    // If we don't have any direction we want to go on, don't do anything
    // (except for idling on the tile when in pedantic mode)
    if (self->move_decision == DIRECTION_NIL &&
        Actor_get_forced_move(self) == DIRECTION_NIL) {
      if (level->lx_state.pedantic_mode &&
          Actor_enter_tile(self, level, true) == TRIRES_DIED) {
        return TRIRES_DIED;
      }
      return TRIRES_SUCCESS;
    }
    TriRes start_res = Actor_start_moving_to(self, level, releasing);
    if (start_res != TRIRES_DIED) {
      self->hidden = false;
    }
    if (level->lx_state.pedantic_mode && start_res == TRIRES_NOTHING &&
        Actor_enter_tile(self, level, true) != TRIRES_DIED)
      return TRIRES_DIED;
    if (start_res == TRIRES_DIED)
      return TRIRES_DIED;
    if (start_res == TRIRES_NOTHING) {
      if (releasing) {
        self->move_decision = previous_releasing_dir;
      }
      return TRIRES_NOTHING;
    }
  }
  if (Actor_reduce_cooldown(self, level))
    return TRIRES_SUCCESS;
  return Actor_enter_tile(self, level, false);
}

static bool Actor_can_be_pushed(Actor* self,
                                Level* level,
                                Direction dir,
                                uint8_t flags) {
  assert(self && self->id == ACTOR_BLOCK);
  assert(Level_get_terrain(level, self->pos) != TILE_CLONE_MACHINE);
  assert(dir != DIRECTION_NIL);
  if (!Actor_check_collision(self, level, dir, flags)) {
    if (!Actor_is_moving(self) &&
        (flags & (CMM_PUSHBLOCKS | CMM_PUSHBLOCKSNOW))) {
      self->direction = dir;
      if (level->lx_state.pedantic_mode) {
        self->move_decision = dir;
      }
    }
    return false;
  }
  if (flags & (CMM_PUSHBLOCKS | CMM_PUSHBLOCKSNOW)) {
    self->direction = dir;
    self->move_decision = dir;
    self->state |= CS_PUSHED;
    if (flags & CMM_PUSHBLOCKSNOW) {
      Actor_advance_movement(self, level, false);
    }
  }
  return true;
}

static Direction const clockwise_directions[4] = {
    DIRECTION_NORTH, DIRECTION_EAST, DIRECTION_SOUTH, DIRECTION_WEST};

static void Actor_get_checked_decision_dirs(Actor* self,
                                            Level* level,
                                            Direction choices[4]) {
  switch (self->id) {
    case ACTOR_TANK:
      choices[0] = self->direction;
      break;
    case ACTOR_BALL:
      choices[0] = self->direction;
      choices[1] = Direction_back(self->direction);
      break;
    case ACTOR_GLIDER:
      choices[0] = self->direction;
      choices[1] = Direction_left(self->direction);
      choices[2] = Direction_right(self->direction);
      choices[3] = Direction_back(self->direction);
      break;
    case ACTOR_FIREBALL:
      choices[0] = self->direction;
      choices[1] = Direction_right(self->direction);
      choices[2] = Direction_left(self->direction);
      choices[3] = Direction_back(self->direction);
      break;
    case ACTOR_BUG:
      choices[0] = Direction_left(self->direction);
      choices[1] = self->direction;
      choices[2] = Direction_right(self->direction);
      choices[3] = Direction_back(self->direction);
      break;
    case ACTOR_PARAMECIUM:
      choices[0] = Direction_right(self->direction);
      choices[1] = self->direction;
      choices[2] = Direction_left(self->direction);
      choices[3] = Direction_back(self->direction);
      break;
    case ACTOR_WALKER:
      if (Actor_check_collision(self, level, self->direction,
                                CMM_CLEARANIMATIONS)) {
        self->move_decision = self->direction;
        return;
      }
      Direction checked_dir = self->direction;
      uint8_t rotate_n = Level_lynx_rng(level) & 3;
      while (rotate_n > 0) {
        checked_dir = Direction_right(checked_dir);
        rotate_n -= 1;
      }

      choices[0] = checked_dir;
      break;
    case ACTOR_BLOB:
      choices[0] = clockwise_directions[Prng_random4(&level->prng)];
      break;
    case ACTOR_TEETH:
      if ((level->current_tick + level->init_step_parity) & 4)
        return;
      Position chip_pos = Level_get_chip(level)->pos;
      int8_t dx = chip_pos % MAP_WIDTH - self->pos % MAP_WIDTH;
      int8_t dy = chip_pos / MAP_WIDTH - self->pos / MAP_WIDTH;
      Direction horiz_dir = dx < 0   ? DIRECTION_WEST
                            : dx > 0 ? DIRECTION_EAST
                                     : DIRECTION_NIL;
      if (dx < 0) {
        dx = -dx;
      }
      Direction vert_dir = dy < 0   ? DIRECTION_NORTH
                           : dy > 0 ? DIRECTION_SOUTH
                                    : DIRECTION_NIL;
      if (dy < 0) {
        dy = -dy;
      }
      if (dx > dy) {
        choices[0] = horiz_dir;
        choices[1] = vert_dir;
        choices[2] = horiz_dir;
      } else {
        choices[0] = vert_dir;
        choices[1] = horiz_dir;
        choices[2] = vert_dir;
      }
      break;
    default:
      break;
  }
}

static void Chip_do_decision(Actor* self, Level* level) {
  level->lx_state.chip_pushing = false;
  self->move_decision = DIRECTION_NIL;

  bool can_move = true;

  // If the current input is non-directional, eg. a mouse move, OR we're
  // "stuck", don't move
  Direction move_dir = GameInput_to_direction(level->game_input);
  if (move_dir == DIRECTION_NIL || level->lx_state.chip_stuck)
    can_move = false;

  // Can we override the current forced move?
  TileID terrain = Level_get_terrain(level, self->pos);
  bool can_override = TileID_is_force_floor(terrain) && (self->state & CS_SLIDETOKEN);
  Direction forced_move = Actor_get_forced_move(self);
  if (forced_move != DIRECTION_NIL && !can_override)
    can_move = false;

  if (!can_move) {
    // Do nothing!
  } else if (!Direction_is_diagonal(move_dir)) {
    // If we're holding an orthogonal direction, just make a collision check
    // there and use that as our decision regardless of if it succeeds or not
    Actor_check_collision(self, level, move_dir, CMM_PUSHBLOCKS);
    self->move_decision = move_dir;
  } else {
    if (!(self->direction & move_dir)) {
      // If we're trying to move in a diagonal neither of which is our current
      // direction, pick the horizonal direction unless it's blocked
      Direction horiz_dir = move_dir & (DIRECTION_WEST | DIRECTION_EAST);
      Direction vert_dir = move_dir & (DIRECTION_NORTH | DIRECTION_SOUTH);
      bool can_go_horiz =
          Actor_check_collision(self, level, horiz_dir, CMM_PUSHBLOCKS);
      self->move_decision = can_go_horiz ? horiz_dir : vert_dir;
    } else {
      // If one of the dirs is out current one, prefer that one, and pick the
      // other if and only if it's available and our current dir is not
      Direction current_dir = self->direction;
      // A diagonal move is two bits in the directions bitfield set, if we XOR
      // (or AND NOT) the current direction out of it, we are only left with the
      // other direction
      Direction other_dir = move_dir ^ self->direction;
      bool can_go_current =
          Actor_check_collision(self, level, current_dir, CMM_PUSHBLOCKS);
      bool can_go_other =
          Actor_check_collision(self, level, other_dir, CMM_PUSHBLOCKS);
      self->move_decision =
          !can_go_current && can_go_other ? other_dir : current_dir;
    }
  }
  if (self->move_decision == DIRECTION_NIL && forced_move == DIRECTION_NIL) {
    Level_stop_terrain_sfx(level);
  }
  // Predict our next position (with flaws!), for the `Actor_start_moving_to`
  // tried-to-enter-just-vacated-cell nonsense
  if (self->move_decision != DIRECTION_NIL) {
    level->lx_state.chip_predicted_pos =
        Position_neighbor(self->pos, self->move_decision);
  }
}

static void Actor_do_decision(Actor* self, Level* level) {
  if (TileID_is_animation(self->id)) {
    self->animation_frame -= 1;
    if (self->animation_frame < 0) {
      Actor_erase_animation(self, level);
    }
    return;
  }
  Direction forced_move = Actor_calculate_forced_move(self, level, true);
  Actor_set_forced_move(self, forced_move);
  if (self == Level_get_chip(level)) {
    Chip_do_decision(self, level);
    return;
  }
  if (self->id == ACTOR_BLOCK)
    return;
  self->move_decision = DIRECTION_NIL;
  if (forced_move)
    return;

  TileID terrain = Level_get_terrain(level, self->pos);
  if (terrain == TILE_CLONE_MACHINE || terrain == TILE_TRAP) {
    self->move_decision = self->direction;
    return;
  }
  Direction directions[4] = {DIRECTION_NIL, DIRECTION_NIL, DIRECTION_NIL,
                             DIRECTION_NIL};
  Actor_get_checked_decision_dirs(self, level, directions);
  for (uint8_t idx = 0; idx < 4; idx += 1) {
    Direction checked_dir = directions[idx];
    if (checked_dir == DIRECTION_NIL)
      return;
    self->move_decision = checked_dir;
    if (Actor_check_collision(self, level, checked_dir, CMM_CLEARANIMATIONS))
      return;
  }
}

static void Level_activate_trap(Level* self, Position pos) {
  assert(pos != POSITION_NULL);
  if (Level_get_terrain(self, pos) != TILE_TRAP) {
    assert(false && "Can't activate a cell with no trap!");
    return;
  }
  Actor* actor = Level_find_actor(self, pos, 0);
  if (actor && actor->direction != DIRECTION_NIL) {
    Actor_advance_movement(actor, self, true);
  }
}

/*
 * Teleport ourselves to the next valid teleport in reverse reading order
 */
static bool Actor_teleport(Actor* self, Level* level) {
  Position start_pos = self->pos;
  Position checked_pos = start_pos;
  Actor* chip = Level_get_chip(level);
  while (true) {
    if (checked_pos == 0) {
      checked_pos = MAP_WIDTH * MAP_HEIGHT;
    }
    checked_pos -= 1;
    TileID terrain = Level_get_terrain(level, checked_pos);
    if (terrain == TILE_TELEPORT) {
      // NOTE: Intentional bug: if a non-Chip actor fails a teleport check due
      // to that cell already being occupied by an actor, the occupier's claim
      // on the cell is ***removed, without the actor itself being removed***
      // due to the teleportee's position still being set to the position of the
      // occupier,
      if (self->id != ACTOR_CHIP) {
        Level_cell_remove_claim(level, self->pos);
      }
      self->pos = checked_pos;
      if (!Level_cell_has_claim(level, checked_pos) &&
          Actor_check_collision(self, level, self->direction, 0))
        break;
      if (checked_pos == start_pos) {
        if (self->id == ACTOR_CHIP) {
          level->lx_state.chip_stuck = true;
        } else {
          Level_cell_add_claim(level, self->pos);
        }
        return false;
      }
    } else if (Level_cell_ever_had_teleport(level, checked_pos)) {
      // Pedantic Lynx only: if there was a teleport on this cell, but due to a
      // monster standing on a recessed wall, it was overwritten
      Level_set_terrain(level, checked_pos, TILE_TELEPORT);
      if (checked_pos == chip->pos) {
        chip->hidden = true;
      }
    }
  }
  if (self->id == ACTOR_CHIP) {
    Level_add_sfx(level, SND_TELEPORTING);
  } else {
    Level_cell_add_claim(level, self->pos);
  }
  self->state |= CS_TELEPORTED;
  return true;
}

static void lynx_tick_level(Level* self) {
  Actor* chip = Level_get_chip(self);
  if (chip->id == ACTOR_PUSHING_CHIP) {
    chip->id = ACTOR_CHIP;
  }
  if (!Level_in_endgame(self)) {
    if (self->level_complete) {
      Level_start_endgame(self);
    } else if (self->time_limit && self->current_tick >= self->time_limit) {
      Level_remove_chip(self, CHIP_OUTOFTIME, NULL);
    }
  }
  for (uint32_t i = 0; i < self->actors_n; i += 1) {
    Actor* actor = &self->actors[i];
    if (actor->hidden || !(actor->state & CS_REVERSE))
      continue;
    actor->state &= ~CS_REVERSE;
    if (!Actor_is_moving(actor)) {
      actor->direction = Direction_back(actor->direction);
    }
  }
  for (uint32_t i = 0; i < self->actors_n; i += 1) {
    Actor* actor = &self->actors[i];
    if (!(actor->state & CS_PUSHED))
      continue;
    if (actor->hidden || !Actor_is_moving(actor)) {
      Level_stop_sfx(self, SND_BLOCK_MOVING);
      actor->state &= ~CS_PUSHED;
    }
  }
  if (self->lx_state.toggle_walls_xor) {
    for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
      MapCell* cell = &self->map[pos];
      if (cell->top.id == TILE_TOGGLE_DOOR_OPEN ||
          cell->top.id == TILE_TOGGLE_DOOR_CLOSED) {
        cell->top.id ^= self->lx_state.toggle_walls_xor;
      }
    }
    self->lx_state.toggle_walls_xor = 0;
  }
  self->lx_state.chip_predicted_pos = POSITION_NULL;
  self->lx_state.chip_colliding_actor = NULL;
  // Decision/intent phase: all actors decide which direction to go in
  for (ptrdiff_t i = self->actors_n - 1; i >= 0; i -= 1) {
    Actor* actor = &self->actors[i];
    if (actor != chip && actor->hidden)
      continue;
    if (!TileID_is_animation(actor->id) && Actor_is_moving(actor))
      continue;
    Actor_do_decision(actor, self);
  }
  // Movement phase: all actors try to move in their predetermined directions
  for (ptrdiff_t i = self->actors_n - 1; i >= 0; i -= 1) {
    Actor* actor = &self->actors[i];
    if (actor == chip && self->level_complete)
      continue;
    if (actor != chip && actor->hidden)
      continue;
    TriRes move_res = Actor_advance_movement(actor, self, false);
    if (move_res == TRIRES_DIED)
      continue;
    actor->move_decision = DIRECTION_NIL;
    Actor_set_forced_move(actor, DIRECTION_NIL);
    TileID terrain = Level_get_terrain(self, actor->pos);
    // In pedantic Lynx, if there's an actor on a recessed wall, the terrain
    // under chip is replaced with a wall
    if (actor != chip && self->lx_state.pedantic_mode && terrain == TILE_POPUP_WALL) {
      self->lx_state.to_place_wall_pos = chip->pos;
    }
    // We also activate traps at this point
    if (terrain == TILE_BUTTON_TRAP && !Actor_is_moving(actor)) {
      Position linked_pos = Level_find_connected_cell(
          self, actor->pos, TILE_TRAP, &self->trap_connections);
      if (linked_pos != POSITION_NULL) {
        Level_activate_trap(self, linked_pos);
      }
    }
  }
  // Teleport phase: teleport actors on teleports
  for (ptrdiff_t i = self->actors_n - 1; i >= 0; i -= 1) {
    Actor* actor = &self->actors[i];
    if (actor->hidden || Actor_is_moving(actor))
      continue;
    TileID terrain = Level_get_terrain(self, actor->pos);
    if (terrain != TILE_TELEPORT)
      continue;
    Actor_teleport(actor, self);
  }
  // Pedantic Lynx only: put down the wall at the position Chip was at
  if (self->lx_state.to_place_wall_pos != POSITION_NULL) {
    if (!chip->hidden) {
      TileID terrain = Level_get_terrain(self, chip->pos);
      if (terrain == TILE_TRAP) {
        Level_activate_trap(self, chip->pos);
      }
      Level_set_terrain(self, self->lx_state.to_place_wall_pos, TILE_WALL);
    }
    self->lx_state.to_place_wall_pos = POSITION_NULL;
  }
  // Choose terrain SFX and stuff
  if (!chip->hidden) {
    TileID terrain = Level_get_terrain(self, chip->pos);
    if (terrain == TILE_HINT && chip->move_cooldown <= 0) {
      self->status_flags |= SF_SHOW_HINT;
    } else {
      self->status_flags &= ~SF_SHOW_HINT;
    }
    if (chip->id == ACTOR_CHIP && self->lx_state.chip_pushing) {
      chip->id = ACTOR_PUSHING_CHIP;
    }
    if (chip->move_cooldown) {
      Level_stop_terrain_sfx(self);
      if (terrain == TILE_FIRE && Level_player_has_item(self, TILE_BOOTS_FIRE))
        Level_add_sfx(self, SND_FIREWALKING);
      else if (terrain == TILE_WATER && Level_player_has_item(self, TILE_BOOTS_WATER))
        Level_add_sfx(self, SND_WATERWALKING);
      else if (TileID_is_ice(terrain)) {
        if (Level_player_has_item(self, TILE_BOOTS_ICE))
          Level_add_sfx(self, SND_ICEWALKING);
        else if (terrain == TILE_ICE)
          Level_add_sfx(self, SND_SKATING_FORWARD);
        else
          Level_add_sfx(self, SND_SKATING_TURN);
      } else if (TileID_is_force_floor(terrain)) {
        if (Level_player_has_item(self, TILE_BOOTS_FORCE_FLOOR))
          Level_add_sfx(self, SND_SLIDEWALKING);
        else
          Level_add_sfx(self, SND_SLIDING);
      }
    }
    if (self->lx_state.chip_stuck && TileID_is_ice(terrain)) {
      Level_add_sfx(self, SND_SKATING_FORWARD);
    }
  }
  if (self->lx_state.endgame_timer) {
    self->timer_offset -= 1;
    self->lx_state.endgame_timer -= 1;
    if (self->lx_state.endgame_timer == 0) {
      Level_stop_terrain_sfx(self);
      Level_stop_sfx(self, SND_BLOCK_MOVING);
      self->win_state = self->level_complete ? TRIRES_SUCCESS : TRIRES_DIED;
    }
  }
}

static void lynx_uninit_level(Level* level) {
  return;
}

static void lynx_hash_level(Level const* self, hash_t* hash) {
  *hash = hash_scalar(self->lx_state.pedantic_mode, *hash);
  ptrdiff_t chip_colliding_actor = 0;
  if (self->lx_state.chip_colliding_actor) {
    chip_colliding_actor = self->lx_state.chip_colliding_actor - self->actors;
  }
  *hash = hash_scalar(chip_colliding_actor, *hash);
  *hash = hash_scalar(self->lx_state.chip_predicted_pos, *hash);
  *hash = hash_scalar(self->lx_state.to_place_wall_pos, *hash);
  *hash = hash_scalar(self->lx_state.prng1, *hash);
  *hash = hash_scalar(self->lx_state.prng2, *hash);
  *hash = hash_scalar(self->lx_state.endgame_timer, *hash);
  *hash = hash_scalar(self->lx_state.toggle_walls_xor, *hash);
  *hash = hash_scalar(self->lx_state.chip_stuck, *hash);
  *hash = hash_scalar(self->lx_state.chip_pushing, *hash);
  *hash = hash_scalar(self->lx_state.chip_bonked, *hash);
  *hash = hash_scalar(self->lx_state.map_breached, *hash);
}

static bool lynx_level_equals(Level const* self, Level const* other) {
  if (self == other)
    return true;
  if (self->lx_state.pedantic_mode != other->lx_state.pedantic_mode)
    return false;
  if (self->lx_state.chip_predicted_pos != other->lx_state.chip_predicted_pos)
    return false;
  if (self->lx_state.to_place_wall_pos != other->lx_state.to_place_wall_pos)
    return false;
  if (self->lx_state.prng1 != other->lx_state.prng1)
    return false;
  if (self->lx_state.prng2 != other->lx_state.prng2)
    return false;
  if (self->lx_state.endgame_timer != other->lx_state.endgame_timer)
    return false;
  if (self->lx_state.toggle_walls_xor != other->lx_state.toggle_walls_xor)
    return false;
  if (self->lx_state.chip_stuck != other->lx_state.chip_stuck)
    return false;
  if (self->lx_state.chip_pushing != other->lx_state.chip_pushing)
    return false;
  if (self->lx_state.chip_bonked != other->lx_state.chip_bonked)
    return false;
  if (self->lx_state.map_breached != other->lx_state.map_breached)
    return false;

  ptrdiff_t chip_colliding_actor_self = 0;
  ptrdiff_t chip_colliding_actor_other = 0;
  if (self->lx_state.chip_colliding_actor) {
    chip_colliding_actor_self = self->lx_state.chip_colliding_actor - self->actors;
  }
  if (other->lx_state.chip_colliding_actor) {
    chip_colliding_actor_other = other->lx_state.chip_colliding_actor - other->actors;
  }
  if (chip_colliding_actor_self != chip_colliding_actor_other)
    return false;

  return true;
}

static bool lynx_chip_can_move(Level* self) {
  Actor* chip = Level_get_chip(self);
  if (chip->move_cooldown) {
    return false;
  }
  if (chip->id != ACTOR_CHIP) {
    return false;
  }
  if (TileID_is_force_floor(Level_get_terrain(self, chip->pos)) && (chip->state & CS_SLIDETOKEN)) {
    return true;
  }
  if (Actor_calculate_forced_move(chip, self, false) != DIRECTION_NIL) {
    return false;
  }
  if (self->lx_state.chip_stuck) {
    return false;
  }
  return true;
}

Ruleset const lynx_logic = {.id = RULESET_LYNX,
                            .init_level = lynx_init_level,
                            .tick_level = lynx_tick_level,
                            .uninit_level = lynx_uninit_level,
                            .add_hash_level = lynx_hash_level,
                            .level_equals = lynx_level_equals,
                            .chip_can_move = lynx_chip_can_move,
};
