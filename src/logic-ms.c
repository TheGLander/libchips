#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "logic.h"
#include "misc.h"

static bool Actor_advance_movement(Actor* self, Level* level, Direction dir);

/* Creature state flags.
 */
enum ActorState {
  CS_RELEASED = 0x0001,  /* can leave a beartrap */
  CS_CLONING = 0x0002,   /* cannot move this tick */
  CS_HASMOVED = 0x0004,  /* already used current move */
  CS_TURNING = 0x0008,   /* is turning around */
  CS_SLIP = 0x0010,      /* is on the slip list */
  CS_SLIDE = 0x0020,     /* is on the slip list but can move */
  CS_DEFERPUSH = 0x0040, /* button pushes will be delayed */
  CS_MUTANT = 0x0080,    /* block is mutant, looks like Chip */
  CS_SDIRMASK = 0x0F00,  /* creature needs to store another direction (currently
                            used for tank top glitch) */
  CS_SPONTANEOUS =
      0x1000, /* creature has potential to spontaneously generate */
};

enum {
  CS_SDIRSHIFT = 8,
};

static void Actor_set_spare_direction(Actor* self, Direction dir) {
  self->state &= ~CS_SDIRMASK;
  self->state |= dir << CS_SDIRSHIFT;
}

static Direction Actor_get_spare_direction(Actor const* self) {
  return (self->state & CS_SDIRMASK) >> CS_SDIRSHIFT;
}

/* Including the flag CMM_NOLEAVECHECK in a call to canmakemove()
 * indicates that the tile the creature is moving out of is
 * automatically presumed to permit such movement. CMM_NOEXPOSEWALLS
 * causes blue and hidden walls to remain unexposed.
 * CMM_CLONECANTBLOCK means that the creature will not be prevented
 * from moving by an identical creature standing in the way.
 * CMM_NOPUSHING prevents Chip from pushing blocks inside this
 * function. CMM_TELEPORTPUSH indicates to the block-pushing logic
 * that Chip is teleporting. This prevents a stack of two blocks from
 * being treated as a single block, and allows Chip to push a slipping
 * block away from him. CMM_NOFIRECHECK causes bugs and walkers to not
 * avoid fire. Finally, CMM_NODEFERBUTTONS causes buttons pressed by
 * pushed blocks to take effect immediately.
 */
enum CollisionCheckFlags {
  CMM_NOLEAVECHECK = 0x0001,
  CMM_NOEXPOSEWALLS = 0x0002,
  CMM_CLONECANTBLOCK = 0x0004,
  CMM_NOPUSHING = 0x0008,
  CMM_TELEPORTPUSH = 0x0010,
  CMM_NOFIRECHECK = 0x0020,
  CMM_NODEFERBUTTONS = 0x0040,

  CMM_ALL = CMM_NOLEAVECHECK | CMM_NOEXPOSEWALLS | CMM_CLONECANTBLOCK |
            CMM_NOPUSHING | CMM_TELEPORTPUSH | CMM_NOFIRECHECK |
            CMM_NODEFERBUTTONS,
};

/* Floor state flags.
 */
enum TerrainState {
  FS_BUTTONDOWN = 0x01, /* button press is deferred */
  FS_CLONING = 0x02,    /* clone machine is activated */
  FS_BROKEN = 0x04,     /* teleport/toggle wall doesn't work */
  FS_HASMUTANT = 0x08,  /* beartrap contains mutant block */
  FS_MARKER = 0x10,     /* marker used during initialization */
};

static inline MapCell* Level_get_map_cell(Level* self, Position pos) {
  return &self->map[pos];
}

static inline MapTile* MapCell_get_top_tile(MapCell* self) {
  return &self->top;
}

static inline MapTile* MapCell_get_bottom_tile(MapCell* self) {
  return &self->bottom;
}

static inline TileID MapCell_get_top_floor(MapCell const* self) {
  return self->top.id;
}

static inline TileID MapCell_get_bottom_floor(MapCell const* self) {
  return self->bottom.id;
}

static inline TileID MapCell_set_top_floor(MapCell* self, TileID tile) {
  return self->top.id = tile;
}

static inline TileID MapCell_set_bottom_floor(MapCell* self, TileID tile) {
  return self->bottom.id = tile;
}

static inline MapTile MapCell_pop_tile(MapCell* self) {
  MapTile tile = self->top;
  self->top = self->bottom;
  self->bottom.id = TILE_FLOOR;
  self->bottom.state = 0;
  return tile;
}

static inline void MapCell_push_tile(MapCell* self, MapTile tile) {
  self->bottom = self->top;
  self->top = tile;
}

static inline TileID MapTile_get_floor(MapTile const* self) {
  return self->id;
}

static inline TileID MapTile_set_floor(MapTile* self, TileID tile) {
  return self->id = tile;
}

static inline TileID MapTile_get_state(MapTile const* self) {
  return self->state;
}

static inline void MapTile_clear_state(MapTile* self) {
  self->state = 0;
}

static inline void MapTile_add_button_down_state(MapTile* self) {
  self->state |= FS_BUTTONDOWN;
}

static inline void MapTile_remove_button_down_state(MapTile* self) {
  self->state &= ~FS_BUTTONDOWN;
}

static inline void MapTile_add_cloning_state(MapTile* self) {
  self->state |= FS_CLONING;
}

static inline void MapTile_remove_cloning_state(MapTile* self) {
  self->state &= ~FS_CLONING;
}

static inline void MapTile_add_broken_state(MapTile* self) {
  self->state |= FS_BROKEN;
}

static inline void MapTile_remove_broken_state(MapTile* self) {
  self->state &= ~FS_BROKEN;
}

static inline void MapTile_add_mutant_state(MapTile* self) {
  self->state |= FS_HASMUTANT;
}

static inline void MapTile_remove_mutant_state(MapTile* self) {
  self->state &= ~FS_HASMUTANT;
}

static inline void MapTile_add_marker_state(MapTile* self) {
  self->state |= FS_MARKER;
}

static inline void MapTile_remove_marker_state(MapTile* self) {
  self->state &= ~FS_MARKER;
}

static inline TileID Level_cell_get_top_floor(Level const* self, Position pos) {
  return self->map[pos].top.id;
}

static inline void Level_cell_set_top_floor(Level* self,
                                            Position pos,
                                            TileID tile) {
  self->map[pos].top.id = tile;
}

static inline TileID Level_cell_get_bottom_floor(Level const* self,
                                                 Position pos) {
  return self->map[pos].bottom.id;
}

static inline void Level_cell_set_bottom_floor(Level* self,
                                               Position pos,
                                               TileID tile) {
  self->map[pos].bottom.id = tile;
}

/* Return the terrain tile found at the given location.
 */
static inline TileID Level_cell_get_terrain(Level const* self, Position pos) {
  MapCell const* cell = &self->map[pos];
  if (!TileID_is_key(cell->top.id) && !TileID_is_boots(cell->top.id) &&
      !TileID_is_actor(cell->top.id))
    return cell->top.id;
  if (!TileID_is_key(cell->bottom.id) && !TileID_is_boots(cell->bottom.id) &&
      !TileID_is_actor(cell->bottom.id))
    return cell->bottom.id;
  return TILE_FLOOR;
}

static inline void Level_cell_set_terrain(Level* self,
                                          Position pos,
                                          TileID tile) {
  MapCell* cell = &self->map[pos];
  if (!TileID_is_key(cell->top.id) && !TileID_is_boots(cell->top.id) &&
      !TileID_is_actor(cell->top.id))
    cell->top.id = tile;
  else if (!TileID_is_key(cell->bottom.id) &&
           !TileID_is_boots(cell->bottom.id) &&
           !TileID_is_actor(cell->bottom.id))
    cell->bottom.id = tile;
  else
    cell->bottom.id = tile;  // idk why but this is technically how TW does it
}

static inline Actor* Level_get_chip(Level* self) {
  return &self->actors[0];
}

static inline Position Level_get_mouse_goal(Level const* self) {
  return self->ms_state.mouse_goal;
}

static inline Position Level_has_mouse_goal(Level const* self) {
  return self->ms_state.mouse_goal >= 0;
}

static inline void Level_set_mouse_goal(Level* self, Position goal) {
  self->ms_state.mouse_goal = goal;
}

static inline bool Level_cancel_mouse_goal(Level* self) {
  self->ms_state.mouse_goal = POSITION_NULL;
  return true;
}

/* Return TRIRES_DIED or TRIRES_SUCCESS if gameplay is over.
 */
static TriRes Level_check_for_ending(Level* self) {
  if (self->ms_state.chip_status != CHIP_OKAY &&
      self->ms_state.chip_status != CHIP_SQUISHED) { /* Squish patch */
    if (self->win_state != TRIRES_DIED) {
      Level_add_sfx(self, SND_CHIP_LOSES);
    }
    self->win_state = TRIRES_DIED;
  } else if (self->level_complete) {
    if (self->win_state != TRIRES_SUCCESS) {
      Level_add_sfx(self, SND_CHIP_WINS);
    }
    self->win_state = TRIRES_SUCCESS;
  }
  return self->win_state;
}

/* Empty the list of sliding creatures.
 */
static void Level_reset_sliplist(Level* self) {
  self->ms_state.slips_n = 0;
}

/* Append the given creature to the end of the slip list.
 */
static Actor* Level_append_to_slip_list(Level* self,
                                        Actor* actor,
                                        Direction direction) {
  for (uint32_t n = 0; n < self->ms_state.slips_n; n += 1) {
    if (self->ms_state.slip_list[n].actor == actor) {
      self->ms_state.slip_list[n].direction = direction;
      return actor;
    }
  }

  self->ms_state.slip_list[self->ms_state.slips_n].actor = actor;
  self->ms_state.slip_list[self->ms_state.slips_n].direction = direction;
  self->ms_state.slips_n += 1;
  self->ms_state.mscc_slippers += 1; /* new accounting */
  return actor;
}

/* Add the given creature to the start of the slip list.
 */
static Actor* Level_prepend_to_slip_list(Level* self,
                                         Actor* actor,
                                         Direction direction) {
  if (self->ms_state.slips_n && self->ms_state.slip_list[0].actor == actor) {
    self->ms_state.slip_list[0].direction = direction;
    return actor;
  }

  for (uint32_t n = self->ms_state.slips_n; n; n -= 1)
    self->ms_state.slip_list[n] = self->ms_state.slip_list[n - 1];
  self->ms_state.slips_n += 1;
  self->ms_state.slip_list[0].actor = actor;
  self->ms_state.slip_list[0].direction = direction;
  return actor;
}

