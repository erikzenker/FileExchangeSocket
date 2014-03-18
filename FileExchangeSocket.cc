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


/*************************************************************************
 *  SEND FILE TOOLS                                                      *
 *                                                                       *
 *                                                                       *
 *************************************************************************/
std::string messageHeader(boost::filesystem::path path){
  const size_t file_size  = fileSize(path);

  std::stringstream header;
  header << "<transmission>" << std::endl;
  header << "<filename>" << path.filename().string() << "</filename>" << std::endl;
  header << "<path>" << path.parent_path().string() << "</path>" << std::endl;
  header << "<size>" << file_size << "</size>" << std::endl;
  header << "<md5sum>" << "" << "</md5sum>" << std::endl;
  header << "</transmission>" << std::endl << std::endl;

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

void sendStream(std::istream &stream, tcp::socket &socket, const size_t size){
  size_t transferred = 0;
  const size_t chunk_size = CHUNK_SIZE;
  boost::array<char, chunk_size> chunk;
    
  while (transferred != size){ 
      size_t remaining = size - transferred; 
      size_t write_size = (remaining > chunk_size) ? chunk_size : remaining;
      stream.read(&chunk[0], chunk_size); 
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

  // Contruct message = header + body
  std::stringstream message;
  message << header << body;

  // Send message size
  uint32_t message_size = header.size() + body.size();

  std::cout << "Sending to " 
	    << socket.remote_endpoint().address() 
	    << ":" 
	    << socket.remote_endpoint().port() 
	    << " " 
	    << path.filename().string() << std::endl;

  boost::asio::write(socket, boost::asio::buffer(&message_size, sizeof(message_size)));

  // Send message
  sendStream(message, socket, message_size);

}

/*************************************************************************
 *  RECEIVE FILE TOOLS                                                   *
 *                                                                       *
 *                                                                       *
 *************************************************************************/
std::string recvStream(tcp::socket &socket, const size_t size){
  std::stringstream stream;
  const size_t chunk_size = CHUNK_SIZE;
  boost::array<char, chunk_size> chunk; 
  size_t transferred = 0;
    
  while (transferred != size) { 
    size_t remaining = size - transferred; 
    size_t read_size = (remaining > chunk_size) ? chunk_size : remaining;
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

  uint32_t size  = 0;
  boost::asio::read(socket, boost::asio::buffer(&size, sizeof(size))); 

  // Receive message
  std::string message = recvStream(socket, size);

  // Seperate header(xml) from body
  std::string endSeq = "</transmission>";
  std::size_t xmlEndPos = message.find(endSeq) + endSeq.length();
  std::stringstream header; header << std::string(message.begin(), message.begin() + xmlEndPos);
  std::string body(message.begin() + xmlEndPos + 2, message.end());



  // Read Xml
  boost::property_tree::ptree ptree;
  read_xml(header, ptree);

  std::string filename = ptree.get<std::string>("transmission.filename");

  std::cout << "Receive from " 
	    << socket.remote_endpoint().address() 
	    << ":" 
	    << socket.remote_endpoint().port() 
	    << " " 
	    << filename << std::endl;

  return std::make_tuple(body, filename);

}
