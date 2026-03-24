#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "formats.h"

// Sure would be great to have `constexpr` for `TileID_with_dir`
#define north(id) id
#define west(id) ((id) | 1)
#define south(id) ((id) | 2)
#define east(id) ((id) | 3)

char const* LevelMetadata_get_title(LevelMetadata const* self) {
  return self->title;
}

uint16_t LevelMetadata_get_level_number(LevelMetadata const* self) {
  return self->level_number;
}

uint16_t LevelMetadata_get_time_limit(LevelMetadata const* self) {
  return self->time_limit;
}

uint16_t LevelMetadata_get_chips_required(LevelMetadata const* self) {
  return self->chips_required;
}

char const* LevelMetadata_get_password(LevelMetadata const* self) {
  return self->password;
}

char const* LevelMetadata_get_hint(LevelMetadata const* self) {
  return self->hint;
}

char const* LevelMetadata_get_author(LevelMetadata const* self) {
  return self->author;
}

void LevelSet_set_name(LevelSet* self, char const* set_name) {
  self->name = (set_name != NULL) ? strdup(set_name) : NULL;
}

char const* LevelSet_get_name(LevelSet const* self) {
  return self->name;
}

uint16_t LevelSet_get_levels_n(LevelSet const* self) {
  return self->levels_n;
}

LevelMetadata* LevelSet_get_level(LevelSet* self, uint16_t idx) {
  if (idx >= self->levels_n) {
    return NULL;
  }
  return &self->levels[idx];
}

LevelMetadata* LevelSet_get_level_by_level_num(LevelSet* self, uint16_t level_num) {
  uint16_t idx = LevelSet_get_level_idx_by_level_num(self, level_num);
  if (idx == (uint16_t)-1) {
    return NULL;
  }
  return &self->levels[idx];
}

uint16_t LevelSet_get_level_idx_by_level_num(LevelSet const* self, uint16_t level_num) {
  for (uint16_t idx = 0; idx < self->levels_n; idx += 1) {
    if (self->levels[idx].level_number == level_num) {
      return idx;
    }
  }
  return -1;
}

LevelMetadata* LevelSet_get_level_by_pass(LevelSet* self, char const pass[10]) {
  uint16_t idx = LevelSet_get_level_idx_by_pass(self, pass);
  if (idx == (uint16_t)-1) {
    return NULL;
  }
  return &self->levels[idx];
}

uint16_t LevelSet_get_level_idx_by_pass(LevelSet const* self, char const pass[10]) {
  if (pass == NULL)
    return -1;
  for (uint16_t idx = 0; idx < self->levels_n; idx += 1) {
    if (strncmp(self->levels[idx].password, pass, 10) == 0) {
      return idx;
    }
  }
  return -1;
}