/* Return the sliding direction of a creature on the slip list.
 */
static Direction Level_get_actor_slip_dir(Level const* self,
                                          Actor const* actor) {
  for (uint32_t n = 0; n < self->ms_state.slips_n; n += 1)
    if (self->ms_state.slip_list[n].actor == actor)
      return self->ms_state.slip_list[n].direction;
  return DIRECTION_NIL;
}

/* Remove the given creature from the slip list.
 */
static void Level_remove_actor_from_slip_list(Level* self, Actor const* actor) {
  uint32_t n;

  for (n = 0; n < self->ms_state.slips_n; n += 1) {
    if (self->ms_state.slip_list[n].actor == actor) {
      break;
    }
  }
  if (n == self->ms_state.slips_n) {
    return;
  }
  self->ms_state.slips_n -= 1;
  for (; n < self->ms_state.slips_n; n += 1) {
    self->ms_state.slip_list[n] = self->ms_state.slip_list[n + 1];
  }
}

/*
 * Simple floor functions.
 */

/* Translate a slide floor into the direction it points in. In the
 * case of a random slide floor, a new direction is selected.
 */
static Direction Level_get_slide_dir(Level* self, TileID floor) {
  switch (floor) {
    case TILE_FORCE_FLOOR_NORTH:
      return DIRECTION_NORTH;
    case TILE_FORCE_FLOOR_WEST:
      return DIRECTION_WEST;
    case TILE_FORCE_FLOOR_SOUTH:
      return DIRECTION_SOUTH;
    case TILE_FORCE_FLOOR_EAST:
      return DIRECTION_EAST;
    case TILE_FORCE_FLOOR_RANDOM:
      return 1 << Prng_random4(&self->prng);
    default:
      return DIRECTION_NIL;
  }
}

/* Alter a creature's direction if they are at an ice wall.
 */
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

/* Find the location of a bear trap from one of its buttons.
 */
static Position Level_locate_trap_by_button(Level const* self,
                                            Position button_pos) {
  ConnList const* traps = &self->trap_connections;
  for (uint8_t i = 0; i < traps->length; i += 1)
    if (traps->items[i].from == button_pos)
      return traps->items[i].to;
  return POSITION_NULL;
}

/* Find the location of a bear trap from one of its buttons.
 */
static Position Level_locate_cloner_by_button(Level const* self,
                                              Position button_pos) {
  ConnList const* cloners = &self->cloner_connections;
  for (uint8_t i = 0; i < cloners->length; i += 1)
    if (cloners->items[i].from == button_pos)
      return cloners->items[i].to;
  return POSITION_NULL;
}

/* Return TRUE if the brown button at the give location is currently
 * held down.
 */
static bool Level_is_trap_button_down(Level const* self, Position pos) {
  return pos >= 0 && pos < MAP_WIDTH * MAP_HEIGHT &&
         Level_cell_get_top_floor(self, pos) != TILE_BUTTON_TRAP;
}

/* Return TRUE if a bear trap is currently passable.
 */
static bool Level_is_trap_open(Level* self, Position pos, Position skip_pos) {
  ConnList* traps = &self->trap_connections;
  for (uint8_t i = 0; i < traps->length; i += 1) {
    if (traps->items[i].to == pos && traps->items[i].from != skip_pos &&
        Level_is_trap_button_down(self, traps->items[i].from)) {
      return true;
    }
  }
  return false;
}

/* Flip-flop the state of any toggle walls.
 */
static void Level_toggle_walls(Level* level) {
  for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
    MapCell* cell = Level_get_map_cell(level, pos);
    MapTile* top = MapCell_get_top_tile(cell);
    MapTile* bottom = MapCell_get_bottom_tile(cell);
    if ((MapTile_get_floor(top) == TILE_TOGGLE_DOOR_OPEN ||
         MapTile_get_floor(top) == TILE_TOGGLE_DOOR_CLOSED) &&
        !(MapTile_get_state(top) & FS_BROKEN)) {
      MapTile_set_floor(top, MapTile_get_floor(top) == TILE_TOGGLE_DOOR_OPEN
                                 ? TILE_TOGGLE_DOOR_CLOSED
                                 : TILE_TOGGLE_DOOR_OPEN);
    }
    if ((MapTile_get_floor(bottom) == TILE_TOGGLE_DOOR_OPEN ||
         MapTile_get_floor(bottom) == TILE_TOGGLE_DOOR_CLOSED) &&
        !(MapTile_get_state(bottom) & FS_BROKEN)) {
      MapTile_set_floor(bottom, MapTile_get_floor(bottom) == TILE_TOGGLE_DOOR_OPEN
                                    ? TILE_TOGGLE_DOOR_CLOSED
                                    : TILE_TOGGLE_DOOR_OPEN);
    }
  }
}

/*
 * Functions that manage the list of entities.
 */

static Actor* Level_create_actor(Level* self) {
  if (self->actors_n == MAX_CREATURES) {
    warn("%d: Actor array full (NOTE: THIS SHOULD NOT BE POSSIBLE)", self->current_tick);
    return NULL;
  }
  Actor* actor = &self->actors[self->actors_n];
  *actor = (Actor){.pos = POSITION_NULL,
                   .id = TILE_NOTHING,
                   .direction = DIRECTION_NIL,
                   .move_cooldown = 0,
                   .animation_frame = 0,
                   .hidden = false,
                   .state = 0,
                   .move_decision = DIRECTION_NIL,};

  self->actors_n += 1;
  return actor;
}

/* Return the creature located at pos. Ignores Chip unless includechip
 * is TRUE. Return NULL if no such creature is present.
 */
static Actor* Level_look_up_creature(Level* self,
                                     Position pos,
                                     bool includechip) {
  if (!self->actors)
    return NULL;
  for (uint32_t n = 0; n < self->actors_n; n += 1) {
    if (self->actors[n].hidden)
      continue;
    if (self->actors[n].pos == pos)
      if (self->actors[n].id != CREATURE_CHIP || includechip)
        return &self->actors[n];
  }
  return NULL;
}

/* Return the block located at pos. If the block in question is not
 * currently "active", it is automatically added to the block list.
 */
static Actor* Level_look_up_block(Level* self, Position pos) {
  for (uint32_t n = 0; n < self->actors_n; n += 1) {
    if (self->actors[n].id == CREATURE_BLOCK && self->actors[n].pos == pos
        && !self->actors[n].hidden) {
      return &self->actors[n];
    }
  }

  Actor* block = Level_create_actor(self);
  if (!block) {
    warn("%d: Level_look_up_block unable to create block",
         self->current_tick);
    return NULL;
  }
  block->id = CREATURE_BLOCK;
  block->pos = pos;
  TileID id = Level_cell_get_top_floor(self, pos);
  if (id == TILE_BLOCK_STATIC)
    block->direction = DIRECTION_NIL;
  else if (TileID_actor_get_id(id) == CREATURE_BLOCK)
    block->direction = TileID_actor_get_dir(id);
  else
    warn("%d: Level_look_up_block called on blockless location",
         self->current_tick);
  return block;
}

/* Update the given creature's tile on the map to reflect its current
 * state.
 */
static void Actor_update_floor(Actor* self, Level* level) {
  if (self->hidden)
    return;
  MapTile* tile = MapCell_get_top_tile(Level_get_map_cell(level, self->pos));
  TileID id = self->id;
  if (id == CREATURE_BLOCK) {
    Level_cell_set_top_floor(level, self->pos, TILE_BLOCK_STATIC);
    if (self->state & CS_MUTANT)
      MapTile_set_floor(tile, TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_NORTH));
    return;
  } else if (id == CREATURE_CHIP) {
    if (level->ms_state.chip_status) {
      switch (level->ms_state.chip_status) {
        case CHIP_BURNED:
          MapTile_set_floor(tile, TileID_actor_with_dir(CREATURE_CHIP, TILE_BURNED_CHIP));
          return;
        case CHIP_DROWNED:
          MapTile_set_floor(tile, TileID_actor_with_dir(CREATURE_CHIP, TILE_DROWNED_CHIP));
          return;
      }
    } else if (Level_cell_get_bottom_floor(level, self->pos) == TILE_WATER) {
      id = CREATURE_SWIMMING_CHIP;
    }
  }

  Direction dir = self->direction;
  if (self->state & CS_TURNING)
    dir = Direction_right(self->direction);

  MapTile_set_floor(tile, TileID_actor_with_dir(id, dir));
  MapTile_clear_state(tile);
}

/* Add the given creature's tile to the map.
 */
static void Actor_add_to_map(Actor* self, Level* level) {
  static MapTile const dummy = {TILE_FLOOR, 0};

  if (self->hidden)
    return;
  MapCell_push_tile(Level_get_map_cell(level, self->pos), dummy);
  Actor_update_floor(self, level);
}

/* Enervate an inert creature.
 */
static Actor* Level_awaken_creature(Level* self, Position pos) {
  TileID tileid = Level_cell_get_top_floor(self, pos);
  if (!TileID_is_actor(tileid) || TileID_actor_get_id(tileid) == CREATURE_CHIP)
    return NULL;
  Actor* new = Level_create_actor(self);
  if (!new) {
    return NULL;
  }
  new->id = TileID_actor_get_id(tileid);
  new->direction = TileID_actor_get_dir(tileid);
  new->pos = pos;
  return new;
}

/* Mark a creature as dead.
 */
static void Actor_remove(Actor* self, Level* level) {
  self->state &= ~(CS_SLIP | CS_SLIDE);
  if (self->id == CREATURE_CHIP) {
    if (level->ms_state.chip_status == CHIP_OKAY)
      level->ms_state.chip_status = CHIP_NOTOKAY;
  } else
    self->hidden = true;
}

/* Turn around any and all tanks. (A tank that is halfway through the
 * process of moving at the time is given special treatment.)
 */
static void Level_turn_tanks(Level* self, Actor const* invoking_actor) {
  for (uint32_t n = 0; n < self->actors_n; n += 1) {
    Actor* actor = &self->actors[n]; /* convenience, Tank Top Glitch */
    if (actor->hidden || actor->id != CREATURE_TANK)
      continue;
    actor->direction = Direction_back(actor->direction);
    if (actor->state & CS_SLIP && !(actor->state & CS_SLIDE) &&
        Actor_get_spare_direction(actor) != DIRECTION_NIL &&
        !(actor->state & CS_SPONTANEOUS))
      actor->direction = Direction_back(
          Actor_get_spare_direction(actor)); /* Tank Top Glitch */
    if (!(actor->state & CS_TURNING))
      actor->state |= CS_TURNING | CS_HASMOVED;
    if (actor == invoking_actor)
      continue;
    if (TileID_actor_get_id(Level_cell_get_top_floor(self, actor->pos)) ==
        CREATURE_TANK) {
      Actor_update_floor(actor, self);
    } else if ((actor->state & CS_SPONTANEOUS)) {
      /* handle Spontaneous Generation */
      if (actor->state & CS_TURNING) {
        /* always TRUE? */
        actor->state &= ~CS_TURNING;
        Actor_update_floor(actor, self);
        actor->state |= CS_TURNING;
      }
      actor->direction = Direction_back(
          actor->direction); /* OK with SGG, bad for stacked tanks */
    }
  }
}

