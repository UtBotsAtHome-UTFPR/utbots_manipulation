from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, TextSubstitution, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
from moveit_config_utils import MoveItConfigsBuilder
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    is_sim_arg = DeclareLaunchArgument(
        "is_sim",
        default_value="False"
    )

    is_sim = LaunchConfiguration("is_sim")

    model_arg = DeclareLaunchArgument(
        name="model", 
        default_value=os.path.join(get_package_share_directory("theseus_description"), "urdf", "theseus.urdf.xacro"),
        description="Absolute path to the robot URDF file"
    )

    # Build the robot_description using xacro
    xacro_filename = [LaunchConfiguration("model"), TextSubstitution(text=".urdf.xacro")]

    move_group_node = Node(
        package="moveit_ros_move_group",
        executable="move_group",
        output="screen",
        parameters=[moveit_config.to_dict(), {"use_sim_time": is_sim}, {"publish_robot_description_semantic": True}],
        arguments=["--ros-args", "--log-level", "info"]
    )

    moveit_config = {
        MoveItConfigsBuilder("theseus", package_name="theseus_moveit")
        .robot_description(file_path=PathJoinSubstitution([FindPackageShare("theseus_description"), "urdf", xacro_filename]))#TextSubstitution(text=""), xacro_filename])
        .robot_description_semantic(file_path="config/theseus.srdf")
        .trajectory_controllers(file_path="config/moveit_controllers.yaml")
        .to_moveit_configs()
    } # TODO: ajust it to recieve different robot models srdf and trajectory controllers

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", os.path.join(get_package_share_directory("theseus_moveit"), "rviz", "moveit.rviz")],
        parameters=[moveit_config.robot_description,
                    moveit_config.robot_description_semantic,
                    moveit_config.robot_description_kinematics,
                    moveit_config.joint_limits]
    )

    return LaunchDescription([
        is_sim_arg,
        model_arg,
        move_group_node,
        rviz_node
    ])