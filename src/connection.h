#pragma once

#include "../libext/asio_bluetooth/wrapper.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

namespace rb {

    class BtConnection : public Connection {
    public:
        explicit BtConnection(boost::shared_ptr<Hive> hive);

        ~BtConnection() override;

    private:
        void OnAccept(const std::string &addr, uint8_t channel) override;

        void OnConnect(const std::string &addr, uint8_t channel) override;

        void OnSend(const std::vector<uint8_t> &buffer) override;

        void OnRecv(std::vector<uint8_t> &buffer) override;

        void OnTimer(const boost::posix_time::time_duration &delta) override;

        void OnError(const boost::system::error_code &error) override;

    private:
        boost::mutex global_stream_lock;
    };
}