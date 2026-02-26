/*
 * bench.cpp
 *
 * Implementação do BenchMonitor. Medições com std::chrono::high_resolution_clock,
 * RSS lida de /proc/self/status no Linux, painel renderizado em OpenGL imediato.
 */

#ifdef BENCH_MODE

#include "bench.h"
#include <GL/glut.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <numeric>

BenchMonitor gBench;

static double toMs(BenchTP a, BenchTP b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
static double toUs(BenchTP a, BenchTP b) {
    return std::chrono::duration<double, std::micro>(b - a).count();
}

void BenchMonitor::frameBegin() {
    frameStart   = BenchClock::now();
    luaAccumUs  = 0.0;
    cppAccumUs  = 0.0;

    if (!timerInit) {
        fpsTimer  = frameStart;
        timerInit = true;
    }
}

void BenchMonitor::frameEnd() {
    auto now    = BenchClock::now();
    float fms   = (float)toMs(frameStart, now);
    float lus   = (float)luaAccumUs;
    float cus   = (float)cppAccumUs;

    pushSnap(fms, lus, cus);

    frameCount++;
    fpsAccumMs += fms;
    if (fpsAccumMs >= 500.0) {
        fps        = (float)(frameCount * 1000.0 / fpsAccumMs);
        frameCount = 0;
        fpsAccumMs = 0.0;
    }
}

void BenchMonitor::luaBegin()         { luaStart = BenchClock::now(); }
void BenchMonitor::luaEnd()           { luaAccumUs += toUs(luaStart, BenchClock::now()); }

void BenchMonitor::cppRenderBegin()   { cppStart = BenchClock::now(); }
void BenchMonitor::cppRenderEnd()     { cppAccumUs += toUs(cppStart, BenchClock::now()); }

void BenchMonitor::setTexInfo(int activeCount, long estimatedKb) {
    texCount = activeCount;
    texKb    = estimatedKb;
}

void BenchMonitor::pushSnap(float fms, float lus, float cus) {
    history.push_back({fms, lus, cus});
    if ((int)history.size() > HISTORY)
        history.pop_front();

    double sf = 0, sl = 0, sc = 0;
    for (auto& s : history) { sf += s.frameMs; sl += s.luaUs; sc += s.cppUs; }
    int n  = (int)history.size();
    avgFrameMs = (float)(sf / n);
    avgLuaUs   = (float)(sl / n);
    avgCppUs   = (float)(sc / n);
}

long BenchMonitor::getRssKb() {
#ifdef __linux__
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            long kb = 0;
            sscanf(line.c_str(), "VmRSS: %ld kB", &kb);
            return kb;
        }
    }
#endif
    return 0;
}

namespace {

void btext(float x, float y, const char* s) {
    glRasterPos2f(x, y);
    for (; *s; ++s)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (unsigned char)*s);
}

void quad(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1,y1); glVertex2f(x2,y1);
    glVertex2f(x2,y2); glVertex2f(x1,y2);
    glEnd();
}

void rect(float x1, float y1, float x2, float y2) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1,y1); glVertex2f(x2,y1);
    glVertex2f(x2,y2); glVertex2f(x1,y2);
    glEnd();
}

void bar(float x, float y, float w, float h, float frac,
         float r, float g, float b) {
    glColor4f(0.15f,0.15f,0.20f,0.85f);
    quad(x, y, x+w, y+h);
    glColor4f(r, g, b, 0.90f);
    quad(x, y, x + w*frac, y+h);
    glColor4f(0.4f,0.4f,0.55f,0.7f);
    rect(x, y, x+w, y+h);
}

}

