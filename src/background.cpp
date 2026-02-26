/*
 * background.cpp
 *
 * Renderiza o fundo estrelado. O quad escuro de fundo é desenhado direto em
 * OpenGL aqui; as estrelas vêm do background.lua, que retorna
 * uma tabela flat com x, y, r, g, b por estrela. O C++ só itera esse vetor e
 * chama glVertex2f/glColor3f fazendo com que nenhuma matemática de animação aconteça aqui.
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
 * Configura projeção 2D ortogonal, desenha o quad de fundo e então
 * delega ao Lua o cálculo das posições das estrelas para o tempo atual.
 * Em BENCH_MODE a chamada Lua é isolada dos timers de C++ para medir
 * separadamente no painel de benchmark.
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

#ifdef BENCH_MODE
        gBench.luaBegin();
#endif
        ponteiroBridge->obterPosicoesEstrelas(t, cacheEstrelas);
#ifdef BENCH_MODE
        gBench.luaEnd();
#endif

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
