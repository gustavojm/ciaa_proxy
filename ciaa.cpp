#include <iostream>
#include <string>
#include <restbed>

#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <ciaa.hpp>

constexpr std::chrono::milliseconds TIMEOUT = std::chrono::milliseconds(200);
using boost::asio::ip::tcp;

void CIAA::connect() {
    if (!isConnected) {
        socket.reset(new boost::asio::ip::tcp::socket(serv.ioservice));	// Create new socket (old one is destroyed automatically)
        tcp::resolver resolver(serv.ioservice);
        tcp::resolver::iterator endpoint_iter = resolver.resolve(ip, port);

        boost::asio::async_connect(*socket, endpoint_iter,
                [&](boost::system::error_code ec, tcp::resolver::iterator it) {
                    if (ec)
                        throw std::runtime_error(
                                "Error connecting to CIAA: " + ec.message());
                    std::cout << "Connected to CIAA: " << it->endpoint()
                            << std::endl;
                    isConnected = true;
                });
        serv.await_operation_ex(TIMEOUT, [&] {
            throw std::runtime_error("Error connecting to CIAA: timeout\n");
        });
    }
}

size_t CIAA::receive(boost::asio::streambuf &rx_buffer) {
    size_t bytes;

    boost::asio::async_read_until(*socket, rx_buffer, '\0',
            [&](boost::system::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    isConnected = false;
                    throw std::runtime_error(
                            "Error receiving message: " + ec.message());
                }
                //std::cout << "Received message is: " << &rx_buffer << '\n';
                bytes = bytes_transferred;
            });

    serv.await_operation(TIMEOUT, *socket);
    return bytes;
}

void CIAA::send(const restbed::Bytes &tx_buffer) {
    boost::asio::async_write(*socket, boost::asio::buffer(tx_buffer),
            [&](boost::system::error_code ec, size_t /*bytes_transferred*/) {
                if (ec) {
                    isConnected = false;
                    throw std::runtime_error(
                            "Error sending message: " + ec.message());
                }
            });
    serv.await_operation(TIMEOUT, *socket);
}

void CIAA::tx_rx(const restbed::Bytes &tx_buffer,
        boost::asio::streambuf &rx_buffer) {
    send(tx_buffer);
    receive(rx_buffer);
}
