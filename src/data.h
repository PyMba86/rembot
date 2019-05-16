#pragma once

#include <vector>

namespace rb {

    enum Direction: int {
        Up = 1,
        Down = 2,
        Left = 3,
        Right = 4
    };

    struct Command {
        int size;
        int length;
        Direction direction;
    };

    enum StatusControl : int {
        Play = 11,
        Stop = 12
    };

    enum StatusConnection : int {
        Closed,
        Closing,
        Connecting,
        Connected,
        Recived
    };

    enum StatusCommand : int {
        No = 0,
        Ok = 1
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