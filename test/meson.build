dependencies = [
  dependency('gtest'),
  dependency('gmock'),
  dependency('gtest_main'),
  libchips_dep
]

gtest_parallel = meson.project_source_root() + '/utils/gtest-parallel-wrapper'

add_test_setup(
  'parallel_gtest',
  exe_wrapper: [gtest_parallel],
  is_default: true
)

sources = [
  'ccl/format-ccl.cpp',
  'tws/format-tws.cpp',
  'hash/hash_equals.cpp',
  'game_logic/cclp_public_tws_playthrough.cpp',
  'game_logic/cclp5_submissions_tws.cpp',
  'game_logic/tws_test_helpers.cpp',
  'game_logic/cid_patchtests_ms_tws.cpp',
]

gtest_libchips_test = executable(
  'libchips-test',
  sources,
  dependencies: dependencies,
)

test('gtest test', gtest_libchips_test, protocol: 'gtest', verbose: true, args: '--gtest_color=yes')
