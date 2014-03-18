#pragma once

#include <boost/filesystem.hpp>
#include <string>
#include <tuple>


/***
 * @brief Send a file to given host:port
 * 
 * @param path path of local file
 * @param host url of host
 * @param port port number on which the host is reachable
 *
 */
void sendFile(const boost::filesystem::path path, const std::string host, const std::string port);

/***
 * @brief Waits on port for incoming files
 *
 * @param port is the port to listen on
 *
 * @return std::tuple of (fileContent, filename)
 *
 */
std::tuple<std::string, std::string> recvFile(const unsigned port);
