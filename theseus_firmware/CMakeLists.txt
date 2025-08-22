cmake_minimum_required(VERSION 3.8)
project(theseus_firmware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SERIAL libserial)

ament_python_install_package(${PROJECT_NAME})

add_executable(simple_serial_interface src/simple_serial_interface.cpp)
ament_target_dependencies(simple_serial_interface rclcpp std_msgs)

# Includes from pkg-config
target_include_directories(simple_serial_interface PUBLIC ${SERIAL_INCLUDE_DIRS})

# Link with libserial
target_link_libraries(simple_serial_interface ${SERIAL_LIBRARIES})

install(TARGETS simple_serial_interface
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
