#include <stdlib.h>
#include <string.h>

#include "logic.h"
#include "misc.h"

bool TileID_is_slide(TileID id) {
  return id >= TILE_FORCE_FLOOR_NORTH && id <= TILE_FORCE_FLOOR_RANDOM;
}
bool TileID_is_ice(TileID id) {
  return id >= TILE_ICE && id <= TILE_ICE_CORNER_SOUTH_EAST;
}
bool TileID_is_door(TileID id) {
  return id >= TILE_DOOR_RED && id <= TILE_DOOR_GREEN;
}
bool TileID_is_key(TileID id) {
  return id >= TILE_KEY_RED && id <= TILE_KEY_GREEN;
}
bool TileID_is_boots(TileID id) {
  return id >= TILE_BOOTS_ICE && id <= TILE_BOOTS_WATER;
}
bool TileID_is_ms_special(TileID id) {
  return id >= TILE_DROWNED_CHIP && id <= TILE_OVERLAY_BUFFER;
}
bool TileID_is_terrain(TileID id) {
  return id <= TILE_FINAL;
}
bool TileID_is_actor(TileID id) {
  return id >= CREATURE_CHIP && id < ANIM_WATER;
}
bool TileID_is_animation(TileID id) {
  return id >= ANIM_WATER && id <= ANIM_UNUSED;
}
bool TileID_is_block(TileID id) {
  return id == TILE_BLOCK_STATIC || (TileID_is_actor(id) && TileID_actor_get_id(id) == CREATURE_BLOCK);
}
uint8_t Direction_to_idx(Direction dir) {
  return (0x30210 >> ((dir) * 2)) & 3;
}
Direction Direction_from_idx(uint8_t idx) {
  return 1 << ((idx) & 3);
}
Direction Direction_left(Direction dir) {
  return ((dir << 1) | (dir >> 3)) & 15;
}
Direction Direction_back(Direction dir) {
  return ((dir << 2) | (dir >> 2)) & 15;
}
Direction Direction_right(Direction dir) {
  return ((dir << 3) | ((dir) >> 1)) & 15;
}

TileID TileID_actor_with_dir(TileID id, Direction dir) {
  return id | Direction_to_idx(dir);
}
Direction TileID_actor_get_dir(TileID id) {
  return Direction_from_idx(id & 3);
}
TileID TileID_actor_get_id(TileID id) {
  return id & ~3;
}

Position Position_from_xy(int16_t x, int16_t y) {
  return y * MAP_WIDTH + x;
}

int16_t Position_get_x(Position self) {
  return self % MAP_WIDTH;
}

int16_t Position_get_y(Position self) {
  return self / MAP_WIDTH;
}

static int8_t const direction_offsets[] = {0, -MAP_WIDTH, -1, 0, +MAP_WIDTH,
                                           0, 0,          0,  +1};
Position Position_neighbor(Position self, Direction dir) {
  return self + direction_offsets[dir];
}

bool GameInput_is_directional(GameInput self) {
  return GAME_INPUT_DIR_MOVE_FIRST <= self && self <= GAME_INPUT_DIR_MOVE_LAST;
}

bool GameInput_is_cardinal(GameInput self) {
  if (!GameInput_is_directional(self)) {
    return false;
  }
  return Direction_is_cardinal((Direction) self);
}

bool GameInput_is_diagonal(GameInput self) {
  if (!GameInput_is_directional(self)) {
    return false;
  }
  return Direction_is_diagonal((Direction) self);
}

bool GameInput_is_mouse_move(GameInput self) {
  return GAME_INPUT_MOUSE_MOVE_FIRST <= self && self <= GAME_INPUT_MOUSE_MOVE_LAST;
}

bool Direction_is_diagonal(Direction dir) {
  return (dir & (DIRECTION_NORTH | DIRECTION_SOUTH)) &&
         (dir & (DIRECTION_EAST | DIRECTION_WEST));
}

bool Direction_is_cardinal(Direction dir) {
  return dir == DIRECTION_NORTH || dir == DIRECTION_SOUTH ||
         dir == DIRECTION_EAST || dir == DIRECTION_WEST;
}

void Level_add_sfx(Level* level, Sfx sfx) {
  level->sfx |= 1 >> sfx;
}

