#include <theseus_controller/theseus_interface.hpp>
#include <hardware_interface/types/hardware_interface_type_values.hpp>
#include <pluginlib/class_list_macros.hpp>

namespace theseus_controller
{

std::string compensateZeros(int value)
{
    std::string compensate_zeros = "";
    if(value < 10)
    {
        compensate_zeros = "00";
    }
    else if(value < 100)
    {
        compensate_zeros = "0";
    }
    return compensate_zeros;
}

TheseusInterface::TheseusInterface()
{

}

TheseusInterface::~TheseusInterface()
{
    if(device_.IsOpen())
    {
        try
        {
            device_.Close();
        }
        catch(...)
        {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger("TheseusInterface"), "Something went wrong with the connection with port " << port_);
        }
    }
}

/* Lifecycle node init state that defines serial port and position vectors */
CallbackReturn TheseusInterface::on_init(const hardware_interface::HardwareInfo & hardware_info)
{
    CallbackReturn result = hardware_interface::SystemInterface::on_init(hardware_info);

    if(result != CallbackReturn::SUCCESS)
    {
        return result;
    }

    try
    {
        port_ = info_.hardware_parameters.at("port");
    }
    catch(const std::out_of_range &e)
    {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger("TheseusInterface"), "No Serial Port provided! Aborting");
        return CallbackReturn::FAILURE;
    }

    position_commands_.reserve(info_.joints.size());
    position_states_.reserve(info_.joints.size());
    prev_position_commands_.reserve(info_.joints.size());

    return CallbackReturn::SUCCESS;
}

/* Defines the interfaces to store each joint state (feedback data)*/
std::vector<hardware_interface::StateInterface> TheseusInterface::export_state_interfaces()
{
    std::vector<hardware_interface::StateInterface> state_interfaces;
    for(size_t i = 0; i < info_.joints.size(); ++i)
    {
        state_interfaces.emplace_back(hardware_interface::StateInterface(info_.joints[i].name, hardware_interface::HW_IF_POSITION, &position_states_[i]));
    }

    return state_interfaces;
}

/* Defines the interfaces to store the commands for each joint (data to send to hardware)*/
std::vector<hardware_interface::CommandInterface> TheseusInterface::export_command_interfaces()
{
    std::vector<hardware_interface::CommandInterface> command_interfaces;
    for(size_t i = 0; i < info_.joints.size(); ++i)
    {
        command_interfaces.emplace_back(hardware_interface::CommandInterface(info_.joints[i].name, hardware_interface::HW_IF_POSITION, &position_commands_[i]));
    }

    return command_interfaces;
}

/* Lifecycle node activate state that initializes position vectors and attemps serial communication*/
CallbackReturn TheseusInterface::on_activate(const rclcpp_lifecycle::State & previous_state)
{
    RCLCPP_INFO(rclcpp::get_logger("TheseusInterface"), "Starting manipulator hardware...");
    position_commands_ = {0.0, 0.0, 0.0};
    prev_position_commands_ = {0.0, 0.0, 0.0};
    position_states_ = {0.0, 0.0, 0.0};

    try
    {
        device_.Open(port_, std::ios::in | std::ios::out);
        device_.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
    }
    catch(...)
    {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger("TheseusInterface"), "Something went wrong while communicating with port " << port_);
        return CallbackReturn::FAILURE;
    }

    RCLCPP_INFO(rclcpp::get_logger("TheseusInterface"), "Hardware started, ready to take commands");
    return CallbackReturn::SUCCESS;
}

/* Lifecycle node deactivate state that attemps to shutdown connection to serial port */
CallbackReturn TheseusInterface::on_deactivate(const rclcpp_lifecycle::State & previous_state)
{
    RCLCPP_INFO(rclcpp::get_logger("TheseusInterface"), "Stopping manipulator hardware...");

    if(device_.IsOpen())
    {
        try
        {
            device_.Close();
        }
        catch(...)
        {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger("TheseusInterface"), "Something went wrong while closing port " << port_);
            return CallbackReturn::FAILURE;
        }
    }

    RCLCPP_INFO(rclcpp::get_logger("TheseusInterface"), "Hardware stopped");
    return CallbackReturn::SUCCESS;
}

/* Reads the current state of each joint from hardware*/
hardware_interface::return_type TheseusInterface::read(const rclcpp::Time & time, const rclcpp::Duration & period)
{
    position_states_ = position_commands_; // Change when using encoders
    return hardware_interface::return_type::OK;
}

/* Write the goal state of each joint to hardware*/
hardware_interface::return_type TheseusInterface::write(const rclcpp::Time & time, const rclcpp::Duration & period)
{
    if(position_commands_ == prev_position_commands_)
    {
        return hardware_interface::return_type::OK;
    }

    std::string msg;
    // int base = static_cast<int>((position_commands_.at(0) * 180) / M_PI + (135)); 
    // msg.append("b");
    // msg.append(compensateZeros(base));
    // msg.append(std::to_string(base));
    // msg.append(",");
    int shoulder = static_cast<int>((position_commands_.at(1) * 180) / M_PI + (135)); 
    msg.append("s");
    msg.append(compensateZeros(shoulder));
    msg.append(std::to_string(shoulder));
    msg.append(",");
    int elbow = static_cast<int>((position_commands_.at(2) * 180) / M_PI + (135)); 
    msg.append("e");
    msg.append(compensateZeros(elbow));
    msg.append(std::to_string(elbow));
    msg.append(",");
    //Add gripper

    try
    {
        device_.Write(msg);
        std::string response;
        device_.ReadLine(response, '\n', 100); // Read until newline or timeout (100 ms)
        RCLCPP_INFO_STREAM(rclcpp::get_logger("TheseusInterface"), "Received response from hardware: " << response);
    } catch(...)
    {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger("TheseusInterface"), "Something went wrong while writing to port " << port_);
        return hardware_interface::return_type::ERROR;
    }

    prev_position_commands_ = position_commands_; // Update previous commands to current commands
        
    // Print the whole command vector nicely
    std::ostringstream oss;
    oss << "[ ";
    for (size_t i = 0; i < position_commands_.size(); ++i) {
        oss << position_commands_[i];
        if (i < position_commands_.size() - 1) oss << ", ";
    }
    oss << " ]";

    RCLCPP_INFO_STREAM(rclcpp::get_logger("TheseusInterface"),
                       "Received position commands: " << oss.str());
    RCLCPP_INFO_STREAM(rclcpp::get_logger("TheseusInterface"), "Sent commands to hardware: " << msg);
    return hardware_interface::return_type::OK;
}

}

PLUGINLIB_EXPORT_CLASS(theseus_controller::TheseusInterface, hardware_interface::SystemInterface)
// Register the TheseusInterface class as a hardware interface plugin
// This allows the ROS 2 controller framework to discover and use this interface
// when loading the controller for the manipulator.