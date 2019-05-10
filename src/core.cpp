#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <iostream>
#include "core.h"
#include "data.h"
#include "queue.h"

namespace rb {

    inline void updateStateData(const StateData * bsrc, StateData * bdst) {

    }

    struct Core::Data {
        Data()= default;

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

        std::thread workerMain;

        bool needRecache = false;
        bool cacheUpdated = false;
        bool encodeIdParity = true;
        bool useChecksum = false;
        std::atomic<bool> isRunning;

        mutable std::mutex mutexStateData;

        std::weak_ptr<StateInput> stateInput;
        std::array<std::shared_ptr<StateData>, 3> stateData;

        RingBuffer<std::function<void()>, 256> inputQueue;

    };

    Core::Core() : _data(new Data()) {
        _data->stateData[Data::BUFFER_UI]     = std::make_shared<StateData>();
        _data->stateData[Data::BUFFER_CACHED] = std::make_shared<StateData>();
        _data->stateData[Data::BUFFER_ACTIVE] = std::make_shared<StateData>();
    }

    Core::~Core() {
        if (_data->workerMain.joinable()) _data->workerMain.join();
    }

    void Core::init() {
        _data->isRunning = true;

       // _data->workerMain = std::thread(&Core::main, this);
    }

    void Core::update() {

        if (!_data->cacheUpdated) return;

        std::lock_guard<std::mutex> lock(_data->mutexStateData);

        auto & bsrc = _data->stateData[Data::BUFFER_CACHED];
        auto & bdst = _data->stateData[Data::BUFFER_UI];

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

        switch(event) {
            case Init: {
                std::cout << "Hello" << std::endl;
            }
        }

        auto inp = _data->stateInput.lock();

        if (inp == nullptr) return;

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
            input();

            cache();
        }
    }

    void Core::cache() {
        if (!_data->needRecache) return;

        std::lock_guard<std::mutex> lock(_data->mutexStateData);

        auto & bsrc = _data->stateData[Data::BUFFER_ACTIVE];
        auto & bdst = _data->stateData[Data::BUFFER_CACHED];

        updateStateData(bsrc.get(), bdst.get());

        _data->cacheUpdated = true;
        _data->needRecache = false;
    }


}



