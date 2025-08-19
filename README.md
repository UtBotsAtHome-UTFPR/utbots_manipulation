# utbots_manipulation

**This stack contains packages related to manipulators, such as:**
- theseus_controller: control configurations, custom hardware interface
- theseus_description: manipulator, controller and simulation description files
- theseus_firmware: hardware firmware

## Installation

```bash
cd <ros2_ws>/src
git clone --recurse-submodules https://github.com/UtBotsAtHome-UTFPR/utbots_manipulation.git
```

### Dependencies
```bash
sudo rosdep init
rosdep update
cd <ros2_ws>/src/utbots_manipulation
rosdep install --from-paths theseus_controller theseus_description theseus_firmware -y --ignore-src
```

### Building

```bash
cd ..
colcon build --packages-select theseus_controller theseus_description theseus_firmware --symlink-install
```

## Running

See the usage explanation below

## Overview

### View URDF model in rviz2
```bash
ros2 launch theseus_description display.launch.py
```

### Run manipulator controller in simulation
To initiate the simulator:
```bash
ros2 launch theseus_description gazebo.launch.py
```
To initiate the controllers (it will run and end):
```bash
ros2 launch theseus_controller controller.launch.py is_sim:=True
```
The control of the manipulator is available with JointTrajectoryController (ideal for MoveIt kinematics for control)