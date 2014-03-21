#pragma once

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <tuple>

using boost::asio::ip::tcp;

/***
 * @brief Send a stream over tcp to socket
 *
 * @param stream is the istream which should be send
 * @param socket where the stream should be send to
 * @param size is the number of bytes which should be send
 */
void sendStream(tcp::socket &socket, std::istream &stream, const size_t size);


/***
 * @brief Send a string over tcp to socket
 *
 * @param string is the string to send
 * @param socket where the string will be send to
 */
void sendString(tcp::socket &socket, const std::string string);


/***
 * @brief Send a file to given host:port
 *
 * @param path path of local file
 * @param host url of host
 * @param port port number on which the host is reachable
 */
void sendFile(const boost::filesystem::path path, const std::string host, const std::string port);


std::string recvStringUntil(tcp::socket &socket, const std::string delim);

/***
 * @brief Receive string on socket, which
 *        also was originally a string. So no binary
 *        data was transfered to a string. If you
 *        want to receive binary data, then you should
 *        use recvString(socket, size).
 *
 * @param socket where to wait for the string
 * @return string received
 */
std::string recvString(tcp::socket &socket);


/***
 * @brief Receive a string with size bytes from socket.
 *        Should be used for binary data.
 *
 * @param socket which receives tcp packets
 * @param size number of bytes to receive
 * @return string of received stream
 */
std::string recvString(tcp::socket &socket, const size_t size);


/***
 * @brief Receives size bytes on socket and returns
 *        it as stream. Should be used when transfering
 *        binary data where the transfered data size is
 *        known before.
 *
 * @param socket where the data arrives
 * @param size number of bytes that will be received
 */
//std::ostream& recvStream(tcp::socket &socket, const size_t size);

/***
 * @brief Waits on port for incoming files
 *
 * @param port is the port to listen on
 * @return std::tuple of (fileContent, filename)
 */
std::tuple<std::string, std::string> recvFile(const unsigned port);