/*
 * Maintaining the slip list.
 */

/* Add the given creature to the slip list if it is not already on it
 * (assuming that the given floor is a kind that causes slipping).
 */
static void Actor_start_floor_movement(Actor* self,
                                       Level* level,
                                       TileID floor,
                                       Direction fdir) {
  Direction dir = fdir; /* fdir used with tank reversal when stuck on teleporter */

  self->state &= ~(CS_SLIP | CS_SLIDE);

  if (TileID_is_ice(floor)) {
    if (fdir == DIRECTION_NIL) {
      /* tank reversal patch */
      dir = get_ice_wall_turn_dir(floor, self->direction);
    }
  } else if (TileID_is_slide(floor)) {
    dir = Level_get_slide_dir(level, floor);
  } else if (floor == TILE_TELEPORT) {
    if (fdir == DIRECTION_NIL)
      dir = self->direction; /* tank reversal patch */
  } else if (floor == TILE_TRAP && self->id == CREATURE_BLOCK) {
    dir = self->direction;
  } else if (self->id != CREATURE_CHIP) {
    return; /* new with Convergence Patch */
  } else {
    dir = self->direction; /* new with Convergence Patch */
  }

  if (self->id == CREATURE_CHIP) {
    /* changed with Convergence Patch */
    /* cr->state |= isslide(floor) ? CS_SLIDE : CS_SLIP; */
    self->state |= (TileID_is_ice(floor) || (floor == TILE_TELEPORT && dir != DIRECTION_NIL)) ? CS_SLIP : CS_SLIDE;
    Level_prepend_to_slip_list(level, self, dir);
    self->direction = dir;
    Actor_update_floor(self, level);
  } else {
    self->state |= CS_SLIP;
    Actor_set_spare_direction(self, DIRECTION_NIL);  // tank top glitch
    Level_append_to_slip_list(level, self, dir);
  }
}

/* Remove the given creature from the slip list.
 */
static void Actor_end_floor_movement(Actor* self, Level* level) {
  self->state &= ~(CS_SLIP | CS_SLIDE);
  Level_remove_actor_from_slip_list(level, self);
}

/* Clean out deadwood entries in the slip list.
 */
static void Level_update_sliplist(Level* self) {
  if (self->ms_state.slips_n == 0)
    return;
  for (int64_t n = self->ms_state.slips_n - 1; n >= 0; n -= 1) {
    if (!(self->ms_state.slip_list[n].actor->state & (CS_SLIP | CS_SLIDE))) {
      Actor_end_floor_movement(self->ms_state.slip_list[n].actor, self);
    }
  }
}

/* Including the flag CMM_NOLEAVECHECK in a call to canmakemove()
 * indicates that the tile the creature is moving out of is
 * automatically presumed to permit such movement. CMM_NOEXPOSEWALLS
 * causes blue and hidden walls to remain unexposed.
 * CMM_CLONECANTBLOCK means that the creature will not be prevented
 * from moving by an identical creature standing in the way.
 * CMM_NOPUSHING prevents Chip from pushing blocks inside this
 * function. CMM_TELEPORTPUSH indicates to the block-pushing logic
 * that Chip is teleporting. This prevents a stack of two blocks from
 * being treated as a single block, and allows Chip to push a slipping
 * block away from him. CMM_NOFIRECHECK causes bugs and walkers to not
 * avoid fire. Finally, CMM_NODEFERBUTTONS causes buttons pressed by
 * pushed blocks to take effect immediately.
 */

/* Move a block at the given position forward in the given direction.
 * FALSE is returned if the block cannot be pushed.
 */
static bool Level_push_block(Level* self,
                             Position pos,
                             Direction dir,
                             enum CollisionCheckFlags flags) {
  /* new */
  // _assert(cellat(pos)->top.id == Block_Static);
  // _assert(dir != NIL);

  Actor* cr = Level_look_up_block(self, pos);
  if (!cr) {
    warn("%d: attempt to push disembodied block!", self->current_tick);
    return false;
  }
  bool slipping = (cr->state & (CS_SLIP | CS_SLIDE)); /* accounting */
  if (cr->state & (CS_SLIP | CS_SLIDE)) {
    Direction slipdir = Level_get_actor_slip_dir(self, cr);
    if (dir == slipdir || dir == Direction_back(slipdir)) {
      if (!(flags & CMM_TELEPORTPUSH)) {
        return false;
      }
    }
  }

  if (!(flags & CMM_TELEPORTPUSH) &&
      Level_cell_get_bottom_floor(self, pos) == TILE_BLOCK_STATIC)
    Level_cell_set_bottom_floor(self, pos, TILE_FLOOR);
  if (!(flags & CMM_NODEFERBUTTONS))
    cr->state |= CS_DEFERPUSH;
  bool advanced = Actor_advance_movement(cr, self, dir);
  if (!(flags & CMM_NODEFERBUTTONS))
    cr->state &= ~CS_DEFERPUSH;
  if (!advanced) {
    cr->state &= ~(CS_SLIP | CS_SLIDE);
    if (slipping) {
      /* new MSCC-like accounting */
      self->ms_state.mscc_slippers -= 1;
      Level_remove_actor_from_slip_list(self, cr);
    }
  }
  return advanced;
}

static bool TileID_impedes_move_into(TileID self,
                                     Actor const* actor,
                                     Direction dir) {
  switch (self) {
    case TILE_NOTHING:
    case TILE_WALL:
    case TILE_INVISIBLE_WALL:
    case TILE_TOGGLE_DOOR_CLOSED:
    case TILE_CLONE_MACHINE:
    case TILE_DROWNED_CHIP:
    case TILE_BURNED_CHIP:
    case TILE_BOMBED_CHIP:
    case TILE_EXITED_CHIP:
    case TILE_EXIT_ANIM_1:
    case TILE_EXIT_ANIM_2:
    case TILE_OVERLAY_BUFFER:
    case TILE_UNUSED_1:
    case TILE_UNUSED_2:
    case TILE_ICE_BLOCK:
    case ANIM_WATER:
    case ANIM_BOMB:
    case ANIM_ENTITY:
      return true;

    case TILE_FLOOR:
    case TILE_FORCE_FLOOR_NORTH:
    case TILE_FORCE_FLOOR_WEST:
    case TILE_FORCE_FLOOR_SOUTH:
    case TILE_FORCE_FLOOR_EAST:
    case TILE_ICE:
    case TILE_WATER:
    case TILE_FIRE:
    case TILE_BOMB:
    case TILE_TRAP:
    case TILE_HINT:
    case TILE_BUTTON_TANK:
    case TILE_BUTTON_TOGGLE:
    case TILE_BUTTON_CLONE:
    case TILE_BUTTON_TRAP:
    case TILE_TELEPORT:
    case TILE_TOGGLE_DOOR_OPEN:
    case TILE_KEY_RED:
    case TILE_KEY_BLUE:
    case TILE_KEY_YELLOW:
    case TILE_KEY_GREEN:
      return false;

    case TILE_FORCE_FLOOR_RANDOM:
    case TILE_GRAVEL:
    case TILE_EXIT:
    case TILE_BOOTS_ICE:
    case TILE_BOOTS_FORCE_FLOOR:
    case TILE_BOOTS_FIRE:
    case TILE_BOOTS_WATER:
      return actor->id != CREATURE_CHIP && actor->id != CREATURE_BLOCK;
    case TILE_DIRT:
    case TILE_THIEF:
    case TILE_HIDDEN_WALL:
    case TILE_BLUE_WALL_FAKE:
    case TILE_BLUE_WALL_REAL:
    case TILE_POPUP_WALL:
    case TILE_DOOR_RED:
    case TILE_DOOR_BLUE:
    case TILE_DOOR_YELLOW:
    case TILE_DOOR_GREEN:
    case TILE_SOCKET:
    case TILE_IC_CHIP:
    case TILE_BLOCK_STATIC:
      return actor->id != CREATURE_CHIP;

    case TILE_THIN_WALL_SOUTH_EAST:
    case TILE_ICE_CORNER_NORTH_WEST:  // dir != instead of just dir == because a
                             // NIL can rarely get passed here as a result of tank top
      return dir != DIRECTION_SOUTH && dir != DIRECTION_EAST;
    case TILE_ICE_CORNER_NORTH_EAST:
      return dir != DIRECTION_SOUTH && dir != DIRECTION_WEST;
    case TILE_ICE_CORNER_SOUTH_WEST:
      return dir != DIRECTION_NORTH && dir != DIRECTION_EAST;
    case TILE_ICE_CORNER_SOUTH_EAST:
      return dir != DIRECTION_NORTH && dir != DIRECTION_WEST;
    case TILE_THIN_WALL_NORTH:
      return dir != DIRECTION_NORTH && dir != DIRECTION_EAST &&
             dir != DIRECTION_WEST;
    case TILE_THIN_WALL_EAST:
      return dir != DIRECTION_NORTH && dir != DIRECTION_SOUTH &&
             dir != DIRECTION_EAST;
    case TILE_THIN_WALL_SOUTH:
      return dir != DIRECTION_SOUTH && dir != DIRECTION_EAST &&
             dir != DIRECTION_WEST;
    case TILE_THIN_WALL_WEST:
      return dir != DIRECTION_NORTH && dir != DIRECTION_SOUTH &&
             dir != DIRECTION_WEST;

    default:
      return false;
  }
}

/* Return TRUE if the given creature is allowed to attempt to move in
 * the given direction. Side effects can and will occur from calling
 * this function, as indicated by flags.
 */
