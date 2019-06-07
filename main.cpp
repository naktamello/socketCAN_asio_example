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

#define IFACE_NAME "slcan0"
using namespace std::chrono_literals;
std::sig_atomic_t signal_value;
using CanFrame = struct can_frame;

void set_signal(int value) {
    signal_value = static_cast<decltype(signal_value)>(value);
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

void print_can_msg(CanFrame frame) {

}

int main(int argc, char *argv[]) {
    int flags, opt;
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
            reinterpret_cast<struct sockaddr*>(&addr),
            sizeof(addr)
    );
    if (-1 == rc) {
        std::perror("bind");
        ::close(s);
        return errno;
    }

    while (0 == signal_value){
        CanFrame frame;
        auto numBytes = ::read(s, &frame, CAN_MTU);
        switch (numBytes) {
            case CAN_MTU:
                std::cout << "msg received!" << std::endl;
                frame.data[0]++;
                ::write(s, &frame, CAN_MTU);
                break;
            case CANFD_MTU:
                break;
            case -1:
                // Check the signal value on interrupt
                if (EINTR == errno)
                    continue;

                // Delay before continuing
                std::perror("read");
                std::this_thread::sleep_for(100ms);
            default:
                continue;
        }
    }

    if (::close(s) == -1) {
        std::perror("close");
        return errno;
    }

    std::cout << std::endl << "done" << std::endl;
    return EXIT_SUCCESS;

}