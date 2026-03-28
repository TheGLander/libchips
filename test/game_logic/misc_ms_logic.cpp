#include "tws_test_helpers.h"

extern "C" {
#include "formats.h"
#include "format-tws.h"
}

#include "data/ccl/ccl_embeds.h"

class TrapDatLogicTestMS : public testing::Test { // Put into its own since editing a trap DAT will require manual editing after
protected:
  LevelSet* level_set;
  Level* level = nullptr;
  TrapDatLogicTestMS() {
    Result_LevelSetPtr res = parse_ccl(TrapDatTestsMS_ccl, sizeof(TrapDatTestsMS_ccl));
    EXPECT_TRUE(res.success);
    level_set = res.value;
  }

  void LoadLevel(char const pass[10]) {
    if (level != nullptr)
      Level_free(level);

    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level_by_pass(level_set, pass), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;
  }

  void TickLevel(GameInput input) {
    Level_set_game_input(level, input);
    Level_tick(level);
    Level_set_game_input(level, INPUT_NIL);
    Level_tick(level);
  }

  void QuadTickLevel(GameInput input) {
    TickLevel(input);
    TickLevel(INPUT_NIL);
  }
};

namespace {
  TEST_F(TrapDatLogicTestMS, TrapOpen) {
    LoadLevel("TRPO");
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), BLOCK_STATIC);
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 1)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(3, 1)), ICE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 1)), WATER);

    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 3)), BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 3)), BUTTON_CLONE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 3)), TileID_actor_with_dir(BLOCK, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 3)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(3, 3)), ICE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 3)), WATER);

    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 5)), BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), FLOOR);
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 6)), BLOCK_STATIC);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(3, 5)), ICE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 5)), WATER);

    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), TileID_actor_with_dir(CHIP, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 1)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 1)), DIRT);

    LoadLevel("TRPO");
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 3)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 3)), DIRT);

    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), TileID_actor_with_dir(CHIP, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);
    QuadTickLevel(INPUT_NORTH);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 4)), TileID_actor_with_dir(CHIP, INPUT_NORTH));

    LoadLevel("TRPO");
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_NORTH);
    QuadTickLevel(INPUT_NORTH);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), TileID_actor_with_dir(CHIP, INPUT_NORTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 4)), BLOCK_STATIC);
  }

  TEST_F(TrapDatLogicTestMS, TrapClosed) {
      LoadLevel("TRPC");
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 1)), BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), BLOCK_STATIC);
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 1)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(3, 1)), ICE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 1)), WATER);

    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 3)), BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 3)), BUTTON_CLONE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 3)), TileID_actor_with_dir(BLOCK, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 3)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(3, 3)), ICE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 3)), WATER);

    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 5)), BUTTON_TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), FLOOR);
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 6)), BLOCK_STATIC);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(3, 5)), ICE);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 5)), WATER);

    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 1)), BLOCK_STATIC);
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 1)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 1)), WATER);

    LoadLevel("TRPC");
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 3)), TileID_actor_with_dir(BLOCK, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 3)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 3)), WATER);
    QuadTickLevel(INPUT_WEST);
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_NIL);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 3)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(4, 3)), DIRT);

    LoadLevel("TRPC");
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), TileID_actor_with_dir(CHIP, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);
    QuadTickLevel(INPUT_NORTH);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), TileID_actor_with_dir(CHIP, INPUT_NORTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);

    LoadLevel("TRPC");
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_SOUTH);
    QuadTickLevel(INPUT_EAST);
    QuadTickLevel(INPUT_NORTH);
    QuadTickLevel(INPUT_NORTH);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 5)), BLOCK_STATIC);
      EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(2, 5)), TRAP);
      EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(2, 4)), FLOOR);
  }
}