static bool Actor_can_make_move(Actor* self,
                                Level* level,
                                Direction dir,
                                enum CollisionCheckFlags flags) {

  if (dir == DIRECTION_NIL) {
    warn("%d: Actor_can_make_move called with DIRECTION_NIL",
         level->current_tick);
  }
  if (!self) {
    warn("%d: Actor_can_make_move called with NULL actor", level->current_tick);
  }

  Position y = self->pos / MAP_WIDTH;
  Position x = self->pos % MAP_WIDTH;
  y += dir == DIRECTION_NORTH ? -1 : dir == DIRECTION_SOUTH ? +1 : 0;
  x += dir == DIRECTION_WEST ? -1 : dir == DIRECTION_EAST ? +1 : 0;
  if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH)
    return false;
  Position to = y * MAP_WIDTH + x;

  if (!(flags & CMM_NOLEAVECHECK)) {
    switch (Level_cell_get_bottom_floor(level, self->pos)) {
      case TILE_THIN_WALL_NORTH:
        if (dir == DIRECTION_NORTH)
          return false;
        break;
      case TILE_THIN_WALL_WEST:
        if (dir == DIRECTION_WEST)
          return false;
        break;
      case TILE_THIN_WALL_SOUTH:
        if (dir == DIRECTION_SOUTH)
          return false;
        break;
      case TILE_THIN_WALL_EAST:
        if (dir == DIRECTION_EAST)
          return false;
        break;
      case TILE_THIN_WALL_SOUTH_EAST:
        if (dir & (DIRECTION_SOUTH | DIRECTION_EAST))
          return false;
        break;
      case TILE_TRAP:
        if (!(self->state & CS_RELEASED))
          return false;
        break;
      default:
        break;
    }
  }

  if (self->id == CREATURE_CHIP) {
    TileID floor = Level_cell_get_terrain(level, to);
    if (TileID_impedes_move_into(floor, self, dir))
      return false;
    if (floor == TILE_SOCKET && level->chips_left > 0)
      return false;
    if (TileID_is_door(floor) && !Level_player_has_item(level, floor))
      return false;
    if (TileID_is_actor(Level_cell_get_top_floor(level, to))) {
      TileID id = TileID_actor_get_id(Level_cell_get_top_floor(level, to));
      if (id == CREATURE_CHIP || id == CREATURE_SWIMMING_CHIP || id == CREATURE_BLOCK)
        return false;
    }
    if (floor == TILE_HIDDEN_WALL || floor == TILE_BLUE_WALL_FAKE) {
      if (!(flags & CMM_NOEXPOSEWALLS))
        Level_cell_set_terrain(level, to, TILE_WALL);
      return false;
    }
    if (floor == TILE_BLOCK_STATIC) {
      if (!Level_push_block(level, to, dir, flags))
        return false;
      else if (flags & CMM_NOPUSHING)
        return false;
      if (Level_cell_get_bottom_floor(level, to) == TILE_CLONE_MACHINE)
        return false; /* totally backwards: need to check this first */
      if ((flags & CMM_TELEPORTPUSH) &&
          Level_cell_get_terrain(level, to) == TILE_BLOCK_STATIC)
        /* totally backwards: remove "&& cellat(to)->bot.id == Empty)" */
        return true;
      return Actor_can_make_move(self, level, dir, flags | CMM_NOPUSHING);
    }
  } else if (self->id == CREATURE_BLOCK) {
    TileID floor = Level_cell_get_top_floor(level, to);
    if (TileID_is_actor(floor)) {
      TileID id = TileID_actor_get_id(floor);
      return id == CREATURE_CHIP || id == CREATURE_SWIMMING_CHIP;
    }
    if (TileID_impedes_move_into(floor, self, dir))
      return false;
  } else {
    TileID floor = Level_cell_get_top_floor(level, to);
    if (TileID_is_actor(floor)) {
      TileID id = TileID_actor_get_id(floor);
      if (id == CREATURE_CHIP || id == CREATURE_SWIMMING_CHIP) {
        floor = Level_cell_get_bottom_floor(level, to);
        if (TileID_is_actor(floor)) {
          id = TileID_actor_get_id(floor);
          return id == CREATURE_CHIP || id == CREATURE_SWIMMING_CHIP;
        }
      }
    }
    if (TileID_is_actor(floor)) {
      /* turning tank cloning patch */
      Actor* F = Level_look_up_creature(level, to, false);
      if (!(flags & CMM_CLONECANTBLOCK)) /* not cloning */
        return false;
      if ((F == NULL || !(F->state & CS_TURNING)) &&
          floor == TileID_actor_with_dir(self->id, self->direction))
        return true;
      /* must check "floor", so same-dir non-creature tank will clone */
      if (F == NULL)
        return false;
      if (F->direction == self->direction)
        return true;
      return false;
    }
    if (TileID_impedes_move_into(floor, self, dir))
      return false;
    if (floor == TILE_FIRE && (self->id == CREATURE_BUG || self->id == CREATURE_WALKER))
      if (!(flags & CMM_NOFIRECHECK))
        return false;
  }

  if (Level_cell_get_bottom_floor(level, to) == TILE_CLONE_MACHINE)
    return false;

  return true;
}

/*
 * How everyone selects their move.
 */

/* This function embodies the movement behavior of all the creatures.
 * Given a creature, this function enumerates its desired direction of
 * movement and selects the first one that is permitted. Note that
 * calling this function also updates the current controller
 * direction.
 */
static void Actor_choose_move_creature(Actor* self, Level* level) {
  self->move_decision = DIRECTION_NIL;

  if (self->hidden)
    return;
  if (self->id == CREATURE_BLOCK)
    return;
  if (level->current_tick & 2)
    return;
  if (self->id == CREATURE_TEETH || self->id == CREATURE_BLOB) {
    if ((level->current_tick + level->init_step_parity) & 4) {
      return;
    }
  }
  if (self->state & CS_TURNING) {
    self->state &= ~(CS_TURNING | CS_HASMOVED);
    Actor_update_floor(self, level);
  }
  if (self->state & CS_HASMOVED) {
    /* should be a stalled tank */
    TileID floor = Level_cell_get_top_floor(level, self->pos); /* stacked tank patch */
    TileID id = TileID_actor_get_id(floor);
    if (TileID_is_actor(floor) && (id == CREATURE_CHIP || id == CREATURE_SWIMMING_CHIP))
      floor = Level_cell_get_bottom_floor(level, self->pos);
    if (!TileID_is_actor(floor) && !TileID_impedes_move_into(floor, self, DIRECTION_NIL))
      self->hidden = true; /* hack with (0,0) movement success */
    /* maybe should check if (0,0) move goes on sliplist, but that's UB */
  }
  if (self->state & CS_HASMOVED) {
    level->ms_state.controller_dir = DIRECTION_NIL;
    return;
  }
  if (self->state & (CS_SLIP | CS_SLIDE))
    return;

  TileID floor = Level_cell_get_terrain(level, self->pos);

  Direction dir;
  Direction pdir = dir = self->direction;
  Direction choices[4] = {DIRECTION_NIL, DIRECTION_NIL, DIRECTION_NIL,
                        DIRECTION_NIL};

  if (floor == TILE_CLONE_MACHINE || floor == TILE_TRAP) {
    switch (self->id) {
      case CREATURE_TANK:
      case CREATURE_BALL:
      case CREATURE_GLIDER:
      case CREATURE_FIREBALL:
      case CREATURE_WALKER:
        choices[0] = dir;
        break;
      case CREATURE_BLOB:
        choices[0] = dir;
        choices[1] = Direction_left(dir);
        choices[2] = Direction_back(dir);
        choices[3] = Direction_right(dir);
        Prng_permute4(&level->prng, choices, sizeof(Direction));
        break;
      case CREATURE_BUG:
      case CREATURE_PARAMECIUM:
      case CREATURE_TEETH:
        choices[0] = level->ms_state.controller_dir;
        self->move_decision = level->ms_state.controller_dir;
        return;
      default:
        warn("%d: Non-creature %02X at (%d, %d) trying to move",
             level->current_tick, self->pos % MAP_WIDTH, self->pos / MAP_WIDTH,
             self->id);
        break;
    }
  } else {
    switch (self->id) {
      case CREATURE_TANK:
        choices[0] = dir;
        break;
      case CREATURE_BALL:
        choices[0] = dir;
        choices[1] = Direction_back(dir);
        break;
      case CREATURE_GLIDER:
        choices[0] = dir;
        choices[1] = Direction_left(dir);
        choices[2] = Direction_right(dir);
        choices[3] = Direction_back(dir);
        break;
      case CREATURE_FIREBALL:
        choices[0] = dir;
        choices[1] = Direction_right(dir);
        choices[2] = Direction_left(dir);
        choices[3] = Direction_back(dir);
        break;
      case CREATURE_WALKER:
        choices[0] = dir;
        choices[1] = Direction_left(dir);
        choices[2] = Direction_back(dir);
        choices[3] = Direction_right(dir);
        Prng_permute3(&level->prng, choices + 1, sizeof(Direction));
        break;
      case CREATURE_BLOB:
        choices[0] = dir;
        choices[1] = Direction_left(dir);
        choices[2] = Direction_back(dir);
        choices[3] = Direction_right(dir);
        Prng_permute4(&level->prng, choices, sizeof(Direction));
        break;
      case CREATURE_BUG:
        choices[0] = Direction_left(dir);
        choices[1] = dir;
        choices[2] = Direction_right(dir);
        choices[3] = Direction_back(dir);
        break;
      case CREATURE_PARAMECIUM:
        choices[0] = Direction_right(dir);
        choices[1] = dir;
        choices[2] = Direction_left(dir);
        choices[3] = Direction_back(dir);
        break;
      case CREATURE_TEETH:
        Position y = Level_get_chip(level)->pos / MAP_WIDTH - self->pos / MAP_WIDTH;
        Position x = Level_get_chip(level)->pos % MAP_WIDTH - self->pos % MAP_WIDTH;
        Direction n = y < 0 ? DIRECTION_NORTH : y > 0 ? DIRECTION_SOUTH : DIRECTION_NIL;
        if (y < 0)
          y = -y;
        Direction m = x < 0 ? DIRECTION_WEST : x > 0 ? DIRECTION_EAST : DIRECTION_NIL;
        if (x < 0)
          x = -x;
        if (x > y) {
          choices[0] = m;
          choices[1] = n;
        } else {
          choices[0] = n;
          choices[1] = m;
        }
        pdir = choices[2] = choices[0];
        break;
      default:
        warn("%d: Non-creature %02X at (%d, %d) trying to move",
             level->current_tick, self->pos % MAP_WIDTH, self->pos / MAP_WIDTH,
             self->id);
        // _assert(!"Unknown creature trying to move");
        break;
    }
  }

  for (size_t n = 0; n < 4 && choices[n] != DIRECTION_NIL; n += 1) {
    self->move_decision = choices[n];
    level->ms_state.controller_dir = self->move_decision;
    if (Actor_can_make_move(self, level, choices[n], 0))
      return;
  }

  if (self->id == CREATURE_TANK) {
    if ((self->state & CS_RELEASED) ||
      (floor != TILE_TRAP /*&& floor != CloneMachine*/)) /* (c) bug: tank clones should stall */
      self->state |= CS_HASMOVED;
    self->move_decision = DIRECTION_NIL; /* handle stacked tanks */
  }

  if (self->id != CREATURE_TANK) /* handle stacked tanks */
    self->move_decision = pdir;
}

