import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
from moveit_configs_utils import MoveItConfigsBuilder

def generate_launch_description():

    model_arg = DeclareLaunchArgument(
        "model",
        default_value="theseus",
        description="Manipulator name suffix (e.g., theseus)"
    )

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

        moveit_config = MoveItConfigsBuilder(model, package_name="theseus_moveit") \
            .robot_description(file_path=xacro_file_path) \
            .robot_description_semantic(file_path=srdf_file_path) \
            .robot_description_kinematics(file_path="config/kinematics.yaml") \
            .joint_limits(file_path="config/joint_limits.yaml") \
            .pilz_cartesian_limits(file_path="config/pilz_cartesian_limits.yaml") \
            .trajectory_execution(file_path="config/moveit_controllers.yaml") \
            .to_moveit_configs()


        test_node = Node(
            package="theseus_moveit",
            executable="test_control",
            output="screen",
            parameters=[moveit_config.to_dict()],
        )

        return [test_node]

    return LaunchDescription([
        model_arg,
        OpaqueFunction(function=setup_moveit)   
    ])