test_axitangxi = executable(
  'test-axitangxi',
  meson.current_source_dir() + '/test_axitangxi.c',
  link_with: [lib_axitangxi],
  # must install to test on board
  install: true,
)
test('test driver', test_axitangxi)
