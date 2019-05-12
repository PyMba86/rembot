#pragma once

#include <memory>
#include <condition_variable>
#include <mutex>
#include "data.h"

namespace rb {

    struct StateData;
    struct StateInput;

    class Core {
    public:
        Core();
        ~Core();


        void init();

        void update();

        void exit();

        std::weak_ptr<StateData> getStateData() const;

        void setStateInput(std::weak_ptr<StateInput> stateInput);

        enum Event {
            Close,
            Connect,
            Connected,
            Disconnected,
            Disconnect,
            Play,
            Stop,
            Next
        };

        void notifyEvent(Event event);

        void connectionCbEvent(StatusConnection status, const std::vector<uint8_t> buffer);

    private:
        void input();
        void main();
        void cache();

        struct Data;
        std::unique_ptr<Data> _data;
    };
}