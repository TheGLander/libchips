#include "tws_test_helpers.h"

extern "C" {
#include "formats.h"
#include "format-tws.h"
}

#include "data/ccl/ccl_embeds.h"

class MiscLogicTestMS : public testing::Test {
protected:
  LevelSet* level_set;
  Level* level = nullptr;
  MiscLogicTestMS() {
    Result_LevelSetPtr res = parse_ccl(MiscTestsMS_ccl, sizeof(MiscTestsMS_ccl));
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
  TEST_F(MiscLogicTestMS, TrapOpen) {
    LoadLevel("TRPO");
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CHIP, INPUT_SOUTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(0, 0)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), EXIT);
    QuadTickLevel(INPUT_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), TileID_actor_with_dir(CHIP, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(1, 0)), EXIT);
  }

  TEST_F(MiscLogicTestMS, TrapClosed) {
    LoadLevel("TRPC");
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CHIP, INPUT_SOUTH));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(0, 0)), TRAP);
    QuadTickLevel(INPUT_EAST);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(0, 0)), TileID_actor_with_dir(CHIP, INPUT_EAST));
    EXPECT_EQ(Level_get_bottom_terrain(level, Position_from_xy(0, 0)), TRAP);
    EXPECT_EQ(Level_get_top_terrain(level, Position_from_xy(1, 0)), EXIT);
  }
}
