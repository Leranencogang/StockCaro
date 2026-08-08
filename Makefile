# Stockaro Engine Makefile
CXX      ?= g++
CXXFLAGS  = -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic
TARGET    = Stockaro
SRCDIR    = src
SRC       = $(SRCDIR)/main.cpp

.PHONY: all clean bench

all: $(TARGET)

$(TARGET): $(SRC) $(wildcard $(SRCDIR)/*.hpp)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(SRC) -o $(TARGET)
	@echo "Build successful: ./$(TARGET)"

bench: $(TARGET)
	./$(TARGET) --bench

clean:
	rm -f $(TARGET) $(TARGET).exe
