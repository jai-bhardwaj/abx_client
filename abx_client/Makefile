CXX = g++
CXXFLAGS = -std=c++17 -Wall

TARGET = abx_client
SOURCES = main.cpp network_utils.cpp
HEADERS = network_utils.h json.hpp

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
