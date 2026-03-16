#include <iostream>
#include <sys/socket.h>  // Linux core socket functions
#include <netinet/in.h>  // Internet address structures
#include <unistd.h>      // POSIX API (for closing files)
#include <fcntl.h>       // File control (to make sockets non-blocking)
#include <sys/epoll.h>   // The epoll event loop!
#include <cstring>

class FlashServer {
private:
    int server_socket;
    int epoll_fd;
    int port;

    // Helper function to force a socket into Non-Blocking mode
    void make_non_blocking(int socket_fd) {
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    }

public:
    FlashServer(int listen_port) {
        port = listen_port;
        server_socket = -1;
        epoll_fd = -1;
    }

    // The Infinite Event Loop to Wake up the sockets, while epoll waiting for a connection
    //epoll_wait : which literally puts your thread to sleep until a network packet arrives.
    void run_event_loop() {
        // We can handle up to 64 network events at the exact same time
        const int MAX_EVENTS = 64;
        struct epoll_event events[MAX_EVENTS];

        std::cout << "Epoll Event Loop started. Waiting for connections...\n";

        while (true) {
            // Wait indefinitely (-1) until SOME network activity happens
            int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            
            if (num_events == -1) {
                std::cerr << "CRITICAL: epoll_wait failed!\n";
                break;
            }

            // Loop through all the events that just woke us up
            for (int i = 0; i < num_events; i++) {
                
                if (events[i].data.fd == server_socket) {
                    // SCENARIO 1: A NEW USER IS KNOCKING ON PORT 6379
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    
                    // Let them in! This creates a new socket specifically for this client.
                    int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd == -1) {
                        std::cerr << "Failed to accept client connection.\n";
                        continue;
                    }

                    // Make the client socket non-blocking too!
                    make_non_blocking(client_fd);

                    // Tell the epoll receptionist to start watching this new client
                    struct epoll_event client_event;
                    client_event.events = EPOLLIN; // Watch for incoming data
                    client_event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);

                    std::cout << "New client connected! Assigned File Descriptor: " << client_fd << "\n";
                } 
                else {
                    // SCENARIO 2: AN EXISTING CLIENT SENT US DATA
                    int client_fd = events[i].data.fd;
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer)); // Clear the buffer

                    // Read the data from the client
                    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

                    if (bytes_read <= 0) {
                        // If we read 0 bytes, it means the client closed their terminal
                        std::cout << "Client FD " << client_fd << " disconnected.\n";
                        close(client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL); // Stop watching them
                    } else {
                        // WE GOT DATA! 
                        std::cout << "Received from FD " << client_fd << ": " << buffer;
                        
                        // For right now, let's just echo "+OK\n" back to prove we can talk
                        write(client_fd, "+OK\n", 4);
                    }
                }
            }
        }
    }

    // Opens the door and sets up the epoll receptionist
    void start() {
        // 1. Create the main server socket (IPv4, TCP)
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            std::cerr << "CRITICAL: Failed to create socket.\n";
            exit(EXIT_FAILURE);
        }

        // 2. Prevent the "Address already in use" crash on restart
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

        // 3. Make the main door non-blocking!
        make_non_blocking(server_socket);

        // 4. Bind the socket to port 6379
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY; 
        address.sin_port = htons(port);       

        if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "CRITICAL: Failed to bind to port " << port << ".\n";
            exit(EXIT_FAILURE);
        }

        // 5. Start listening for incoming traffic
        if (listen(server_socket, SOMAXCONN) < 0) {
            std::cerr << "CRITICAL: Failed to listen.\n";
            exit(EXIT_FAILURE);
        }
        std::cout << "FlashKV Network Layer active. Listening on port " << port << "...\n";

        // 6. Create the epoll instance (The Receptionist)
        // The '0' is just a modern flag requirement, it ignores the size hint nowadays.
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            std::cerr << "CRITICAL: Failed to create epoll instance.\n";
            exit(EXIT_FAILURE);
        }

        // 7. Register the main server socket with epoll
        struct epoll_event event;
        event.events = EPOLLIN; // EPOLLIN means "Wake me up when incoming data arrives"
        event.data.fd = server_socket;
        
        // Hand the socket over to the receptionist
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event);
    }
};