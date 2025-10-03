#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <moveit/move_group_interface/move_group_interface.h>
#include <thread>
#include <chrono>

// Define the namespace of the MoveGroup node
static const std::string MOVE_GROUP_NODE_NAME = "/move_group";

// List of critical parameters MoveIt needs to load the model and kinematics
const std::vector<std::string> REQUIRED_PARAMETERS = {
    "robot_description",                 // URDF content
    "robot_description_semantic",        // SRDF content
    "robot_description_kinematics",      // Kinematics solver data
    // Add other necessary configs like joint_limits if planning fails later
    // "robot_description_joint_limits"
};

// ... (includes and constants are the same) ...

bool fetch_and_set_parameters(rclcpp::Node::SharedPtr node)
{
    RCLCPP_INFO(node->get_logger(), "Attempting to fetch required parameters from %s...", MOVE_GROUP_NODE_NAME.c_str());

    // === CRITICAL FIX: Declare parameters first ===
    // We must declare all parameters we intend to set, or set_parameters will throw an exception.
    for (const auto& param_name : REQUIRED_PARAMETERS)
    {
        // Use Node::declare_parameter. Since we're just setting a large string/map later, 
        // we declare it with a generic type and an empty default.
        // It's often safer to use parameter overrides in the launch file, 
        // but this works for direct C++ parameter management.
        if (!node->has_parameter(param_name))
        {
            // For complex MoveIt parameters like robot_description and kinematics, 
            // the type can be complex (string for URDF/SRDF, map for kinematics). 
            // Declaring without a specific type allows rclcpp to infer it later upon setting.
            node->declare_parameter(param_name, rclcpp::ParameterValue());
        }
    }
    // ===============================================

    // 1. Create an asynchronous parameter client targeting the /move_group node
    auto parameters_client = std::make_shared<rclcpp::AsyncParametersClient>(node, MOVE_GROUP_NODE_NAME);

    // 2. Wait for the parameter service to be ready
    if (!parameters_client->wait_for_service(std::chrono::seconds(5)))
    {
        RCLCPP_ERROR(node->get_logger(), "Failed to connect to parameter service on %s.", MOVE_GROUP_NODE_NAME.c_str());
        return false;
    }

    // 3. Request the parameters from the remote node
    auto future_parameters = parameters_client->get_parameters(REQUIRED_PARAMETERS);

    // 4. Wait for the response (blocking call)
    if (future_parameters.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
    {
        RCLCPP_ERROR(node->get_logger(), "Timeout while waiting for parameters from %s.", MOVE_GROUP_NODE_NAME.c_str());
        return false;
    }

    // 5. Set the retrieved parameters on the local node
    std::vector<rclcpp::Parameter> fetched_parameters = future_parameters.get();
    std::vector<rclcpp::Parameter> parameters_to_set;

    // Filter out unset parameters and warn if necessary
    for (const auto& param : fetched_parameters)
    {
        if (param.get_type() != rclcpp::ParameterType::PARAMETER_NOT_SET)
        {
            parameters_to_set.push_back(param);
        }
        else
        {
            RCLCPP_WARN(node->get_logger(), "Parameter '%s' was not set by %s.", param.get_name().c_str(), MOVE_GROUP_NODE_NAME.c_str());
        }
    }

    if (!parameters_to_set.empty())
    {
        // This call is now safe because all parameters in parameters_to_set 
        // were declared at the start of the function.
        node->set_parameters(parameters_to_set);
        RCLCPP_INFO(node->get_logger(), "Successfully fetched and set %zu required MoveIt parameters.", parameters_to_set.size());
        return true;
    }

    RCLCPP_ERROR(node->get_logger(), "No required MoveIt parameters were available on %s.", MOVE_GROUP_NODE_NAME.c_str());
    return false;
}

// ... (main function is the same) ...

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

    // === Parameter Loading (The Fix) ===
    if (!fetch_and_set_parameters(node))
    {
        RCLCPP_FATAL(node->get_logger(), "Failed to load critical MoveIt parameters. Shutting down.");
        rclcpp::shutdown();
        return 1;
    }
    
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
    arm.setPlanningTime(10.0);
    arm.setGoalOrientationTolerance(M_PI); // allow any orientation
    arm.setGoalTolerance(0.01);
    gripper.setPlanningTime(5.0);

    // === 1. Move arm to target pose ===
    geometry_msgs::msg::PoseStamped target_pose;
    // ... (rest of your target pose definition) ...
    target_pose.header.frame_id = "base_arm_link";
    target_pose.pose.position.x = 0.3;
    target_pose.pose.position.y = 0;
    target_pose.pose.position.z = 0.2;
    target_pose.pose.orientation.w = 1;

    arm.setJointValueTarget(target_pose, "gripper_center");  // end-effector link name

    RCLCPP_INFO(node->get_logger(), "Planning arm motion...");
    moveit::planning_interface::MoveGroupInterface::Plan arm_plan;
    bool success = (arm.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);

    // ... (rest of your execution logic) ...

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