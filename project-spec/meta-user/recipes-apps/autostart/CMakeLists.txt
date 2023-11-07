cmake_minimum_required(VERSION 3.10)
if(DEFINED ENV{GITHUB_REF_NAME})
  set(VERSION $ENV{GITHUB_REF_NAME})
else()
  set(VERSION 0.0.0.0)
endif()
project(
  deep-space-detection
  VERSION ${VERSION}
  DESCRIPTION deep-space-detection
  HOMEPAGE_URL https://github.com/Freed-Wu/deep-space-detection)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_subdirectory(src)

configure_file(config.h.in src/config.h)
find_package(PkgConfig REQUIRED)
pkg_get_variable(SYSTEMD_SYSTEM_UNIT_DIR systemd systemd_system_unit_dir)
pkg_get_variable(SYSTEMD_SYSTEM_PRESET_DIR systemd systemd_system_preset_dir)
if(SYSTEMD_SYSTEM_UNIT_DIR STREQUAL "")
  set(SYSTEMD_SYSTEM_UNIT_DIR /lib/systemd/system)
endif()
if(SYSTEMD_SYSTEM_PRESET_DIR STREQUAL "")
  set(SYSTEMD_SYSTEM_PRESET_DIR /lib/systemd/system-preset)
endif()
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/systemd/system/autostart.service
  DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR})
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/systemd/system-preset/98-autostart.preset
  DESTINATION ${SYSTEMD_SYSTEM_PRESET_DIR})

include(CTest)
add_subdirectory(tests EXCLUDE_FORM_ALL)

set(CPACK_PACKAGE_CONTACT
  https://github.com/Freed-Wu/deep-space-detection/issues)
include(CPack)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_ARCHIVE_THREADS 0)
set(CPACK_THREADS 0)
