#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tws_test_helpers.h"
#include "data/ccl/ccl_embeds.h"
#include "data/tws/tws_embeds.h"

namespace {
  TEST(CCLP5MS, LoadAndPlayAcrylic) {
    load_test_set(CCLP5_voting_Acrylic_ccl, sizeof(CCLP5_voting_Acrylic_ccl), CCLP5_voting_Acrylic_ms_tws, sizeof(CCLP5_voting_Acrylic_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayAcrylic) {
    load_test_set(CCLP5_voting_Acrylic_ccl, sizeof(CCLP5_voting_Acrylic_ccl), CCLP5_voting_Acrylic_lynx_tws, sizeof(CCLP5_voting_Acrylic_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayBroadcast) {
    load_test_set(CCLP5_voting_Broadcast_ccl, sizeof(CCLP5_voting_Broadcast_ccl), CCLP5_voting_Broadcast_ms_tws, sizeof(CCLP5_voting_Broadcast_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayBroadcast) {
    load_test_set(CCLP5_voting_Broadcast_ccl, sizeof(CCLP5_voting_Broadcast_ccl), CCLP5_voting_Broadcast_lynx_tws, sizeof(CCLP5_voting_Broadcast_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayChocolate) {
    load_test_set(CCLP5_voting_Chocolate_ccl, sizeof(CCLP5_voting_Chocolate_ccl), CCLP5_voting_Chocolate_ms_tws, sizeof(CCLP5_voting_Chocolate_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayChocolate) {
    load_test_set(CCLP5_voting_Chocolate_ccl, sizeof(CCLP5_voting_Chocolate_ccl), CCLP5_voting_Chocolate_lynx_tws, sizeof(CCLP5_voting_Chocolate_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayDarkness) {
    load_test_set(CCLP5_voting_Darkness_ccl, sizeof(CCLP5_voting_Darkness_ccl), CCLP5_voting_Darkness_ms_tws, sizeof(CCLP5_voting_Darkness_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayDarkness) {
    load_test_set(CCLP5_voting_Darkness_ccl, sizeof(CCLP5_voting_Darkness_ccl), CCLP5_voting_Darkness_lynx_tws, sizeof(CCLP5_voting_Darkness_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayEagle) {
    load_test_set(CCLP5_voting_Eagle_ccl, sizeof(CCLP5_voting_Eagle_ccl), CCLP5_voting_Eagle_ms_tws, sizeof(CCLP5_voting_Eagle_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayEagle) {
    load_test_set(CCLP5_voting_Eagle_ccl, sizeof(CCLP5_voting_Eagle_ccl), CCLP5_voting_Eagle_lynx_tws, sizeof(CCLP5_voting_Eagle_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayFertilizer) {
    load_test_set(CCLP5_voting_Fertilizer_ccl, sizeof(CCLP5_voting_Fertilizer_ccl), CCLP5_voting_Fertilizer_ms_tws, sizeof(CCLP5_voting_Fertilizer_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayFertilizer) {
    load_test_set(CCLP5_voting_Fertilizer_ccl, sizeof(CCLP5_voting_Fertilizer_ccl), CCLP5_voting_Fertilizer_lynx_tws, sizeof(CCLP5_voting_Fertilizer_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayGobbledygook) {
    load_test_set(CCLP5_voting_Gobbledygook_ccl, sizeof(CCLP5_voting_Gobbledygook_ccl), CCLP5_voting_Gobbledygook_ms_tws, sizeof(CCLP5_voting_Gobbledygook_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayGobbledygook) {
    load_test_set(CCLP5_voting_Gobbledygook_ccl, sizeof(CCLP5_voting_Gobbledygook_ccl), CCLP5_voting_Gobbledygook_lynx_tws, sizeof(CCLP5_voting_Gobbledygook_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayHalo) {
    load_test_set(CCLP5_voting_Halo_ccl, sizeof(CCLP5_voting_Halo_ccl), CCLP5_voting_Halo_ms_tws, sizeof(CCLP5_voting_Halo_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayHalo) {
    load_test_set(CCLP5_voting_Halo_ccl, sizeof(CCLP5_voting_Halo_ccl), CCLP5_voting_Halo_lynx_tws, sizeof(CCLP5_voting_Halo_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayImmunity) {
    load_test_set(CCLP5_voting_Immunity_ccl, sizeof(CCLP5_voting_Immunity_ccl), CCLP5_voting_Immunity_ms_tws, sizeof(CCLP5_voting_Immunity_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayImmunity) {
    load_test_set(CCLP5_voting_Immunity_ccl, sizeof(CCLP5_voting_Immunity_ccl), CCLP5_voting_Immunity_lynx_tws, sizeof(CCLP5_voting_Immunity_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayInitiative) {
    load_test_set(CCLP5_voting_Initiative_ccl, sizeof(CCLP5_voting_Initiative_ccl), CCLP5_voting_Initiative_ms_tws, sizeof(CCLP5_voting_Initiative_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayInitiative) {
    load_test_set(CCLP5_voting_Initiative_ccl, sizeof(CCLP5_voting_Initiative_ccl), CCLP5_voting_Initiative_lynx_tws, sizeof(CCLP5_voting_Initiative_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayJellyfish) {
    load_test_set(CCLP5_voting_Jellyfish_ccl, sizeof(CCLP5_voting_Jellyfish_ccl), CCLP5_voting_Jellyfish_ms_tws, sizeof(CCLP5_voting_Jellyfish_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayJellyfish) {
    load_test_set(CCLP5_voting_Jellyfish_ccl, sizeof(CCLP5_voting_Jellyfish_ccl), CCLP5_voting_Jellyfish_lynx_tws, sizeof(CCLP5_voting_Jellyfish_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayJuicy) {
    load_test_set(CCLP5_voting_Juicy_ccl, sizeof(CCLP5_voting_Juicy_ccl), CCLP5_voting_Juicy_ms_tws, sizeof(CCLP5_voting_Juicy_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayJuicy) {
    load_test_set(CCLP5_voting_Juicy_ccl, sizeof(CCLP5_voting_Juicy_ccl), CCLP5_voting_Juicy_lynx_tws, sizeof(CCLP5_voting_Juicy_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayKrypton) {
    load_test_set(CCLP5_voting_Krypton_ccl, sizeof(CCLP5_voting_Krypton_ccl), CCLP5_voting_Krypton_ms_tws, sizeof(CCLP5_voting_Krypton_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayKrypton) {
    load_test_set(CCLP5_voting_Krypton_ccl, sizeof(CCLP5_voting_Krypton_ccl), CCLP5_voting_Krypton_lynx_tws, sizeof(CCLP5_voting_Krypton_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayLlama) {
    size_t skips[] = {3}; // Does not sync as a likely result of TW logic patches
    // > Llama 3 is likely bug #15 (TANKS STUCK ON TELEPORTERS SHOULD NOT MOVE OFF IN REVERSE),
    // > see tank at 21,12 at 441.2 (tick 1196)
    load_test_set(CCLP5_voting_Llama_ccl, sizeof(CCLP5_voting_Llama_ccl), CCLP5_voting_Llama_ms_tws, sizeof(CCLP5_voting_Llama_ms_tws), true, skips, std::size(skips));
  }

  TEST(CCLP5Lynx, LoadAndPlayLlama) {
    load_test_set(CCLP5_voting_Llama_ccl, sizeof(CCLP5_voting_Llama_ccl), CCLP5_voting_Llama_lynx_tws, sizeof(CCLP5_voting_Llama_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayMobius) {
    load_test_set(CCLP5_voting_Mobius_ccl, sizeof(CCLP5_voting_Mobius_ccl), CCLP5_voting_Mobius_ms_tws, sizeof(CCLP5_voting_Mobius_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayMobius) {
    load_test_set(CCLP5_voting_Mobius_ccl, sizeof(CCLP5_voting_Mobius_ccl), CCLP5_voting_Mobius_lynx_tws, sizeof(CCLP5_voting_Mobius_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayNature) {
    load_test_set(CCLP5_voting_Nature_ccl, sizeof(CCLP5_voting_Nature_ccl), CCLP5_voting_Nature_ms_tws, sizeof(CCLP5_voting_Nature_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayNature) {
    load_test_set(CCLP5_voting_Nature_ccl, sizeof(CCLP5_voting_Nature_ccl), CCLP5_voting_Nature_lynx_tws, sizeof(CCLP5_voting_Nature_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayNonsense) {
    load_test_set(CCLP5_voting_Nonsense_ccl, sizeof(CCLP5_voting_Nonsense_ccl), CCLP5_voting_Nonsense_ms_tws, sizeof(CCLP5_voting_Nonsense_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayNonsense) {
    load_test_set(CCLP5_voting_Nonsense_ccl, sizeof(CCLP5_voting_Nonsense_ccl), CCLP5_voting_Nonsense_lynx_tws, sizeof(CCLP5_voting_Nonsense_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayOxford) {
    size_t skips[] = {20}; // Does not sync as a likely result of TW logic patches
    // > Oxford 20 looks like bug #13 [which is now bug #16 I think] (the tank gets cloned but doesn't move,
    // > and the pinkball stalls it before it can move)
    load_test_set(CCLP5_voting_Oxford_ccl, sizeof(CCLP5_voting_Oxford_ccl), CCLP5_voting_Oxford_ms_tws, sizeof(CCLP5_voting_Oxford_ms_tws), true, skips, std::size(skips));
  }

  TEST(CCLP5Lynx, LoadAndPlayOxford) {
    load_test_set(CCLP5_voting_Oxford_ccl, sizeof(CCLP5_voting_Oxford_ccl), CCLP5_voting_Oxford_lynx_tws, sizeof(CCLP5_voting_Oxford_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayPlastic) {
    load_test_set(CCLP5_voting_Plastic_ccl, sizeof(CCLP5_voting_Plastic_ccl), CCLP5_voting_Plastic_ms_tws, sizeof(CCLP5_voting_Plastic_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayPlastic) {
    load_test_set(CCLP5_voting_Plastic_ccl, sizeof(CCLP5_voting_Plastic_ccl), CCLP5_voting_Plastic_lynx_tws, sizeof(CCLP5_voting_Plastic_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayQualification) {
    load_test_set(CCLP5_voting_Qualification_ccl, sizeof(CCLP5_voting_Qualification_ccl), CCLP5_voting_Qualification_ms_tws, sizeof(CCLP5_voting_Qualification_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayQualification) {
    load_test_set(CCLP5_voting_Qualification_ccl, sizeof(CCLP5_voting_Qualification_ccl), CCLP5_voting_Qualification_lynx_tws, sizeof(CCLP5_voting_Qualification_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayRaspberry) {
    load_test_set(CCLP5_voting_Raspberry_ccl, sizeof(CCLP5_voting_Raspberry_ccl), CCLP5_voting_Raspberry_ms_tws, sizeof(CCLP5_voting_Raspberry_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayRaspberry) {
    load_test_set(CCLP5_voting_Raspberry_ccl, sizeof(CCLP5_voting_Raspberry_ccl), CCLP5_voting_Raspberry_lynx_tws, sizeof(CCLP5_voting_Raspberry_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayRazor) {
    load_test_set(CCLP5_voting_Razor_ccl, sizeof(CCLP5_voting_Razor_ccl), CCLP5_voting_Razor_ms_tws, sizeof(CCLP5_voting_Razor_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayRazor) {
    load_test_set(CCLP5_voting_Razor_ccl, sizeof(CCLP5_voting_Razor_ccl), CCLP5_voting_Razor_lynx_tws, sizeof(CCLP5_voting_Razor_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlaySpatula) {
    load_test_set(CCLP5_voting_Spatula_ccl, sizeof(CCLP5_voting_Spatula_ccl), CCLP5_voting_Spatula_ms_tws, sizeof(CCLP5_voting_Spatula_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlaySpatula) {
    load_test_set(CCLP5_voting_Spatula_ccl, sizeof(CCLP5_voting_Spatula_ccl), CCLP5_voting_Spatula_lynx_tws, sizeof(CCLP5_voting_Spatula_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlaySupermarket) {
    load_test_set(CCLP5_voting_Supermarket_ccl, sizeof(CCLP5_voting_Supermarket_ccl), CCLP5_voting_Supermarket_ms_tws, sizeof(CCLP5_voting_Supermarket_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlaySupermarket) {
    load_test_set(CCLP5_voting_Supermarket_ccl, sizeof(CCLP5_voting_Supermarket_ccl), CCLP5_voting_Supermarket_lynx_tws, sizeof(CCLP5_voting_Supermarket_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayTangent) {
    load_test_set(CCLP5_voting_Tangent_ccl, sizeof(CCLP5_voting_Tangent_ccl), CCLP5_voting_Tangent_ms_tws, sizeof(CCLP5_voting_Tangent_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayTangent) {
    load_test_set(CCLP5_voting_Tangent_ccl, sizeof(CCLP5_voting_Tangent_ccl), CCLP5_voting_Tangent_lynx_tws, sizeof(CCLP5_voting_Tangent_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayTechnetium) {
    load_test_set(CCLP5_voting_Technetium_ccl, sizeof(CCLP5_voting_Technetium_ccl), CCLP5_voting_Technetium_ms_tws, sizeof(CCLP5_voting_Technetium_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayTechnetium) {
    load_test_set(CCLP5_voting_Technetium_ccl, sizeof(CCLP5_voting_Technetium_ccl), CCLP5_voting_Technetium_lynx_tws, sizeof(CCLP5_voting_Technetium_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayTuxedo) {
    load_test_set(CCLP5_voting_Tuxedo_ccl, sizeof(CCLP5_voting_Tuxedo_ccl), CCLP5_voting_Tuxedo_ms_tws, sizeof(CCLP5_voting_Tuxedo_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayTuxedo) {
    load_test_set(CCLP5_voting_Tuxedo_ccl, sizeof(CCLP5_voting_Tuxedo_ccl), CCLP5_voting_Tuxedo_lynx_tws, sizeof(CCLP5_voting_Tuxedo_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayUniform) {
    load_test_set(CCLP5_voting_Uniform_ccl, sizeof(CCLP5_voting_Uniform_ccl), CCLP5_voting_Uniform_ms_tws, sizeof(CCLP5_voting_Uniform_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayUniform) {
    load_test_set(CCLP5_voting_Uniform_ccl, sizeof(CCLP5_voting_Uniform_ccl), CCLP5_voting_Uniform_lynx_tws, sizeof(CCLP5_voting_Uniform_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayUniversal) {
    load_test_set(CCLP5_voting_Universal_ccl, sizeof(CCLP5_voting_Universal_ccl), CCLP5_voting_Universal_ms_tws, sizeof(CCLP5_voting_Universal_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayUniversal) {
    load_test_set(CCLP5_voting_Universal_ccl, sizeof(CCLP5_voting_Universal_ccl), CCLP5_voting_Universal_lynx_tws, sizeof(CCLP5_voting_Universal_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayVanadium) {
    load_test_set(CCLP5_voting_Vanadium_ccl, sizeof(CCLP5_voting_Vanadium_ccl), CCLP5_voting_Vanadium_ms_tws, sizeof(CCLP5_voting_Vanadium_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayVanadium) {
    load_test_set(CCLP5_voting_Vanadium_ccl, sizeof(CCLP5_voting_Vanadium_ccl), CCLP5_voting_Vanadium_lynx_tws, sizeof(CCLP5_voting_Vanadium_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayWilderness) {
    load_test_set(CCLP5_voting_Wilderness_ccl, sizeof(CCLP5_voting_Wilderness_ccl), CCLP5_voting_Wilderness_ms_tws, sizeof(CCLP5_voting_Wilderness_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayWilderness) {
    load_test_set(CCLP5_voting_Wilderness_ccl, sizeof(CCLP5_voting_Wilderness_ccl), CCLP5_voting_Wilderness_lynx_tws, sizeof(CCLP5_voting_Wilderness_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayXiphioid) {
    load_test_set(CCLP5_voting_Xiphioid_ccl, sizeof(CCLP5_voting_Xiphioid_ccl), CCLP5_voting_Xiphioid_ms_tws, sizeof(CCLP5_voting_Xiphioid_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayXiphioid) {
    load_test_set(CCLP5_voting_Xiphioid_ccl, sizeof(CCLP5_voting_Xiphioid_ccl), CCLP5_voting_Xiphioid_lynx_tws, sizeof(CCLP5_voting_Xiphioid_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayYogurt) {
    load_test_set(CCLP5_voting_Yogurt_ccl, sizeof(CCLP5_voting_Yogurt_ccl), CCLP5_voting_Yogurt_ms_tws, sizeof(CCLP5_voting_Yogurt_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayYogurt) {
    load_test_set(CCLP5_voting_Yogurt_ccl, sizeof(CCLP5_voting_Yogurt_ccl), CCLP5_voting_Yogurt_lynx_tws, sizeof(CCLP5_voting_Yogurt_lynx_tws));
  }

  TEST(CCLP5MS, LoadAndPlayZipline) {
    load_test_set(CCLP5_voting_Zipline_ccl, sizeof(CCLP5_voting_Zipline_ccl), CCLP5_voting_Zipline_ms_tws, sizeof(CCLP5_voting_Zipline_ms_tws), true);
  }

  TEST(CCLP5Lynx, LoadAndPlayZipline) {
    load_test_set(CCLP5_voting_Zipline_ccl, sizeof(CCLP5_voting_Zipline_ccl), CCLP5_voting_Zipline_lynx_tws, sizeof(CCLP5_voting_Zipline_lynx_tws));
  }
}