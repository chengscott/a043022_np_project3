CXX=clang++
CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system -lboost_regex

all: console.cgi http_server

console.cgi: console.cc
	$(CXX) $? -o $@ -I/usr/local/include -L/usr/local/lib $(CXXFLAGS)

http_server: http_server.cc
	$(CXX) $? -o $@ -I/usr/local/include -L/usr/local/lib $(CXXFLAGS)

.PHONY: clean
clean:
	rm -rf console.cgi http_server

.PHONY: format
format:
	clang-format -i *.cc

.PHONY: check
check:
	clang-tidy -checks='bugprone-*,clang-analyzer-*,modernize-*' *.cc -- -std=c++11
