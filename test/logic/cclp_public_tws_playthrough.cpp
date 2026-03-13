#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tws_test_helpers.h"
#include "data/ccl/ccl_embeds.h"
#include "data/tws/tws_embeds.h"

namespace {
  TEST(MSCCLP, LoadAndPlayCCLP1) {
    load_test_set(CCLP1_ccl, sizeof(CCLP1_ccl), public_CCLP1_tws, sizeof(public_CCLP1_tws));
  }

  TEST(LynxCCLP, LoadAndPlayCCLP1) {
    load_test_set(CCLP1_ccl, sizeof(CCLP1_ccl), public_CCLP1_lynx_tws, sizeof(public_CCLP1_lynx_tws));
  }

  TEST(MSCCLP, LoadAndPlayCCLP2) {
    load_test_set(CCLP2_ccl, sizeof(CCLP2_ccl), public_CCLP2_tws, sizeof(public_CCLP2_tws));
  }

  TEST(LynxCCLP, LoadAndPlayCCLXP2) {
    load_test_set(CCLXP2_ccl, sizeof(CCLXP2_ccl), public_CCLXP2_tws, sizeof(public_CCLXP2_tws));
  }

  TEST(MSCCLP, LoadAndPlayCCLP3) {
    load_test_set(CCLP3_ccl, sizeof(CCLP3_ccl), public_CCLP3_tws, sizeof(public_CCLP3_tws));
  }

  TEST(LynxCCLP, LoadAndPlayCCLP3) {
    load_test_set(CCLP3_ccl, sizeof(CCLP3_ccl), public_CCLP3_lynx_tws, sizeof(public_CCLP3_lynx_tws));
  }

  TEST(MSCCLP, LoadAndPlayCCLP4) {
    load_test_set(CCLP4_ccl, sizeof(CCLP4_ccl), public_CCLP4_tws, sizeof(public_CCLP4_tws));
  }

  TEST(LynxCCLP, LoadAndPlayCCLP4) {
    load_test_set(CCLP4_ccl, sizeof(CCLP4_ccl), public_CCLP4_lynx_tws, sizeof(public_CCLP4_lynx_tws));
  }
}
