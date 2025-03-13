#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

constexpr size_t k_max_msg = 4096;

class Socket {
public:
    explicit Socket(int domain, int type, int protocol) {
        fd = socket(domain, type, protocol);
        if (fd < 0) {
            throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
        }
    }

    ~Socket() {
        if (fd >= 0) {
            close(fd);
        }
    }

    void connect(const struct sockaddr_in& addr) {
        if (::connect(fd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error("connect() failed: " + std::string(strerror(errno)));
        }
    }

    int32_t read_full(char* buf, size_t n) {
        while (n > 0) {
            ssize_t rv = read(fd, buf, n);
            if (rv <= 0) {
                return -1;  // Error or unexpected EOF
            }
            assert(static_cast<size_t>(rv) <= n);
            n -= static_cast<size_t>(rv);
            buf += rv;
        }
        return 0;
    }

    int32_t write_all(const char* buf, size_t n) {
        while (n > 0) {
            ssize_t rv = write(fd, buf, n);
            if (rv <= 0) {
                return -1;  // Error
            }
            assert(static_cast<size_t>(rv) <= n);
            n -= static_cast<size_t>(rv);
            buf += rv;
        }
        return 0;
    }

    int get_fd() const { return fd; }

private:
    int fd;
};

void log_message(const std::string& msg) {
    std::cerr << msg << std::endl;
}

int32_t query(Socket& socket, const std::string& text) {
    uint32_t len = static_cast<uint32_t>(text.size());
    if (len > k_max_msg) {
        log_message("Message too long");
        return -1;
    }

    std::vector<char> wbuf(4 + k_max_msg);
    memcpy(wbuf.data(), &len, 4);  // Assume little-endian
    memcpy(wbuf.data() + 4, text.data(), len);

    if (int32_t err = socket.write_all(wbuf.data(), 4 + len)) {
        return err;
    }

    // Read 4-byte header
    std::vector<char> rbuf(4 + k_max_msg + 1);
    int32_t err = socket.read_full(rbuf.data(), 4);
    if (err) {
        log_message(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, rbuf.data(), 4);  // Assume little-endian
    if (len > k_max_msg) {
        log_message("Response too long");
        return -1;
    }

    // Read response body
    err = socket.read_full(rbuf.data() + 4, len);
    if (err) {
        log_message("read() error");
        return err;
    }

    // Print server response
    std::cout << "Server says: " << std::string(rbuf.data() + 4, len) << std::endl;
    return 0;
}

int main() {
    try {
        Socket socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(6380);  // Fixed incorrect conversion
        addr.sin_addr.s_addr = INADDR_ANY;  // 127.0.0.1

        socket.connect(addr);
        std::string input = "" ; 
        while(true){
            std::cout<<">" ;
            std::getline(std::cin ,input) ; 


            if(input =="exit"){
                std::cout << "Closing connection.\n";
                break;
            }

            if(!input.empty()){
                if(query(socket , input)){
                    std::cerr << "Error occurred while communicating with server.\n";
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
