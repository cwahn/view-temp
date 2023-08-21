#include "efp.hpp"
#include "serialib.h"
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <bitset>
#include <thread>
#include <chrono>

using namespace efp;

struct Max30102SerialFrame
{
    int red;
    int ir;
    int temp;
};

struct SerialFrame
{
    Max30102SerialFrame max30102_data;
    int audio[32];
};

struct IntPair
{
    int a;
    int b;
};

template <typename A, typename F>
struct Rs232Stream
{
public:
    Rs232Stream(const char *device, const unsigned int baudrate, const F &on_read)
        : device_(device), baudrate_(baudrate), on_read_(on_read)
    {
        read_period_us_ = sizeof(A) / (baudrate / 8.) * 1000000;
        // read_period_us_ = 1000000;

        // ! Possible stack overflow
        thread_ = std::thread(&Rs232Stream::on_disconnect_, this);
    }

    ~Rs232Stream()
    {
        keep_alive_ = false;
        serial_.closeDevice();
    }

private:
    void on_connect_()
    {
        std::cout << "connected" << std::endl;

        while (serial_.isDeviceOpen() && keep_alive_)
        {
            auto start = std::chrono::high_resolution_clock::now();

            uint8_t start_byte = 0;
            int removed_bytes = 0;
            while (start_byte != 0x55 && (removed_bytes < 2 * sizeof(A)))
            {
                serial_.readBytes(&start_byte, 1);
                removed_bytes++;
            }

            // std::cout << "number of removed bytes: " << removed_bytes << std::endl;

            if (removed_bytes < 2 * sizeof(A))
            {
                uint8_t rx_buffer[sizeof(A)];
                serial_.readBytes(&rx_buffer, sizeof(A));
                A rx_data = *(A *)(&rx_buffer);

                on_read_(rx_data);
            }
            else
            {
                std::cout << "too many illigal bytes" << std::endl;
                serial_.closeDevice();
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            auto remaining_time = std::chrono::microseconds(read_period_us_) - elapsed_time;
            if (remaining_time > std::chrono::microseconds(0))
            {
                std::this_thread::sleep_for(remaining_time);
            }
        }

        on_disconnect_();
    }

    void on_disconnect_()
    {
        std::cout << "disconnected" << std::endl;

        while (!serial_.isDeviceOpen() && keep_alive_)
        {
            std::cout << "awaiting for connection" << std::endl;

            char res = serial_.openDevice(device_, baudrate_);
            if (res != 1)
            {
                // ! temp 1000, original 50
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            else
            {
                on_connect_();
            }
        }

        std::cout << "terminating" << std::endl;
    }

    serialib serial_;
    const char *device_;
    const unsigned int baudrate_;
    const F on_read_;

    int read_period_us_;
    bool keep_alive_ = true;
    std::thread thread_;
};

int main(int argc, char const *argv[])
{
    const char *serial_port = "/dev/cu.usbserial-0001";
    const int baudrate = 115200;

    auto on_read = [](const SerialFrame &data)
    {
        // std::cout << "len max30102: " << data.max30102_data.num_data << std::endl;
        std::cout << "red: " << data.max30102_data.red
                  << " ir: " << data.max30102_data.ir
                  << " temp: " << data.max30102_data.temp << std::endl;
    };

    Rs232Stream<SerialFrame, decltype(on_read)>
        packet_stream{serial_port, baudrate, on_read};

    assert(sizeof(SerialFrame) == 140);

    // auto on_read = [](const IntPair &data)
    // { std::cout << "a: " << data.a << "b: " << data.b << std::endl; };

    // Rs232Stream<IntPair, decltype(on_read)>
    //     packet_stream{serial_port, baudrate, on_read};

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50000));
    }

    return 0;
}
