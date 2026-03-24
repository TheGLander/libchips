#include "tws_test_helpers.h"

#include <cstdlib>
#include <cmath>

#include <gmock/gmock.h>

LevelsetTwssetPairOptional loadsets(uint8_t const* levelset, size_t levelset_size, uint8_t const* tws, size_t tws_size) {
  LevelsetTwssetPair pair = {};

  Result_LevelSetPtr res = parse_ccl(levelset, levelset_size);
  EXPECT_TRUE(res.success);
  if (!res.success) {
    eprintf("%s\n", res.error);
    return std::nullopt;
  }
  pair.set = res.value;

  Result_TWSSetPtr tws_res = parse_tws(tws, tws_size);
  EXPECT_TRUE(tws_res.success);
  if (!tws_res.success) {
    eprintf("%s\n", tws_res.error);
    return std::nullopt;
  }
  pair.tws = tws_res.value;
  return pair;
}

void freeset(LevelsetTwssetPair pair) {
  LevelSet_free(pair.set);
  TWSSet_free(pair.tws);
}

void print_moves(uint16_t level_num, GameInputList const* move_list, uint32_t num_ticks) {
  char moves_chars[GAME_INPUT_DIR_MOVE_LAST] = {};
  moves_chars[INPUT_NIL] = '-';
  moves_chars[INPUT_NORTH] = 'N';
  moves_chars[INPUT_WEST] = 'W';
  moves_chars[INPUT_SOUTH] = 'S';
  moves_chars[INPUT_EAST] = 'E';
  moves_chars[INPUT_NORTH_WEST] = 'Q';
  moves_chars[INPUT_SOUTH_WEST] = 'Z';
  moves_chars[INPUT_NORTH_EAST] = 'R';
  moves_chars[INPUT_SOUTH_EAST] = 'V';

  printf("%u: ", level_num);
  for (size_t i = 0; i < num_ticks; i += 1) {
    putc(moves_chars[move_list->inputs[i]], stdout);
  }
  putc('\n', stdout);
}

bool test_level(Level* level, TWSMetadata const* solution, GameInputList* input_list, bool allow_tick_difference) {
  EXPECT_GE(input_list->count, TWSMetadata_get_length(solution));

  for (size_t j = 0; j < input_list->count; j += 1) {
    Level_set_game_input(level, GameInputList_get_input(input_list, j));
    Level_tick(level);
  }

  if (Level_get_win_state(level) != TRIRES_SUCCESS && level->ruleset->id == RULESET_MS) {
    //you can thank MS for slides into the exit being weird
    Level_set_game_input(level, INPUT_NIL);
    Level_tick(level);
  }

  if (level->ruleset->id == RULESET_LYNX) {
    // Skip through the Lynx endgame timer
    while (level->lx_state.endgame_timer > 0) {
      Level_tick(level);
    }
  }

  EXPECT_EQ(Level_get_win_state(level), TRIRES_SUCCESS);
  if (!allow_tick_difference) {
    EXPECT_EQ(Level_get_current_tick(level) + Level_get_time_offset(level), TWSMetadata_get_length(solution));
  } else {
    int64_t ticks_diff = std::abs(TWSMetadata_get_length(solution) - static_cast<int64_t>(Level_get_current_tick(level) + Level_get_time_offset(level)));
    EXPECT_LE(ticks_diff, 1);
  }

  return Level_get_win_state(level) == TRIRES_SUCCESS;
}

void testset(LevelsetTwssetPair pair, bool allow_tick_difference, size_t skip_levels[], size_t skip_size) {
  EXPECT_EQ(LevelSet_get_levels_n(pair.set), TWSSet_get_solutions_n(pair.tws));

  Ruleset const* ruleset = TWSSet_get_ruleset(pair.tws) == RULESET_MS ? &ms_logic : &lynx_logic;

  for (size_t i = 0; i < LevelSet_get_levels_n(pair.set); i += 1) {
    bool skip = false;
    for (size_t j = 0; j < skip_size; j += 1) {
      if (skip_levels[j] == (i + 1)) {
        skip = true;
        break;
      }
    }
    if (skip) {
      continue;
    }

    Result_LevelPtr level_res = LevelMetadata_make_level(LevelSet_get_level(pair.set, i), ruleset);
    EXPECT_TRUE(level_res.success);
    Level* level = level_res.value;

    TWSMetadata const* solution = TWSSet_get_solution_by_level_num(pair.tws, level->metadata->level_number);
    Level_set_init_step_parity(level, solution->init_step_parity);
    Level_set_rff_dir(level, solution->rff_dir);
    Prng_init_seeded(Level_get_prng_ptr(level), solution->prng_seed);

    Result_GameInputList res = TWSMetadata_prepare_inputs(solution);
    EXPECT_TRUE(res.success);
    GameInputList input_list = res.value;
    if (!test_level(level, solution, &input_list, allow_tick_difference)) {
      print_moves(TWSMetadata_get_level_num(solution), &input_list, TWSMetadata_get_length(solution));
    }
    GameInputList_free(&input_list);
    Level_free(level);
  }
  freeset(pair);
  return;
}

void load_test_set(uint8_t const* levelset, size_t levelset_size, uint8_t const* tws, size_t tws_size,
  bool allow_tick_difference, size_t skip_levels[], size_t skip_size) {
  LevelsetTwssetPairOptional pair = loadsets(levelset, levelset_size, tws, tws_size);
  ASSERT_TRUE(pair.has_value());
  testset(pair.value(), allow_tick_difference, skip_levels, skip_size);
}
