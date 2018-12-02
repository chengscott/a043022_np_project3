#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
using std::cout;
using std::endl;
using std::string;

boost::asio::io_service ioservice;

class Server {
   public:
    string host, port, file, session;

    Server() : resolv_(ioservice), socket_(ioservice) {}

    void connect() {
        if (file.empty()) return;
        file_.open("test_case/" + file);
        boost::asio::ip::tcp::resolver::query q{host, port};
        resolv_.async_resolve(
            q, [this](const boost::system::error_code& ec,
                      boost::asio::ip::tcp::resolver::iterator it) {
                if (!ec) {
                    socket_.async_connect(
                        *it, [this](const boost::system::error_code& ec) {
                            if (!ec) {
                                receive();
                            }
                        });
                }
            });
    }

    void close() {
        file_.close();
        socket_.close();
    }

   private:
    void receive() {
        boost::asio::async_read_until(
            socket_, buffer_, boost::regex("^% "),
            [this](boost::system::error_code ec, size_t length) {
                std::string res((std::istreambuf_iterator<char>(&buffer_)),
                                std::istreambuf_iterator<char>());
                output_shell(res);
                if (!ec)
                    send();
                else
                    close();
            });
    }

    void send() {
        std::string line;
        if (std::getline(file_, line)) {
            line += '\n';
            output_command(line);
            boost::asio::async_write(
                socket_, boost::asio::buffer(line),
                [this](boost::system::error_code ec, std::size_t length) {
                    if (!ec)
                        receive();
                    else
                        close();
                });
        } else
            close();
    }

    static void html_escape(string& rhs) {
        string res;
        for (char c : rhs) {
            switch (c) {
                case '&':
                    res.append("&amp;");
                    break;
                case '\'':
                    res.append("&quot;");
                    break;
                case '\"':
                    res.append("&apos;");
                    break;
                case '<':
                    res.append("&lt;");
                    break;
                case '>':
                    res.append("&gt;");
                    break;
                case '\r':
                    break;
                case '\n':
                    res.append("&NewLine;");
                    break;
                default:
                    res += c;
                    break;
            }
        }
        res.swap(rhs);
    }

    void output_shell(string rhs) {
        html_escape(rhs);
        cout << "<script>document.getElementById('" << session
             << "').innerHTML += '" << rhs << "';</script>" << endl;
    }

    void output_command(string rhs) {
        html_escape(rhs);
        cout << "<script>document.getElementById('" << session
             << "').innerHTML += '<b>" << rhs << "</b>';</script>" << endl;
    }

    std::ifstream file_;
    boost::asio::ip::tcp::resolver resolv_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf buffer_;
};

void parse_query(Server (&server)[5]) {
    const string query = string(getenv("QUERY_STRING"));
    std::regex pattern("&?([^=]+)=([^&]+)");
    auto it = std::sregex_iterator(query.begin(), query.end(), pattern);
    auto it_end = std::sregex_iterator();
    for (; it != it_end; ++it) {
        string key = (*it)[1].str(), value = (*it)[2].str();
        int idx = key[1] - '0';
        if (key[0] == 'h')
            server[idx].host = value;
        else if (key[0] == 'p')
            server[idx].port = value;
        else if (key[0] == 'f')
            server[idx].file = value;
    }
}

void init_console(const Server (&server)[5]) {
    cout << "Content-type: text/html" << endl << endl;
    const string html_front = R"~~~(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>NP Project 3 Console</title>
    <link
      rel="stylesheet"
      href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css"
      integrity="sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO"
      crossorigin="anonymous"
    />
    <link
      href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
      rel="stylesheet"
    />
    <link
      rel="icon"
      type="image/png"
      href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
        font-size: 1rem !important;
      }
      body {
        background-color: #212529;
      }
      pre {
        color: #cccccc;
      }
      b {
        color: #ffffff;
      }
    </style>
  </head>
  <body>
    <table class="table table-dark table-bordered">
    )~~~";
    const string html_end = R"~~~(
    </table>
  </body>
</html>
    )~~~";
    string table = "<thead><tr>";
    for (const Server& s : server)
        if (!s.host.empty())
            table += R"(<th scope="col">)" + s.host + ':' + s.port + "</th>";
    table += "</tr></thead><tbody><tr>";
    for (const Server& s : server)
        if (!s.host.empty())
            table += R"(<td><pre id=")" + s.session +
                     R"(" class="mb-0"></pre></td>)";
    table += "</tr></tbody>";
    cout << html_front << table << html_end << endl;
}

int main(int argc, char** argv) {
    Server server[5];
    for (size_t i = 0; i < 5; ++i) server[i].session = "s" + std::to_string(i);
    parse_query(server);
    init_console(server);
    for (Server& s : server) s.connect();
    ioservice.run();
    return 0;
}
