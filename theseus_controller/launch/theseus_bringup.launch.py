import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command, TextSubstitution, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

def generate_launch_description():

    is_sim_arg = DeclareLaunchArgument(
        "is_sim",
        default_value="False"
    )

    is_sim = LaunchConfiguration("is_sim")

    # Declare model name as a launch argument
    model_arg = DeclareLaunchArgument(
        name="model",
        default_value="theseus",
        description="Manipulator name suffix (e.g., theseus)"
    )

    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([

            PathJoinSubstitution([
                FindPackageShare('theseus_description'),
                'launch',
                'gazebo.launch.py'
            ])
        ]),
        launch_arguments={
            'model': LaunchConfiguration('model')
        }.items(),
        condition=IfCondition(is_sim)
    )

    controller_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('theseus_controller'),
                'launch',
                'controller.launch.py'
            ])
        ]),
        launch_arguments={
            'is_sim': is_sim,
            'model': LaunchConfiguration('model')
        }.items()
    )

    moveit_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('theseus_moveit'),
                'launch',
                'moveit.launch.py'
            ])
        ]),
        launch_arguments={
            'is_sim': is_sim,
            'model': LaunchConfiguration('model')
        }.items()
    )

    return LaunchDescription([
        is_sim_arg,
        model_arg,
        controller_launch,
        gazebo_launch,
        moveit_launch
    ])