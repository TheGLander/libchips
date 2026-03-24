#ifndef LIB_CHIPS_LOGIC_H
#define LIB_CHIPS_LOGIC_H

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "random.h"
#include "hash.h"
#include "misc.h"

#define MAP_WIDTH (32)
#define MAP_HEIGHT (32)

#define MAX_CREATURES (2 * MAP_WIDTH * MAP_HEIGHT)

ENUM_UNDERLYING(RulesetID, uint8_t) {
  RULESET_NONE = 0,
  RULESET_LYNX = 1,
  RULESET_MS = 2,
  RULESET_COUNT,
  RULESET_FIRST = RULESET_LYNX
};

ENUM_UNDERLYING(TileID, uint8_t) {
  NOTHING = 0,

  FLOOR = 0x01,

  FORCE_FLOOR_NORTH = 0x02,
  FORCE_FLOOR_WEST = 0x03,
  FORCE_FLOOR_SOUTH = 0x04,
  FORCE_FLOOR_EAST = 0x05,
  FORCE_FLOOR_RANDOM = 0x06,
  ICE = 0x07,
  ICE_CORNER_NORTH_WEST = 0x08,
  ICE_CORNER_NORTH_EAST = 0x09,
  ICE_CORNER_SOUTH_WEST = 0x0A,
  ICE_CORNER_SOUTH_EAST = 0x0B,
  GRAVEL = 0x0C,
  DIRT = 0x0D,
  WATER = 0x0E,
  FIRE = 0x0F,
  BOMB = 0x10,
  TRAP = 0x11,
  THIEF = 0x12,
  HINT = 0x13,

  BUTTON_TANK = 0x14,
  BUTTON_TOGGLE = 0x15,
  BUTTON_CLONE = 0x16,
  BUTTON_TRAP = 0x17,
  TELEPORT = 0x18,

  WALL = 0x19,
  THIN_WALL_NORTH = 0x1A,
  THIN_WALL_WEST = 0x1B,
  THIN_WALL_SOUTH = 0x1C,
  THIN_WALL_EAST = 0x1D,
  THIN_WALL_SOUTH_EAST = 0x1E,
  INVISIBLE_WALL = 0x1F,
  HIDDEN_WALL = 0x20,
  BLUE_WALL_FAKE = 0x21,
  BLUE_WALL_REAL = 0x22,
  TOGGLE_DOOR_OPEN = 0x23,
  TOGGLE_DOOR_CLOSED = 0x24,
  POPUP_WALL = 0x25,

  CLONE_MACHINE = 0x26,

  DOOR_RED = 0x27,
  DOOR_BLUE = 0x28,
  DOOR_YELLOW = 0x29,
  DOOR_GREEN = 0x2A,
  SOCKET = 0x2B,
  EXIT = 0x2C,

  IC_CHIP = 0x2D,
  KEY_RED = 0x2E,
  KEY_BLUE = 0x2F,
  KEY_YELLOW = 0x30,
  KEY_GREEN = 0x31,
  BOOTS_ICE = 0x32,
  BOOTS_FORCE_FLOOR = 0x33,
  BOOTS_FIRE = 0x34,
  BOOTS_WATER = 0x35,

  BLOCK_STATIC = 0x36,

  DROWNED_CHIP = 0x37,
  BURNED_CHIP = 0x38,
  BOMBED_CHIP = 0x39,
  EXITED_CHIP = 0x3A,
  EXIT_ANIM_1 = 0x3B,
  EXIT_ANIM_2 = 0x3C,

  OVERLAY_BUFFER = 0x3D,

  UNUSED_TILE_1 = 0x3E,
  UNUSED_TILE_2 = 0x3F,
  ICE_BLOCK = 0x40,

  TILE_FINAL = 0x7F,

  CHIP = 0x80,

  BLOCK = 0x84,

  TANK = 0x88,
  BALL = 0x8C,
  GLIDER = 0x90,
  FIREBALL = 0x94,
  WALKER = 0x98,
  BLOB = 0x9C,
  TEETH = 0xA0,
  BUG = 0xA4,
  PARAMECIUM = 0xA8,

  SWIMMING_CHIP = 0xAC,
  PUSHING_CHIP = 0xB0,

  CREATURE_FINAL = 0xF8,

  ANIM_WATER = 0xFC,
  ANIM_BOMB = 0xFD,
  ANIM_ENTITY = 0xFE,
  ANIM_UNUSED = 0xFF
};

