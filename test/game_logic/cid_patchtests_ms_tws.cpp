#include "tws_test_helpers.h"

extern "C" {
#include "formats.h"
#include "format-tws.h"
}

#include "data/ccl/ccl_embeds.h"
#include "data/tws/tws_embeds.h"

class CIDPatchTestsTWS : public testing::Test {
protected:
  LevelSet* level_set = nullptr;
  TWSSet* tws_set = nullptr;
  Level* level = nullptr;
  TWSMetadata* solution = nullptr;
  GameInputList input_list = {};
  CIDPatchTestsTWS() {
    Result_LevelSetPtr res = parse_ccl(CID_PatchTests_ccl, sizeof(CID_PatchTests_ccl));
    EXPECT_TRUE(res.success);
    level_set = res.value;

    Result_TWSSetPtr res2 = parse_tws(CID_PatchTests_ms_tws, sizeof(CID_PatchTests_ms_tws));
    EXPECT_TRUE(res2.success);
    tws_set = res2.value;
  }

  void LoadLevelTws(uint16_t num) {
    if (level != nullptr)
      Level_free(level);
    if (input_list.inputs != nullptr)
      GameInputList_free(&input_list);

    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, num - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    solution = TWSSet_get_solution_by_level_num(tws_set, num);
    Result_GameInputList res3 = TWSMetadata_prepare_inputs(solution);
    ASSERT_TRUE(res3.success);
    input_list = res3.value;
  }

  void TickLevel(GameInput input) {
    Level_set_game_input(level, input);
    Level_tick(level);
    Level_set_game_input(level, INPUT_NIL);
    Level_tick(level);
  }

  void TearDown() override {
    Level_free(level);
    LevelSet_free(level_set);
    TWSSet_free(tws_set);
    GameInputList_free(&input_list);
  }
};

