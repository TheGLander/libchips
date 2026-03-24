#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "formats.h"
}

#include "data/ccl/ccl_embeds.h"

class HashEqualsTestMS : public testing::Test {
protected:
  LevelSet* set;
  LevelMetadata* meta;
  Level* level1;
  Level* level2;

  HashEqualsTestMS() {
    Result_LevelSetPtr res = parse_ccl(CCLP1_ccl, sizeof(CCLP1_ccl));
    EXPECT_TRUE(res.success);

    set = res.value;
    EXPECT_EQ(LevelSet_get_levels_n(set), 149);

    Result_LevelPtr res_level1 = LevelMetadata_make_level(LevelSet_get_level(set, 0), &ms_logic);
    EXPECT_TRUE(res_level1.success);
    level1 = res_level1.value;
    Result_LevelPtr res_level2 = LevelMetadata_make_level(LevelSet_get_level(set, 0), &ms_logic);
    EXPECT_TRUE(res_level2.success);
    level2 = res_level2.value;
  }

  void TearDown() override {
    Level_free(level1);
    Level_free(level2);
    LevelSet_free(set);
  }
};

class HashEqualsTestLynx : public testing::Test {
protected:
  LevelSet* set;
  LevelMetadata* meta;
  Level* level1;
  Level* level2;

  HashEqualsTestLynx() {
    Result_LevelSetPtr res = parse_ccl(CCLP1_ccl, sizeof(CCLP1_ccl));
    EXPECT_TRUE(res.success);

    set = res.value;
    EXPECT_EQ(LevelSet_get_levels_n(set), 149);

    Result_LevelPtr res_level1 = LevelMetadata_make_level(LevelSet_get_level(set, 0), &lynx_logic);
    EXPECT_TRUE(res_level1.success);
    level1 = res_level1.value;
    Result_LevelPtr res_level2 = LevelMetadata_make_level(LevelSet_get_level(set, 0), &lynx_logic);
    EXPECT_TRUE(res_level2.success);
    level2 = res_level2.value;
  }

  void TearDown() override {
    Level_free(level1);
    Level_free(level2);
    LevelSet_free(set);
  }
};

namespace {
  TEST_F(HashEqualsTestMS, DefaultEquals) {
    ASSERT_NE(level1, nullptr);
    ASSERT_NE(level2, nullptr);
    ASSERT_NE(level1, level2);
    EXPECT_TRUE(Level_equals(level1, level2));
  }

  TEST_F(HashEqualsTestMS, DefaultHash) {
    hash_t hash1 = Level_get_hash(level1);
    hash_t hash2 = Level_get_hash(level2);
    EXPECT_EQ(hash1, hash2);
  }

  TEST_F(HashEqualsTestMS, SameMoves) {
    Level_set_game_input(level1, INPUT_NORTH);
    Level_set_game_input(level2, INPUT_NORTH);
    Level_tick(level1);
    Level_tick(level2);
    EXPECT_TRUE(Level_equals(level1, level2));
    EXPECT_EQ(Level_get_hash(level1), Level_get_hash(level2));
  }

  void quad_tick(Level* level, GameInput input) {
    Level_set_game_input(level, input);
    Level_tick(level);
    Level_set_game_input(level, INPUT_NIL);
    Level_tick(level);
    Level_tick(level);
    Level_tick(level);
  }

