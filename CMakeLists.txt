cmake_minimum_required(VERSION 3.8)
project(ros2_gst_video_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

add_executable(gst_video_bridge_node
  src/main.cc
  src/gst_video_bridge_node.cc
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
