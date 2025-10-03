#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <moveit/move_group_interface/move_group_interface.h>
#include <thread> // Include for std::thread

int main(int argc, char** argv)
{
    // Initialize ROS 2
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("test_control");

    // === Executor Setup (Crucial Change) ===
    // 1. Create the executor and add the node.
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);

    // 2. Start the executor spinning in a separate thread.
    //    This allows ROS callbacks (like the joint_states subscription)
    //    to be processed while your main thread is blocked by plan() or execute().
    std::thread([&executor]() { executor.spin(); }).detach();

    // === Initialize MoveIt C++ interfaces ===
    static const std::string ARM_GROUP = "arm";
    static const std::string GRIPPER_GROUP = "gripper";

    // MoveGroupInterface requires the node to be spinning in the background
    // to receive the current robot state via joint_states topic.
    moveit::planning_interface::MoveGroupInterface arm(node, ARM_GROUP);
    moveit::planning_interface::MoveGroupInterface gripper(node, GRIPPER_GROUP);

    // Give MoveIt a moment to process the first joint_states message
    // This is often a good practice to ensure the current state is read before a plan is requested.
    rclcpp::sleep_for(std::chrono::milliseconds(500)); 

    // Optional: set planning parameters
    arm.setPlanningTime(10.0);
    arm.setGoalOrientationTolerance(M_PI); // allow any orientation
    arm.setGoalTolerance(0.01);
    gripper.setPlanningTime(5.0);

    // === 1. Move arm to target pose ===
    geometry_msgs::msg::PoseStamped target_pose;
    target_pose.header.frame_id = "base_arm_link";  // adjust to your robot
    target_pose.pose.position.x = 0.3;
    target_pose.pose.position.y = 0;
    target_pose.pose.position.z = 0.2;
   
    // target_pose.pose.orientation.x = 0;
    // target_pose.pose.orientation.y = 0;
    // target_pose.pose.orientation.z = 0;
    target_pose.pose.orientation.w = 1;

    arm.setJointValueTarget(target_pose, "gripper_center");  // end-effector link name

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

    // === 2. Close gripper ===
    // gripper.setNamedTarget("closed");  // must exist in SRDF
    // RCLCPP_INFO(node->get_logger(), "Closing gripper...");
    // moveit::planning_interface::MoveGroupInterface::Plan grip_plan;
    // success = (gripper.plan(grip_plan) == moveit::core::MoveItErrorCode::SUCCESS);
    // if (success)
    //     gripper.execute(grip_plan);

    // // === 3. Lift arm ===
    // target_pose.pose.position.z += 0.1;
    // arm.setPoseTarget(target_pose, "gripper_center");
    // RCLCPP_INFO(node->get_logger(), "Planning lift motion...");
    // success = (arm.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);
    // if (success)
    //     arm.execute(arm_plan);

    // // === 4. Open gripper ===
    // gripper.setNamedTarget("open");
    // RCLCPP_INFO(node->get_logger(), "Opening gripper...");
    // success = (gripper.plan(grip_plan) == moveit::core::MoveItErrorCode::SUCCESS);
    // if (success)
    //     gripper.execute(grip_plan);

    // === 5. Return arm to "home" ===
    // arm.setNamedTarget("home");  // must exist in SRDF
    // RCLCPP_INFO(node->get_logger(), "Returning arm to home...");
    // success = (arm.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);
    // if (success)
    //     arm.execute(arm_plan);

    RCLCPP_INFO(node->get_logger(), "Motion sequence completed.");

    rclcpp::shutdown();
    return 0;
}