void Level_stop_sfx(Level* level, Sfx sfx) {
  level->sfx &= ~(1 >> sfx);
}

RulesetID Ruleset_get_id(Ruleset const* self) {
  return self->id;
}

Position Actor_get_position(Actor const* actor) {
  return actor->pos;
}
Direction Actor_get_direction(Actor const* self) {
  return self->direction;
}
TileID Actor_get_id(Actor const* actor) {
  return actor->id;
}
int8_t Actor_get_move_cooldown(Actor const* actor) {
  return actor->move_cooldown;
}
int8_t Actor_get_animation_frame(Actor const* actor) {
  return actor->animation_frame;
}
bool Actor_get_hidden(Actor const* actor) {
  return actor->hidden;
}
void Actor_add_hash(Actor const* actor, hash_t* hash) {
  *hash = hash_scalar(actor->pos, *hash);
  *hash = hash_scalar(actor->id, *hash);
  *hash = hash_scalar(actor->direction, *hash);
  *hash = hash_scalar(actor->move_cooldown, *hash);
  *hash = hash_scalar(actor->animation_frame, *hash);
  *hash = hash_scalar(actor->hidden, *hash);
  *hash = hash_scalar(actor->state, *hash);
  *hash = hash_scalar(actor->move_decision, *hash);
}

bool Actor_equals(Actor const* actor, Actor const* other) {
  if (actor == other)
    return true;
  if (actor->pos != other->pos)
    return false;
  if (actor->id != other->id)
    return false;
  if (actor->direction != other->direction)
    return false;
  if (actor->move_cooldown != other->move_cooldown)
    return false;
  if (actor->animation_frame != other->animation_frame)
    return false;
  if (actor->hidden != other->hidden)
    return false;
  if (actor->state != other->state)
    return false;
  if (actor->move_decision != other->move_decision)
    return false;
  return true;
}

Ruleset const* Level_get_ruleset(Level const* self) {
  return self->ruleset;
}
int8_t Level_get_time_offset(Level const* self) {
  return self->timer_offset;
}
uint32_t Level_get_time_limit(Level const* self) {
  return self->time_limit;
}
uint32_t Level_get_current_tick(Level const* self) {
  return self->current_tick;
}
uint32_t Level_get_chips_left(Level const* self) {
  return self->chips_left;
}
uint8_t* Level_get_player_keys(Level* self) {
  return self->player_keys;
}
uint8_t* Level_get_player_boots(Level* self) {
  return self->player_boots;
}
uint8_t const* Level_get_player_keys_const(Level const* self) {
  return self->player_keys;
}
uint8_t const* Level_get_player_boots_const(Level const* self) {
  return self->player_boots;
}
uint16_t Level_get_status_flags(Level const* self) {
  return self->status_flags;
}
uint32_t Level_get_sfx(Level const* self) {
  return self->sfx;
}
Prng* Level_get_prng_ptr(Level* self) {
  return &self->prng;
}
Prng const* Level_get_prng_const_ptr(Level const* self) {
  return &self->prng;
}
TileID Level_get_top_terrain(Level const* self, Position pos) {
  return self->map[pos].top.id;
}
TileID Level_get_bottom_terrain(Level const* self, Position pos) {
  return self->map[pos].bottom.id;
}
Actor* Level_get_actors_ptr(Level* self) {
  return self->actors;
}
uint32_t Level_get_actors_n(Level const* self) {
  return self->actors_n;
}
Actor* Level_get_actor_by_idx(Level* self, uint32_t idx) {
  return &self->actors[idx];
}
Actor* Level_get_chip_actor(Level* self) {
  return &self->actors[0]; // both MS and Lynx have Chip as the first actor so this is safe
}
Actor const* Level_get_actors_const_ptr(Level const* self) {
  return self->actors;
}
Actor const* Level_get_actor_const_by_idx(Level const* self, uint32_t idx) {
  return &self->actors[idx];
}
Actor const* Level_get_chip_actor_const(Level const* self) {
  return &self->actors[0];
}
uint8_t* Level_player_item_ptr(Level* level, TileID id) {
  switch (id) {
    case TILE_KEY_RED:
    case TILE_DOOR_RED:
      return &level->player_keys[0];
    case TILE_KEY_BLUE:
    case TILE_DOOR_BLUE:
      return &level->player_keys[1];
    case TILE_KEY_YELLOW:
    case TILE_DOOR_YELLOW:
      return &level->player_keys[2];
    case TILE_KEY_GREEN:
    case TILE_DOOR_GREEN:
      return &level->player_keys[3];
    case TILE_BOOTS_ICE:
    case TILE_ICE:
    case TILE_ICE_CORNER_NORTH_WEST:
    case TILE_ICE_CORNER_NORTH_EAST:
    case TILE_ICE_CORNER_SOUTH_WEST:
    case TILE_ICE_CORNER_SOUTH_EAST:
      return &level->player_boots[0];
    case TILE_BOOTS_FORCE_FLOOR:
    case TILE_FORCE_FLOOR_NORTH:
    case TILE_FORCE_FLOOR_WEST:
    case TILE_FORCE_FLOOR_SOUTH:
    case TILE_FORCE_FLOOR_EAST:
    case TILE_FORCE_FLOOR_RANDOM:
      return &level->player_boots[1];
    case TILE_BOOTS_FIRE:
    case TILE_FIRE:
      return &level->player_boots[2];
    case TILE_BOOTS_WATER:
    case TILE_WATER:
      return &level->player_boots[3];
    default:
      return NULL;
  }
}
bool Level_player_has_item(Level const* level, TileID id) {
  // const-discarding pointer cast: it's okay, we don't ever write to the return
  // pointer, which is the only reason why this function isn't const-pointer'd
  return *Level_player_item_ptr((Level*)level, id) > 0;
}
void Level_free(Level* self) {
  if (self == NULL)
    return;
  self->ruleset->uninit_level(self);
  free(self);
}

