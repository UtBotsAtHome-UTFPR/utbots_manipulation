#include <theseus_controller/theseus_interface.hpp>

namespace theseus_controller
{
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

    position_commands_.reserve(info_joints.size());
    position_states_.reserve(info_joints.size());
    prev_position_commands_.reserve(info_joints.size());

    return CallbackReturn::SUCESS;
}

virtual std::vector<hardware_interface::StateInterface> TheseusInterface::export_state_interfaces()
{
    std::vector<hardware_interface::StateInterface> state_interfaces;
    //TODO
}