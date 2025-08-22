#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <libserial/SerialPort.h>

using std::placeholders::_1;

/**
 * @class SimpleSerialInterface
 * @brief ROS2 node for interfacing with a serial device (e.g., Arduino) using LibSerial.
 *
 * This class subscribes to the "serial_transmitter" topic to receive messages
 * and sends them to the hardware via a serial port. It also periodically checks
 * for incoming data from the serial device and publishes received messages on
 * the "serial_receiver" topic.
 *
 * Parameters:
 * - port (string): Serial port device path (default: "/dev/ttyUSB0").
 *
 * Publishers:
 * - "serial_receiver" (std_msgs::msg::String): Publishes messages received from the serial device.
 *
 * Subscriptions:
 * - "serial_transmitter" (std_msgs::msg::String): Receives messages to send to the serial device.
 *
 * Timer:
 * - Periodically checks for new data from the serial device and publishes it.
 *
 * Usage:
 * - Instantiate and spin this node in a ROS2 application to enable serial communication.
 */
class SimpleSerialInterface : public rclcpp::Node
{
public:
    SimpleSerialInterface() : Node("simple_serial_interface")
    {
        declare_parameter(std::string("port"), "/dev/ttyUSB0");
        std::string port_ = get_parameter("port").as_string();

        sub_ = create_subscription<std_msgs::msg::String>("serial_transmitter", 10, std::bind(&SimpleSerialInterface::msgCallback, this, _1));
        pub_ = create_publisher<std_msgs::msg::String>("serial_receiver", 10);
        using namespace std::chrono_literals;
        timer_ = create_wall_timer(10ms, std::bind(&SimpleSerialInterface::timerCallback, this));

        device_.Open(port_);
        device_.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
    }
    ~SimpleSerialInterface()
    {
        device_.Close();
    }

    void msgCallback(const std_msgs::msg::String &msg)
    {
        RCLCPP_INFO_STREAM(get_logger(), "New message recieved from subscriber, sending to hardware on serial port: " << msg.data);
        device_.Write(msg.data);
    }

    void timerCallback()
    {
        auto msg = std_msgs::msg::String();
        if (rclcpp::ok() && device_.IsDataAvailable())
        {
            device_.ReadLine(msg.data);
            pub_->publish(msg);
            RCLCPP_INFO_STREAM(get_logger(), "New message recieved from hardware, publishing on topic: " << msg.data);
        }
    }

private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    LibSerial::SerialPort device_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SimpleSerialInterface>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}