void Level_tick(Level* self) {
  self->sfx &= ~((1 << SND_ONESHOT_COUNT) - 1);
  self->ruleset->tick_level(self);
  self->current_tick += 1;
}

GameInput Level_get_game_input(Level const* self) {
  return self->game_input;
}
void Level_set_game_input(Level* self, GameInput game_input) {
  self->game_input = game_input;
}

TriRes Level_get_win_state(Level const* self) {
  return self->win_state;
}

Direction Level_get_rff_dir(Level const* self) {
  return self->rff_dir;
}

void Level_set_rff_dir(Level* self, Direction dir) {
  self->rff_dir = dir;
}

int8_t Level_get_init_step_parity(Level const* self) {
  return self->init_step_parity;
}

void Level_set_init_step_parity(Level* self, int8_t parity) {
  self->init_step_parity = parity;
}

LevelMetadata const* Level_get_metadata(Level const* self) {
  return self->metadata;
}

void Level_set_prng(Level* self, Prng other) {
  self->prng = other;
}

Level Level_clone(Level const* self) {
  Level clone;
  memcpy(&clone, self, sizeof(Level));
  return clone;
}

hash_t Level_get_hash(Level const* self) { // FNV-1a algorithm
  hash_t hash = HASH_INIT;

  hash = hash_scalar(self->ruleset->id, hash);

  hash = hash_scalar(self->timer_offset, hash);
  hash = hash_scalar(self->time_limit, hash);
  hash = hash_scalar(self->game_input, hash);
  hash = hash_scalar(self->current_tick, hash);
  hash = hash_scalar(self->chips_left, hash);
  hash = hash_scalar(self->camera_pos, hash);
  hash = hash_scalar(self->actors_n, hash);
  for (uint32_t i = 0; i < 4; i += 1) {
    hash = hash_scalar(self->player_keys[i], hash);
  }
  for (uint32_t i = 0; i < 4; i += 1) {
    hash = hash_scalar(self->player_boots[i], hash);
  }
  hash = hash_scalar(self->status_flags, hash);
  hash = hash_scalar(self->rff_dir, hash);
  hash = hash_scalar(self->init_step_parity, hash);
  hash = hash_scalar(self->sfx, hash);
  hash = hash_scalar(self->prng.initial_seed, hash);
  hash = hash_scalar(self->prng.value, hash);

  hash = hash_scalar(self->trap_connections.length, hash);
  for (size_t i = 0; i < self->trap_connections.length; i += 1) {
    hash = hash_scalar(self->trap_connections.items[i].from, hash);
    hash = hash_scalar(self->trap_connections.items[i].to, hash);
  }
  hash = hash_scalar(self->cloner_connections.length, hash);
  for (size_t i = 0; i < self->cloner_connections.length; i += 1) {
    hash = hash_scalar(self->cloner_connections.items[i].from, hash);
    hash = hash_scalar(self->cloner_connections.items[i].to, hash);
  }
  hash = hash_scalar(self->level_complete, hash);
  hash = hash_scalar(self->win_state, hash);
  for (size_t i = 0; i < lengthof(self->map); i += 1) {
    MapCell const* cell = &self->map[i];
    hash = hash_scalar(cell->top.id, hash);
    hash = hash_scalar(cell->top.state, hash);
    hash = hash_scalar(cell->bottom.id, hash);
    hash = hash_scalar(cell->bottom.state, hash);
  }
  for (size_t i = 0; i < self->actors_n; i += 1) {
    Actor_add_hash(&self->actors[i], &hash);
  }
  self->ruleset->add_hash_level(self, &hash);

  return hash;
}

