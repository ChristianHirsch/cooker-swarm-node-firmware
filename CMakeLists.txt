cmake_minimum_required(VERSION 3.8.2)

#set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
#set(BOARD cooker_swarm_node_hardware)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(cooker-node)

target_sources(app PRIVATE
  src/main.c
  src/cooker.c
  src/cos.c
  src/cts.c
  src/ess.c
  src/remote_temp.c
  src/pid.c
)
