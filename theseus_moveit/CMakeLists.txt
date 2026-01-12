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
find_package(rclcpp_action REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "action/ArmGoal.action"
  "action/GripperGoal.action"
  DEPENDENCIES geometry_msgs
)

ament_export_dependencies(rosidl_default_runtime)

# Add your executable
add_executable(test_control src/test_control.cpp)
add_executable(theseus_action_server src/theseus_action_server.cpp)

add_dependencies(theseus_action_server ${PROJECT_NAME}__rosidl_generator_cpp)
add_dependencies(test_control ${PROJECT_NAME}__rosidl_generator_cpp)

# Link against MoveIt2 and ROS 2 libraries
ament_target_dependencies(
  test_control
  rclcpp
  geometry_msgs
  moveit_ros_planning_interface
  moveit_core
  moveit_msgs
)

target_include_directories(theseus_action_server
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
    $<INSTALL_INTERFACE:include>
)

# Install the executable
install(
  TARGETS test_control
  DESTINATION lib/${PROJECT_NAME}
)

ament_target_dependencies(
  theseus_action_server
  rclcpp
  rclcpp_action
  geometry_msgs
  moveit_ros_planning_interface
  moveit_core
  moveit_msgs
  rosidl_typesupport_cpp
)

target_link_libraries(theseus_action_server
  ${PROJECT_NAME}__rosidl_typesupport_cpp
)

install(TARGETS
  theseus_action_server
  DESTINATION lib/${PROJECT_NAME})


# Install launch, rviz, config (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/launch")
  install(
    DIRECTORY launch rviz config
    DESTINATION share/${PROJECT_NAME}
    PATTERN "setup_assistant.launch" EXCLUDE
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
  install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
endif()

install(FILES .setup_assistant DESTINATION share/${PROJECT_NAME})

ament_package()