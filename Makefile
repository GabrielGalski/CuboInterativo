CXX      = g++
CXXFLAGS = -Wall -std=c++17 \
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

# ── Fontes ───────────────────────────────────────────────────────────────────
SRCS      = $(wildcard $(SRC_DIR)/*.cpp)
OBJS      = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

BENCH_OBJ_DIR = obj_bench
BENCH_OBJS    = $(patsubst $(SRC_DIR)/%.cpp, $(BENCH_OBJ_DIR)/%.o, $(SRCS))

# ── Build normal ──────────────────────────────────────────────────────────────
all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ── Build bench ───────────────────────────────────────────────────────────────
#
#   Compila com -DBENCH_MODE=1, produzindo o binário cubo_bench.
#   Abre automaticamente uma segunda janela "Bench Monitor" ao ser iniciado,
#   exibindo em tempo real:
#     • FPS e tempo de frame (média deslizante de 90 frames)
#     • Tempo de render C++ vs tempo de runtime Lua (µs, barras coloridas)
#     • Memória RSS do processo (kB, via /proc/self/status)
#     • Cache de texturas: nº de faces com foto e estimativa de RAM de GPU
#
bench: $(BIN_BENCH)

$(BIN_BENCH): $(BENCH_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "  ╔══════════════════════════════════════════╗"
	@echo "  ║  cubo_bench compilado com -DBENCH_MODE   ║"
	@echo "  ║  Execute:  make run   ou   ./cubo_bench  ║"
	@echo "  ╚══════════════════════════════════════════╝"
	@echo ""

$(BENCH_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BENCH_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -DBENCH_MODE=1 -c -o $@ $<

# ── Executar ──────────────────────────────────────────────────────────────────
#
#   'make run'  → executa cubo_bench se existir, senão executa cubo normal.
#   Permite o fluxo  "make bench run"  em um único comando.
#
run:
	@if [ -f "./$(BIN_BENCH)" ]; then \
	    echo "» Iniciando $(BIN_BENCH) (modo bench)..."; \
	    ./$(BIN_BENCH); \
	elif [ -f "./$(BIN)" ]; then \
	    echo "» Iniciando $(BIN) (modo normal)..."; \
	    ./$(BIN); \
	else \
	    echo "Nenhum binário encontrado. Execute 'make' ou 'make bench' primeiro."; \
	    exit 1; \
	fi

# Atalho para rodar apenas o binário normal
start: all
	@echo "» Iniciando $(BIN) (modo normal)..."
	@./$(BIN)

# ── Limpeza ───────────────────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(BENCH_OBJ_DIR) $(BIN) $(BIN_BENCH)

# ── Ajuda ─────────────────────────────────────────────────────────────────────
help:
	@echo ""
	@echo "  Targets disponíveis:"
	@echo "    make            Compila binário normal ($(BIN))"
	@echo "    make bench      Compila binário com monitor de performance ($(BIN_BENCH))"
	@echo "    make bench run  Compila e executa o modo bench"
	@echo "    make run        Executa $(BIN_BENCH) se existir, senão $(BIN)"
	@echo "    make start      Compila e executa o binário normal"
	@echo "    make clean      Remove binários e objetos"
	@echo ""

.PHONY: all bench run start clean help