void BenchMonitor::draw(int winW, int winH) const {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0, winW, 0, winH, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    const float PW   = 360.0f;
    const float PH   = 230.0f;
    const float PAD  = 12.0f;
    const float x1   = PAD;
    const float y2   = (float)winH - PAD;
    const float x2   = x1 + PW;
    const float y1   = y2 - PH;

    glColor4f(0.04f, 0.04f, 0.08f, 0.88f);
    quad(x1, y1, x2, y2);

    glColor4f(0.35f, 0.45f, 0.90f, 0.70f);
    glLineWidth(1.2f);
    rect(x1, y1, x2, y2);

    glColor4f(0.55f, 0.70f, 1.00f, 0.95f);
    btext(x1 + 8.0f, y2 - 16.0f, "BENCH MONITOR");
    glColor4f(0.30f, 0.35f, 0.55f, 0.80f);
    glBegin(GL_LINES);
    glVertex2f(x1+6.0f, y2-22.0f); glVertex2f(x2-6.0f, y2-22.0f);
    glEnd();

    char buf[128];
    float ty = y2 - 36.0f;
    const float LX  = x1 + 10.0f;
    const float BX  = x1 + 170.0f;
    const float BW  = PW - 180.0f;
    const float BH  = 10.0f;
    const float LS  = 26.0f;

    glColor4f(0.85f, 0.95f, 0.70f, 0.95f);
    snprintf(buf, sizeof(buf), "FPS          %.1f", fps);
    btext(LX, ty, buf);
    float fpsFrac = std::min(fps / 60.0f, 1.0f);
    float fpsR = (fps < 30.0f) ? 1.0f : (fps < 50.0f) ? 0.9f : 0.25f;
    float fpsG = (fps < 30.0f) ? 0.3f : (fps < 50.0f) ? 0.75f : 0.90f;
    bar(BX, ty - 2.0f, BW, BH, fpsFrac, fpsR, fpsG, 0.25f);
    ty -= LS;

    glColor4f(0.85f, 0.85f, 0.90f, 0.95f);
    snprintf(buf, sizeof(buf), "Frame time   %.2f ms", avgFrameMs);
    btext(LX, ty, buf);
    float ftFrac = std::min(avgFrameMs / 33.0f, 1.0f);
    bar(BX, ty - 2.0f, BW, BH, ftFrac, 0.75f, 0.55f, 0.90f);
    ty -= LS;

    glColor4f(0.55f, 0.85f, 1.00f, 0.95f);
    snprintf(buf, sizeof(buf), "C++ render   %.0f µs", avgCppUs);
    btext(LX, ty, buf);
    float maxUs = std::max(avgCppUs + avgLuaUs, 1.0f);
    bar(BX, ty - 2.0f, BW, BH, avgCppUs/maxUs, 0.30f, 0.70f, 1.00f);
    ty -= LS;

    glColor4f(1.00f, 0.80f, 0.40f, 0.95f);
    snprintf(buf, sizeof(buf), "Lua runtime  %.0f µs", avgLuaUs);
    btext(LX, ty, buf);
    bar(BX, ty - 2.0f, BW, BH, avgLuaUs/maxUs, 1.00f, 0.70f, 0.20f);
    ty -= LS;

    long rss = getRssKb();
    glColor4f(0.85f, 0.85f, 0.85f, 0.90f);
    if (rss > 0)
        snprintf(buf, sizeof(buf), "Mem RSS      %ld kB", rss);
    else
        snprintf(buf, sizeof(buf), "Mem RSS      n/a");
    btext(LX, ty, buf);
    float memFrac = (rss > 0) ? std::min(rss / 256000.0f, 1.0f) : 0.0f;
    bar(BX, ty - 2.0f, BW, BH, memFrac, 0.70f, 0.60f, 0.85f);
    ty -= LS;

    glColor4f(0.70f, 0.95f, 0.80f, 0.95f);
    if (texKb > 0)
        snprintf(buf, sizeof(buf), "Tex cache    %d tex / ~%ld kB", texCount, texKb);
    else
        snprintf(buf, sizeof(buf), "Tex cache    %d tex / sem foto", texCount);
    btext(LX, ty, buf);
    float texFrac = std::min(texCount / 6.0f, 1.0f);
    bar(BX, ty - 2.0f, BW, BH, texFrac, 0.35f, 0.85f, 0.55f);
    ty -= LS;

    glColor4f(0.30f, 0.35f, 0.55f, 0.80f);
    glBegin(GL_LINES);
    glVertex2f(x1+6.0f, ty+10.0f); glVertex2f(x2-6.0f, ty+10.0f);
    glEnd();
    glColor4f(0.30f, 0.70f, 1.00f, 0.80f);
    quad(LX, ty-2.0f, LX+10.0f, ty+8.0f);
    glColor4f(0.80f, 0.80f, 0.85f, 0.90f);
    btext(LX + 14.0f, ty, "C++");
    glColor4f(1.00f, 0.70f, 0.20f, 0.80f);
    quad(LX + 55.0f, ty-2.0f, LX+65.0f, ty+8.0f);
    glColor4f(0.80f, 0.80f, 0.85f, 0.90f);
    btext(LX + 69.0f, ty, "Lua");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

#endif /* BENCH_MODE */
