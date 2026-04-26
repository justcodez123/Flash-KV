# Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -pthread

# Target Binary
TARGET = flash_kv

# Source Files
SRC = main.cpp

# Header Files (for dependency tracking)
HEADERS = engine/FlashKV.h engine/FlashThreadPool.h network/FlashServer.h

# Build Rules
all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