bool TileID_is_slide(TileID id);
bool TileID_is_ice(TileID id);
bool TileID_is_door(TileID id);
bool TileID_is_key(TileID id);
bool TileID_is_boots(TileID id);
bool TileID_is_ms_special(TileID id);
bool TileID_is_terrain(TileID id);
bool TileID_is_actor(TileID id);
bool TileID_is_animation(TileID id);
bool TileID_is_block(TileID id);

ENUM_UNDERLYING(Position, int16_t) {
  POSITION_NULL = -1
};

ENUM_UNDERLYING(Direction, uint8_t) {
  DIRECTION_NIL = 0,
  DIRECTION_NORTH = 1,
  DIRECTION_WEST = 2,
  DIRECTION_SOUTH = 4,
  DIRECTION_EAST = 8,
};

uint8_t Direction_to_idx(Direction dir);
Direction Direction_from_idx(uint8_t idx);
Direction Direction_left(Direction dir);
Direction Direction_back(Direction dir);
Direction Direction_right(Direction dir);

TileID TileID_actor_with_dir(TileID id, Direction dir);
Direction TileID_actor_get_dir(TileID id);
Direction TileID_actor_get_id(TileID id);
bool Direction_is_diagonal(Direction dir);
bool Direction_is_cardinal(Direction dir);

Position Position_from_xy(int16_t x, int16_t y);
int16_t Position_get_x(Position self);
int16_t Position_get_y(Position self);
Position Position_neighbor(Position self, Direction dir);

enum {  // Mouse moves are a 19x19 square relative to Chip, packing them into 9
        // bits, I don't know where else to put this
  MOUSE_RANGE_MIN = -9,
  MOUSE_RANGE_MAX = +9,
  MOUSE_RANGE = MOUSE_RANGE_MAX - MOUSE_RANGE_MIN + 1,
};

ENUM_UNDERLYING(GameInput, uint16_t) {
  INPUT_NIL = DIRECTION_NIL,
  INPUT_NORTH = DIRECTION_NORTH,
  INPUT_WEST = DIRECTION_WEST,
  INPUT_SOUTH = DIRECTION_SOUTH,
  INPUT_EAST = DIRECTION_EAST,

  INPUT_NORTH_WEST = DIRECTION_NORTH | DIRECTION_WEST,
  INPUT_SOUTH_WEST = DIRECTION_SOUTH | DIRECTION_WEST,
  INPUT_NORTH_EAST = DIRECTION_NORTH | DIRECTION_EAST,
  INPUT_SOUTH_EAST = DIRECTION_SOUTH | DIRECTION_EAST,

  GAME_INPUT_DIR_MOVE_FIRST = INPUT_NORTH,
  GAME_INPUT_DIR_MOVE_LAST =
      INPUT_NORTH | INPUT_EAST | INPUT_SOUTH | INPUT_WEST,

  GAME_INPUT_MOUSE_MOVE_FIRST,
  GAME_INPUT_MOUSE_MOVE_LAST =
      GAME_INPUT_MOUSE_MOVE_FIRST + MOUSE_RANGE * MOUSE_RANGE - 1,
};

bool GameInput_is_directional(GameInput self);
bool GameInput_is_cardinal(GameInput self);
bool GameInput_is_diagonal(GameInput self);
bool GameInput_is_mouse_move(GameInput self);

static inline Direction GameInput_to_direction(GameInput self) {
  if (!GameInput_is_directional(self)) {
    return DIRECTION_NIL;
  }
  return (Direction) self;
}
static inline GameInput GameInput_from_direction(Direction dir) {
  return (GameInput) dir;
}

