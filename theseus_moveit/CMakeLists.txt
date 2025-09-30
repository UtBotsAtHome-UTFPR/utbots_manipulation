cmake_minimum_required(VERSION 3.22)
project(theseus_moveit)

find_package(ament_cmake REQUIRED)
find_package(rclpy REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(moveit_commander REQUIRED)

install(
  PROGRAMS
    src/test_control.py
  DESTINATION lib/${PROJECT_NAME}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/launch")
  install(
    DIRECTORY launch rviz
    DESTINATION share/${PROJECT_NAME}
    PATTERN "setup_assistant.launch" EXCLUDE)
endif()

install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
install(FILES .setup_assistant DESTINATION share/${PROJECT_NAME})

ament_package()