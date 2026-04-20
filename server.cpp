#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// 🔹 Base Class (Inheritance + Polymorphism)
class NetworkDevice {
public:
    virtual void showType() {
        std::cout << "Generic Network Device\n";
    }

    virtual ~NetworkDevice() {}
};

// 🔹 Logger Class
class Logger {
public:
    explicit Logger(const std::string& file_path) : file_path_(file_path) {}

    void logLine(const std::string& line) {
        std::lock_guard<std::mutex> lock(mu_);
        std::ofstream out(file_path_, std::ios::app);
        if (!out) return;
        out << line << "\n";
    }

private:
    std::string file_path_;
    std::mutex mu_;
};

// 🔹 Time Helper
static std::string nowText() {
    std::time_t t = std::time(nullptr);
    char buf[64];
    std::tm* tm_ptr = std::localtime(&t);
    if (!tm_ptr) return "UNKNOWN_TIME";
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_ptr);
    return std::string(buf);
}

// 🔹 Data Parser
class DataParser {
public:
    static int getValue(const std::string& data, const std::string& key) {
        std::istringstream ss(data);
        std::string token;

        while (std::getline(ss, token, ';')) {
            auto pos = token.find(':');
            if (pos == std::string::npos) continue;

            std::string k = token.substr(0, pos);
            std::string v = token.substr(pos + 1);

            if (k == key) {
                try {
                    return std::stoi(v);
                } catch (...) {
                    return -1;
                }
            }
        }
        return -1;
    }
};

// 🔹 Main Server Class
class MonitorServer : public NetworkDevice {
public:
    // Constructor
    MonitorServer(int port, std::string auth_key, std::string log_file)
        : port_(port), auth_key_(std::move(auth_key)), logger_(std::move(log_file)) {}

    // Destructor
    ~MonitorServer() {
        if (server_fd_ != -1) {
            close(server_fd_);
            std::cout << "Server closed\n";
        }
    }

    // Polymorphism
    void showType() override {
        std::cout << "Monitor Server Device\n";
    }

    // Operator Overloading
    bool operator==(const MonitorServer& other) {
        return port_ == other.port_;
    }

    bool start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "socket() failed\n";
            return false;
        }

        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(static_cast<uint16_t>(port_));

        if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "bind() failed\n";
            close(server_fd_);
            return false;
        }

        if (listen(server_fd_, 10) < 0) {
            std::cerr << "listen() failed\n";
            close(server_fd_);
            return false;
        }

        std::cout << "Server started on port " << port_ << "\n";
        std::cout << "Waiting for clients...\n\n";

        while (true) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);

            int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &len);
            if (client_fd < 0) {
                std::cerr << "accept() failed\n";
                continue;
            }

            std::thread(&MonitorServer::handleClient, this, client_fd, client_addr).detach();
        }
        return true;
    }

private:
    std::vector<std::string> client_list_; // STL usage

    void handleClient(int client_fd, sockaddr_in client_addr) {
        char ipbuf[INET_ADDRSTRLEN] = {0};
        const char* ip_ptr = inet_ntop(AF_INET, &client_addr.sin_addr, ipbuf, sizeof(ipbuf));
        std::string client_ip = ip_ptr ? ip_ptr : "UNKNOWN_IP";
        int client_port = ntohs(client_addr.sin_port);

        client_list_.push_back(client_ip); // STL usage

        // Authentication
        std::string auth_msg = recvLine(client_fd);
        if (auth_msg != ("AUTH:" + auth_key_)) {
            std::string msg = "Rejected client " + client_ip;
            std::cout << msg << "\n";
            logger_.logLine(msg);
            close(client_fd);
            return;
        }

        send(client_fd, "OK\n", 3, 0);

        std::cout << "Client connected: " << client_ip << "\n";

        while (true) {
            std::string data = recvLine(client_fd);
            if (data.empty()) break;

            std::cout << "Data: " << data << "\n";
            logger_.logLine(data);

            generateAlerts(data, client_ip, client_port);
        }

        close(client_fd);
        std::cout << "Client disconnected\n\n";
    }

    void generateAlerts(const std::string& data, const std::string& ip, int port) {
        try {
            int cpu = DataParser::getValue(data, "CPU");
            int fails = DataParser::getValue(data, "LOGIN_FAIL");

            if (cpu > 80) {
                std::cout << "ALERT: High CPU (" << cpu << ")\n";
            }

            if (fails > 5) {
                std::cout << "ALERT: Brute Force (" << fails << ")\n";
            }
        } catch (...) {
            std::cout << "Error processing data\n";
        }
    }

    static std::string recvLine(int fd) {
        std::string out;
        char ch;

        while (true) {
            ssize_t n = recv(fd, &ch, 1, 0);
            if (n <= 0) return "";
            if (ch == '\n') break;
            out.push_back(ch);
        }
        return out;
    }

    int port_;
    std::string auth_key_;
    Logger logger_;
    int server_fd_ = -1;
};

// 🔹 Main Function
int main() {
    const int PORT = 8080;
    const std::string AUTH_KEY = "1234";
    const std::string LOG_FILE = "logs.txt";

    NetworkDevice* device = new MonitorServer(PORT, AUTH_KEY, LOG_FILE);
    device->showType();

    MonitorServer* server = dynamic_cast<MonitorServer*>(device);
    if (server) {
        server->start();
    }

    delete device;
    return 0;
}