CPPFLAGS = -Wall -Werror -pedantic -std=c++17 -ggdb3

TARGETS=HTTPBase.o HTTPRequest.o HTTPResponse.o

all: $(TARGETS)

HTTPBase.o: HTTPBase.cpp HTTPBase.hpp ../myUtils.hpp
	g++ $(CPPFLAGS) -c HTTPBase.cpp

HTTPRequest.o: HTTPRequest.cpp HTTPRequest.hpp
	g++ $(CPPFLAGS) -c HTTPRequest.cpp

HTTPResponse.o: HTTPResponse.cpp HTTPResponse.hpp
	g++ $(CPPFLAGS) -c HTTPResponse.cpp

.PHONY clean:
	rm -f *~ *.o