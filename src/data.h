#pragma once

#include <vector>

namespace rb {

    struct Position {
        int x;
        int y;
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
        std::vector<Position> positions;
    };

    struct StateData {
        StatusConnection statusConnection = StatusConnection::Closed;

        int positionActive = 0;

        StatusControl statusControl = StatusControl::Stop;

        std::string message = "";
    };
}