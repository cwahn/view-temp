#ifndef RS232_STREAM_HPP_
#define RS232_STREAM_HPP_

#include "serialib.h"

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

    bool is_connected()
    {
        return is_connected_;
    }

private:
    void on_connect_()
    {
        is_connected_ = true;
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
        is_connected_ = false;
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
    bool is_connected_;
    bool keep_alive_ = true;
    std::thread thread_;
};

#endif