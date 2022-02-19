CPPFLAGS = -Wall -Werror -pedantic -std=c++17 -ggdb3

TARGETS=BaseServer.o BaseSocket.o ProxyServer.o

all: $(TARGETS)

BaseServer.o: BaseServer.cpp BaseServer.hpp 
	g++ $(CPPFLAGS) -c BaseServer.cpp

BaseSocket.o: BaseSocket.cpp BaseSocket.hpp
	g++ $(CPPFLAGS) -c BaseSocket.cpp

ProxyServer.o: ProxyServer.cpp ProxyServer.hpp ../HTTPModule/HTTPRequest.hpp ../HTTPModule/HTTPResponse.hpp ../myUtils.hpp ../MyLog/MyLog.hpp
	g++ $(CPPFLAGS) -c ProxyServer.cpp

.PHONY clean:
	rm -f *~ *.o