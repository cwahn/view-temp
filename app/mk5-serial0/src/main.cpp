#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <bitset>
#include <thread>
#include <chrono>

#include "efp.hpp"
#include "rs232-stream.hpp"

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

int main(int argc, char const *argv[])
{
    const char *serial_port = "/dev/cu.usbserial-0001";
    const int baudrate = 115200;

    auto on_read = [](const SerialFrame &data)
    {
        std::cout << "red: " << data.max30102_data.red
                  << " ir: " << data.max30102_data.ir
                  << " temp: " << data.max30102_data.temp << std::endl;
    };

    Rs232Stream<SerialFrame, decltype(on_read)>
        packet_stream{serial_port, baudrate, on_read};

    assert(sizeof(SerialFrame) == 140);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50000));
    }

    return 0;
}
