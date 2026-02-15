CXX = g++
CXXFLAGS = -Wall $(shell pkg-config --cflags lua5.4 || pkg-config --cflags lua5.3 || pkg-config --cflags lua || echo "-I/usr/include/lua5.4")
LDFLAGS = -lGL -lGLU -lglut $(shell pkg-config --libs lua5.4 || pkg-config --libs lua5.3 || pkg-config --libs lua || echo "-llua5.4")

SRC_DIR = src
OBJ_DIR = obj
BIN = cube_editor

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
