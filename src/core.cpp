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
        bdst->statusConnection = bsrc->statusConnection;
        bdst->positionActive = bsrc->positionActive;
        bdst->message = bsrc->message;
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
        std::atomic<bool> isRunning{};

        mutable std::mutex mutexStateData;

        std::weak_ptr<StateInput> stateInput;
        std::array<std::shared_ptr<StateData>, 3> stateData;

        RingBuffer<std::function<void()>, 256> inputQueue;


        std::mutex m;
        std::condition_variable cond_var;
        bool notified = false;

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

        _data->connection->onEvent([this](StatusConnection status, const std::vector<uint8_t> buffer) {
            this->connectionCbEvent(status, buffer);
        });

        _data->workerConnect = std::thread([&] { _data->hive->Run(); });
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
            case Core::Event::Close: {
                _data->inputQueue.push([this]() {

                    _data->hive->Stop();

                    if (_data->workerConnect.joinable()) _data->workerConnect.join();

                    _data->isRunning = false;
                });
            }
                break;
            case Core::Event::Connect: {
                std::cout << "Connect" << std::endl;

                auto macAddress = inp->macAddress;
                auto chanel = inp->chanel;

                _data->inputQueue.push([this, macAddress, chanel]() {
                    _data->needRecache = true;
                    _data->connection->Connect(macAddress, chanel);

                    _data->stateData[Data::BUFFER_ACTIVE]->statusConnection = StatusConnection::Connecting;
                    _data->stateData[Data::BUFFER_ACTIVE]->message = "Connecting...";
                });
            }
                break;
            case Core::Event::Connected: {
                _data->inputQueue.push([this]() {
                    _data->needRecache = true;
                    _data->stateData[Data::BUFFER_ACTIVE]->statusConnection = StatusConnection::Connected;
                    _data->stateData[Data::BUFFER_ACTIVE]->message = "Connected";
                });
            }
                break;
            case Core::Event::Disconnected: {
                _data->inputQueue.push([this]() {
                    _data->needRecache = true;
                    _data->stateData[Data::BUFFER_ACTIVE]->statusConnection = StatusConnection::Closed;
                    _data->stateData[Data::BUFFER_ACTIVE]->message = "Disconnected";
                });
            }
                break;
            case Core::Event::Disconnect: {
                _data->inputQueue.push([this]() {
                    _data->needRecache = true;
                    _data->connection->Disconnect();
                    _data->stateData[Data::BUFFER_ACTIVE]->statusConnection = StatusConnection::Closing;
                    _data->stateData[Data::BUFFER_ACTIVE]->message = "Disconnecting...";
                });
            }
                break;
            case Core::Event::Play: {
                // Отправляем первую команду роботу из списка
            }
                break;
            case Core::Event::Stop: {
                _data->inputQueue.push([this]() {
                    // Останавливаем робота и зануляем значения
                    _data->needRecache = true;
                    _data->stateData[Data::BUFFER_ACTIVE]->positionActive = 0;
                    _data->stateData[Data::BUFFER_ACTIVE]->message = "Stop control";
                });
            }
                break;
            case Core::Event::Next: {
                // Отправляем следующую команду и отрисовываем
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

    void Core::connectionCbEvent(StatusConnection status, const std::vector<uint8_t> buffer) {

        switch (status) {
            case StatusConnection::Closed: {
                this->notifyEvent(Core::Event::Disconnected);
            }
                break;
            case StatusConnection::Closing:
                break;
            case StatusConnection::Connecting:
                break;
            case StatusConnection::Connected: {
                this->notifyEvent(Core::Event::Connected);
            }
                break;
            case StatusConnection::Recived: {
                // Проверяем ответ и отправляем команду
                this->notifyEvent(Core::Event::Next);
            }
                break;
        }
    }


}