/* Select a direction for Chip to move towards the goal position.
 */
static Direction Level_get_chip_mouse_direction(Level* level) {
  if (!Level_has_mouse_goal(level))
    return DIRECTION_NIL;
  Actor* chip = Level_get_chip(level);
  if (Level_get_mouse_goal(level) == chip->pos) {
    Level_cancel_mouse_goal(level);
    return DIRECTION_NIL;
  }

  Position y = Level_get_mouse_goal(level) / MAP_WIDTH - chip->pos / MAP_WIDTH;
  Position x = Level_get_mouse_goal(level) % MAP_WIDTH - chip->pos % MAP_WIDTH;
  Direction dir;
  Direction d1 = y < 0   ? DIRECTION_NORTH
                 : y > 0 ? DIRECTION_SOUTH
                         : DIRECTION_NIL;
  if (y < 0)
    y = -y;
  Direction d2 = x < 0   ? DIRECTION_WEST
                 : x > 0 ? DIRECTION_EAST
                         : DIRECTION_NIL;
  if (x < 0)
    x = -x;
  if (x > y) {
    dir = d1;
    d1 = d2;
    d2 = dir;
  }
  if (d1 != DIRECTION_NIL && d2 != DIRECTION_NIL)
    dir = Actor_can_make_move(chip, level, d1, 0) ? d1 : d2;
  else
    dir = d2 == DIRECTION_NIL ? d1 : d2;

  return dir;
}

/* Unpack a Chip-relative map location.
 */
static Position Level_chip_rel_position_to_absolute(Actor const* chip, Position relpos) {
  Position x = relpos % MOUSE_RANGE + MOUSE_RANGE_MIN;
  Position y = relpos / MOUSE_RANGE + MOUSE_RANGE_MIN;
  return chip->pos + y * MAP_WIDTH + x;
}

/* Determine the direction of Chip's next move. If discard is TRUE,
 * then Chip is not currently permitted to select a direction of
 * movement, and the player's input should not be retained.
 */
static void Actor_choose_move_chip(Actor* chip, Level* level, bool discard) {
  chip->move_decision = DIRECTION_NIL;

  if (chip->hidden)
    return;

  if (!(level->current_tick & 3))
    chip->state &= ~CS_HASMOVED;
  if (chip->state & CS_HASMOVED) {
    if (level->game_input != INPUT_NIL && Level_has_mouse_goal(level)) {
      Level_cancel_mouse_goal(level);
    }
    return;
  }

  GameInput input = level->game_input;
  if (discard || ((chip->state & CS_SLIDE) && GameInput_to_direction(input) == chip->direction)) {
    if (level->current_tick && !(level->current_tick & 1))
      Level_cancel_mouse_goal(level);
    return;
  }

  if (GameInput_is_mouse_move(input)) {
    Level_set_mouse_goal(level, Level_chip_rel_position_to_absolute(chip, input - GAME_INPUT_MOUSE_MOVE_FIRST));
    input = INPUT_NIL;
  } else {
    if (GameInput_is_diagonal(input)) {
      input &= INPUT_NORTH | INPUT_SOUTH;
    }
    if ((input & INPUT_NORTH) && (input & INPUT_SOUTH)) {
      input &= INPUT_NORTH;
    }
  }

  if (input == INPUT_NIL && Level_has_mouse_goal(level) && (level->current_tick & 3) == 2)
    input = GameInput_from_direction(Level_get_chip_mouse_direction(level));

  chip->move_decision = GameInput_to_direction(input);
}

/* Teleport the given creature instantaneously from the teleport tile
 * at start to another teleport tile (if possible).
 */
static Position Actor_teleport(Actor* self, Level* level, Position start) {
  Direction origdir = self->direction; /* tank push IB onto blue button via teleporter */
  if (self->direction == DIRECTION_NIL) {
    warn("%d: directionless creature %02X on teleport at (%d %d)",
         level->current_tick, self->id, self->pos % MAP_WIDTH,
         self->pos / MAP_WIDTH);
  } else if (self->hidden) {
    warn("%d: hidden creature %02X on teleport at (%d %d)", level->current_tick,
         self->id, self->pos % MAP_WIDTH, self->pos / MAP_WIDTH);
  }

  Position origpos = self->pos;
  Position dest = start;

  for (;;) {
    dest -= 1;
    if (dest < 0)
      dest += MAP_WIDTH * MAP_HEIGHT;
    if (dest == start)
      break;
    MapTile* tile = MapCell_get_top_tile(Level_get_map_cell(level, dest));
    if (MapTile_get_floor(tile) != TILE_TELEPORT ||
        (MapTile_get_state(tile) & FS_BROKEN))
      continue;
    self->pos = dest;
    bool can_move = Actor_can_make_move(self, level, self->direction,
                                 CMM_NOLEAVECHECK | CMM_NOEXPOSEWALLS |
                                     CMM_NODEFERBUTTONS | CMM_NOFIRECHECK |
                                     CMM_TELEPORTPUSH);
    self->direction = origdir; /* tank push IB onto blue button via teleporter */
    self->pos = origpos;
    if (can_move)
      break;
  }

  return dest;
}

/* Determine the move(s) a creature will make on the current tick.
 */
static void Actor_choose_move(Actor* self, Level* level) {
  if (self->id == CREATURE_CHIP) {
    Actor_choose_move_chip(self, level, self->state & CS_SLIP);
  } else {
    if (self->state & CS_SLIP)
      self->move_decision = DIRECTION_NIL;
    else
      Actor_choose_move_creature(self, level);
  }
}

/* Initiate the cloning of a creature.
 */
static void Level_activate_cloner(Level* self, Position button_pos) {
  Position pos = Level_locate_cloner_by_button(self, button_pos);
  if (pos < 0 || pos >= MAP_WIDTH * MAP_HEIGHT)
    return;
  TileID tileid = Level_cell_get_top_floor(self, pos);
  if (!TileID_is_actor(tileid) || TileID_actor_get_id(tileid) == CREATURE_CHIP)
    return;
  if (TileID_actor_get_id(tileid) == CREATURE_BLOCK) {
    Actor* actor = Level_look_up_block(self, pos);
    if (!actor) {
      warn("%d: attempt to clone disembodied block!", self->current_tick);
      return;
    }
    if (actor->direction != DIRECTION_NIL) {
      Actor_advance_movement(actor, self, actor->direction);
    }
  } else {
    if (MapTile_get_state(MapCell_get_bottom_tile(Level_get_map_cell(self, pos))) & FS_CLONING) {
      return;
    }
    Actor dummy = {0};
    dummy.id = TileID_actor_get_id(tileid);
    dummy.direction = TileID_actor_get_dir(tileid);
    dummy.pos = pos;
    if (!Actor_can_make_move(&dummy, self, dummy.direction, CMM_CLONECANTBLOCK)) {
      return;
    }
    Actor* actor = Level_awaken_creature(self, pos);
    if (!actor) {
      return;
    }
    actor->state |= CS_CLONING;
    if (Level_cell_get_bottom_floor(self, pos) == TILE_CLONE_MACHINE) {
      MapTile_add_cloning_state(MapCell_get_bottom_tile(Level_get_map_cell(self, pos)));
    }
  }
}

/* Open a bear trap. Any creature already in the trap is released.
 */
static void Level_spring_trap(Level* self, Position buttonpos) {
  Position pos = Level_locate_trap_by_button(self, buttonpos);
  if (pos < 0)
    return;
  if (pos >= MAP_WIDTH * MAP_HEIGHT) {
    warn("%d: Off-map trap opening attempted: (%d %d)", self->current_tick,
         pos % MAP_WIDTH, pos / MAP_WIDTH);
    return;
  }
  TileID id = Level_cell_get_top_floor(self, pos);
  if (id == TILE_BLOCK_STATIC || (MapTile_get_state(MapCell_get_bottom_tile(
                                 Level_get_map_cell(self, pos))) &
                             FS_HASMUTANT)) {
    Actor* actor = Level_look_up_block(self, pos);
    if (actor) {
      actor->state |= CS_RELEASED;
    }
  } else if (TileID_is_actor(id)) {
    Actor* actor = Level_look_up_creature(self, pos, true);
    if (actor) {
      actor->state |= CS_RELEASED;
    }
  }
}

/* Mark all buttons everywhere as having been handled.
 */
static void Level_reset_buttons(Level* self) {
  for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
    MapCell* cell = Level_get_map_cell(self, pos);
    MapTile_remove_button_down_state(&cell->top);
    MapTile_remove_button_down_state(&cell->bottom);
  }
}

/* Apply the effects of all deferred button presses, if any.
 */
static void Level_handle_buttons(Level* self) {
  TileID id;

  for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
    MapCell* cell = Level_get_map_cell(self, pos);
    MapTile* top_tile = MapCell_get_top_tile(cell);
    MapTile* bottom_tile = MapCell_get_bottom_tile(cell);
    if (MapTile_get_state(top_tile) & FS_BUTTONDOWN) {
      MapTile_remove_button_down_state(top_tile);
      id = MapTile_get_floor(top_tile);
    } else if (MapTile_get_state(bottom_tile) & FS_BUTTONDOWN) {
      MapTile_remove_button_down_state(bottom_tile);
      id = MapTile_get_floor(bottom_tile);
    } else {
      continue;
    }
    switch (id) {
      case TILE_BUTTON_TANK:
        Level_add_sfx(self, SND_BUTTON_PUSHED);
        Level_turn_tanks(self, NULL);
        break;
      case TILE_BUTTON_TOGGLE:
        Level_toggle_walls(self);
        break;
      case TILE_BUTTON_CLONE:
        Level_activate_cloner(self, pos);
        Level_add_sfx(self, SND_BUTTON_PUSHED);
        break;
      case TILE_BUTTON_TRAP:
        Level_spring_trap(self, pos);
        Level_add_sfx(self, SND_BUTTON_PUSHED);
        break;
      default:
        warn("%d: Fooey! Tile %02X is not a button!", self->current_tick, id); //Who even says fooey/phooey?
        break;
    }
  }
}

/*
 * When something actually moves.
 */

/* Initiate a move by the given creature in the given direction.
 * Return FALSE if the creature cannot initiate the indicated move
 * (side effects may still occur).
 */
