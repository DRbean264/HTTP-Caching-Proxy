CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3

myserver: server.o myUtils.o ProxyServer/BaseServer.o ProxyServer/ProxyServer.o ProxyServer/BaseSocket.o HTTPModule/HTTPBase.o HTTPModule/HTTPRequest.o HTTPModule/HTTPResponse.o MyLog/MyLog.o CacheController.o Cache/cache_synchronized.h
	g++ $(CPPFLAGS) -o myserver server.o myUtils.o \
	ProxyServer/BaseServer.o ProxyServer/ProxyServer.o ProxyServer/BaseSocket.o \
	HTTPModule/HTTPBase.o HTTPModule/HTTPRequest.o HTTPModule/HTTPResponse.o \
	MyLog/MyLog.o CacheController.o

server.o: server.cpp ProxyServer/ProxyServer.hpp
	g++ $(CPPFLAGS) -c server.cpp

myUtils.o: myUtils.cpp myUtils.hpp
	g++ $(CPPFLAGS) -c myUtils.cpp

CacheController.o: CacheController.cpp CacheController.hpp
	g++ $(CPPFLAGS) -c CacheController.cpp

.PHONY clean:
	rm -f *~ *.o HTTPModule/*.o MyLog/*.o ProxyServer/*.o test
