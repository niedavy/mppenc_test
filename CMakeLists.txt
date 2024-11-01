# 项目信息
cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "") # For clangd.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0048 NEW)
project(
  mppenc_test
  VERSION 1
  LANGUAGES CXX C)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -O0 -g -ggdb  -Wall \
               -Werror -fstack-protector-all -Wall -Wextra  -pthread \
               -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address")  
                                          

find_package(PkgConfig REQUIRED)

pkg_check_modules(fmt REQUIRED IMPORTED_TARGET fmt)

pkg_check_modules(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)

set(LIBRARIES ${LIBRARIES} PkgConfig::gstreamer PkgConfig::fmt)
set(apps mppenc_test.cpp)

foreach(source_file ${apps})
  # get file name
  get_filename_component(name ${source_file} NAME_WE)
  add_executable(${name} ${source_file} ${LIB_SRCS})
  target_link_libraries(
    ${name}
    -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,noexecstack
    -Wl,--disable-new-dtags
    -pie
    -Wl,--start-group
    ${LIBRARIES}
    -Wl,--end-group)
endforeach()
