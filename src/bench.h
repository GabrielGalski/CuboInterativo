/*
 * bench.h
 *
 * Monitor de performance para o Cube Editor em modo BENCH_MODE.
 * Mede FPS, tempo de frame, tempo isolado de Lua vs C++, uso de memória
 * RSS via /proc/self/status (Linux) e estatísticas de cache de texturas.
 *
 * Uso típico em display():
 *   gBench.frameBegin();
 *     gBench.cppRenderBegin(); background.render(); cube.render(); gBench.cppRenderEnd();
 *     gBench.luaBegin();       bridge.getStarPositions(...);       gBench.luaEnd();
 *   gBench.frameEnd();
 *   // na janela de bench: gBench.draw(winW, winH);
 */

#ifndef BENCH_H
#define BENCH_H

#ifdef BENCH_MODE

#include <chrono>
#include <deque>
#include <string>

/* ── Tipos auxiliares ─────────────────────────────────────────────────────── */
using BenchClock = std::chrono::high_resolution_clock;
using BenchTP    = BenchClock::time_point;

/* ── Snapshot de um frame (usado para médias deslizantes) ─────────────────── */
struct FrameSnap {
    float frameMs;  // duração total do frame em ms
    float luaUs;    // tempo gasto em chamadas Lua (µs)
    float cppUs;    // tempo gasto em render C++ (µs)
};

/* ── Monitor principal ────────────────────────────────────────────────────── */
class BenchMonitor {
public:
    static constexpr int HISTORY = 90;   // frames guardados para média

    /* Ciclo de frame — chame no início/fim de display() */
    void frameBegin();
    void frameEnd();

    /* Seção Lua — envolva cada chamada bridge que execute código Lua */
    void luaBegin();
    void luaEnd();

    /* Seção C++ render — envolva background.render() + cube.render() */
    void cppRenderBegin();
    void cppRenderEnd();

    /* Texturas — atualize uma vez por frame com dados do Cubo */
    void setTexInfo(int activeCount, long estimatedKb);

    /* Leituras instantâneas (médias dos últimos HISTORY frames) */
    float getFPS()       const { return fps; }
    float getFrameMs()   const { return avgFrameMs; }
    float getLuaUs()     const { return avgLuaUs; }
    float getCppUs()     const { return avgCppUs; }
    int   getTexCount()  const { return texCount; }
    long  getTexKb()     const { return texKb; }

    /* Memória RSS do processo em kB (lê /proc/self/status no Linux) */
    static long getRssKb();

    /* Renderiza o painel de stats em coordenadas de janela [0..winW x 0..winH] */
    void draw(int winW, int winH) const;

private:
    /* Temporização */
    BenchTP frameStart;
    BenchTP luaStart;
    BenchTP cppStart;
    double  luaAccumUs  = 0.0;
    double  cppAccumUs  = 0.0;

    /* FPS */
    int     frameCount  = 0;
    double  fpsAccumMs  = 0.0;
    BenchTP fpsTimer;
    bool    timerInit   = false;
    float   fps         = 0.0f;

    /* Médias deslizantes */
    std::deque<FrameSnap> history;
    float avgFrameMs = 0.0f;
    float avgLuaUs   = 0.0f;
    float avgCppUs   = 0.0f;

    void pushSnap(float fms, float lus, float cus);

    /* Texturas */
    int  texCount = 0;
    long texKb    = 0;
};

/* Instância global acessível por main.cpp, background.cpp, etc. */
extern BenchMonitor gBench;

#endif /* BENCH_MODE */
#endif /* BENCH_H */
