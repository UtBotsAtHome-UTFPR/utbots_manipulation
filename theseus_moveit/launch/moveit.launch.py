from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from moveit_configs_utils.moveit_configs_builder import MoveItConfigsBuilder
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    is_sim_arg = DeclareLaunchArgument(
        "is_sim",
        default_value="False",
        description="Run in simulation mode"
    )
    model_arg = DeclareLaunchArgument(
        "model",
        default_value="theseus",
        description="Manipulator name suffix (e.g., theseus)"
    )

    is_sim = LaunchConfiguration("is_sim")

    # Use OpaqueFunction to resolve the urdf and srdf models into a real string path for MoveItConfigsBuilder
    def setup_moveit(context, *args, **kwargs):
        model = LaunchConfiguration("model").perform(context)  # Resolve to str
        print(f"[INFO] Using model: {model}")

        # Build absolute path to xacro
        xacro_file_path = os.path.join(
            get_package_share_directory("theseus_description"),
            "urdf",
            f"{model}.urdf.xacro"
        )

        srdf_file_path = os.path.join(
            get_package_share_directory("theseus_moveit"),
            "config",
            f"{model}.srdf"
        )

        print(f"[INFO] Resolved xacro file path: {xacro_file_path}")

        # Create MoveIt configuration
        moveit_config = MoveItConfigsBuilder(model, package_name="theseus_moveit") \
            .robot_description(file_path=xacro_file_path) \
            .robot_description_semantic(file_path=srdf_file_path) \
            .robot_description_kinematics(file_path="config/kinematics.yaml") \
            .joint_limits(file_path="config/joint_limits.yaml") \
            .pilz_cartesian_limits(file_path="config/pilz_cartesian_limits.yaml") \
            .trajectory_execution(file_path="config/moveit_controllers.yaml") \
            .to_moveit_configs()

        # Move Group Node
        move_group_node = Node(
            package="moveit_ros_move_group",
            executable="move_group",
            output="screen",
            parameters=[moveit_config.to_dict(), {"use_sim_time": is_sim}, {"publish_robot_description_semantic": True}],
            arguments=["--ros-args", "--log-level", "info"]
        )

        # RViz Node
        rviz_node = Node(
            package="rviz2",
            executable="rviz2",
            name="rviz2",
            output="screen",
            arguments=["-d", os.path.join(get_package_share_directory("theseus_moveit"), "rviz", "moveit.rviz")],
            parameters=[
                moveit_config.robot_description,
                moveit_config.robot_description_semantic,
                moveit_config.robot_description_kinematics,
                moveit_config.joint_limits,
                {"use_sim_time": is_sim}
            ]
        )

        return [move_group_node, rviz_node]

    return LaunchDescription([
        is_sim_arg,
        model_arg,
        OpaqueFunction(function=setup_moveit)  # resolves urdf and srdf path at runtime
    ])