namespace {
  TEST_F(CIDPatchTestsTWS, Level1) {
    LoadLevelTws(1);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level2) {
    LoadLevelTws(2);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level3) {
    LoadLevelTws(3);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level4) {
    LoadLevelTws(4);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level5) {
    LoadLevelTws(5);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level6) {
    LoadLevelTws(6);
    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_SOUTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 2)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_SOUTH));
    Actor const* actors = Level_get_actors_const_ptr(level);
    bool found = false;
    for (size_t i = 0; i < Level_get_actors_n(level); i += 1) {
      int32_t x = Position_get_x(Actor_get_position(&actors[i]));
      int32_t y = Position_get_y(Actor_get_position(&actors[i]));
      if (actors[i].id == CREATURE_BLOCK && x == 2 && y == 1) {
        found = true; // Explicitly check that convergence occurred
        break;
      }
    }
    EXPECT_TRUE(found);
  }

  TEST_F(CIDPatchTestsTWS, Level7) {
    LoadLevelTws(7);
    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_SOUTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 2)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_SOUTH));
    Actor const* actors = Level_get_actors_const_ptr(level);
    bool found = false;
    for (size_t i = 0; i < Level_get_actors_n(level); i += 1) {
      int16_t x = Position_get_x(Actor_get_position(&actors[i]));
      int16_t y = Position_get_y(Actor_get_position(&actors[i]));
      if (Actor_get_id(&actors[i]) == CREATURE_BLOCK && x == 2 && y == 1) {
        found = true; // Explicitly check that convergence occurred
        break;
      }
    }
    EXPECT_TRUE(found);

    LoadLevelTws(7);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_WEST);
    TickLevel(INPUT_SOUTH);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_DIED);
    EXPECT_TRUE(TileID_is_block(Level_get_top_terrain(level, Position_from_xy(2, 1))));
    EXPECT_TRUE(TileID_is_block(Level_get_top_terrain(level, Position_from_xy(1, 1))));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 2)), TILE_EXIT);
  }

  TEST_F(CIDPatchTestsTWS, Level8) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 8 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
  }

  TEST_F(CIDPatchTestsTWS, Level9) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 9 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 0)), TILE_CLONE_MACHINE);
    TickLevel(INPUT_EAST);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_NORTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 0)), TILE_CLONE_MACHINE);
    TickLevel(INPUT_WEST);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_EAST);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 0)), TILE_CLONE_MACHINE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
  }

  TEST_F(CIDPatchTestsTWS, Level10) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 10 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_TANK);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_ICE_CORNER_SOUTH_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_NORTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TILE_FORCE_FLOOR_NORTH);
  }

  TEST_F(CIDPatchTestsTWS, Level11) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 11 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(0, 1)), TILE_BUTTON_TANK);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TILE_BUTTON_TANK);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 2)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
  }

  TEST_F(CIDPatchTestsTWS, Level12) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 12 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_NORTH));
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_FORCE_FLOOR_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_TANK);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TILE_BOMB);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_FORCE_FLOOR_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_TANK);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TILE_FLOOR);

    // I'm honestly unsure what's up with the other 3 tanks so I don't know what to do with them
  }

  TEST_F(CIDPatchTestsTWS, Level13) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 13 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    TickLevel(INPUT_NORTH);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NORTH);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_WEST);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_SOUTH);
    TickLevel(INPUT_SOUTH);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(1, 1)), TILE_BUTTON_TANK);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_NORTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TILE_BUTTON_TANK);
  }

  TEST_F(CIDPatchTestsTWS, Level14) {
    LoadLevelTws(14);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_FLOOR);

    test_level(level, solution, &input_list);
    Actor const* actors = Level_get_actors_const_ptr(level);
    for (size_t i = 0; i < Level_get_actors_n(level); i += 1) {
      if (Actor_get_id(&actors[i]) == CREATURE_FIREBALL) { // Verify both fireballs either died or were erased
        EXPECT_TRUE(Actor_get_hidden(&actors[i])); // The non-existence patch does also kill the thing
        EXPECT_NE(Level_get_top_terrain(level, Actor_get_position(&actors[i])), TileID_actor_with_dir(CREATURE_FIREBALL, DIRECTION_NORTH));
        EXPECT_NE(Level_get_top_terrain(level, Actor_get_position(&actors[i])), TileID_actor_with_dir(CREATURE_FIREBALL, DIRECTION_WEST));
        EXPECT_NE(Level_get_top_terrain(level, Actor_get_position(&actors[i])), TileID_actor_with_dir(CREATURE_FIREBALL, DIRECTION_SOUTH));
        EXPECT_NE(Level_get_top_terrain(level, Actor_get_position(&actors[i])), TileID_actor_with_dir(CREATURE_FIREBALL, DIRECTION_EAST));
      }
    }
  }

  TEST_F(CIDPatchTestsTWS, Level16) {
    LoadLevelTws(16);
    test_level(level, solution, &input_list);

    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(30, 28)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_NORTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(28, 29)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_NORTH));
    Actor const* actors = Level_get_actors_const_ptr(level);
    for (size_t i = 0; i < Level_get_actors_n(level); i += 1) {
      if (Actor_get_id(&actors[i]) == CREATURE_BLOCK) { // Verify the blocks became mutants
        EXPECT_EQ(Level_get_top_terrain(level, Actor_get_position(&actors[i])), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_NORTH));
      }
    }
  }

  TEST_F(CIDPatchTestsTWS, Level17) {
    LoadLevelTws(17);
    test_level(level, solution, &input_list);

    LoadLevelTws(17);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_NORTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 2)), TILE_THIN_WALL_WEST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 3)), TILE_HINT);

    TickLevel(INPUT_SOUTH);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_SOUTH);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_SOUTH);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);
    TickLevel(INPUT_NIL);

    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TILE_FLOOR);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 2)), TILE_THIN_WALL_WEST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 3)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_SOUTH));
    TickLevel(INPUT_EAST);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_DIED);
  }

  TEST_F(CIDPatchTestsTWS, Level18) {
    LoadLevelTws(18);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_FLOOR);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_WEST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TILE_THIN_WALL_SOUTH_EAST);

    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_CHIP, DIRECTION_NORTH));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
  }

  TEST_F(CIDPatchTestsTWS, Level19) {
    LoadLevelTws(19);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TILE_BUTTON_CLONE);

    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_FLOOR);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_WEST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
  }

  TEST_F(CIDPatchTestsTWS, Level20) {
    LoadLevelTws(20);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_FORCE_FLOOR_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TILE_BUTTON_CLONE);

    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_FLOOR);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_WEST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 0)), TILE_BUTTON_CLONE);
  }

  TEST_F(CIDPatchTestsTWS, Level21) {
    LoadLevelTws(21);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_TELEPORT);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_CLONE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_EAST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TILE_TELEPORT);

    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_WEST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_CLONE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TILE_THIN_WALL_SOUTH);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 1)), TILE_TELEPORT);
  }

  TEST_F(CIDPatchTestsTWS, Level22) {
    LoadLevelTws(22);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TILE_ICE_CORNER_SOUTH_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_CLONE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_NORTH));

    test_level(level, solution, &input_list);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CREATURE_TANK, DIRECTION_WEST));
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TILE_BUTTON_CLONE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), TILE_THIN_WALL_SOUTH);
  }

  TEST_F(CIDPatchTestsTWS, Level23) {
    LoadLevelTws(23);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level24) {
    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, 24 - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    TickLevel(INPUT_NORTH);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    TickLevel(INPUT_WEST);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    TickLevel(INPUT_EAST);
    TickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_NOTHING);
    TickLevel(INPUT_WEST);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_SUCCESS);
  }
}
