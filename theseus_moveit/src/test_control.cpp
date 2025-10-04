#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <moveit/move_group_interface/move_group_interface.h>
#include <thread>
#include <chrono>

int main(int argc, char** argv)
{
    // Initialize ROS 2
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("test_control");

    // Allow MoveGroupInterface to use this node
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);

    // Start the executor spinning in a separate thread.
    // This is essential for processing the joint_states topic AND the parameter service client response.
    std::thread([&executor]() { executor.spin(); }).detach();
    
    // Give the parameter server and current state a moment to settle
    rclcpp::sleep_for(std::chrono::milliseconds(500)); 

    // === Initialize MoveIt C++ interfaces ===
    static const std::string ARM_GROUP = "arm";
    static const std::string GRIPPER_GROUP = "gripper";

    // Now MoveGroupInterface will find 'robot_description_kinematics' in the local node's parameter space
    // because it was just set by fetch_and_set_parameters().
    moveit::planning_interface::MoveGroupInterface arm(node, ARM_GROUP);
    moveit::planning_interface::MoveGroupInterface gripper(node, GRIPPER_GROUP);

    // Give MoveIt a moment to initialize with the newly loaded parameters
    rclcpp::sleep_for(std::chrono::milliseconds(500));

    // Optional: set planning parameters
    arm.setPlanningTime(100.0);
    arm.setGoalOrientationTolerance(M_PI); // allow any orientation
    arm.setGoalTolerance(0.01);
    gripper.setPlanningTime(5.0);

    // === 1. Move arm to target pose ===
    geometry_msgs::msg::PoseStamped target_pose;
    // ... (rest of your target pose definition) ...
    target_pose.header.frame_id = "base_arm_link";
    target_pose.pose.position.x = 0.3;
    target_pose.pose.position.y = -0.20;
    target_pose.pose.position.z = 0.2;
    target_pose.pose.orientation.w = 1;

      // Get the current pose of the end-effector
    geometry_msgs::msg::Pose current_pose = arm.getCurrentPose().pose;

    // Print the current pose
    RCLCPP_INFO(node->get_logger(), "Current end-effector pose:");
    RCLCPP_INFO(node->get_logger(), "Position: x=%f, y=%f, z=%f",
                current_pose.position.x, current_pose.position.y, current_pose.position.z);
    RCLCPP_INFO(node->get_logger(), "Orientation: x=%f, y=%f, z=%f, w=%f",
                current_pose.orientation.x, current_pose.orientation.y, current_pose.orientation.z, current_pose.orientation.w);


    arm.setPositionTarget(target_pose.pose.position.x, target_pose.pose.position.y, target_pose.pose.position.z, "gripper_center");  // end-effector link name

    RCLCPP_INFO(node->get_logger(), "Planning arm motion...");
    moveit::planning_interface::MoveGroupInterface::Plan arm_plan;
    bool success = (arm.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);

    if (success)
    {
        RCLCPP_INFO(node->get_logger(), "Executing arm motion...");
        arm.execute(arm_plan);
    }
    else
    {
        RCLCPP_WARN(node->get_logger(), "Arm planning failed!");
    }

    RCLCPP_INFO(node->get_logger(), "Motion sequence completed.");

    executor.cancel();
    rclcpp::shutdown();
    return 0;
}