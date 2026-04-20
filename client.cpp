#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

// 🔹 Base class (Inheritance + Polymorphism)
class Device {
public:
    virtual void info() {
        std::cout << "Generic Device\n";
    }

    virtual ~Device() {}
};

class MonitorClient : public Device {
public:
    // 🔹 Constructor
    MonitorClient(std::string server_ip, int server_port, std::string auth_key)
        : server_ip_(std::move(server_ip)),
          server_port_(server_port),
          auth_key_(std::move(auth_key)) {}

    // 🔹 Destructor
    ~MonitorClient() {
        std::cout << "Client object destroyed\n";
    }

    // 🔹 Polymorphism
    void info() override {
        std::cout << "Monitor Client Device\n";
    }

    // 🔹 Operator Overloading
    bool operator==(const MonitorClient& other) {
        return server_ip_ == other.server_ip_;
    }

    bool run() {
        int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "socket() failed\n";
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<uint16_t>(server_port_));

        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) != 1) {
            std::cerr << "Invalid IP address\n";
            ::close(sockfd);
            return false;
        }

        if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            std::cerr << "connect() failed\n";
            ::close(sockfd);
            return false;
        }

        // 🔹 Authentication
        sendLine(sockfd, "AUTH:" + auth_key_);

        std::string resp = recvLine(sockfd);
        if (resp != "OK") {
            std::cout << "Authentication failed\n";
            ::close(sockfd);
            return true;
        }

        std::cout << "Authenticated. Sending data...\n";

        // 🔹 Send monitoring data
        for (int i = 0; i < 8; i++) {
            int cpu = randomInt(30, 95);
            int login_fail = randomInt(0, 10);

            std::string data = "CPU:" + std::to_string(cpu) +
                               ";LOGIN_FAIL:" + std::to_string(login_fail);

            sendLine(sockfd, data);
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
        }

        std::cout << "Done.\n";
        ::close(sockfd);
        return true;
    }

private:
    static void sendLine(int fd, const std::string& msg) {
        std::string line = msg + "\n";
        ::send(fd, line.c_str(), line.size(), 0);
    }

    static std::string recvLine(int fd) {
        std::string out;
        char ch;
        while (true) {
            ssize_t n = ::recv(fd, &ch, 1, 0);
            if (n <= 0) return "";
            if (ch == '\n') break;
            out.push_back(ch);
            if (out.size() > 1024) break;
        }
        return out;
    }

    // 🔹 Exception Handling
    static int randomInt(int low, int high) {
        try {
            int r = std::rand();
            return low + (r % (high - low + 1));
        } catch (...) {
            return low;
        }
    }

    std::string server_ip_;
    int server_port_;
    std::string auth_key_;
};

int main(int argc, char** argv) {
    std::string ip = "127.0.0.1";
    int port = 8080;
    std::string key = "1234";

    if (argc >= 2) ip = argv[1];
    if (argc >= 3) port = std::atoi(argv[2]);
    if (argc >= 4) key = argv[3];

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 🔹 Polymorphism using pointer
    Device* device = new MonitorClient(ip, port, key);
    device->info();

    MonitorClient* client = dynamic_cast<MonitorClient*>(device);
    if (client) {
        client->run();
    }

    delete device;
    return 0;
}