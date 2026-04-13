cmake_minimum_required(VERSION 3.8)
project(ros2_gst_video_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_TIDY "Enable clang-tidy during C++ compilation" ON)
option(ROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_FORMAT_CHECK "Enable clang-format check target" ON)

if(ROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
  else()
    message(WARNING "clang-tidy requested but not found. Continuing without clang-tidy.")
  endif()
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

if(ROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_FORMAT_CHECK)
  find_program(CLANG_FORMAT_EXE NAMES clang-format)
  if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ROS2_GST_VIDEO_BRIDGE_FORMAT_FILES CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    )
    add_custom_target(clang_format_check
      COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ROS2_GST_VIDEO_BRIDGE_FORMAT_FILES}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Running clang-format check on C++ source files"
      VERBATIM
    )
  else()
    message(WARNING "clang-format check requested but clang-format was not found.")
  endif()
endif()

add_executable(gst_video_bridge_node
  src/main.cc
  src/gst_video_bridge_node.cc
  src/core/config_loader.cc
  src/core/pipeline_builder.cc
  src/runtime/capability_probe.cc
  src/runtime/topic_introspector.cc
  src/runtime/stream_engine.cc
  src/runtime/metrics_publisher.cc
)

target_include_directories(gst_video_bridge_node
  PRIVATE
  include
)

ament_target_dependencies(gst_video_bridge_node
  rclcpp
)

install(TARGETS gst_video_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