bool Level_equals(Level const* self, Level const* other) {
  if (self == other)
    return true;
  if (self->ruleset->id != other->ruleset->id)
    return false;
  if (self->timer_offset != other->timer_offset)
    return false;
  if (self->time_limit != other->time_limit)
    return false;
  if (self->game_input != other->game_input)
    return false;
  if (self->current_tick != other->current_tick)
    return false;
  if (self->chips_left != other->chips_left)
    return false;
  if (self->camera_pos != other->camera_pos)
    return false;
  if (self->actors_n != other->actors_n)
    return false;
  for (size_t i = 0; i < lengthof(self->player_keys); i += 1) {
    if (self->player_keys[i] != other->player_keys[i]) {
      return false;
    }
  }
  for (size_t i = 0; i < lengthof(self->player_boots); i += 1) {
    if (self->player_boots[i] != other->player_boots[i]) {
      return false;
    }
  }
  if (self->status_flags != other->status_flags)
    return false;
  if (self->rff_dir != other->rff_dir)
    return false;
  if (self->init_step_parity != other->init_step_parity)
    return false;
  if (self->sfx != other->sfx)
    return false;
  if (self->prng.initial_seed != other->prng.initial_seed)
    return false;
  if (self->prng.value != other->prng.value)
    return false;
  if (self->level_complete != other->level_complete)
    return false;
  if (self->win_state != other->win_state)
    return false;
  if (self->trap_connections.length != other->trap_connections.length)
    return false;
  if (self->cloner_connections.length != other->cloner_connections.length)
    return false;
  for (size_t i = 0; i < self->trap_connections.length; i += 1) {
    if (self->trap_connections.items[i].from != other->trap_connections.items[i].from) {
      return false;
    }
    if (self->trap_connections.items[i].to != other->trap_connections.items[i].to) {
      return false;
    }
  }
  for (size_t i = 0; i < self->cloner_connections.length; i += 1) {
    if (self->cloner_connections.items[i].from != other->cloner_connections.items[i].from) {
      return false;
    }
    if (self->cloner_connections.items[i].to != other->cloner_connections.items[i].to) {
      return false;
    }
  }
  for (size_t i = 0; i < lengthof(self->map); i += 1) {
    MapCell const* cell_self = &self->map[i];
    MapCell const* cell_other = &other->map[i];
    if (cell_self->top.id != cell_other->top.id) {
      return false;
    }
    if (cell_self->top.state != cell_other->top.state) {
      return false;
    }
    if (cell_self->bottom.id != cell_other->bottom.id) {
      return false;
    }
    if (cell_self->bottom.state != cell_other->bottom.state) {
      return false;
    }
  }
  for (size_t i = 0; i < self->actors_n; i += 1) {
    if (!Actor_equals(&self->actors[i], &other->actors[i])) {
      return false;
    }
  }

  if (!self->ruleset->level_equals(self, other))
    return false;
  return true;
}

bool Level_chip_can_move(Level* self) {
  return self->ruleset->chip_can_move(self);
}
