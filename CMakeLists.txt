cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(cooker-node)

target_sources(app PRIVATE
  src/main.c
  src/cooker.c
  src/cos.c
  src/cts.c
)

#zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