typedef struct Actor {
  Position pos;
  TileID id;
  Direction direction;
  int8_t move_cooldown;
  int8_t animation_frame;
  bool hidden;
  // Ruleset-specific state
  uint16_t state;
  Direction move_decision;
} Actor;
Position Actor_get_position(Actor const* actor);
TileID Actor_get_id(Actor const* actor);
Direction Actor_get_direction(Actor const* actor);
int8_t Actor_get_move_cooldown(Actor const* actor);
int8_t Actor_get_animation_frame(Actor const* actor);
bool Actor_get_hidden(Actor const* actor);
void Actor_add_hash(Actor const* actor, hash_t* hash);
bool Actor_equals(Actor const* actor, Actor const* other);

typedef struct TileConn {
  Position from;
  Position to;
  bool init_state;
} TileConn;

typedef struct ConnList {
  uint8_t length;
  TileConn items[256];
} ConnList;

typedef struct MapTile {
  TileID id;
  uint8_t state;
} MapTile;

typedef struct MapCell {
  MapTile top;
  MapTile bottom;
} MapCell;

ENUM_UNDERLYING(ChipStatus, uint8_t) {
  CHIP_OKAY = 0,
  CHIP_DROWNED,
  CHIP_BURNED,
  CHIP_BOMBED,
  CHIP_OUTOFTIME,
  CHIP_COLLIDED,
  CHIP_SQUISHED,
  CHIP_SQUISHED_DEATH,
  CHIP_NOTOKAY
};

typedef struct MsSlipper {
  Actor* actor;
  Direction direction;
} MsSlipper;

typedef struct MsState {
  uint32_t slips_n;
  uint32_t mscc_slippers; // transient field, reset each tick
  uint8_t chip_ticks_since_moved;
  ChipStatus chip_status;
  Direction chip_last_slip_dir;
  Position mouse_goal;
  Direction controller_dir;
  uint16_t init_actors_n; // used for building level
  Position init_actor_list[256]; // ditto
  MsSlipper slip_list[MAX_CREATURES]; // placed at the end so that important field offsets aren't massive
} MsState;

typedef struct LxState {
  bool pedantic_mode;
  Actor* chip_colliding_actor;
  Position chip_predicted_pos;
  Position to_place_wall_pos;
  uint8_t prng1;
  uint8_t prng2;
  uint8_t endgame_timer;
  uint8_t toggle_walls_xor;
  bool chip_stuck;
  bool chip_pushing;
  bool chip_bonked;
  bool map_breached;
} LxState;

typedef struct Level Level;

typedef struct Ruleset {
  RulesetID id;
  bool (*init_level)(Level*);
  void (*tick_level)(Level*);
  void (*uninit_level)(Level*);
  void (*add_hash_level)(Level const*, hash_t*);
  bool (*level_equals)(Level const*, Level const*);
  bool (*chip_can_move)(Level*);
} Ruleset;
RulesetID Ruleset_get_id(const Ruleset* self);

enum { TRIRES_DIED = -1, TRIRES_NOTHING = 0, TRIRES_SUCCESS = 1 };
typedef int8_t TriRes;

typedef struct LevelMetadata LevelMetadata;

typedef struct Level {
  LevelMetadata const* metadata; 
  Ruleset const* ruleset;
  // `replay`?
  int8_t timer_offset;
  uint32_t time_limit;
  GameInput game_input;
  uint32_t current_tick;
  uint16_t chips_left;
  Position camera_pos;
  uint32_t actors_n;
  uint8_t player_keys[4];
  uint8_t player_boots[4];
  // `lastmove`?
  uint16_t status_flags;
  Direction rff_dir;
  int8_t init_step_parity;
  uint32_t sfx;
  Prng prng;
  ConnList trap_connections;
  ConnList cloner_connections;
  bool level_complete;
  TriRes win_state;
  union {
    MsState ms_state;
    LxState lx_state;
  };
  MapCell map[MAP_WIDTH * MAP_HEIGHT];
  Actor actors[MAX_CREATURES];
} Level;