static TileID const DAT_TILDID_MAP[] = {
    // 0x00
    FLOOR, WALL, IC_CHIP, WATER, FIRE, INVISIBLE_WALL, THIN_WALL_NORTH, THIN_WALL_WEST,
    THIN_WALL_SOUTH, THIN_WALL_EAST, BLOCK_STATIC, DIRT, ICE, FORCE_FLOOR_SOUTH,
    // 0x10
    north(BLOCK), west(BLOCK), south(BLOCK), east(BLOCK), FORCE_FLOOR_NORTH,
    FORCE_FLOOR_EAST, FORCE_FLOOR_WEST, EXIT, DOOR_BLUE, DOOR_RED, DOOR_GREEN, DOOR_YELLOW,
    ICE_CORNER_SOUTH_EAST, ICE_CORNER_SOUTH_WEST, ICE_CORNER_NORTH_WEST, ICE_CORNER_NORTH_EAST,
    BLUE_WALL_REAL, BLUE_WALL_FAKE,
    // 0x20
    OVERLAY_BUFFER, THIEF, SOCKET, BUTTON_TOGGLE, BUTTON_CLONE, TOGGLE_DOOR_CLOSED,
    TOGGLE_DOOR_OPEN, BUTTON_TRAP, BUTTON_TANK, TELEPORT, BOMB, TRAP,
    HIDDEN_WALL, GRAVEL, POPUP_WALL, HINT,
    // 0x30
    THIN_WALL_SOUTH_EAST, CLONE_MACHINE, FORCE_FLOOR_RANDOM, DROWNED_CHIP, BURNED_CHIP,
    BOMBED_CHIP, UNUSED_TILE_1, UNUSED_TILE_2, ICE_BLOCK, EXITED_CHIP, EXIT_ANIM_1, EXIT_ANIM_2,
    north(SWIMMING_CHIP), west(SWIMMING_CHIP), south(SWIMMING_CHIP),
    east(SWIMMING_CHIP),
    // 0x40
    north(BUG), west(BUG), south(BUG), east(BUG), north(FIREBALL),
    west(FIREBALL), south(FIREBALL), east(FIREBALL), north(BALL), west(BALL),
    south(BALL), east(BALL), north(TANK), west(TANK), south(TANK), east(TANK),
    // 0x50
    north(GLIDER), west(GLIDER), south(GLIDER), east(GLIDER), north(TEETH),
    west(TEETH), south(TEETH), east(TEETH), north(WALKER), west(WALKER),
    south(WALKER), east(WALKER), north(BLOB), west(BLOB), south(BLOB),
    east(BLOB),
    // 0x60
    north(PARAMECIUM), west(PARAMECIUM), south(PARAMECIUM), east(PARAMECIUM),
    KEY_BLUE, KEY_RED, KEY_GREEN, KEY_YELLOW, BOOTS_WATER, BOOTS_FIRE,
    BOOTS_ICE, BOOTS_FORCE_FLOOR, north(CHIP), west(CHIP), south(CHIP), east(CHIP)
};

enum CllChunkTypes {
  CCL_CHUNK_REDUNDANT_TIME = 1,
  CCL_CHUNK_REDUNDANT_CHIPS = 2,
  CCL_CHUNK_TITLE = 3,
  CCL_CHUNK_TRAPS = 4,
  CCL_CHUNK_CLONERS = 5,
  CCL_CHUNK_PASSWORD = 6,
  CCL_CHUNK_HINT = 7,
  CCL_CHUNK_REDUNDANT_PASSWORD = 8,
  CCL_CHUNK_AUTHOR = 9,
  CCL_CHUNK_MONSTER_LIST = 10,
};

