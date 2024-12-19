# Đường dẫn tới thư mục thư viện OpenSSL
OPENSSL_LIB_PATH = include/openssl/lib/VC/x64/MD
OPENSSL_INCLUDE_PATH = include/OpenSSL-Win64/include

# Compiler và flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -g -Iinclude -I$(OPENSSL_INCLUDE_PATH)
LDFLAGS = -L$(OPENSSL_LIB_PATH) -lssl -lcrypto -lws2_32 -lgdi32 -lgdiplus -mconsole

# Các nguồn và đối tượng
SOURCES = src/network_handle.cpp src/blacklist.cpp src/font.cpp src/main.cpp src/network_init.cpp src/ui.cpp src/whitelist.cpp
OBJECTS = $(patsubst src/%.cpp, build/%.o, $(SOURCES))
EXECUTABLE = bin/proxy_server.exe

# Mục tiêu mặc định
all: $(EXECUTABLE)

# Tạo tệp thực thi từ các đối tượng
$(EXECUTABLE): $(OBJECTS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Tạo các đối tượng từ các nguồn
build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Mục tiêu dọn dẹp
clean:
	@if exist build (rmdir /S /Q build)
	@if exist bin (rmdir /S /Q bin)
