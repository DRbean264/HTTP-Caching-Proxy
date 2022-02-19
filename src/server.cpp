#include "ProxyServer/ProxyServer.hpp"
#include <iostream>
#include <cstdlib>

using namespace std;

#define LOG_FILE "/var/log/erss/proxy.log"
#define ERR_FILE "/var/log/erss/err.log"

int main(int argc, char const *argv[]) {
    pid_t pid = getpid();
    // cout << "Current PID: " << pid << endl;
    
    // the initial process should not be of pid 1, namely, init
    // if the pid is 1, then the daemon will be killed when this proces exits
    if (pid == 1) {
        pid = fork();
        // parent process should never exit
        if (pid != 0) {
            while (1) {}
        } else if (pid == -1) {
            cerr << "Fork system call failure" << endl;
            exit(EXIT_FAILURE);
        }
    }

    // chile process continues
    struct addrinfo host_info;
    const char *hostname = NULL;
    const char *port = "12345";

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;   // IPv4/IPv6
    host_info.ai_socktype = SOCK_STREAM; // TCP
    host_info.ai_flags    = AI_PASSIVE;
    try {
        ProxyServer myServer(host_info, LOG_FILE, ERR_FILE,
                        hostname, port, 100);
        cout << "Launching the server...\n";
        myServer.launch();
    }
    catch(const std::exception& e) {
        cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    
    return 0;
}
