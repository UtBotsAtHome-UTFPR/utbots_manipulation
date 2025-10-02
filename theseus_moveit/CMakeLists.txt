cmake_minimum_required(VERSION 3.22)
project(theseus_moveit)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(moveit_core REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(rclcpp_components REQUIRED)

# Add your executable
add_executable(test_control src/test_control.cpp)

# Link against MoveIt2 and ROS 2 libraries
ament_target_dependencies(
  test_control
  rclcpp
  geometry_msgs
  moveit_ros_planning_interface
  moveit_core
  moveit_msgs
)

# Install the executable
install(
  TARGETS test_control
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch, rviz, config (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/launch")
  install(
    DIRECTORY launch rviz
    DESTINATION share/${PROJECT_NAME}
    PATTERN "setup_assistant.launch" EXCLUDE
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
  install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
endif()

install(FILES .setup_assistant DESTINATION share/${PROJECT_NAME})

ament_package()