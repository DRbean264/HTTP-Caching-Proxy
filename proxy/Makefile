CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3

myserver: server.o myUtils.o ProxyServer/BaseServer.o ProxyServer/ProxyServer.o ProxyServer/BaseSocket.o HTTPModule/HTTPBase.o HTTPModule/HTTPRequest.o HTTPModule/HTTPResponse.o MyLog/MyLog.o CacheController.o cache_synchronized.o
	g++ $(CPPFLAGS) -o myserver server.o myUtils.o \
	ProxyServer/BaseServer.o ProxyServer/ProxyServer.o ProxyServer/BaseSocket.o \
	HTTPModule/HTTPBase.o HTTPModule/HTTPRequest.o HTTPModule/HTTPResponse.o \
	MyLog/MyLog.o CacheController.o cache_synchronized.o

server.o: server.cpp ProxyServer/ProxyServer.hpp
	g++ $(CPPFLAGS) -c server.cpp

myUtils.o: myUtils.cpp myUtils.hpp
	g++ $(CPPFLAGS) -c myUtils.cpp

CacheController.o: CacheController.cpp CacheController.hpp
	g++ $(CPPFLAGS) -c CacheController.cpp

cache_synchronized.o: cache_synchronized.cpp cache_synchronized.hpp
	g++ $(CPPFLAGS) -c cache_synchronized.cpp

.PHONY clean:
	rm -f *~ *.o myserver