Result_LevelSetPtr parse_ccl(uint8_t const* data, size_t data_len) {
  uint8_t const* const base_data = data;
#define assert_data_avail(...)                                     \
  if (data - base_data __VA_OPT__(-1 +) __VA_ARGS__ >= data_len) { \
    LevelSet_free(set);                                            \
    return res_err(LevelSetPtr, "CCL file ends too soon");         \
  }
  if (data == NULL)
    return res_err(LevelSetPtr, "CCL data pointer is null");

  LevelSet* set = NULL;

  assert_data_avail(4);
  uint32_t magic_bytes = read_uint32_le(data);
  data += 4;
  if (magic_bytes != 0x0002AAAC && magic_bytes != 0x0102AAAC)
    return res_err(LevelSetPtr,
                   "Invalid CCL signature. Are you sure this is a CCL file?");
  assert_data_avail(2);
  uint16_t levels_n = read_uint16_le(data);
  data += 2;

  set = xmalloc(sizeof(LevelSet) + levels_n * sizeof(LevelMetadata));
  *set = (LevelSet){};
  // We will update `levels_n` as we parse the levels, so that uninit in case of
  // error is easier
  set->levels_n = 0;

  for (uint16_t idx = 0; idx < levels_n; idx += 1) {
    set->levels_n += 1;
    LevelMetadata* meta = &set->levels[idx];
    *meta = (LevelMetadata){};
    assert_data_avail(12);
    uint8_t const* const level_data_ptr = data;
    uint16_t level_data_len = read_uint16_le(data);
    meta->level_number = read_uint16_le(data + 2);
    meta->time_limit = read_uint16_le(data + 4);
    meta->chips_required = read_uint16_le(data + 6);
    // data+8 is unused
    meta->layer_top_size = read_uint16_le(data + 10);
    data += 12;
    assert_data_avail(meta->layer_top_size);
    meta->layer_top = xmalloc(meta->layer_top_size);
    memcpy(meta->layer_top, data, meta->layer_top_size);
    data += meta->layer_top_size;

    meta->layer_bottom_size = read_uint16_le(data);
    data += 2;
    assert_data_avail(meta->layer_bottom_size);
    meta->layer_bottom = xmalloc(meta->layer_bottom_size);
    memcpy(meta->layer_bottom, data, meta->layer_bottom_size);
    data += meta->layer_bottom_size;
    assert_data_avail(2);
    uint16_t level_chunks_size = read_uint16_le(data);
    data += 2;
    assert_data_avail(level_chunks_size);
    while (level_chunks_size > 0) {
      assert_data_avail(2);
      uint8_t chunk_type = data[0];
      uint8_t chunk_len = data[1];
      data += 2;
      assert_data_avail(chunk_len);
      if (chunk_type == CCL_CHUNK_TITLE) {
        meta->title =
            strndup((char const*)data, chunk_len > 64 ? chunk_len : 64);
      } else if (chunk_type == CCL_CHUNK_TRAPS) {
        uint8_t traps_n = chunk_len / 10;
        meta->trap_links = xcalloc(sizeof(ConnList), 1);
        meta->trap_links->length = traps_n;
        for (uint8_t trap_idx = 0; trap_idx < traps_n; trap_idx += 1) {
          TileConn* conn = &meta->trap_links->items[trap_idx];
          uint16_t from_x = read_uint16_le(&data[trap_idx * 10]);
          uint16_t from_y = read_uint16_le(&data[trap_idx * 10 + 2]);
          uint16_t to_x = read_uint16_le(&data[trap_idx * 10 + 4]);
          uint16_t to_y = read_uint16_le(&data[trap_idx * 10 + 6]);
          bool is_open = read_uint16_le(&data[trap_idx * 10 + 8]) == 0;
          conn->from = from_x + from_y * MAP_WIDTH;
          conn->to = to_x + to_y * MAP_WIDTH;
          conn->init_state = is_open;
        }
      } else if (chunk_type == CCL_CHUNK_CLONERS) {
        uint8_t cloners_n = chunk_len / 8;
        meta->cloner_links = xcalloc(sizeof(ConnList), 1);
        meta->cloner_links->length = cloners_n;
        for (uint8_t cloner_idx = 0; cloner_idx < cloners_n; cloner_idx += 1) {
          TileConn* conn = &meta->cloner_links->items[cloner_idx];
          uint16_t from_x = read_uint16_le(&data[cloner_idx * 8]);
          uint16_t from_y = read_uint16_le(&data[cloner_idx * 8 + 2]);
          uint16_t to_x = read_uint16_le(&data[cloner_idx * 8 + 4]);
          uint16_t to_y = read_uint16_le(&data[cloner_idx * 8 + 6]);
          conn->from = from_x + from_y * MAP_WIDTH;
          conn->to = to_x + to_y * MAP_WIDTH;
          conn->init_state = false;
        }
      } else if (chunk_type == CCL_CHUNK_PASSWORD) {
        strncpy(meta->password, (char const*)data,
                chunk_len > 10 ? 10 : chunk_len);
        meta->password[chunk_len - 1] = 0; // force a nul term as strncpy doesn't
        // Decode the password
        for (char* password_char = meta->password; *password_char != 0;
             password_char += 1) {
          *password_char ^= 0x99;
        }
      } else if (chunk_type == CCL_CHUNK_HINT) {
        meta->hint =
            strndup((char const*)data, chunk_len > 128 ? 128 : chunk_len);
      } else if (chunk_type == CCL_CHUNK_MONSTER_LIST) {
        uint8_t monsters_n = chunk_len / 2;
        meta->monster_list = xmalloc(monsters_n * sizeof(Position));
        meta->monsters_n = monsters_n;
        for (uint8_t monster_idx = 0; monster_idx < monsters_n;
             monster_idx += 1) {
          uint8_t monster_x = data[monster_idx * 2];
          uint8_t monster_y = data[monster_idx * 2 + 1];
          meta->monster_list[monster_idx] = monster_x + monster_y * MAP_WIDTH;
        }
      } else if (chunk_type == CCL_CHUNK_AUTHOR) {
        meta->author =
            strndup((char const*)data, chunk_len > 128 ? 128 : chunk_len);
      } else {
        // Unknown chunk type, ignore
      }
      data += chunk_len;
      level_chunks_size -= 2 + chunk_len;
    }
  }
  if (data != base_data + data_len) {
    LevelSet_free(set);
    return res_err(LevelSetPtr, "CCL larger than needed");
  }
  return res_val(LevelSetPtr, set);
}


