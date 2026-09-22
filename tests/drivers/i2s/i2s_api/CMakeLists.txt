# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2s_api)

target_sources(app PRIVATE
  src/common.c
  src/main.c
  src/test_i2s_dir_both_loopback.c
  src/test_i2s_dir_both_states.c
  src/test_i2s_errors.c
  src/test_i2s_loopback.c
  src/test_i2s_states.c
)
target_sources_ifdef(CONFIG_I2S_TEST_MONO app PRIVATE src/test_i2s_mono.c)
