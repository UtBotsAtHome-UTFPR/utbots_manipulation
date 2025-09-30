#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped
from moveit_commander import (
    MoveGroupCommander,
    RobotCommander,
    PlanningSceneInterface,
    roscpp_initialize,
    roscpp_shutdown
)

def main():
    rclpy.init()
    roscpp_initialize([])

    # Initialize MoveIt interfaces
    robot = RobotCommander()
    scene = PlanningSceneInterface()
    arm_group = MoveGroupCommander("arm")
    gripper_group = MoveGroupCommander("gripper")

    # === 1. Move arm to a pose target ===
    target_pose = PoseStamped()
    target_pose.header.frame_id = "base_arm_link"   # your arm's base link
    target_pose.pose.position.x = 0.3
    target_pose.pose.position.y = 0.0
    target_pose.pose.position.z = 0.2
    target_pose.pose.orientation.w = 1.0

    # Use claw_link as the EE tip
    arm_group.set_pose_target(target_pose, end_effector_link="gripper_center")

    print("Planning arm motion...")
    plan = arm_group.plan()

    if plan and len(plan.joint_trajectory.points) > 0:
        print("Executing arm motion...")
        arm_group.execute(plan, wait=True)
    else:
        print("Planning failed!")

    arm_group.stop()
    arm_group.clear_pose_targets()

    # === 2. Close gripper ===
    print("Closing gripper...")
    gripper_group.set_named_target("closed")  # needs to be defined in SRDF
    gripper_group.go(wait=True)

    # === 3. Move arm somewhere else (lift) ===
    lift_pose = target_pose
    lift_pose.pose.position.z += 0.1
    arm_group.set_pose_target(lift_pose, end_effector_link="gripper_center")

    print("Planning lift motion...")
    plan2 = arm_group.plan()
    if plan2 and len(plan2.joint_trajectory.points) > 0:
        print("Executing lift...")
        arm_group.execute(plan2, wait=True)

    arm_group.stop()
    arm_group.clear_pose_targets()

    # === 4. Open gripper ===
    print("Opening gripper...")
    gripper_group.set_named_target("open")
    gripper_group.go(wait=True)

    # Shutdown
    roscpp_shutdown()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
