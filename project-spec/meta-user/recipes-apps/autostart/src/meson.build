conf_data = configuration_data()
conf_data.set_quoted('PROJECT_VERSION', '0.0.0.1')
configure_file(
  output: 'config.h',
  configuration: conf_data,
)
thread_dep = dependency('threads')
dir = meson.current_source_dir() + '/'
lib_axitangxi = library('axitangxi', dir + 'axitangxi.c', install: true)
lib_utils = library('utils', dir + 'utils.c', install: true)
lib_coding = library(
  'coding',
  [dir + 'coding.cpp', dir + 'arithmetic_coding.cpp'],
  install: true,
)
lib_crc = library(
  'crc',
  dir + 'crc.c',
  install: true,
)
lib_transmission_protocol = library(
  'transmission_protocol',
  dir + 'transmission_protocol.c',
  link_with: [lib_crc, lib_utils],
  install: true,
)
executable(
  'main',
  dir + 'main.c',
  link_with: [lib_axitangxi, lib_utils, lib_transmission_protocol, lib_coding],
  install: true,
)
executable(
  'master',
  dir + 'master.c',
  link_with: [lib_utils, lib_transmission_protocol],
  dependencies: thread_dep,
)
