#ifndef LIBCHIPS_HASH_H
#define LIBCHIPS_HASH_H

#include <stdint.h>
#include <stdlib.h>

#if SIZE_WIDTH >= 32 && SIZE_WIDTH <= 256 && (SIZE_WIDTH % 8) == 0
  typedef size_t hash_t;
  #define HASH_WIDTH SIZE_WIDTH
#else
  typedef _BitInt(32) hash_t;
  #define HASH_WIDTH 32
#endif

#if HASH_WIDTH == 32
  static hash_t const HASH_INIT = 0x811c9dc5;
  static hash_t const HASH_PRIME = 0x01000193;
#elif HASH_WIDTH == 64
  static hash_t const HASH_INIT = 0xcbf29ce484222325;
  static hash_t const HASH_PRIME = 0x00000100000001b3;
#elif HASH_WIDTH == 128
  static hash_t const HASH_INIT = 0x6c62272e07bb014262b821756295c58d;
  static hash_t const HASH_PRIME = 0x0000000001000000000000000000013b;
#elif HASH_WIDTH == 256
  static hash_t const HASH_INIT = 0xdd268dbcaac550362d98c384c4e576ccc8b1536847b6bbb31023b4c8caee0535;
  static hash_t const HASH_PRIME = 0x0000000000000000000001000000000000000000000000000000000000000163;
#endif

static inline hash_t hash_byte(uint8_t const byte, hash_t hash) {
  hash ^= byte;
  hash *= HASH_PRIME;
  return hash;
}

static inline hash_t hash_bytes(uint8_t const* bytes, size_t const n, hash_t hash) {
  for (size_t i = 0; i < n; i += 1) { // Go Go Gadget Optimizing Compiler Plus Inline
    hash = hash_byte(bytes[i], hash);
  }
  return hash;
}

#define hash_scalar(s, hash) _Generic((s), \
  uint8_t: hash_byte(s, hash), \
  int8_t: hash_byte(s, hash), \
  bool: hash_byte(s, hash), \
  default: hash_bytes((uint8_t const*)&(s), sizeof(s), hash) \
)

#endif //LIBCHIPS_HASH_H
