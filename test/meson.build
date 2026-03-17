dependencies = [
  dependency('gtest'),
  dependency('gmock'),
  dependency('gtest_main'),
  libchips_dep
]

sources = [
  'ccl/format-ccl.cpp',
  'tws/format-tws.cpp',
  'hash/hash_equals.cpp',
  'game_logic/cclp_public_tws_playthrough.cpp',
  'game_logic/cclp5_submissions_tws.cpp',
  'game_logic/tws_test_helpers.cpp',
]

gtest_libchips_test = executable(
  'libchips-test',
  sources,
  dependencies: dependencies,
)

test('gtest test', gtest_libchips_test, protocol: 'gtest', verbose: true, args: '--gtest_color=yes')