static bool Actor_start_movement(Actor* self, Level* level, Direction dir) {
  TileID floor = Level_cell_get_bottom_floor(level, self->pos);
  Direction odir = self->direction; /* b2 fix with convergence glitch */

  if (dir == DIRECTION_NIL) {
    warn("%d: Actor_start_movement called with DIRECTION_NIL", level->current_tick);
  }

  if (!Actor_can_make_move(self, level, dir, 0)) {
    if (self->id == CREATURE_CHIP || (floor != TILE_TRAP && floor != TILE_CLONE_MACHINE &&
                             !(self->state & CS_SLIP))) {
      if (self->id != CREATURE_CHIP || odir != DIRECTION_NIL)
        self->direction = dir; /* b2 fix */
      Actor_update_floor(self, level);
    }
    return false;
  }

  if (floor == TILE_TRAP) {
    if (!(self->state & CS_RELEASED)) {
      warn("%d: Actor_start_movement from a beartrap without CS_RELEASED set", level->current_tick);
    }
    if (self->state & CS_MUTANT) {
      MapTile_remove_mutant_state(&Level_get_map_cell(level, self->pos)->bottom);
    }
  }
  self->state &= ~CS_RELEASED;

  self->direction = dir;

  return true;
}

/* Complete the movement of the given creature. Most side effects
 * produced by moving onto a tile occur at this point. This function
 * is also the only place where a creature can be added to the slip
 * list.
 */
static void Actor_end_movement(Actor* self, Level* level, Direction dir) {
  bool dead = false;
  bool blockcloning = false; /* Squish patch */

  Position oldpos = self->pos;
  Position newpos = Position_neighbor(self->pos, dir);

  MapCell* cell = Level_get_map_cell(level, newpos);
  MapTile* tile = MapCell_get_top_tile(cell);
  TileID floor = MapTile_get_floor(tile);
  TileID actor_id_top = TileID_actor_get_id(Level_cell_get_top_floor(level, oldpos)); /* Non-existence patch */
  TileID floor_bottom = MapTile_get_floor(tile);
  if (self->id == CREATURE_CHIP) {
    switch (floor) {
      case TILE_FLOOR:
        MapCell_pop_tile(cell);
        break;
      case TILE_WATER:
        if (!Level_player_has_item(level, floor))
          level->ms_state.chip_status = CHIP_DROWNED;
        break;
      case TILE_FIRE:
        if (!Level_player_has_item(level, floor))
          level->ms_state.chip_status = CHIP_BURNED;
        break;
      case TILE_DIRT:
        MapCell_pop_tile(cell);
        break;
      case TILE_BLUE_WALL_REAL:
        MapCell_pop_tile(cell);
        break;
      case TILE_POPUP_WALL:
        tile->id = TILE_WALL;
        break;
      case TILE_DOOR_RED:
      case TILE_DOOR_BLUE:
      case TILE_DOOR_YELLOW:
      case TILE_DOOR_GREEN:
        if (!Level_player_has_item(level, floor)) {
          warn("%d: Player entered door %0X without key!", level->current_tick, floor);
        }
        if (floor != TILE_DOOR_GREEN && Level_player_has_item(level, floor)) {
          (*Level_player_item_ptr(level, floor)) -= 1;
        }
        MapCell_pop_tile(cell);
        Level_add_sfx(level, SND_DOOR_OPENED);
        break;
      case TILE_BOOTS_ICE:
      case TILE_BOOTS_FORCE_FLOOR:
      case TILE_BOOTS_FIRE:
      case TILE_BOOTS_WATER:
      case TILE_KEY_RED:
      case TILE_KEY_BLUE:
      case TILE_KEY_YELLOW:
      case TILE_KEY_GREEN:
        if (TileID_is_actor(floor_bottom))
          level->ms_state.chip_status = CHIP_COLLIDED;
        *Level_player_item_ptr(level, floor) += 1;
        MapCell_pop_tile(cell);
        Level_add_sfx(level, SND_ITEM_COLLECTED);
        break;
      case TILE_THIEF:
        *Level_player_item_ptr(level, TILE_BOOTS_ICE) = 0;
        *Level_player_item_ptr(level, TILE_BOOTS_FORCE_FLOOR) = 0;
        *Level_player_item_ptr(level, TILE_BOOTS_FIRE) = 0;
        *Level_player_item_ptr(level, TILE_BOOTS_WATER) = 0;
        Level_add_sfx(level, SND_BOOTS_STOLEN);
        break;
      case TILE_IC_CHIP:
        if (level->chips_left)
          level->chips_left -= 1;
        MapCell_pop_tile(cell);
        Level_add_sfx(level, SND_IC_COLLECTED);
        break;
      case TILE_SOCKET:
        if (level->chips_left)
          warn("%d: Entered socket with IC Chips still remaining",
               level->current_tick);
        MapCell_pop_tile(cell);
        Level_add_sfx(level, SND_SOCKET_OPENED);
        break;
      case TILE_BOMB:
        level->ms_state.chip_status = CHIP_BOMBED;
        Level_add_sfx(level, SND_BOMB_EXPLODES);
        break;
      default:
        if (TileID_is_actor(floor))
          level->ms_state.chip_status = CHIP_COLLIDED;
        break;
    }
  } else if (self->id == CREATURE_BLOCK) {
    switch (floor) {
      case TILE_FLOOR:
        MapCell_pop_tile(cell);
        break;
      case TILE_WATER:
        MapTile_set_floor(tile, TILE_DIRT);
        dead = true;
        Level_add_sfx(level, SND_WATER_SPLASH);
        break;
      case TILE_BOMB:
        MapTile_set_floor(tile, TILE_FLOOR);
        dead = true;
        Level_add_sfx(level, SND_BOMB_EXPLODES);
        break;
      case TILE_TELEPORT:
        if (!(MapTile_get_state(tile) & FS_BROKEN))
          newpos = Actor_teleport(self, level, newpos);
        break;
      default:
        break;
    }
    TileID id = Level_cell_get_top_floor(level, oldpos);
    if (TileID_is_actor(id) && TileID_actor_get_id(id) == CREATURE_CHIP) {
      self->state |= CS_MUTANT;
    }
  } else {
    if (TileID_is_actor(floor)) {
      tile = MapCell_get_bottom_tile(cell);
      floor = MapTile_get_floor(tile);
    }
    switch (floor) {
      case TILE_WATER:
        if (actor_id_top != CREATURE_GLIDER) /* use actor_id_top with Non-existence patch */
          dead = true;
        break;
      case TILE_FIRE:
        if (actor_id_top != CREATURE_FIREBALL) /* use actor_id_top with Non-existence patch */
          dead = true;
        break;
      case TILE_BOMB:
        MapTile_set_floor(tile, TILE_FLOOR);
        dead = true;
        Level_add_sfx(level, SND_BOMB_EXPLODES);
        break;
      case TILE_TELEPORT:
        if (!(MapTile_get_state(tile) & FS_BROKEN))
          newpos = Actor_teleport(self, level, newpos);
        break;
      default:
        break;
    }
  }

  MapCell* old_cell = Level_get_map_cell(level, oldpos);
  if (MapCell_get_bottom_floor(old_cell) != TILE_CLONE_MACHINE || self->id == CREATURE_CHIP)
    MapCell_pop_tile(old_cell);
  if (dead) {
    Actor_remove(self, level);
    if (MapCell_get_bottom_floor(old_cell) == TILE_CLONE_MACHINE) {
      MapTile_remove_cloning_state(MapCell_get_bottom_tile(old_cell));
    }
    return;
  }

  if (self->id == CREATURE_CHIP && floor == TILE_TELEPORT && !(tile->state & FS_BROKEN)) {
    Position i = newpos;
    newpos = Actor_teleport(self, level, newpos);
    if (true || newpos != i) {
      /* Convergence Patch */
      /* no idea, but Icysanity lvl 1 requires newpos=i to work */
      Level_add_sfx(level, SND_TELEPORTING);
      if (Level_cell_get_terrain(level, newpos) == TILE_BLOCK_STATIC) {
        if (level->ms_state.chip_last_slip_dir == DIRECTION_NIL) {
          /* // these seem cosmetic/superfluous with new patch
          cr->dir = NORTH;
          cellat(newpos)->top.id = crtile(Chip, NORTH);
          */
          self->direction = DIRECTION_NIL; /* Convergence Patch */
          /* floor = Empty; */
          /* Removed with Convergence Patch, cf Chip on sliplist */
        } else {
          /* seems ok still, with new Convergence logic */
          self->direction = level->ms_state.chip_last_slip_dir;
        }
      }
    }
  }

  self->pos = newpos;
  Actor_add_to_map(self, level);
  self->pos = oldpos;

  tile = MapCell_get_bottom_tile(cell);
  switch (floor) {
    case TILE_BUTTON_TANK:
      if (self->state & CS_DEFERPUSH)
        MapTile_add_button_down_state(tile);
      else
        Level_turn_tanks(level, self);
      Level_add_sfx(level, SND_BUTTON_PUSHED);
      break;
    case TILE_BUTTON_TOGGLE:
      if (self->state & CS_DEFERPUSH)
        MapTile_add_button_down_state(tile);
      else
        Level_toggle_walls(level);
      break;
    case TILE_BUTTON_CLONE:
      self->state |= CS_SPONTANEOUS;
      if (self->state & CS_DEFERPUSH)
        MapTile_add_button_down_state(tile);
      else
        Level_activate_cloner(level, newpos);
      Level_add_sfx(level, SND_BUTTON_PUSHED);
      self->state &= ~CS_SPONTANEOUS; /* Hack with SGG */
      break;
    case TILE_BUTTON_TRAP:
      if (self->state & CS_DEFERPUSH)
        MapTile_add_button_down_state(tile);
      else
        Level_spring_trap(level, newpos);
      Level_add_sfx(level, SND_BUTTON_PUSHED);
      break;
    default:
      break;
  }
  self->pos = newpos;

  if (MapCell_get_bottom_floor(old_cell) == TILE_CLONE_MACHINE && self->id == CREATURE_BLOCK &&
      MapCell_get_top_floor(old_cell) != TILE_BLOCK_STATIC)
    blockcloning = true; /* Squish patch */

  if (MapCell_get_bottom_floor(old_cell) == TILE_CLONE_MACHINE)
    MapTile_remove_cloning_state(MapCell_get_bottom_tile(old_cell));

  if (floor == TILE_TRAP) {
    if (Level_is_trap_open(level, newpos, oldpos))
      self->state |= CS_RELEASED;
  } else if (Level_cell_get_bottom_floor(level, newpos) == TILE_TRAP) {
    for (size_t i = 0; i < level->trap_connections.length; i += 1) {
      if (level->trap_connections.items[i].to == newpos) {
        self->state |= CS_RELEASED;
        break;
      }
    }
  }

  if (self->id == CREATURE_CHIP) {
    if (Level_get_mouse_goal(level) == self->pos)
      Level_cancel_mouse_goal(level);
    if (level->ms_state.chip_status != CHIP_OKAY &&
        level->ms_state.chip_status != CHIP_SQUISHED)
      return; /* CHIP_SQUISHED added with Squish patch */
    if (MapCell_get_bottom_floor(cell) == TILE_EXIT) {
      level->level_complete = true;
      return;
    }
  } else {
    if (TileID_is_actor(MapCell_get_bottom_floor(cell))) {
      TileID id = MapCell_get_bottom_floor(cell);
      if (TileID_actor_get_id(id) == CREATURE_CHIP ||
          TileID_actor_get_id(id) == CREATURE_SWIMMING_CHIP) {
        if (self->id != CREATURE_BLOCK || !blockcloning) /* Squish patch */
          level->ms_state.chip_status = CHIP_COLLIDED;
        else
          level->ms_state.chip_status = CHIP_SQUISHED; /* Squish patch */
        return;
      }
    }
  }

  bool was_slipping = self->state & (CS_SLIP | CS_SLIDE);

  if (floor == TILE_TELEPORT) {
    Actor_start_floor_movement(self, level, floor, DIRECTION_NIL); /* NIL for tank reversal patch */
  } else if (TileID_is_ice(floor) && (self->id != CREATURE_CHIP || !Level_player_has_item(level, TILE_BOOTS_ICE))) {
    Actor_start_floor_movement(self, level, floor, DIRECTION_NIL); /* NIL for tank reversal patch */
  } else if (TileID_is_slide(floor) && (self->id != CREATURE_CHIP || !Level_player_has_item(level, TILE_BOOTS_FORCE_FLOOR))) {
    Actor_start_floor_movement(self, level, floor, DIRECTION_NIL); /* NIL for tank reversal patch */
  } else if (floor == TILE_TRAP && self->id == CREATURE_BLOCK && was_slipping) {
    Actor_start_floor_movement(self, level, floor, DIRECTION_NIL); /* NIL for tank reversal patch */
    if (self->state & CS_MUTANT) {
      MapTile_add_mutant_state(MapCell_get_bottom_tile(cell));
    }
  } else {
    /* changes for MSCC-style sliplist */
    self->state &= ~(CS_SLIP | CS_SLIDE);
    if (was_slipping && self->id != CREATURE_CHIP) {
      level->ms_state.mscc_slippers -= 1;
      Level_remove_actor_from_slip_list(level, self);
    }
  }
  if (!was_slipping && (self->state & (CS_SLIP | CS_SLIDE)) && self->id != CREATURE_CHIP)
    level->ms_state.controller_dir = Level_get_actor_slip_dir(level, self);
}

