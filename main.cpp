#include <iostream>
#include <thread>
#include <chrono>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#define IFACE_NAME "slcan0"

using namespace std::chrono_literals;
std::sig_atomic_t signal_value;
using CanFrame = struct can_frame;


void set_signal(int value) {
    signal_value = static_cast<decltype(signal_value)>(value);
}

void can_filter() {

//    struct can_filter filter[0];
//    filter[0].can_id   = 0x000;
//    filter[0].can_mask = CAN_SFF_MASK;
//
//    rc = ::setsockopt(
//            s,
//            SOL_CAN_RAW,
//            CAN_RAW_FILTER,
//            &filter,
//            sizeof(filter)
//    );
//    if (-1 == rc) {
//        std::perror("setsockopt filter");
//        ::close(s);
//        return errno;
//    }
}

void init_system_signals(struct sigaction action) {
    action.sa_handler = set_signal;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    ::sigaction(SIGINT, &action, nullptr);
    ::sigaction(SIGTERM, &action, nullptr);
    ::sigaction(SIGQUIT, &action, nullptr);
    ::sigaction(SIGHUP, &action, nullptr);

    // Initialize the signal value to zero
    signal_value = 0;
}

void print_can_msg(CanFrame &frame) {
    std::cout << std::hex << frame.can_id << "(" << static_cast<int>(frame.can_dlc) << "):";
    for (std::size_t i = 0; i < frame.can_dlc; ++i) {
        if (i > 0)
            std::cout << " ";
        std::cout << std::hex << static_cast<int>(frame.data[i]);
    }
    std::cout << std::endl;
}

class CanDevice {
public:
    explicit CanDevice(int can_fileno) : can_fileno_(can_fileno) {
        io_service_ = std::make_shared<boost::asio::io_service>();
        socket_ = std::make_unique<boost::asio::posix::stream_descriptor>(*io_service_, can_fileno);
        boost::thread t(boost::bind(&boost::asio::io_service::run, io_service_));
        start_reading();
    }
    // TODO close socket when exiting
    void write_async(CanFrame &frame) {
        boost::asio::async_write(*socket_, boost::asio::buffer(&frame, sizeof(frame)),
                                 boost::bind(&CanDevice::write_async_complete, this, boost::asio::placeholders::error));
    }

private:
    void start_reading() {
        boost::asio::async_read(*socket_, boost::asio::buffer(read_buffer_, read_length),
                                boost::bind(&CanDevice::read_async_complete, this, boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void read_async_complete(const boost::system::error_code &error, size_t bytes_transferred) {
        if (!error) {
            std::cout << "received:" + std::to_string(bytes_transferred) << std::endl;
            std::memcpy(&frame_, read_buffer_, bytes_transferred);
            write_async(frame_);
            print_can_msg(frame_);
            start_reading();
        } else {
            std::cout << "read error!" << std::endl;
        }
    }

    void write_async_complete(const boost::system::error_code &error) {
        if (!error) {
            std::cout << "write async complete" << std::endl;
        } else {
            std::cout << "write error!" << std::endl;
        }
    }

    int can_fileno_;
    static const size_t read_length = CAN_MTU;
    char read_buffer_[read_length] = {0};
    CanFrame frame_{};
    std::unique_ptr<boost::asio::posix::stream_descriptor> socket_;
    std::shared_ptr<boost::asio::io_service> io_service_;
};

int main(int argc, char *argv[]) {
    int s, rc;
    std::string iface = IFACE_NAME;
    struct sockaddr_can addr{};
    struct ifreq ifr{};
    struct sigaction sa{};
    init_system_signals(sa);

    s = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (-1 == s) {
        std::perror("socket");
        return errno;
    }


    int enable = 1;

    rc = ::setsockopt(
            s,
            SOL_CAN_RAW,
            CAN_RAW_FD_FRAMES,
            &enable,
            sizeof(enable)
    );
    if (-1 == rc) {
        std::perror("setsockopt CAN FD");
        ::close(s);
        return errno;
    }

    std::strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ);
    if (::ioctl(s, SIOCGIFINDEX, &ifr) == -1) {
        std::perror("ioctl");
        ::close(s);
        return errno;
    }

    // Bind the socket to the network interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    rc = ::bind(
            s,
            reinterpret_cast<struct sockaddr *>(&addr),
            sizeof(addr)
    );
    if (-1 == rc) {
        std::perror("bind");
        ::close(s);
        return errno;
    }

    CanDevice can_device = CanDevice(s);

    while (0 == signal_value) {

//        CanFrame frame;
////        auto numBytes = ::read(s, &frame, CAN_MTU);
//        switch (1) {
//            case CAN_MTU:
//                std::cout << "msg received!" << std::endl;
//                frame.data[0]++;
//                can_device.write_async(frame);
////                ::write(s, &frame, CAN_MTU);
//                break;
//            case CANFD_MTU:
//                break;
//            case -1:
//                // Check the signal value on interrupt
//                if (EINTR == errno)
//                    continue;
//
//                // Delay before continuing
//                std::perror("read");
//                std::this_thread::sleep_for(100ms);
//            default:
//                continue;
//        }
        std::this_thread::sleep_for(100ms);
    }

    if (::close(s) == -1) {
        std::perror("close");
        return errno;
    }

    std::cout << std::endl << "done" << std::endl;
    return EXIT_SUCCESS;

}