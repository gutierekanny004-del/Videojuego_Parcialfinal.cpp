CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2 -Iinclude \
            -I/nix/store/0cc0zf49np55swj0j6q17wmyzxyg0a1y-ncurses-6.4-dev/include
LDFLAGS  := -L/nix/store/3vrc74aawany22dr1f03rmhsangzy0by-ncurses-6.4/lib -lncurses

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:.cpp=.o)
BIN := dungeon

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -f src/*.o $(BIN)
