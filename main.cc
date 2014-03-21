#include <FileExchangeSocket.h>

#include <string>
#include <iostream>
#include <exception>
#include <boost/asio.hpp>


using boost::asio::ip::tcp;

int main(int argc, char* argv[]){
  if(argc != 5){
    std::cerr << "Usage: ./mirror TYPE HOST OTHERPORT OWNPORT" << std::endl;
    return 0;
  }

  const unsigned type = atoi(argv[1]);
  const std::string otherHost = argv[2];
  const std::string otherPort = argv[3];
  const unsigned    ownPort   = atoi(argv[4]);
  
  try {
  
    // Recv
    if(type == 0){
      // Create socket
      boost::asio::io_service io_service;
      tcp::endpoint endpoint(tcp::v4(), ownPort);
      tcp::acceptor acceptor(io_service, endpoint); 

      // Accept connections to socket
      tcp::socket socket(io_service); 
      acceptor.accept(socket); 

      std::cout << recvString(socket) << std::endl;

    }
    // Send
    else if(type == 1){
      // Connect to socket
      boost::asio::io_service io_service; 
      tcp::resolver resolver(io_service); 
      tcp::resolver::query url(otherHost, otherPort);
      tcp::resolver::iterator endpoint_iterator = resolver.resolve(url);
      tcp::socket socket(io_service);
      boost::asio::connect(socket, endpoint_iterator);

      std::stringstream ss;
      ss << "<transmission>" << std::endl;
      for(unsigned i = 0; i < 2000; ++i){
	ss << "</nosense id=" << i << ">"<< std::endl;
      }
      ss << "</transmission>";
      
      std::string string = "Hello World";
      std::cout << "Sending string" << std::endl;
      sendString(socket, ss.str());
    
    }
  
  }
  catch(std::exception e){
    e.what();
  }

  return 0;
}
