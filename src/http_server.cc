#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
using namespace boost::asio::ip;

class Server {
   public:
    Server(boost::asio::io_service& ioservice, unsigned short port)
        : ioservice_(ioservice),
          sigset_(ioservice, SIGCHLD),
          acceptor_(ioservice, {tcp::v4(), port}, true),
          socket_(ioservice) {
        wait_for_signal();
        accept();
    }

   private:
    void wait_for_signal() {
        sigset_.async_wait(
            [this](boost::system::error_code /*ec*/, int /*signo*/) {
                if (acceptor_.is_open()) {
                    int status = 0;
                    while (waitpid(-1, &status, WNOHANG) > 0)
                        ;
                    wait_for_signal();
                }
            });
    }

    void accept() {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec) {
                ioservice_.notify_fork(boost::asio::io_service::fork_prepare);
                if (fork() == 0) {
                    ioservice_.notify_fork(boost::asio::io_service::fork_child);
                    acceptor_.close();
                    sigset_.cancel();
                    parse_request();
                } else {
                    ioservice_.notify_fork(
                        boost::asio::io_service::fork_parent);
                    socket_.close();
                    accept();
                }
            } else
                accept();
        });
    }

    void parse_request() {
        boost::asio::async_read_until(
            socket_, buffer_, "\r\n",
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string req((std::istreambuf_iterator<char>(&buffer_)),
                                    std::istreambuf_iterator<char>());
                    std::cout << req << std::endl;
                    std::string line, method, url, protocol;
                    std::stringstream ss(req);
                    ss >> method >> url >> protocol;
                    assert(method == "GET");
                    size_t pos = url.find("?");
                    std::string file = url.substr(1, pos - 1),
                                query = url.substr(pos + 1, url.length() - pos),
                                host = "";
                    while (ss >> line) {
                        if (line == "Host:") {
                            ss >> line;
                            if ((pos = line.find(":")) != std::string::npos)
                                host = line.substr(0, pos);
                            else
                                host = pos;
                        } else
                            getline(ss, line);
                    }
                    tcp::endpoint local_ep = socket_.local_endpoint(),
                                  remote_ep = socket_.remote_endpoint();
                    setenv("REQUEST_METHOD", method.c_str(), 1);
                    setenv("REQUEST_URI", url.c_str(), 1);
                    setenv("QUERY_STRING", query.c_str(), 1);
                    setenv("SERVER_PROTOCOL", protocol.c_str(), 1);
                    setenv("HTTP_HOST", host.c_str(), 1);
                    setenv("SERVER_ADDR",
                           local_ep.address().to_string().c_str(), 1);
                    setenv("SERVER_PORT",
                           std::to_string(local_ep.port()).c_str(), 1);
                    setenv("REMOTE_ADDR",
                           remote_ep.address().to_string().c_str(), 1);
                    setenv("REMOTE_PORT",
                           std::to_string(remote_ep.port()).c_str(), 1);
                    int fd = socket_.native_handle();
                    dup2(fd, 0);
                    dup2(fd, 1);
                    dup2(fd, 2);
                    std::cout << protocol << " 200 OK\r\n";
                    const char* args[] = {file.c_str(), nullptr};
                    execv(file.c_str(), (char* const*)args);
                }
            });
    }

    boost::asio::io_service& ioservice_;
    boost::asio::signal_set sigset_;
    boost::asio::streambuf buffer_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};
int main(int argc, char** argv) {
    unsigned short port = std::atoi(argv[1]);
    boost::asio::io_service ioservice;
    Server server(ioservice, port);
    ioservice.run();
    return 0;
}
