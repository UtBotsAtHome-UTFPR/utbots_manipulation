import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command, TextSubstitution, PathJoinSubstitution
from launch_ros.actions import Node, SetParameter
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import UnlessCondition

def generate_launch_description():

    is_sim_arg = DeclareLaunchArgument(
        "is_sim",
        default_value="False"
    )

    is_sim = LaunchConfiguration("is_sim")
    is_ignition = "True" if os.environ["ROS_DISTRO"] == "humble" else "False"

    set_sim_time = SetParameter(name='use_sim_time', value=is_sim)

    # Declare model name as a launch argument
    model_arg = DeclareLaunchArgument(
        name="model",
        default_value="theseus",
        description="Manipulator name suffix (e.g., theseus)"
    )

    # Build the robot_description using xacro
    xacro_filename = [LaunchConfiguration("model"), TextSubstitution(text=".urdf.xacro")]

    def debug(context, *args, **kwargs):
        model = LaunchConfiguration("model").perform(context)
        print(f"CONTROLLER : {model}")
        return []

    robot_description = ParameterValue(
        Command([
            "xacro ",
            PathJoinSubstitution([
                FindPackageShare("theseus_description"),
                "urdf",
                TextSubstitution(text=""),
                xacro_filename
            ]),
            " is_sim:=", LaunchConfiguration("is_sim"),
            " is_ignition:=", is_ignition
        ]),
        value_type=str
    )

    # Load robot_state_publisher (only for real robot)
    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": robot_description}]
    )

    # Controller manager (only for real robot)
    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[
            {"robot_description": robot_description,
             "use_sim_time": is_sim},
            PathJoinSubstitution([
                FindPackageShare("theseus_controller"),
                "config",
                [LaunchConfiguration("model"), TextSubstitution(text="_controllers.yaml")]
            ])
        ],
        condition=UnlessCondition(is_sim)
    )

    # Spawner nodes
    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=[
            "joint_state_broadcaster",
            "--controller-manager",
            "/controller_manager"
        ],
        output="screen"
    )


    arm_controller_spawner = Node(
    package="controller_manager",
    executable="spawner",
    arguments=[
        "arm_controller",
        "--controller-manager",
        "/controller_manager",
    ],
    output="screen"
)

    gripper_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=[
            "gripper_controller",
            "--controller-manager",
            "/controller_manager",
        ],
        output="screen"
    )


    # Return all actions
    return LaunchDescription([
        is_sim_arg,
        set_sim_time,
        model_arg,
        robot_state_publisher,
        controller_manager,
        joint_state_broadcaster_spawner,
        arm_controller_spawner,
        gripper_controller_spawner,
        OpaqueFunction(function=debug)

    ])