  TEST_F(HashEqualsTestMS, DifferentMovesDifferentState) {
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level2, INPUT_EAST);
    EXPECT_FALSE(Level_equals(level1, level2));
    EXPECT_NE(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST_F(HashEqualsTestMS, DifferentMovesSameState) {
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_EAST);
    quad_tick(level1, INPUT_NORTH); // Crash into the wall to ensure same dir

    quad_tick(level2, INPUT_NORTH);
    quad_tick(level2, INPUT_EAST);
    quad_tick(level2, INPUT_NORTH);
    quad_tick(level2, INPUT_NORTH); // Ditto on wall crash

    EXPECT_TRUE(Level_equals(level1, level2));
    EXPECT_EQ(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST_F(HashEqualsTestMS, DifferentMoves2) {
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_EAST); // Without the north move Chip's direction will change

    quad_tick(level2, INPUT_NORTH);
    quad_tick(level2, INPUT_EAST);
    quad_tick(level2, INPUT_NORTH);

    EXPECT_FALSE(Level_equals(level1, level2));
    EXPECT_NE(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST_F(HashEqualsTestLynx, DefaultEquals) {
    ASSERT_NE(level1, nullptr);
    ASSERT_NE(level2, nullptr);
    ASSERT_NE(level1, level2);
    EXPECT_TRUE(Level_equals(level1, level2));
  }

  TEST_F(HashEqualsTestLynx, DefaultHash) {
    hash_t hash1 = Level_get_hash(level1);
    hash_t hash2 = Level_get_hash(level2);
    EXPECT_EQ(hash1, hash2);
  }

  TEST_F(HashEqualsTestLynx, SameMoves) {
    Level_set_game_input(level1, INPUT_NORTH);
    Level_set_game_input(level2, INPUT_NORTH);
    Level_tick(level1);
    Level_tick(level2);
    EXPECT_TRUE(Level_equals(level1, level2));
    EXPECT_EQ(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST_F(HashEqualsTestLynx, DifferentMovesDifferentState) {
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level2, INPUT_EAST);
    EXPECT_FALSE(Level_equals(level1, level2));
    EXPECT_NE(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST_F(HashEqualsTestLynx, DifferentMovesSameState) {
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_EAST);
    quad_tick(level1, INPUT_NORTH); // Crash into the wall to ensure same dir

    quad_tick(level2, INPUT_NORTH);
    quad_tick(level2, INPUT_EAST);
    quad_tick(level2, INPUT_NORTH);
    quad_tick(level2, INPUT_NORTH); // Ditto on wall crash

    EXPECT_TRUE(Level_equals(level1, level2));
    EXPECT_EQ(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST_F(HashEqualsTestLynx, DifferentMoves2) {
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_NORTH);
    quad_tick(level1, INPUT_EAST); // Without the north move Chip's direction will change

    quad_tick(level2, INPUT_NORTH);
    quad_tick(level2, INPUT_EAST);
    quad_tick(level2, INPUT_NORTH);

    EXPECT_FALSE(Level_equals(level1, level2));
    EXPECT_NE(Level_get_hash(level1), Level_get_hash(level2));
  }

  TEST(HashDifferentLevels, DifferentLevels) {
    Result_LevelSetPtr res = parse_ccl(CCLP1_ccl, sizeof(CCLP1_ccl));
    EXPECT_TRUE(res.success);

    LevelSet* set = res.value;
    EXPECT_EQ(LevelSet_get_levels_n(set), 149);

    Result_LevelPtr res_level1 = LevelMetadata_make_level(LevelSet_get_level(set, 0), &lynx_logic);
    EXPECT_TRUE(res_level1.success);
    Level* level1 = res_level1.value;
    Result_LevelPtr res_level2 = LevelMetadata_make_level(LevelSet_get_level(set, 1), &lynx_logic);
    EXPECT_TRUE(res_level2.success);
    Level* level2 = res_level2.value;

    EXPECT_FALSE(Level_equals(level1, level2));
    EXPECT_NE(Level_get_hash(level1), Level_get_hash(level2));
    Level_free(level1);
    Level_free(level2);

    res_level1 = LevelMetadata_make_level(LevelSet_get_level(set, 0), &ms_logic);
    EXPECT_TRUE(res_level1.success);
    level1 = res_level1.value;
    res_level2 = LevelMetadata_make_level(LevelSet_get_level(set, 1), &ms_logic);
    EXPECT_TRUE(res_level2.success);
    level2 = res_level2.value;

    EXPECT_FALSE(Level_equals(level1, level2));
    EXPECT_NE(Level_get_hash(level1), Level_get_hash(level2));
    Level_free(level1);
    Level_free(level2);

    LevelSet_free(set);
  }
}
