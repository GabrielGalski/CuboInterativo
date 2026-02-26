/*
 * background.cpp
 *
 * desenha o fundo estrelado usando dados calculados inteiramente em lua.
 * render() pede ao LuaBridge a tabela flat { x,y,r,g,b ... } via
 * getStarPositions(t) e itera com glVertex2f/glColor3f.
 * o quad escuro de fundo é desenhado diretamente em OpenGL aqui.
 * nenhuma matemática de partícula existe neste arquivo.
 *
 * em modo BENCH_MODE, a chamada lua é cercada por gBench.luaBegin/luaEnd
 * e o tempo de desenho OpenGL puro fica em gBench.cppRenderBegin/End
 * (instrumentado em main.cpp).
 */

#include "background.h"
#include "lua_bridge.h"
#include <GL/glut.h>

#ifdef BENCH_MODE
#include "bench.h"
#endif

Background::Background(LuaBridge* b)
    : ponteiroBridge(b) {}

void Background::definirPadrao() {
}

/*
 * Configura projeção 2D orto (0-1), desenha o quad de fundo escuro e
 * em seguida solicita ao Lua as posições/cores das estrelas para o
 * instante t atual, desenhando cada uma como um GL_POINT.
 *
 * Em BENCH_MODE a chamada getStarPositions (Lua pura) é isolada do
 * desenho OpenGL para que o painel de bench exiba tempos precisos.
 */
void Background::renderizar() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(0.01f, 0.01f, 0.03f);
    glVertex2f(0, 0); glVertex2f(1, 0);
    glVertex2f(1, 1); glVertex2f(0, 1);
    glEnd();

    if (ponteiroBridge) {
        float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

        /* ── Chamada Lua: isolada para medição individual ── */
#ifdef BENCH_MODE
        gBench.luaBegin();
#endif
        ponteiroBridge->obterPosicoesEstrelas(t, cacheEstrelas);
#ifdef BENCH_MODE
        gBench.luaEnd();
#endif

        /* ── Desenho OpenGL puro ── */
        glPointSize(1.6f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i + 4 < cacheEstrelas.size(); i += 5) {
            glColor3f(cacheEstrelas[i+2], cacheEstrelas[i+3], cacheEstrelas[i+4]);
            glVertex2f(cacheEstrelas[i+0], cacheEstrelas[i+1]);
        }
        glEnd();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}
