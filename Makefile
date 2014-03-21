LIBS = -lboost_system -lboost_filesystem -lboost_thread -lboost_program_options -lpthread

all:
	clang++ main.cc FileExchangeSocket.cc  -I . -std=c++11 $(LIBS)
	
