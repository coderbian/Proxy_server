TARGET = bin/proxy_server.exe
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include  

CXX = g++
CXXFLAGS = -Wall -std=c++17 -g -I$(INCLUDE_DIR)  
LDFLAGS = -lws2_32 -lgdi32 -lgdiplus -mconsole 

SRCS = $(wildcard $(SRC_DIR)/*.cpp) 
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

$(TARGET): $(OBJS)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)
