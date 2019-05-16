#pragma once

#include <vector>

namespace rb {

    enum Direction: int {
        Up,
        Down,
        Left,
        Right
    };

    struct Command {
        int size;
        int length;
        Direction direction;
    };

    enum StatusControl : int {
        Play,
        Stop
    };

    enum StatusConnection : int {
        Closed,
        Closing,
        Connecting,
        Connected,
        Recived
    };

    enum StatusPosition : int {
        No,
        Ok
    };

    struct StateInput {

        // Connection
        char macAddress[18] = "00:16:53:18:8E:08";
        int chanel = 1;

        // Map
        std::vector<Command> commands;
    };

    struct StateData {
        StatusControl statusControl = StatusControl::Stop;

        StatusConnection statusConnection = StatusConnection::Closed;

        int positionActive = 0;

        std::string message = "";
    };
}