#include <utility>

#include <utility>

#include "connection.h"

namespace rb {

    BtConnection::~BtConnection() = default;

    BtConnection::BtConnection(boost::shared_ptr<Hive> hive) : Connection(std::move(hive)) {}

    void BtConnection::OnAccept(const std::string &addr, uint8_t channel) {
        global_stream_lock.lock();
        std::cout << "[OnAccept] " << addr << ":" << channel << "\n";
        global_stream_lock.unlock();

        // Start the next receive
        Recv();
    }

    void BtConnection::OnConnect(const std::string &addr, uint8_t channel) {
        global_stream_lock.lock();
        std::cout << "[OnConnect] " << addr << ":" << channel << "\n";
        global_stream_lock.unlock();

        runCbEvent(StatusConnection::Connected, {});
        // Start the next receive
        Recv();
    }

    void BtConnection::OnSend(const std::vector<uint8_t> &buffer) {
        global_stream_lock.lock();
        std::cout << "[OnSend] " << buffer.size() << " bytes\n";
        for(size_t x=0; x<buffer.size(); x++) {

            std::cout << (char)buffer[x];
            if((x + 1) % 16 == 0)
                std::cout << "\n";
        }
        std::cout << "\n";
        global_stream_lock.unlock();
    }

    void BtConnection::OnRecv(std::vector<uint8_t> &buffer) {
        global_stream_lock.lock();
        std::cout << "[OnRecv] " << buffer.size() << " bytes\n";
        for(size_t x=0; x<buffer.size(); x++) {

            std::cout << (char)buffer[x];
            if((x + 1) % 16 == 0)
                std::cout << "\n";
        }
        std::cout << "\n";
        global_stream_lock.unlock();

        runCbEvent(StatusConnection::Recived, buffer);

        // Start the next receive
        Recv();
    }

    void BtConnection::OnTimer(const boost::posix_time::time_duration &delta) {
        global_stream_lock.lock();
        std::cout << "[OnTimer] " << delta << std::endl;
        global_stream_lock.unlock();
    }

    void BtConnection::OnError(const boost::system::error_code &error) {
        global_stream_lock.lock();
        std::cout << "[OnError] " << error.message() << "\n";
        global_stream_lock.unlock();
        runCbEvent(StatusConnection::Closed, {});
    }

    void BtConnection::onEvent(BtConnectionEvent cbEvent) {
        this->_cbEvent = std::move(cbEvent);
    }

    void BtConnection::runCbEvent(StatusConnection status, const std::vector<uint8_t> buffer) {
        if(_cbEvent) {
            _cbEvent(status, buffer);
        }
    }

}



