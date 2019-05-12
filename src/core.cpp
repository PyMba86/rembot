#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <iostream>
#include "core.h"
#include "data.h"
#include "queue.h"
#include "../libext/asio_bluetooth/wrapper.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include "connection.h"

namespace rb {

    inline void updateStateData(const StateData *bsrc, StateData *bdst) {
        bdst->number = bsrc->number;
    }

    struct Core::Data {
        Data() : hive(new Hive()), connection(new BtConnection(hive)) {};

        ~Data() {
            free();

        }

        bool init() {

            return true;
        }

        void free() {

        }

        enum BufferId {
            BUFFER_UI,
            BUFFER_CACHED,
            BUFFER_ACTIVE,
        };

        boost::shared_ptr<Hive> hive;

        boost::shared_ptr<BtConnection> connection;

        std::thread workerMain;
        std::thread workerConnect;

        bool needRecache = false;
        bool cacheUpdated = false;
        bool encodeIdParity = true;
        bool useChecksum = false;
        std::atomic<bool> isRunning{};

        mutable std::mutex mutexStateData;

        std::weak_ptr<StateInput> stateInput;
        std::array<std::shared_ptr<StateData>, 3> stateData;

        RingBuffer<std::function<void()>, 256> inputQueue;


        std::mutex m;
        std::condition_variable cond_var;
        bool notified = false;

        std::mutex g_mutex;
        std::condition_variable g_cond;

    };

    Core::Core() : _data(new Data()) {
        _data->stateData[Data::BUFFER_UI] = std::make_shared<StateData>();
        _data->stateData[Data::BUFFER_CACHED] = std::make_shared<StateData>();
        _data->stateData[Data::BUFFER_ACTIVE] = std::make_shared<StateData>();
    }

    Core::~Core() {
        if (_data->workerMain.joinable()) _data->workerMain.join();
    }

    void Core::init() {
        _data->isRunning = true;

        _data->workerMain = std::thread(&Core::main, this);

        _data->workerConnect = std::thread([&]{ _data->hive->Run(); });
    }

    void Core::update() {

        if (!_data->cacheUpdated) return;

        std::lock_guard<std::mutex> lock(_data->mutexStateData);

        auto &bsrc = _data->stateData[Data::BUFFER_CACHED];
        auto &bdst = _data->stateData[Data::BUFFER_UI];

        updateStateData(bsrc.get(), bdst.get());

        _data->cacheUpdated = false;
    }

    void Core::exit() {
        _data->isRunning = false;
    }

    std::weak_ptr<StateData> Core::getStateData() const {
        return _data->stateData[Data::BUFFER_UI];
    }

    void Core::setStateInput(std::weak_ptr<StateInput> stateInput) {
        _data->stateInput = stateInput;
    }

    void Core::notifyEvent(Core::Event event) {

        auto inp = _data->stateInput.lock();

        if (inp == nullptr) return;

        std::unique_lock<std::mutex> lock(_data->m);

        switch (event) {
            case Close: {
                _data->inputQueue.push([this]() {

                    _data->hive->Stop();

                    if (_data->workerConnect.joinable()) _data->workerConnect.join();

                    _data->isRunning = false;
                });
            }
                break;
            case Init: {
                std::cout << "Init" << std::endl;
                _data->inputQueue.push([this]() {
                     _data->connection->Connect("00:16:53:18:8E:08", 1);
                });
            }
                break;
            case Disconnect: {
                std::cout << "Disconnect" << std::endl;
                _data->inputQueue.push([this]() {
                    _data->connection->Disconnect();
                });
            }
                break;
            default:
                break;
        }
        _data->notified = true;
        _data->cond_var.notify_one();
    }

    void Core::input() {
        {
            auto inp = _data->stateInput.lock();

            if (inp == nullptr) return;


        }

        while (_data->inputQueue.size() > 0) {
            _data->inputQueue.pop()();
        }
    }

    void Core::main() {
        auto data = _data->stateData[Data::BUFFER_ACTIVE];

        while (_data->isRunning) {
            std::unique_lock<std::mutex> lock(_data->m);

            while (!_data->notified)
               _data->cond_var.wait(lock);

            input();

            //connection->Send({'S'});

            cache();

                _data->notified = false;
        }
    }

    void Core::cache() {
        if (!_data->needRecache) return;

        std::lock_guard<std::mutex> lock(_data->mutexStateData);

        auto &bsrc = _data->stateData[Data::BUFFER_ACTIVE];
        auto &bdst = _data->stateData[Data::BUFFER_CACHED];

        updateStateData(bsrc.get(), bdst.get());

        _data->cacheUpdated = true;
        _data->needRecache = false;
    }


}



