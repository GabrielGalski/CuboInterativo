/*
 * bench.h
 *
 * Monitor de performance mede FPS, tempo de frame,
 * tempo isolado de Lua vs C++, RSS de memória e uso do cache de texturas.
 * Os timers usam std::chrono::high_resolution_clock e as médias são calculadas
 * sobre uma janela dos últimos history frames.
 */

#ifndef BENCH_H
#define BENCH_H

#ifdef BENCH_MODE

#include <chrono>
#include <deque>
#include <string>

using BenchClock = std::chrono::high_resolution_clock;
using BenchTP    = BenchClock::time_point;

struct FrameSnap {
    float frameMs;
    float luaUs;
    float cppUs;
};

class BenchMonitor {
public:
    static constexpr int HISTORY = 90;

    void frameBegin();
    void frameEnd();

    void luaBegin();
    void luaEnd();

    void cppRenderBegin();
    void cppRenderEnd();

    void setTexInfo(int activeCount, long estimatedKb);

    float getFPS()       const { return fps; }
    float getFrameMs()   const { return avgFrameMs; }
    float getLuaUs()     const { return avgLuaUs; }
    float getCppUs()     const { return avgCppUs; }
    int   getTexCount()  const { return texCount; }
    long  getTexKb()     const { return texKb; }

    static long getRssKb();

    void draw(int winW, int winH) const;

private:
    BenchTP frameStart;
    BenchTP luaStart;
    BenchTP cppStart;
    double  luaAccumUs  = 0.0;
    double  cppAccumUs  = 0.0;

    int     frameCount  = 0;
    double  fpsAccumMs  = 0.0;
    BenchTP fpsTimer;
    bool    timerInit   = false;
    float   fps         = 0.0f;

    std::deque<FrameSnap> history;
    float avgFrameMs = 0.0f;
    float avgLuaUs   = 0.0f;
    float avgCppUs   = 0.0f;

    void pushSnap(float fms, float lus, float cus);

    int  texCount = 0;
    long texKb    = 0;
};

extern BenchMonitor gBench;

#endif /* BENCH_MODE */
#endif /* BENCH_H */
