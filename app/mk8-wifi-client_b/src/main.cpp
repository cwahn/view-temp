#include <iostream>
#include <thread>


#include "server.hpp"

int main(int, char **)
{

    mainX();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    return 0;
}