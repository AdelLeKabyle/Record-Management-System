# Record Management System — build with: make
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
SRC       = src/main.cpp src/Book.cpp src/RecordManager.cpp src/ConsoleUI.cpp
BIN       = rms

ifeq ($(OS),Windows_NT)
BIN       = rms.exe
CXXFLAGS += -static
endif

all: $(BIN)

$(BIN): $(SRC) src/Book.h src/RecordManager.h src/ConsoleUI.h
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f rms rms.exe

.PHONY: all run clean
