#ifndef BLE_VITAL_CLIENT_HPP_
#define BLE_VITAL_CLIENT_HPP_

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

#include "efp.hpp"

#include "simpleble/SimpleBLE.h"
#include "simpleble/Logging.h"

#include "../src/utils.hpp"

struct GapCallback
{
    std::function<void(SimpleBLE::Adapter &)> on_scan_start;
    std::function<void(SimpleBLE::Adapter &)> on_scan_stop;
    std::function<void(SimpleBLE::Adapter &, SimpleBLE::Peripheral)> on_scan_found;
    std::function<void(SimpleBLE::Adapter &, SimpleBLE::Peripheral)> on_scan_update;
};

struct ClientConfig
{
    std::function<void(SimpleBLE::Adapter &)> setup;
    GapCallback gap_callback;
};

class Client
{
public:
    Client(ClientConfig config)
        : config_(config)
    {
        adapter_ = Utils::getAdapter().value();

        adapter_.set_callback_on_scan_start(
            [&]()
            { config_.gap_callback.on_scan_start(adapter_); });
        adapter_.set_callback_on_scan_stop(
            [&]()
            { config_.gap_callback.on_scan_stop(adapter_); });
        adapter_.set_callback_on_scan_updated(
            [&](SimpleBLE::Peripheral p)
            { config_.gap_callback.on_scan_update(adapter_, p); });
        adapter_.set_callback_on_scan_found(
            [&](SimpleBLE::Peripheral p)
            { config_.gap_callback.on_scan_found(adapter_, p); });

        config_.setup(adapter_);

        adapter_.scan_start();
    }
    ~Client() {}

private:
    const ClientConfig config_;
    SimpleBLE::Adapter adapter_;
};

#endif