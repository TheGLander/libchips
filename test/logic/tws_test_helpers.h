#ifndef LIBCHIPS_TWS_TEST_HELPERS_H
#define LIBCHIPS_TWS_TEST_HELPERS_H

#include <gtest/gtest.h>

extern "C" {
#include "formats.h"
#include "format-tws.h"
#include "logic.h"
}

struct LevelsetTwssetPair {
    LevelSet* set;
    TWSSet* tws;
};

typedef std::optional<LevelsetTwssetPair> LevelsetTwssetPairOptional;

void load_test_set(uint8_t const* levelset, size_t levelset_size, uint8_t const* tws, size_t tws_size,
    bool allow_tick_difference = false, size_t skip_levels[] = nullptr, size_t skip_size = 0);

#endif //LIBCHIPS_TWS_TEST_HELPERS_H
