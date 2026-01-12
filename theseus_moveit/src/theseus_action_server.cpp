#include <memory>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "moveit/move_group_interface/move_group_interface.h"
#include "geometry_msgs/msg/pose_stamped.hpp"

// Action headers
#include "theseus_moveit/action/arm_goal.hpp"
#include "theseus_moveit/action/gripper_goal.hpp"

class TheseusActionServer : public rclcpp::Node
{
public:
    
    explicit TheseusActionServer(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
    : Node("theseus_action_server", options)
    {
        using namespace std::placeholders;

        // --- Declare and Get Parameters ---
        this->declare_parameter<double>("arm_planning_time", 10.0);
        this->declare_parameter<double>("gripper_planning_time", 10.0);
        this->declare_parameter<double>("arm_orientation_tolerance", 3.14);
        this->declare_parameter<double>("arm_position_tolerance", 0.01);

        this->get_parameter("arm_planning_time", arm_planning_time_);
        this->get_parameter("gripper_planning_time", gripper_planning_time_);
        this->get_parameter("arm_orientation_tolerance", arm_orientation_tolerance_);
        this->get_parameter("arm_position_tolerance", arm_position_tolerance_);

        RCLCPP_INFO(this->get_logger(), "Arm Planning Parameters:");
        RCLCPP_INFO(this->get_logger(), "  - Planning Time: %.2f s", arm_planning_time_);
        RCLCPP_INFO(this->get_logger(), "  - Gripper Planning Time: %.2f s", gripper_planning_time_);
        RCLCPP_INFO(this->get_logger(), "  - Orientation Tolerance: %.2f rad", arm_orientation_tolerance_);
        RCLCPP_INFO(this->get_logger(), "  - Position Tolerance: %.2f m", arm_position_tolerance_);

        this->position_action_server_ = rclcpp_action::create_server<theseus_moveit::action::ArmGoal>(
            this,
            "arm_goal",
            std::bind(&TheseusActionServer::handle_arm_goal, this, _1, _2),
            std::bind(&TheseusActionServer::handle_arm_cancel, this, _1),
            std::bind(&TheseusActionServer::handle_arm_accepted, this, _1));

        RCLCPP_INFO(this->get_logger(), "Manipulator action server started, waiting for goals");

        // Create the Action Server for the Gripper
        this->gripper_action_server_ = rclcpp_action::create_server<theseus_moveit::action::GripperGoal>(
            this,
            "gripper_goal",
            std::bind(&TheseusActionServer::handle_gripper_goal, this, _1, _2),
            std::bind(&TheseusActionServer::handle_gripper_cancel, this, _1),
            std::bind(&TheseusActionServer::handle_gripper_accepted, this, _1));

        RCLCPP_INFO(this->get_logger(), "Gripper action server started.");
        RCLCPP_INFO(this->get_logger(), "Ready to receive goals.");
    }
    
private:
    double arm_planning_time_;
    double gripper_planning_time_;
    double arm_orientation_tolerance_;
    double arm_position_tolerance_;
    rclcpp_action::Server<theseus_moveit::action::ArmGoal>::SharedPtr position_action_server_;
    rclcpp_action::Server<theseus_moveit::action::GripperGoal>::SharedPtr gripper_action_server_;
    using GoalHandleArmGoal = rclcpp_action::ServerGoalHandle<theseus_moveit::action::ArmGoal>;
    using GoalHandleGripperGoal = rclcpp_action::ServerGoalHandle<theseus_moveit::action::GripperGoal>;

    // --- Action Callbacks ---

    // Decides whether to accept or reject a new goal.
    rclcpp_action::GoalResponse handle_arm_goal(
        const rclcpp_action::GoalUUID & uuid,
        std::shared_ptr<const theseus_moveit::action::ArmGoal::Goal> goal)
    {
        RCLCPP_INFO(this->get_logger(), "Received goal request");
        (void)uuid;
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    // Reacts to a client's request to cancel the action.
    rclcpp_action::CancelResponse handle_arm_cancel(
        const std::shared_ptr<GoalHandleArmGoal> goal_handle)
    {
        RCLCPP_INFO(this->get_logger(), "Received request to cancel goal");
        (void)goal_handle;
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    // Starts the execution of the goal.
    void handle_arm_accepted(const std::shared_ptr<GoalHandleArmGoal> goal_handle)
    {
        std::thread{std::bind(&TheseusActionServer::execute_arm, this, std::placeholders::_1), goal_handle}.detach();
    }

    rclcpp_action::GoalResponse handle_gripper_goal(const rclcpp_action::GoalUUID &, std::shared_ptr<const theseus_moveit::action::GripperGoal::Goal>) {
        RCLCPP_INFO(this->get_logger(), "Received gripper goal request");
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    rclcpp_action::CancelResponse handle_gripper_cancel(const std::shared_ptr<GoalHandleGripperGoal>) {
        RCLCPP_INFO(this->get_logger(), "Received request to cancel gripper goal");
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    void handle_gripper_accepted(const std::shared_ptr<GoalHandleGripperGoal> goal_handle) {
        std::thread{std::bind(&TheseusActionServer::execute_gripper, this, std::placeholders::_1), goal_handle}.detach();
    }

    // --- Main Execution Logic ---
    void execute_arm(const std::shared_ptr<GoalHandleArmGoal> goal_handle)
    {
        RCLCPP_INFO(this->get_logger(), "Executing goal...");
        auto feedback = std::make_shared<theseus_moveit::action::ArmGoal::Feedback>();
        auto result = std::make_shared<theseus_moveit::action::ArmGoal::Result>();
        const auto goal = goal_handle->get_goal();
        bool position_only = goal->position_only;

        if (goal->target_pose.header.frame_id.empty() || goal->position_only) {
            position_only = true;
        }

        // Create a separate node for MoveIt operations
        auto moveit_node = rclcpp::Node::make_shared("move_arm_node");
        rclcpp::executors::SingleThreadedExecutor executor;
        executor.add_node(moveit_node);
        std::thread([&executor]() { executor.spin(); }).detach();

        // Initialize MoveIt
        static const std::string ARM_GROUP = "arm";
        moveit::planning_interface::MoveGroupInterface arm(moveit_node, ARM_GROUP);
        arm.setPlanningTime(arm_planning_time_);
        if (position_only) {
            arm.setGoalOrientationTolerance(3.14);  // Ignore orientation
        } else {
            arm.setGoalOrientationTolerance(arm_orientation_tolerance_);
        }
        arm.setGoalPositionTolerance(arm_position_tolerance_);

        arm.clearPathConstraints();
        arm.clearPoseTargets();

        // Get the current pose of the end-effector
        geometry_msgs::msg::Pose current_pose = arm.getCurrentPose().pose;

        // Print the current pose
        RCLCPP_INFO(moveit_node->get_logger(), "Current end-effector pose:");
        RCLCPP_INFO(moveit_node->get_logger(), "Frame ID: %s", arm.getPlanningFrame().c_str());
        RCLCPP_INFO(moveit_node->get_logger(), "Position: x=%f, y=%f, z=%f",
                    current_pose.position.x, current_pose.position.y, current_pose.position.z);
        RCLCPP_INFO(moveit_node->get_logger(), "Orientation: x=%f, y=%f, z=%f, w=%f",
                    current_pose.orientation.x, current_pose.orientation.y, current_pose.orientation.z, current_pose.orientation.w);

        // Prioritize named target over cartesian goal
        if (!goal->standard_pose.empty()) {

            feedback->status = "Moving to named target: " + goal->standard_pose;
            goal_handle->publish_feedback(feedback);
            arm.setNamedTarget(goal->standard_pose);

        } else {

            feedback->status = "Planning arm motion...";
            goal_handle->publish_feedback(feedback);

            if (position_only) {
                // ================================
                // POSITION ONLY IK
                // ================================
                RCLCPP_INFO(this->get_logger(), "Using POSITION ONLY kinematics");

                arm.setPositionTarget(
                    goal->target_pose.pose.position.x,
                    goal->target_pose.pose.position.y,
                    goal->target_pose.pose.position.z,
                    "gripper_center"
                );

            } else {
                // ================================
                // FULL POSE IK
                // ================================
                RCLCPP_INFO(this->get_logger(), "Using FULL POSE kinematics");

                geometry_msgs::msg::Pose target_pose = goal->target_pose.pose;
                arm.setPoseTarget(target_pose, "gripper_center");
            }
        }

        // Plan the motion
        moveit::planning_interface::MoveGroupInterface::Plan arm_plan;
        bool success = (arm.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);
        
        // Check for cancellation requests
        if (goal_handle->is_canceling())
        {
            arm.stop(); // Stop any potential motion
            result->success = false;
            result->message = "Action canceled during planning.";
            goal_handle->canceled(result);
            RCLCPP_INFO(this->get_logger(), "Goal canceled");
            return;
        }

        if (success)
        {
            // Publish feedback: Executing
            feedback->status = "Executing arm motion...";
            goal_handle->publish_feedback(feedback);
            RCLCPP_INFO(this->get_logger(), feedback->status.c_str());
            
            // Execute the plan
            arm.execute(arm_plan);

            // Final result
            result->success = true;
            result->message = "Arm motion successful.";
            goal_handle->succeed(result);
            RCLCPP_INFO(this->get_logger(), "Goal succeeded");
        }
        else
        {
            result->success = false;
            result->message = "Arm planning failed!";
            goal_handle->abort(result);
            RCLCPP_WARN(this->get_logger(), "Goal aborted");
        }
        executor.cancel();
    }

    void execute_gripper(const std::shared_ptr<GoalHandleGripperGoal> goal_handle) {
        RCLCPP_INFO(this->get_logger(), "Executing gripper goal...");
        auto feedback = std::make_shared<theseus_moveit::action::GripperGoal::Feedback>();
        auto result = std::make_shared<theseus_moveit::action::GripperGoal::Result>();
        const auto goal = goal_handle->get_goal();

        auto moveit_node = rclcpp::Node::make_shared("move_gripper_node");
        rclcpp::executors::SingleThreadedExecutor executor;
        executor.add_node(moveit_node);
        std::thread([&executor]() { executor.spin(); }).detach();

        static const std::string GRIPPER_GROUP = "gripper";
        moveit::planning_interface::MoveGroupInterface gripper(moveit_node, GRIPPER_GROUP);
        gripper.setPlanningTime(gripper_planning_time_);

        // Prioritize named target over raw value
        if (!goal->standard_pose.empty()) {
            feedback->status = "Moving to named target: " + goal->standard_pose;
            goal_handle->publish_feedback(feedback);
            gripper.setNamedTarget(goal->standard_pose);
        } else {
            feedback->status = "Moving to joint value: " + std::to_string(goal->target_angle);
            goal_handle->publish_feedback(feedback);
            gripper.setJointValueTarget("gripper_joint_1", goal->target_angle);
        }
        
        moveit::planning_interface::MoveGroupInterface::Plan gripper_plan;
        bool success = (gripper.plan(gripper_plan) == moveit::core::MoveItErrorCode::SUCCESS);

        if (goal_handle->is_canceling()) {
            result->success = false;
            result->message = "Action canceled during planning.";
            goal_handle->canceled(result);
            return;
        }

        if (success) {
            feedback->status = "Executing gripper motion...";
            goal_handle->publish_feedback(feedback);
            gripper.execute(gripper_plan);
            result->success = true;
            result->message = "Gripper motion successful.";
            goal_handle->succeed(result);
        } else {
            result->success = false;
            result->message = "Gripper planning failed!";
            goal_handle->abort(result);
        }
        executor.cancel();
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto action_server = std::make_shared<TheseusActionServer>();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(action_server);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}