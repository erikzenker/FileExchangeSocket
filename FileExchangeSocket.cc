#include <FileExchangeSocket.h>

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <sstream>
#include <tuple>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>


#define CHUNK_SIZE 8096

using boost::asio::ip::tcp;

size_t fileSize(boost::filesystem::path path){
  std::ifstream file(path.c_str(), std::ios::in  | std::ios::binary | std::ios::ate);
  const size_t file_size  = file.tellg();
  file.close();

  return file_size;
}


std::string messageHeader(boost::filesystem::path path){
  const size_t file_size  = fileSize(path);

  std::stringstream header;
  header << "<transmission>" << std::endl;
  header << "<filename>" << path.filename().string() << "</filename>" << std::endl;
  header << "<path>" << path.parent_path().string() << "</path>" << std::endl;
  header << "<size>" << file_size << "</size>" << std::endl;
  header << "<md5sum>" << "" << "</md5sum>" << std::endl;
  header << "</transmission>" << std::endl;

  return header.str();
}


std::string messageBody(boost::filesystem::path path){
  std::ifstream file(path.c_str(), std::ios::in  | std::ios::binary | std::ios::ate);
  const size_t file_size  = file.tellg();

  char* memblock;
  if(file.is_open()){
    memblock = new char[file_size];
    file.seekg (0, std::ios::beg);
    file.read (memblock, file_size);
    file.close();
    std::string body = std::string (memblock, file_size); 
    delete[] memblock;
    return body;
    
  }

  return std::string("");
}


/*************************************************************************
 *  SEND FILE TOOLS                                                      *
 *                                                                       *
 *                                                                       *
 *************************************************************************/
void sendString(tcp::socket &socket, const std::string string){
  const size_t size = string.size();
  std::stringstream ss;
  ss << string;

  sendStream(socket, ss, size);
}

void sendStream(tcp::socket &socket, std::istream &stream,  const size_t size){
  size_t transferred = 0;
  boost::array<char, CHUNK_SIZE> chunk;
    
  while (transferred != size){ 
      size_t remaining = size - transferred; 
      size_t write_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
      stream.read(&chunk[0], CHUNK_SIZE); 
      boost::asio::write(socket, boost::asio::buffer(chunk, write_size)); 
      transferred += write_size; 
    } 

}

void sendFile(boost::filesystem::path path, std::string host, std::string port){
  // Connect to socket
  boost::asio::io_service io_service; 
  tcp::resolver resolver(io_service); 
  tcp::resolver::query url(host, port);
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(url);
  tcp::socket socket(io_service);
  boost::asio::connect(socket, endpoint_iterator);

  // Prepare XML-Header and Body
  const std::string header = messageHeader(path); 
  const std::string body   = messageBody(path);

  std::cout << "Sending to " 
	    << socket.remote_endpoint().address() 
	    << ":" 
	    << socket.remote_endpoint().port() 
	    << " " 
	    << path.filename().string() 
	    << " (" << body.size() << " Bytes)"<<std::endl;
  std::cout << header << std::endl;

  // Send header
  sendString(socket, header);

  // Send body (binary data)
  sendString(socket, body);

}

/*************************************************************************
 *  RECEIVE FILE TOOLS                                                   *
 *                                                                       *
 *                                                                       *
 *************************************************************************/
std::string recvStringUntil(tcp::socket &socket, const std::string delim){

  boost::asio::streambuf streambuf;
  std::stringstream stream;

  boost::asio::read_until(socket, streambuf, delim);
  std::istream is(&streambuf);

  while(is.good()){
    std::string string;
    std::getline(is,string);
    stream << string << std::endl;
  }

  return stream.str();
}

std::string recvString(tcp::socket &socket){
  std::stringstream stream;
  boost::array<char, CHUNK_SIZE> buffer; 
  boost::system::error_code error;

  while(error != boost::asio::error::eof){
    memset(&buffer, 0, CHUNK_SIZE);
    size_t size = socket.read_some(boost::asio::buffer(buffer), error);
    stream.write(buffer.data(), size);
  }
  return stream.str();

}

std::string recvString(tcp::socket &socket, const size_t size){
  std::stringstream stream;
  boost::array<char, CHUNK_SIZE> chunk; 
  size_t transferred = 0;
    
  while (transferred != size) { 
    size_t remaining = size - transferred; 
    size_t read_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
    boost::asio::read(socket, boost::asio::buffer(chunk, read_size)); 
    stream.write(&chunk[0], read_size); 
    transferred += read_size; 
  }

  return stream.str();

}

std::tuple<std::string, std::string> recvFile(const unsigned port){
  // Create socket
  boost::asio::io_service io_service;
  tcp::endpoint endpoint(tcp::v4(), port);
  tcp::acceptor acceptor(io_service, endpoint); 

  // Accept connections to socket
  tcp::socket socket(io_service); 
  acceptor.accept(socket); 

  // Recv header 
  std::stringstream header; header << recvStringUntil(socket, "</transmission>");

  std::cout << header.str() << std::endl;

  // Read Xml
  boost::property_tree::ptree ptree;
  read_xml(header, ptree);

  const size_t          size = ptree.get<size_t>("transmission.size");
  const std::string filename = ptree.get<std::string>("transmission.filename");

  // Receive body (binary data)
  std::string body = recvString(socket, size);

  std::cout << "Receive from " 
	    << socket.remote_endpoint().address() 
	    << ":" 
	    << socket.remote_endpoint().port() 
	    << " " 
	    << filename 
	    << " (" << body.size() << " Bytes)" << std::endl;

  return std::make_tuple(body, filename);

}
