#pragma once

#include "../libext/asio_bluetooth/wrapper.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include "data.h"

namespace rb {

    using  BtConnectionEvent =  std::function<void(StatusConnection, const std::vector<uint8_t>)>;

    class BtConnection : public Connection {
    public:
        explicit BtConnection(boost::shared_ptr<Hive> hive);

        ~BtConnection() override;

        void onEvent(BtConnectionEvent cbEvent);

    protected:
        virtual void runCbEvent(StatusConnection status, const std::vector<uint8_t> buffer);

    private:
        void OnAccept(const std::string &addr, uint8_t channel) override;

        void OnConnect(const std::string &addr, uint8_t channel) override;

        void OnSend(const std::vector<uint8_t> &buffer) override;

        void OnRecv(std::vector<uint8_t> &buffer) override;

        void OnTimer(const boost::posix_time::time_duration &delta) override;

        void OnError(const boost::system::error_code &error) override;

    private:
        boost::mutex global_stream_lock;
        BtConnectionEvent _cbEvent;
    };
}