void LevelSet_free(LevelSet* self) {
  if (self == NULL)
    return;
  free(self->name);
  for (uint16_t level_idx = 0; level_idx < self->levels_n; level_idx += 1) {
    LevelMetadata* meta = &self->levels[level_idx];
    free(meta->title);
    free(meta->hint);
    free(meta->author);
    free(meta->trap_links);
    free(meta->cloner_links);
    free(meta->monster_list);
    free(meta->layer_top);
    free(meta->layer_bottom);
  }
  free(self);
}

static bool uncompress_field(uint8_t* to,
                             uint8_t const* from,
                             size_t from_size) {
  size_t from_idx = 0;
  size_t to_idx = 0;
  size_t const to_size = MAP_WIDTH * MAP_HEIGHT;
  while (to_idx != to_size) {
    if (from_size - from_idx < 1)
      return false;
    if (from[from_idx] != 0xFF) {
      to[to_idx] = from[from_idx];
      to_idx += 1;
      from_idx += 1;
    } else {
      if (from_size - from_idx < 3)
        return false;
      size_t rle_count = from[from_idx + 1];
      if (rle_count > to_size - to_idx)
        return false;
      uint8_t rle_val = from[from_idx + 2];
      memset(to + to_idx, rle_val, rle_count);
      to_idx += rle_count;
      from_idx += 3;
    }
  }
  if (from_idx != from_size)
    return false;
  return true;
}

Result_LevelPtr LevelMetadata_make_level(LevelMetadata const* self,
                                         Ruleset const* ruleset) {
  Level* level = xmalloc(sizeof(Level));
  *level = (Level){.ruleset = ruleset,
                   .chips_left = self->chips_required,
                   .time_limit = self->time_limit * 20};
  if (self->trap_links) {
    level->trap_connections = *self->trap_links;
  }
  if (self->cloner_links) {
    level->cloner_connections = *self->cloner_links;
  }
  if (self->monster_list) {
    memcpy(level->ms_state.init_actor_list, self->monster_list,
           self->monsters_n * sizeof(Position));
    level->ms_state.init_actors_n = self->monsters_n;
  }
  uint8_t uncompressed_field[MAP_WIDTH * MAP_HEIGHT];
  if (!uncompress_field(uncompressed_field, self->layer_top,
                        self->layer_top_size)) {
    free(level);
    return res_err(LevelPtr, "Failed to uncompress top field");
  }
  for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
    uint8_t ccl_tile_id = uncompressed_field[pos];
    if (ccl_tile_id >= lengthof(DAT_TILDID_MAP)) {
      free(level);
      return res_err(LevelPtr, "Unknown CCL tile id %02X", ccl_tile_id);
    }
    level->map[pos].top.id = DAT_TILDID_MAP[ccl_tile_id];
  }
  if (!uncompress_field(uncompressed_field, self->layer_bottom,
                        self->layer_bottom_size)) {
    free(level);
    return res_err(LevelPtr, "Failed to uncompress bottom field");
  }
  for (Position pos = 0; pos < MAP_WIDTH * MAP_HEIGHT; pos += 1) {
    uint8_t ccl_tile_id = uncompressed_field[pos];
    if (ccl_tile_id >= lengthof(DAT_TILDID_MAP)) {
      free(level);
      return res_err(LevelPtr, "Unknown CCL tile id %02X", ccl_tile_id);
    }
    level->map[pos].bottom.id = DAT_TILDID_MAP[ccl_tile_id];
  }

  level->ruleset = ruleset;
  level->metadata = self;
  bool init_success = ruleset->init_level(level);
  return res_val(LevelPtr, level);
}