/* Move the given creature in the given direction.
 */
static bool Actor_advance_movement(Actor* self, Level* level, Direction dir) {
  if (dir == DIRECTION_NIL)
    return true;

  if (self->id == CREATURE_CHIP)
    level->ms_state.chip_ticks_since_moved = 0;

  if (!Actor_start_movement(self, level, dir)) {
    if (self->id == CREATURE_CHIP) {
      Level_add_sfx(level, SND_CANT_MOVE);
      Level_reset_buttons(level);
      Level_cancel_mouse_goal(level);
    }
    return false;
  }

  Actor_end_movement(self, level, dir);
  if (self->id == CREATURE_CHIP)
    Level_handle_buttons(level);

  return true;
}

/*
 * Automatic activities.
 */

/* Execute all forced moves for creatures on the slip list. (Note the
 * use of the savedcount variable, which is how slide delay is
 * implemented.)
 */
static void Level_chip_floor_movements(Level* self) { /* split into two */
  for (uint32_t n = 0; n < self->ms_state.slips_n; n += 1) {
    Actor* actor = self->ms_state.slip_list[n].actor;
    if (!(actor->state & (CS_SLIP | CS_SLIDE)))
      continue;
    if (actor->id != CREATURE_CHIP)
      continue; /* new, non-Chip ignored */
    Direction slipdir = self->ms_state.slip_list[n].direction;
    if (slipdir == DIRECTION_NIL) { /* Convergence Patch */
      Level_cell_set_top_floor(self, actor->pos, TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_NORTH));
      continue;
    }
    self->ms_state.chip_last_slip_dir = slipdir;
    bool advanced = Actor_advance_movement(actor, self, slipdir); /* useful to have advanced */
    if (advanced) {
      actor->state &= ~CS_HASMOVED;
    } else {
      TileID floor = Level_cell_get_bottom_floor(self, actor->pos);
      if (TileID_is_slide(floor)) {
        actor->state &= ~CS_HASMOVED;
      } else if (TileID_is_ice(floor)) {
        slipdir = get_ice_wall_turn_dir(floor, Direction_back(slipdir));
        self->ms_state.chip_last_slip_dir = slipdir;
        advanced = Actor_advance_movement(actor, self, slipdir); /* again useful with ac */
        if (advanced)
          actor->state &= ~CS_HASMOVED;
      } else if (floor == TILE_TELEPORT || floor == TILE_BLOCK_STATIC) {
        self->ms_state.chip_last_slip_dir = slipdir = Direction_back(slipdir);
        if (Actor_advance_movement(actor, self, slipdir))
          actor->state &= ~CS_HASMOVED;
      }
      if (actor->state & (CS_SLIP | CS_SLIDE)) {
        Actor_end_floor_movement(actor, self);
        Actor_start_floor_movement(
            actor, self,
            Level_cell_get_bottom_floor(self, actor->pos),
            DIRECTION_NIL); /* 3rd argument with tank reversal patch */
      }
    }
    if (Level_check_for_ending(self))
      return;
    // todo: we can probably add a break/return here, I don't believe its possible to ever have more than one Chip around
    //  do try and confirm though
  }
}

static void Level_non_chip_floor_movements(Level* self) { /* split into two */
  int64_t advance = 0;

  for (uint32_t n = 0; n < self->ms_state.slips_n;) {
    uint32_t oldmsccslippers = self->ms_state.mscc_slippers;
    Actor* actor = self->ms_state.slip_list[n].actor;
    if (actor->id == CREATURE_CHIP) {
      /* new splitting */
      n += 1;
      continue;
    }
    if (advance) {
      advance -= 1;
      n += 1;
      continue;
    }
    if (!(self->ms_state.slip_list[n].actor->state & (CS_SLIP | CS_SLIDE))) {
      n += 1;
      continue;
    }
    Direction slipdir = self->ms_state.slip_list[n].direction;
    Direction origdir = slipdir; /* tank reversal patch */
    if (slipdir == DIRECTION_NIL) {
      n += 1;
      continue;
    }
    Actor_set_spare_direction(actor, actor->direction); /* Tank Top Glitch */
    bool advanced = Actor_advance_movement(actor, self, slipdir); /* useful to have advanced */
    if (!advanced) {
      TileID floor = Level_cell_get_bottom_floor(self, actor->pos);
      if (TileID_is_ice(floor)) {
        slipdir = get_ice_wall_turn_dir(floor, Direction_back(slipdir));
        advanced = Actor_advance_movement(actor, self, slipdir); /* again useful with ac */
      }
      if (actor->state & (CS_SLIP | CS_SLIDE)) {
        Actor_end_floor_movement(actor, self);
        self->ms_state.mscc_slippers -= 1; /* new MSCC accounting */
        Actor_start_floor_movement(actor, self, Level_cell_get_bottom_floor(self, actor->pos), advanced ? DIRECTION_NIL : origdir); /* 3rd argument with tank reversal patch */
      }
    }
    if (actor->state & CS_SLIP && advanced) {
      actor->state |= CS_SLIDE; /* Tank Top Glitch */
    }
    Actor_set_spare_direction(actor, DIRECTION_NIL);  // tank top glitch
    if (Level_check_for_ending(self)) {
      return;
    }
    if (self->ms_state.mscc_slippers == oldmsccslippers) {
      advance += 1;
    }
  }
}

static void Level_do_floor_movements(Level* self) { /* split version with patch */
  Level_chip_floor_movements(self);
  Level_update_sliplist(self); /* remove deadwood */
  /* TSG stuff, not yet included */
  if (!Level_check_for_ending(self)) /* Squish patch (maybe was oversight?) */
    Level_non_chip_floor_movements(self);
  if (!self->level_complete && self->ms_state.chip_status == CHIP_SQUISHED)
    self->ms_state.chip_status = CHIP_SQUISHED_DEATH;
}

static void Level_create_clones(Level* self) {
  for (uint32_t n = 0; n < self->actors_n; n += 1) {
    if (self->actors[n].state & CS_CLONING) {
      self->actors[n].state &= ~CS_CLONING;
    }
  }
}

static void Level_check_and_clear_actors(Level* self) {
  // WIDTH * HEIGHT + 1 so that it's ensured it won't fire unless we're well over max possible alive creatures
  if (self->actors_n <= MAP_WIDTH * MAP_HEIGHT + 1) {
    return;
  }
  // warn("%d: filled the actor array, removing dead creatures", self->current_tick);
  size_t i = 0;
  while (i < self->actors_n) {
    Actor* actor = &self->actors[i];
    if (actor->hidden) {
      memmove(&self->actors[i], &self->actors[i + 1], (self->actors_n - i - 1) * sizeof(Actor));
      self->actors_n -= 1;
      for (size_t j = 0; j < self->ms_state.slips_n; j += 1) { // Adjust sliplist entries
        MsSlipper* slipper = &self->ms_state.slip_list[j];
        if (slipper->actor - self->actors >= i) {
          slipper->actor -= 1;
        }
      }
    } else {
      i += 1;
    }
  }
}

