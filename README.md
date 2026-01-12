# utbots_manipulation

**This stack contains packages related to manipulators, such as:**
- theseus_controller: controllers configurations, custom hardware interface
- theseus_description: manipulator, controller and simulation description files
- theseus_firmware: hardware firmware
- theseus_moveit: kinematics implementation and manipulation goals interface

## Installation

```bash
cd <ros2_ws>/src
git clone https://github.com/UtBotsAtHome-UTFPR/utbots_manipulation.git
```

### Dependencies
```bash
sudo rosdep init
rosdep update
cd <ros2_ws>/src/utbots_manipulation
rosdep install --from-paths theseus_controller theseus_description theseus_firmware theseus_moveit -y --ignore-src
```

As of now, we need to build libserial in theseus_controller from source (for real hardware controllers):
```bash
sudo apt update
sudo apt install g++ git autogen autoconf build-essential cmake graphviz \
                 libboost-dev libboost-test-dev libgtest-dev libtool \
                 python3-sip-dev doxygen python3-sphinx pkg-config \
                 python3-sphinx-rtd-theme
mkdir serial_dir
cd serial_dir
git clone https://github.com/crayzeewulf/libserial.git
cd libserial
./compile.sh
cd build
sudo make install
```
Link path of library:
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/ # if error in build, run <find /usr -name "libserial.so.1"> and add the path returned by the command
```
### Building

```bash
cd ..
colcon build --packages-select theseus_controller theseus_description theseus_firmware --symlink-install
```

## Running

### Parametric Launches
This packages' launches were built to support multiple manipulator definitions (urdf, srdf, control interfaces, etc.) and real/simulated environments. **Almost every launchfile can be launches with two important arguments**:

- **model**: name of the manipulator model, the suffix string included in the files for that manipulator as "model_..."

> *Available models*: "manipulator" for simpler 3DOF and "theseus" for 5DOF.

- **is_sim**: True for running with simulated hardware in Gazebo/False for running with real hardware (must be connected to a microcontroller with the loaded firmware connected do /dev/ttyUSB0)

### View URDF model in rviz2
```bash
ros2 launch theseus_description display.launch.py model:=<model>
```

### Launch complete manipulator
To run Simulation (if is_sim), Controllers, MoveIt and the Action Server, launch:

```bash
ros2 launch theseus_controller theseus_bringup.launch.py model:=<model> is_sim:=<True or False>
```

#### Send Manipulator Goals:

**OBS**: In CPP or Python, just do the same goals with Action Clients

- PositionGoal (Move End Effector to a XYZ coodinate)

You can send it to a standard position like 'home':

```bash
ros2 action send_goal /position_goal theseus_moveit/action/PositionGoal "position_only: false
target_pose:
  header:
    stamp:
      sec: 0
      nanosec: 0
    frame_id: ''
  pose:
    position:
      x: 0.0
      y: 0.0
      z: 0.0
    orientation:
      x: 0.0
      y: 0.0
      z: 0.0
      w: 1.0
standard_pose: 'home'"
```

Or to a 3D target point:

```bash
ros2 action send_goal /position_goal theseus_moveit/action/PositionGoal "position_only: false
target_pose:
  header:
    stamp:
      sec: 0
      nanosec: 0
    frame_id: ''
  pose:
    position:
      x: 0.3
      y: -0.2
      z: 0.2
    orientation:
      x: 0.0
      y: 0.0
      z: 0.0
      w: 1.0
standard_pose: ''"
```

**IMPORTANT!**
If you are using a less than 4DOF manipulator (such as model:=manipulator), you should set the *position_only* goal as True, otherwise it will certainly fail for not being able to ajust for orientation (the orientation values will be ignored in the position_only):
```bash
ros2 action send_goal /position_goal theseus_moveit/action/PositionGoal "position_only: true
target_pose:
  header:
    stamp:
      sec: 0
      nanosec: 0
    frame_id: ''
  pose:
    position:
      x: 0.3
      y: -0.2
      z: 0.2
    orientation:
      x: 0.0
      y: 0.0
      z: 0.0
      w: 1.0
standard_pose: ''"
```

- GripperGoal

You can send it to a standard position like 'open' or 'close':

```bash
ros2 action send_goal /position_goal theseus_moveit/action/PositionGoal "{standard_position: 'home', target_point: {header: {frame_id: 'base_arm_link'}, point: {x: 0.0, y: 0.0, z: 0.0}}}"
```

Or to a target angle:

```bash
ros2 action send_goal /gripper_goal theseus_moveit/action/GripperGoal "{standard_position: '', target_angle: 0.3}"
```

## Tune Parameter Files
*TODO*

## Setup a New Manipulator
*TODO*
