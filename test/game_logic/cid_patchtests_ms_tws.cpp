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

  void LoadLevel(uint16_t num) {
    if (level != nullptr)
      Level_free(level);
    if (input_list.inputs != nullptr)
      GameInputList_free(&input_list);

    Result_LevelPtr res_level = LevelMetadata_make_level(LevelSet_get_level(level_set, num - 1), &ms_logic);
    ASSERT_TRUE(res_level.success);
    level = res_level.value;

    solution = TWSSet_get_solution_by_idx(tws_set, num - 1);
    Result_GameInputList res3 = TWSMetadata_prepare_inputs(solution);
    ASSERT_TRUE(res3.success);
    input_list = res3.value;
  }

  void TickLevel(GameInput input) {
    Level_set_game_input(level, input);
    Level_tick(level);
    Level_set_game_input(level, DIRECTION_NIL);
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
    LoadLevel(1);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level2) {
    LoadLevel(2);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level3) {
    LoadLevel(3);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level4) {
    LoadLevel(4);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level5) {
    LoadLevel(5);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level6) {
    LoadLevel(6);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level7) {
    LoadLevel(7);
    test_level(level, solution, &input_list);

    LoadLevel(7);
    TickLevel(DIRECTION_NIL);
    TickLevel(DIRECTION_WEST);
    TickLevel(DIRECTION_SOUTH);
    EXPECT_EQ(Level_get_win_state(level), TRIRES_DIED);
  }

  // todo: some way to verify the tank top levels, I don't know

  TEST_F(CIDPatchTestsTWS, Level14) {
    LoadLevel(14);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level16) {
    LoadLevel(16);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level17) {
    LoadLevel(17);
    test_level(level, solution, &input_list);
  }
  // todo: something to verify if you move too soon on 17 and end up with multiple tanks

  TEST_F(CIDPatchTestsTWS, Level18) {
    LoadLevel(18);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level19) {
    LoadLevel(19);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level20) {
    LoadLevel(20);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level21) {
    LoadLevel(21);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level22) {
    LoadLevel(14);
    test_level(level, solution, &input_list);
  }

  TEST_F(CIDPatchTestsTWS, Level23) {
    LoadLevel(23);
    test_level(level, solution, &input_list);
  }
}
