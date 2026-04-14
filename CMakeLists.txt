cmake_minimum_required(VERSION 3.8)
project(ros2_gst_video_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_TIDY "Enable clang-tidy during C++ compilation" ON)
option(ROS2_GST_VIDEO_BRIDGE_ENABLE_CLANG_FORMAT_CHECK "Enable clang-format check target" ON)
option(ROS2_GST_VIDEO_BRIDGE_ENABLE_AMENT_LINT "Enable ament_lint_auto checks" OFF)

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
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

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
  ${GSTREAMER_INCLUDE_DIRS}
)

target_link_libraries(gst_video_bridge_node
  ${GSTREAMER_LIBRARIES}
)

target_compile_options(gst_video_bridge_node PRIVATE ${GSTREAMER_CFLAGS_OTHER})

ament_target_dependencies(gst_video_bridge_node
  rclcpp
  sensor_msgs
  std_msgs
)

install(TARGETS gst_video_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_config_loader test/test_config_loader.cpp src/core/config_loader.cc)
  target_include_directories(test_config_loader PRIVATE include)
  ament_target_dependencies(test_config_loader rclcpp)

  ament_add_gtest(test_pipeline_builder test/test_pipeline_builder.cpp src/core/pipeline_builder.cc)
  target_include_directories(test_pipeline_builder PRIVATE include)

  add_test(
    NAME smoke_runtime_validate_config
    COMMAND $<TARGET_FILE:gst_video_bridge_node> --ros-args -p runtime.mode:=validate_config)

  add_test(
    NAME smoke_runtime_list_capabilities
    COMMAND $<TARGET_FILE:gst_video_bridge_node> --ros-args -p runtime.mode:=list_capabilities)

  add_test(
    NAME smoke_launch_minimal_show_args
    COMMAND ros2 launch ros2_gst_video_bridge gst_video_bridge_minimal.launch.py --show-args)

  add_test(
    NAME smoke_launch_advanced_show_args
    COMMAND ros2 launch ros2_gst_video_bridge gst_video_bridge_advanced.launch.py --show-args)

  add_test(
    NAME smoke_launch_compat_show_args
    COMMAND ros2 launch ros2_gst_video_bridge gst_video_bridge.launch.py --show-args)

  set_tests_properties(smoke_runtime_validate_config PROPERTIES TIMEOUT 20)
  set_tests_properties(smoke_runtime_list_capabilities PROPERTIES TIMEOUT 20)
  set_tests_properties(smoke_launch_minimal_show_args PROPERTIES TIMEOUT 20)
  set_tests_properties(smoke_launch_advanced_show_args PROPERTIES TIMEOUT 20)
  set_tests_properties(smoke_launch_compat_show_args PROPERTIES TIMEOUT 20)

  if(ROS2_GST_VIDEO_BRIDGE_ENABLE_AMENT_LINT)
    find_package(ament_lint_auto REQUIRED)
    ament_lint_auto_find_test_dependencies()
  endif()
endif()

ament_package()
