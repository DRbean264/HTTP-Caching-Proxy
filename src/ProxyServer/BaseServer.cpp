#include "BaseServer.hpp"
#include <string>
#include <vector>
#include <sys/time.h>
#include <fcntl.h>
using namespace std;

#define CHUNK_SIZE 2048
#define MAX_LIMIT 65536

BaseServer::BaseServer(const char *_hostname, 
    const char *_port, int _backlog) 
    : sock(nullptr), host_info_list(nullptr),
    hostname(_hostname), port(_port), backlog(_backlog){
}

BaseServer::~BaseServer() {
    freeaddrinfo(host_info_list);
    delete sock;
}

// bind a socket to certain address
void BaseServer::bindSocket(int socket_fd) {
    int yes = 1;
    int status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = ::bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
 
    if (status == -1) {
        freeaddrinfo(host_info_list);
        throw SocketBindFailure();
    }
}

// set this program listening to this socket, allow backlog 
// connections in queue
void BaseServer::listenSocket(int socket_fd) {
    int status = listen(socket_fd, backlog);
    if (status == -1) {
        freeaddrinfo(host_info_list);
        throw SocketListenFailure();
    }
}

// std::string recvSocket_length(int s) {
// 	vector<char> result(MAX_LIMIT);

// 	// peek at the HTTP head
// 	int size_peek = recv(s , &result.data()[0] , MAX_LIMIT , MSG_PEEK);
// 	// get the content length if has one
// 	unsigned long contentLength = MAX_LIMIT;
// 	string tmp(result.begin(), result.begin() + size_peek);
// 	size_t pos1 = tmp.find("Content-Length");
// 	if (pos1 != string::npos) {
// 		size_t pos2 = tmp.find(':') + 2;
// 		contentLength = stoul()
// 	}
	

// 	int size_recv = recv(s , &result.data()[0] , contentLength , 0);
// 	// if client close the connection
// 	if (size_recv == 0) {
// 		cout << "The other side close connection\n";
// 	} else if (size_recv < 0) {
// 		cout << "Something went wrong\n";
// 		return string();
// 		//exit(EXIT_FAILURE);
// 	} 
// 	cout << size_recv << '\n';
	
// 	return string(result.begin(), result.begin() + size_recv);
// }

string BaseServer::recvSocket_block(int s) {
	vector<char> result(MAX_LIMIT);

	// peek at the HTTP head
	// int size_peek = recv(s , &result.data()[0] , MAX_LIMIT , MSG_PEEK);
	// get the content length if has one

	ssize_t size_recv = recv(s , &result.data()[0] , MAX_LIMIT , 0);
	// if client close the connection
	if (size_recv == 0) {
		cout << "The other side close connection\n";
		return string();
	} else if (size_recv < 0) {
		cout << "Something went wrong\n";
		return string();
		//exit(EXIT_FAILURE);
	} 
	//cout << size_recv << '\n';
	
	return string(result.begin(), result.begin() + size_recv);
}

string BaseServer::recvSocket_block_test(int s) {
	fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(s, &readSet);
    int FD_MAX = s + 1;
    int status;
	
	ssize_t size_recv;
	vector<char> result(MAX_LIMIT);
	while (1) {
		status = select(FD_MAX, &readSet, NULL, NULL, NULL);
		if (status == -1) {
			cerr << "Select function failed.\n";
			exit(EXIT_FAILURE);
		}
		if (status > 0) {
			if (FD_ISSET(s, &readSet)) {
				size_recv = recv(s , &result.data()[0] , MAX_LIMIT , 0);
				// the other side closed the connection
				if (size_recv == 0) {
					cout << "The other side close connection\n";
					return string();
				}
				else if (size_recv < 0) {
					cout << "Something went wrong\n";
					return string();
				}
				break;
			}
		}
	}
	cout << size_recv << '\n';
	
	return string(result.begin(), result.begin() + size_recv);
}

string BaseServer::recvSocket_timeout(int s, int timeout) {
	string result;
	ssize_t size_recv , total_size= 0;
	struct timeval begin , now;
	char chunk[CHUNK_SIZE];
	double timediff;
	
	//make socket non blocking
	fcntl(s, F_SETFL, O_NONBLOCK);
	
	//beginning time
	gettimeofday(&begin , NULL);
	
	while(1) {
		gettimeofday(&now , NULL);
		
		//time elapsed in seconds
		timediff = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);
		
		// //if you got some data, then break after timeout
		if( total_size > 0 && timediff > timeout ) {
			cout << "timeout" << endl;
			break;
		}
		//if you got no data at all, wait a little longer, twice the timeout
		else if( timediff > timeout*2) {
			cout << "timeout" << endl;
			break;
		}
		
		memset(chunk, 0, CHUNK_SIZE);	//clear the variable
		size_recv = recv(s , chunk , CHUNK_SIZE - 1 , 0);
		//if nothing was received then we want to wait a little before trying again, 0.1 seconds
		if (size_recv < 0) {
			usleep(100000);
		}
		// if client close the connection
		else if (size_recv == 0) {
			cout << "The other side close connection\n";
			break;
		}
		else {
			total_size += size_recv;
			// chunk[size_recv] = '\0';
            result += string(::begin(chunk), ::begin(chunk) + size_recv);
			//reset beginning time
			gettimeofday(&begin , NULL);
		}
	}
	// cout << "Finally:\n";
	// cout << "Received " << total_size << endl;
	// cout << result.size() << '\n';
	return result;
}

// getter functions
const char *BaseServer::getHostname() const {
    return hostname;
}
const char *BaseServer::getPort() const {
    return port;
}
int BaseServer::getBacklog() const {
    return backlog;
}
const struct addrinfo *BaseServer::getHostInfoList() const {
    return host_info_list;
}
const BaseSocket *BaseServer::getSocket() const {
    return sock;
}

// setter functions
void BaseServer::setBacklog(int b) {
    backlog = b;
}
void BaseServer::setHostInfoList(struct addrinfo *h) {
    host_info_list = h;
}
void BaseServer::setSocket(BaseSocket *s) {
    sock = s;
}