static bool ms_init_level(Level* self) {
  self->actors_n = 0;
  memset(self->actors, 0, sizeof(self->actors));
  self->ms_state.slips_n = 0;
  memset(self->ms_state.slip_list, 0, sizeof(self->ms_state.slip_list));

  self->status_flags &= ~SF_BAD_TILES;
  self->status_flags |= SF_NO_ANIMATION;

  Position pos = 0;
  while (pos < MAP_WIDTH * MAP_HEIGHT) {
    MapCell* cell = Level_get_map_cell(self, pos);
    if (TileID_is_terrain(MapCell_get_top_floor(cell)) ||
        TileID_actor_get_id(MapCell_get_top_floor(cell)) == CREATURE_CHIP ||
        TileID_actor_get_id(MapCell_get_top_floor(cell)) == CREATURE_BLOCK) {
      if (MapCell_get_bottom_floor(cell) == TILE_TELEPORT || MapCell_get_bottom_floor(cell) == TILE_TOGGLE_DOOR_OPEN ||
          MapCell_get_bottom_floor(cell) == TILE_TOGGLE_DOOR_CLOSED) {
        MapTile_add_broken_state(MapCell_get_bottom_tile(cell));
      }
    }
    pos += 1;
    cell += 1;
  }

  Actor* chip = Level_create_actor(self);
  chip->pos = 0;
  chip->id = CREATURE_CHIP;
  chip->direction = DIRECTION_SOUTH;
  for (uint32_t n = 0; n < self->ms_state.init_actors_n; n += 1) {
    pos = self->ms_state.init_actor_list[n];
    if (pos < 0 || pos >= MAP_WIDTH * MAP_HEIGHT) {
      warn("level has invalid creature location (%d %d)", pos % MAP_WIDTH,
           pos / MAP_WIDTH);
      continue;
    }
    MapCell* cell = Level_get_map_cell(self, pos);
    TileID top_id = Level_cell_get_top_floor(self, pos);
    TileID bottom_id = Level_cell_get_bottom_floor(self, pos);
    if (!TileID_is_actor(top_id)) {
      warn("level has no creature at location (%d %d)", pos % MAP_WIDTH, pos / MAP_WIDTH);
      continue;
    }
    if (TileID_actor_get_id(top_id) != CREATURE_BLOCK && bottom_id != TILE_CLONE_MACHINE) {
      Actor* actor = Level_create_actor(self);
      actor->pos = pos;
      actor->id = TileID_actor_get_id(top_id);
      actor->direction = TileID_actor_get_dir(top_id);
      if (TileID_is_actor(bottom_id) && TileID_actor_get_id(bottom_id) == CREATURE_CHIP) {
        chip->pos = pos;
        chip->direction = TileID_actor_get_dir(bottom_id);
      }
    }
    MapTile_add_marker_state(MapCell_get_top_tile(cell));
  }
  pos = 0;
  while (pos < MAP_WIDTH * MAP_HEIGHT) {
    MapCell* cell = Level_get_map_cell(self, pos);
    MapTile* top_tile = MapCell_get_top_tile(cell);
    MapTile* bottom_tile = MapCell_get_bottom_tile(cell);
    if (MapTile_get_state(MapCell_get_top_tile(cell)) & FS_MARKER) {
      MapTile_remove_marker_state(top_tile);
    } else if (TileID_is_actor(MapTile_get_floor(top_tile)) &&
               TileID_actor_get_id(MapTile_get_floor(top_tile)) == CREATURE_CHIP) {
      chip->pos = pos;
      chip->direction = TileID_actor_get_dir(MapTile_get_floor(bottom_tile));
    }
    pos += 1;
  }

  ConnList* traps = &self->trap_connections;
  for (uint8_t n = 0; n < traps->length; n += 1) {
    if (Level_is_trap_button_down(self, traps->items[n].from) ||
        ((traps->items[n].to == Level_get_chip(self)->pos
          || Level_cell_get_top_floor(self, traps->items[n].to) == TILE_BLOCK_STATIC)
        && traps->items[n].init_state)) {
      Level_spring_trap(self, traps->items[n].from);
    }
  }

  memset(self->player_boots, 0, sizeof(self->player_boots));
  memset(self->player_keys, 0, sizeof(self->player_keys));

  self->ms_state.chip_ticks_since_moved = 0;
  self->level_complete = false;
  self->win_state = TRIRES_NOTHING;
  self->ms_state.chip_ticks_since_moved = 0;
  self->ms_state.chip_status = CHIP_OKAY;
  self->ms_state.chip_last_slip_dir = DIRECTION_NIL;
  self->ms_state.controller_dir = DIRECTION_NIL;
  Level_cancel_mouse_goal(self);
  Level_set_rff_dir(self, DIRECTION_NIL);

  return true;
}

/* Advance the game state by one tick.
 */
static void ms_tick_level(Level* self) {
  self->timer_offset = -1;

  if (!(self->current_tick & 3)) {
    for (uint32_t n = 1; n < self->actors_n; n += 1) {
      if (self->actors[n].state & CS_TURNING) {
        self->actors[n].state &= ~(CS_TURNING | CS_HASMOVED);
        Actor_update_floor(&self->actors[n], self);
      }
    }
    self->ms_state.chip_ticks_since_moved += 1;
    if (self->ms_state.chip_ticks_since_moved > 3) {
      self->ms_state.chip_ticks_since_moved = 3;
      if (Level_get_chip(self)->direction != DIRECTION_NIL) {
        Level_get_chip(self)->direction = DIRECTION_SOUTH; /* Convergence Glitch patch (a) */
      }
      Actor_update_floor(Level_get_chip(self), self);
    }
  }

  self->ms_state.mscc_slippers = self->ms_state.slips_n;
  if (Level_get_chip(self)->state & (CS_SLIP | CS_SLIDE)) /* new accounting */
    self->ms_state.mscc_slippers -= 1;

  if (self->current_tick && !(self->current_tick & 1)) {
    self->ms_state.controller_dir = DIRECTION_NIL;
    for (uint32_t n = 0; n < self->actors_n; n += 1) {
      Actor* cr = &self->actors[n];
      if (!cr->hidden && cr->id != CREATURE_CHIP && !(self->current_tick & 3) &&
          self->ms_state.chip_status == CHIP_SQUISHED && !self->level_complete) {
        self->ms_state.chip_status = CHIP_SQUISHED_DEATH; /* Squish patch */
      }
      if (cr->hidden || (cr->state & CS_CLONING) || cr->id == CREATURE_CHIP) {
        continue;
      }
      Actor_choose_move(cr, self);
      if (cr->move_decision != DIRECTION_NIL) {
        Actor_advance_movement(cr, self, cr->move_decision);
      }
    }
    if (Level_check_for_ending(self)) {
      return;
    }
  }

  if (self->current_tick && !(self->current_tick & 1)) {
    Level_do_floor_movements(self);
    if (Level_check_for_ending(self)) {
      return;
    }
  }
  Level_update_sliplist(self);

  self->timer_offset = 0;
  if (self->time_limit) {
    if (self->current_tick >= self->time_limit) {
      self->ms_state.chip_status = CHIP_OUTOFTIME;
      Level_add_sfx(self, SND_TIME_OUT);
      return;
    } else if (self->time_limit - self->current_tick <= 15 * 20 &&
               self->current_tick % 20 == 0) {
      Level_add_sfx(self, SND_TIME_LOW);
    }
  }

  Actor* chip = Level_get_chip(self);
  Actor_choose_move(chip, self);
  if (chip->move_decision != DIRECTION_NIL) {
    Actor_advance_movement(
        chip, self, chip->move_decision); /* Squish patch, TW checked this?! */
    if (Level_check_for_ending(self)) /* TW checks advancecreature() status */
      return; /* guess it's a remnant of Chip starting on exit? */
    chip->state |= CS_HASMOVED;
  }
  Level_update_sliplist(self);
  Level_create_clones(self);

  // putting this at tick end so that it doesn't break iteration
  Level_check_and_clear_actors(self);
}

static void ms_uninit_level(Level* level) {
  return;
}

static void ms_hash_level(Level const* self, hash_t* hash) {
  *hash = hash_scalar(self->ms_state.slips_n, *hash);
  for (size_t i = 0; i < self->ms_state.slips_n; i += 1) {
    MsSlipper const* slipper = &self->ms_state.slip_list[i];
    ptrdiff_t const actor_index = slipper->actor - self->actors;
    hash_t slipper_hash = hash_scalar(actor_index, *hash);
    *hash = hash_scalar(slipper_hash, *hash);
  }
  *hash = hash_scalar(self->ms_state.chip_ticks_since_moved, *hash);
  *hash = hash_scalar(self->ms_state.chip_status, *hash);
  *hash = hash_scalar(self->ms_state.chip_last_slip_dir, *hash);
  *hash = hash_scalar(self->ms_state.mouse_goal, *hash);
  *hash = hash_scalar(self->ms_state.controller_dir, *hash);
}

static bool ms_level_equals(Level const* self, Level const* other) {
  if (self == other)
    return true;
  if (self->ms_state.slips_n != other->ms_state.slips_n)
    return false;
  if (self->ms_state.chip_ticks_since_moved != other->ms_state.chip_ticks_since_moved)
    return false;
  if (self->ms_state.chip_status != other->ms_state.chip_status)
    return false;
  if (self->ms_state.chip_last_slip_dir != other->ms_state.chip_last_slip_dir)
    return false;
  if (self->ms_state.mouse_goal != other->ms_state.mouse_goal)
    return false;
  if (self->ms_state.controller_dir != other->ms_state.controller_dir)
    return false;
  for (size_t i = 0; i < self->ms_state.slips_n; i += 1) {
    MsSlipper const* slipper_self = &self->ms_state.slip_list[i];
    MsSlipper const* slipper_other = &other->ms_state.slip_list[i];
    ptrdiff_t const actor_index_self = slipper_self->actor - self->actors;
    ptrdiff_t const actor_index_other = slipper_other->actor - other->actors;
    if (slipper_self->direction != slipper_other->direction) {
      return false;
    }
    if (actor_index_self != actor_index_other) {
      return false;
    }
  }

  return true;
}

static bool ms_chip_can_move(Level* self) {
  Actor* chip = Level_get_chip(self);
  if (self->ms_state.chip_status != CHIP_OKAY) {
    return false;
  }
  if (self->level_complete) {
    return false;
  }
  if (!(chip->state & CS_HASMOVED)) {
    return true;
  }
  // Since you can set a mouse move/goal at *any point* so long as CS_HASMOVED isn't set, we have to allow a lot of things
  // see Actor_choose_move_chip for details
  if ((self->current_tick & 3) == 0 || Level_has_mouse_goal(self)) {
    return true;
  }
  if (chip->state & (CS_SLIP | CS_SLIDE)) {
    return true; // Slipping will reset HASMOVED if successful, and slipping is too complex to simulate here
  }
  return false;
}

Ruleset const ms_logic = {.id = RULESET_MS,
                          .init_level = ms_init_level,
                          .tick_level = ms_tick_level,
                          .uninit_level = ms_uninit_level,
                          .add_hash_level = ms_hash_level,
                          .level_equals = ms_level_equals,
                          .chip_can_move = ms_chip_can_move,
};
