CXX      = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude \
           $(shell pkg-config --cflags lua5.4 2>/dev/null || \
                   pkg-config --cflags lua5.3 2>/dev/null || \
                   pkg-config --cflags lua    2>/dev/null || \
                   echo "-I/usr/include/lua5.4")
LDFLAGS  = -lGL -lGLU -lglut \
           $(shell pkg-config --libs lua5.4 2>/dev/null || \
                   pkg-config --libs lua5.3 2>/dev/null || \
                   pkg-config --libs lua    2>/dev/null || \
                   echo "-llua5.4")

SRC_DIR  = src
OBJ_DIR  = obj
BIN      = cubo
BIN_BENCH = cubo_bench

# ── fontes ───────────────────────────────────────────────────────────────────
SRCS_COMMON = $(wildcard $(SRC_DIR)/*.cpp)
SRCS_BENCH  = $(SRCS_COMMON) $(SRC_DIR)/bench.cpp
SRCS        = $(SRCS_COMMON)
OBJS        = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

BENCH_OBJ_DIR = obj_bench
BENCH_OBJS    = $(patsubst $(SRC_DIR)/%.cpp, $(BENCH_OBJ_DIR)/%.o, $(SRCS_COMMON)) $(BENCH_OBJ_DIR)/bench.o

# ── build normal ──────────────────────────────────────────────────────────────
all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ── build bench ───────────────────────────────────────────────────────────────
#
#   compila com -DBENCH_MODE=1, produzindo o binário cubo_bench.
#   abre automaticamente uma segunda janela "Bench Monitor" ao ser iniciado,
#   exibindo em tempo real:
#     • fps e tempo de frame (média deslizante de 90 frames)
#     • tempo de render c++ vs tempo de runtime lua (µs, barras coloridas)
#     • memória rss do processo (kb, via /proc/self/status)
#     • cache de texturas: nº de faces com foto e estimativa de ram de gpu
#
bench: $(BIN_BENCH)

$(BIN_BENCH): $(BENCH_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BENCH_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BENCH_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -DBENCH_MODE=1 -c -o $@ $<

$(BENCH_OBJ_DIR)/bench.o: $(SRC_DIR)/bench.cpp
	@mkdir -p $(BENCH_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -DBENCH_MODE=1 -c -o $@ $<

# ── executar ──────────────────────────────────────────────────────────────────
#
#   'make run'  → compila e executa o binário normal ($(BIN)).
#               (não usa o modo bench; para bench use `make bench run`)
#
run: all
	@./$(BIN)

# atalho para rodar apenas o binário normal
start: all
	@./$(BIN)

# ── limpeza ───────────────────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(BENCH_OBJ_DIR) $(BIN) $(BIN_BENCH)

# ── ajuda ─────────────────────────────────────────────────────────────────────
help:
	@echo ""
	@echo "  targets disponíveis:"
	@echo "    make            compila binário normal ($(BIN))"
	@echo "    make bench      compila binário com monitor de performance ($(BIN_BENCH))"
	@echo "    make bench run  compila e executa o modo bench"
	@echo "    make run        compila e executa o binário normal ($(BIN))"
	@echo "    make start      compila e executa o binário normal"
	@echo "    make clean      remove binários e objetos"
	@echo ""

.PHONY: all bench run start clean help
