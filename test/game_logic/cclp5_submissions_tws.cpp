#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tws_test_helpers.h"
#include "data/ccl/ccl_embeds.h"
#include "data/tws/tws_embeds.h"

#define CCLP5_VOTING_TEST_SET(name) TEST(CCLP5MS, LoadAndPlay##name) {\
  load_test_set(CCLP5_voting_##name##_ccl, sizeof(CCLP5_voting_##name##_ccl), CCLP5_voting_##name##_ms_tws, sizeof(CCLP5_voting_##name##_ms_tws), true);\
}\
TEST(CCLP5Lynx, LoadAndPlay##name) {\
  load_test_set(CCLP5_voting_##name##_ccl, sizeof(CCLP5_voting_##name##_ccl), CCLP5_voting_##name##_lynx_tws, sizeof(CCLP5_voting_##name##_lynx_tws));\
}

namespace {
  CCLP5_VOTING_TEST_SET(Acrylic) // MS warnings about "no creature at location (31 31)" expected

  CCLP5_VOTING_TEST_SET(Broadcast)

  CCLP5_VOTING_TEST_SET(Chocolate)

  CCLP5_VOTING_TEST_SET(Darkness)

  CCLP5_VOTING_TEST_SET(Eagle)

  CCLP5_VOTING_TEST_SET(Fertilizer)

  CCLP5_VOTING_TEST_SET(Gobbledygook)

  CCLP5_VOTING_TEST_SET(Halo)

  CCLP5_VOTING_TEST_SET(Immunity)

  CCLP5_VOTING_TEST_SET(Initiative) // Lynx warnings about "Red button not connected to a clone machine" expected

  CCLP5_VOTING_TEST_SET(Jellyfish)

  CCLP5_VOTING_TEST_SET(Juicy)

  CCLP5_VOTING_TEST_SET(Krypton)

  TEST(CCLP5MS, LoadAndPlayLlama) {
    size_t skips[] = {3, 47}; // Does not sync as a likely result of TW logic patches
    // > Llama 3 is likely bug #15 (TANKS STUCK ON TELEPORTERS SHOULD NOT MOVE OFF IN REVERSE),
    // > see tank at 21,12 at 441.2 (tick 1196)
    // Llama 47 is tank top, the tank at 12,30 at 623.8 doesn't slide back and forth anymore
    load_test_set(CCLP5_voting_Llama_ccl, sizeof(CCLP5_voting_Llama_ccl), CCLP5_voting_Llama_ms_tws, sizeof(CCLP5_voting_Llama_ms_tws), true, skips, std::size(skips));
  }

  TEST(CCLP5Lynx, LoadAndPlayLlama) {
    load_test_set(CCLP5_voting_Llama_ccl, sizeof(CCLP5_voting_Llama_ccl), CCLP5_voting_Llama_lynx_tws, sizeof(CCLP5_voting_Llama_lynx_tws));
  }

  CCLP5_VOTING_TEST_SET(Mobius)

  CCLP5_VOTING_TEST_SET(Nature)

  CCLP5_VOTING_TEST_SET(Nonsense)

  TEST(CCLP5MS, LoadAndPlayOxford) {
    size_t skips[] = {20}; // Does not sync as a likely result of TW logic patches
    // > Oxford 20 looks like bug #13 [which is now bug #16 I think] (the tank gets cloned but doesn't move,
    // > and the pinkball stalls it before it can move)
    load_test_set(CCLP5_voting_Oxford_ccl, sizeof(CCLP5_voting_Oxford_ccl), CCLP5_voting_Oxford_ms_tws, sizeof(CCLP5_voting_Oxford_ms_tws), true, skips, std::size(skips));
  }

  TEST(CCLP5Lynx, LoadAndPlayOxford) {
    load_test_set(CCLP5_voting_Oxford_ccl, sizeof(CCLP5_voting_Oxford_ccl), CCLP5_voting_Oxford_lynx_tws, sizeof(CCLP5_voting_Oxford_lynx_tws));
  }

  CCLP5_VOTING_TEST_SET(Plastic)

  CCLP5_VOTING_TEST_SET(Qualification)

  CCLP5_VOTING_TEST_SET(Raspberry)

  CCLP5_VOTING_TEST_SET(Razor)

  CCLP5_VOTING_TEST_SET(Spatula)

  CCLP5_VOTING_TEST_SET(Supermarket)

  CCLP5_VOTING_TEST_SET(Tangent)

  CCLP5_VOTING_TEST_SET(Technetium)

  CCLP5_VOTING_TEST_SET(Tuxedo)

  CCLP5_VOTING_TEST_SET(Uniform)

  CCLP5_VOTING_TEST_SET(Universal)

  CCLP5_VOTING_TEST_SET(Vanadium)

  CCLP5_VOTING_TEST_SET(Wilderness)

  CCLP5_VOTING_TEST_SET(Xiphioid)

  CCLP5_VOTING_TEST_SET(Yogurt)

  CCLP5_VOTING_TEST_SET(Zipline)
}