const Ruleset* Level_get_ruleset(Level const* self);
int8_t Level_get_time_offset(Level const* self);
uint32_t Level_get_time_limit(Level const* self);
uint32_t Level_get_current_tick(Level const* self);
uint32_t Level_get_chips_left(Level const* self);
uint8_t* Level_get_player_keys(Level* self);
uint8_t* Level_get_player_boots(Level* self);
uint8_t const* Level_get_player_keys_const(Level const* self);
uint8_t const* Level_get_player_boots_const(Level const* self);
uint16_t Level_get_status_flags(Level const* self);
uint32_t Level_get_sfx(Level const* self);
Prng* Level_get_prng_ptr(Level* self);
Prng const* Level_get_prng_const_ptr(Level const* self);
TileID Level_get_top_terrain(Level const* self, Position pos);
TileID Level_get_bottom_terrain(Level const* self, Position pos);
Actor* Level_get_actors_ptr(Level* self);
uint32_t Level_get_actors_n(Level const* self);
Actor* Level_get_actor_by_idx(Level* self, uint32_t idx);
Actor* Level_get_chip_actor(Level* self);
Actor const* Level_get_actors_const_ptr(Level const* self);
Actor const* Level_get_actor_const_by_idx(Level const* self, uint32_t idx);
Actor const* Level_get_chip_actor_const(Level const* self);
uint8_t* Level_player_item_ptr(Level* self, TileID id);
bool Level_player_has_item(Level const* self, TileID id);
void Level_set_game_input(Level* self, GameInput game_input);
GameInput Level_get_game_input(Level const* self);
TriRes Level_get_win_state(Level const* self);
Direction Level_get_rff_dir(Level const* self);
void Level_set_rff_dir(Level* self, Direction dir);
int8_t Level_get_init_step_parity(Level const* self);
void Level_set_init_step_parity(Level* self, int8_t parity);
LevelMetadata const* Level_get_metadata(Level const* self);
void Level_set_prng(Level* self, Prng other);
Level Level_clone(Level const* self);
hash_t Level_get_hash(Level const* self);
bool Level_equals(Level const* self, Level const* other);
bool Level_chip_can_move(Level* self);

ENUM_UNDERLYING(Sfx, uint32_t) {
  SND_CHIP_LOSES = 0,
  SND_CHIP_WINS = 1,
  SND_TIME_OUT = 2,
  SND_TIME_LOW = 3,
  SND_DEREZZ = 4,
  SND_CANT_MOVE = 5,
  SND_IC_COLLECTED = 6,
  SND_ITEM_COLLECTED = 7,
  SND_BOOTS_STOLEN = 8,
  SND_TELEPORTING = 9,
  SND_DOOR_OPENED = 10,
  SND_SOCKET_OPENED = 11,
  SND_BUTTON_PUSHED = 12,
  SND_TILE_EMPTIED = 13,
  SND_WALL_CREATED = 14,
  SND_TRAP_ENTERED = 15,
  SND_BOMB_EXPLODES = 16,
  SND_WATER_SPLASH = 17,
  SND_ONESHOT_COUNT = 18,

  SND_BLOCK_MOVING = 18,
  SND_SKATING_FORWARD = 19,
  SND_SKATING_TURN = 20,
  SND_SLIDING = 21,
  SND_SLIDEWALKING = 22,
  SND_ICEWALKING = 23,
  SND_WATERWALKING = 24,
  SND_FIREWALKING = 25,
  SND_COUNT = 26,
};

void Level_add_sfx(Level* self, Sfx sfx);
void Level_stop_sfx(Level* self, Sfx sfx);
void Level_free(Level* self);

void Level_tick(Level* self);

ENUM_UNDERLYING(StateFlags, uint16_t) {
  SF_INVALID = 0x2,
  SF_BAD_TILES = 0x4,
  SF_SHOW_HINT = 0x8,
  SF_NO_ANIMATION = 0x10,
  SF_SHUTTERRED = 0x20,
};

extern Ruleset const lynx_logic;
extern Ruleset const ms_logic;

#endif  // LIB_CHIPS_LOGIC_H
