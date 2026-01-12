import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
from moveit_configs_utils import MoveItConfigsBuilder

def generate_launch_description():

    # ------------------------------------
    # Declare launch arguments
    # ------------------------------------

    is_sim_arg = DeclareLaunchArgument(
        "is_sim",
        default_value="False"
    )

    model_arg = DeclareLaunchArgument(
        name="model",
        default_value="theseus",
        description="Manipulator name suffix (e.g., theseus)"
    )

    is_sim = LaunchConfiguration("is_sim")
    model = LaunchConfiguration("model")

    # ------------------------------------
    # Initialize Gazebo Simulation 
    # (only if is_sim==True)
    # ------------------------------------

    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([

            PathJoinSubstitution([
                FindPackageShare('theseus_description'),
                'launch',
                'gazebo.launch.py'  
            ])
        ]),
        launch_arguments={
            'model': model
        }.items(),
        condition=IfCondition(is_sim)
    )

    # ------------------------------------
    # Initialize Controllers
    # (Hardware or Simulation based on is_sim)
    # ------------------------------------
    
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

    # ------------------------------------
    # Initialize MoveIt2
    # ------------------------------------
    # - Uses OpaqueFunction in order to
    #   allow for parametric loading of 
    #   different manipulator models
    #   (needs to resolve file paths in
    #   launch time)
    #
    # - Runs Action Server Node for
    #   recieving PositionGoal and 
    #   GripperGoal actions
    # ------------------------------------

    def setup_moveit(context, *args, **kwargs):
        model = LaunchConfiguration("model").perform(context)  # Resolve to str
        print(f"BRINGUP \n\n\n\n\n\n[INFO] Using model: {model}")

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
        
        # Include moveit launch file
        moveit_launch_file = os.path.join(
            get_package_share_directory('theseus_moveit'),
            'launch',
            'moveit.launch.py'
        )

        moveit_launch = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(moveit_launch_file),
            launch_arguments={
                'model': model,
                'is_sim': is_sim
            }.items()
        )

        action_server_node = Node(
            package="theseus_moveit",
            executable="theseus_action_server",
            output="screen",
            parameters=[moveit_config.to_dict()],
        )

        return [moveit_launch, action_server_node]


    return LaunchDescription([
        is_sim_arg,
        model_arg,
        controller_launch,
        gazebo_launch,
        OpaqueFunction(function=setup_moveit)  
    ])