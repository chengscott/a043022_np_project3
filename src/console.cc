#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
using std::cout;
using std::endl;
using std::string;

struct server_info_t {
    string host;
    short port;
    string file;
} server_info[5];

void init_console(const std::vector<server_info_t>& sinfo) {
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
    for (const auto& s : sinfo)
        table += R"(<th scope="col">)" + s.host + ':' + std::to_string(s.port) +
                 "</th>";
    table += "</tr></thead><tbody><tr>";
    for (size_t i = 0; i < sinfo.size(); ++i)
        table += R"(<td><pre id="s)" + std::to_string(i) +
                 R"(" class="mb-0"></pre></td>)";
    table += "</tr></tbody>";
    cout << html_front << table << html_end << endl;
}

int main(int argc, char** argv) {
    // parse QUERY_STRING
    const string query = string(getenv("QUERY_STRING"));
    std::regex pattern("&?([^=]+)=([^&]+)");
    auto it = std::sregex_iterator(query.begin(), query.end(), pattern);
    auto it_end = std::sregex_iterator();
    for (; it != it_end; ++it) {
        string key = (*it)[1].str(), value = (*it)[2].str();
        int idx = key[1] - '0';
        if (key[0] == 'h')
            server_info[idx].host = value + ".cs.nctu.edu.tw";
        else if (key[0] == 'p')
            server_info[idx].port = std::stoi(value);
        else if (key[0] == 'f')
            server_info[idx].file = value;
    }
    std::vector<server_info_t> sinfo;
    for (const auto& s : server_info)
        if (!s.host.empty()) sinfo.push_back(s);
    // initialize
    init_console(sinfo);
    return 0;
}
