#include <iostream>
#include <sys/socket.h>  // Core socket functions
#include <netinet/in.h>  // Internet address structures
#include <unistd.h>      // POSIX operating system API (for closing the socket)
#include <fcntl.h>       // File control options (crucial for epoll later)

int create_server_socket(int port) {
    // 1. Create the Socket (IPv4, TCP)
    // AF_INET = IPv4, SOCK_STREAM = TCP (reliable delivery)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket!\n";
        exit(EXIT_FAILURE);
    }

    // 2. The "Developer Sanity" Saver
    // This prevents the annoying "Address already in use" error if you crash 
    // your server and try to restart it immediately.
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // 3. Define the Address (Where does the door lead?)
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on any IP address on this machine
    address.sin_port = htons(port);       // Convert port number to network byte order

    // 4. Bind the Socket to Port 6379
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port << "!\n";
        exit(EXIT_FAILURE);
    }

    // 5. Start Listening!
    // SOMAXCONN tells the OS to queue up as many connections as the hardware allows
    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen!\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "FlashKV Server listening on port " << port << "...\n";
    return server_fd; // We return the File Descriptor (the ID number of the socket)
}