/*
 * background.cpp
 *
 * Implementação do desenho do fundo da cena. Em modo 0 desenha um quad 2D
 * sólido em projeção orto (0–1); em modo 1 o mesmo quad com gradiente de cor;
 * em modo 2 desenha um quad escuro de base e sobre ele uma curva matemática
 * (Lissajous, espiral ou onda) usando GL_LINE_STRIP, com tempo vindo de
 * glutGet(GLUT_ELAPSED_TIME) e parâmetros vindos do Lua. As matrizes de projeção
 * e modelo são salvas e restauradas para não interferir no desenho do cubo.
 */

#include "background.h"
#include <GL/glut.h>
#include <cmath>

namespace {
unsigned int lcg(unsigned int& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

float rand01(unsigned int& state) {
    return (lcg(state) & 0x00FFFFFFu) / 16777215.0f;
}
}

/*
 * Construtor: inicializa índice de cor e modo em 0, tipo de padrão 1 e
 * parâmetros padrão; preenche o vetor de cores com branco, preto, laranja,
 * rosa, azul escuro, vermelho escuro e verde escuro (RGB normalizado).
 */
Background::Background() : currentColorIndex(0), currentModel(0) {
    colors.push_back({1.0f, 1.0f, 1.0f});
    colors.push_back({0.0f, 0.0f, 0.0f});
    colors.push_back({1.0f, 0.5f, 0.0f});
    colors.push_back({1.0f, 0.75f, 0.8f});
    colors.push_back({0.0f, 0.0f, 0.5f});
    colors.push_back({0.5f, 0.0f, 0.0f});
    colors.push_back({0.0f, 0.5f, 0.0f});

    unsigned int seed = 1337u;
    stars.reserve(420);
    for (int i = 0; i < 420; ++i) {
        Star s;
        s.x = rand01(seed);
        s.y = rand01(seed);
        float depth = rand01(seed);
        s.speed = 0.02f + depth * 0.10f;
        s.brightness = 0.45f + (1.0f - depth) * 0.55f;
        s.twinkle = rand01(seed) * 6.2831853f;
        stars.push_back(s);
    }
}

/*
 * Define o estado inicial do background: cor preta (índice 1) e modo sólido (0).
 */
void Background::setDefault() {
    currentColorIndex = 1;
    currentModel = 0;
}

/*
 * Desenha o background em espaço 2D orto (0–1). Desativa depth test, empilha
 * projeção orto e modelo identidade, escolhe a cor pelo índice atual. Modo 0:
 * um quad com essa cor. Modo 1: quad com gradiente vertical (metade escura em
 * baixo). Modo 2: quad escuro (cor * 0.2) e depois GL_LINE_STRIP com 401
 * vértices; o padrão 1 é Lissajous (sin(f1*u), sin(f2*u)), 2 é espiral
 * (r crescente, cos/sin), 3 é onda ao longo de x com produto de senos. t vem
 * de glutGet(GLUT_ELAPSED_TIME). Restaura matrizes e reativa depth test.
 */
void Background::render() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    BgColor c = colors[currentColorIndex];

    if (currentModel == 0) {
        glBegin(GL_QUADS);
        glColor3f(c.r, c.g, c.b);
        glVertex2f(0, 0);
        glVertex2f(1, 0);
        glVertex2f(1, 1);
        glVertex2f(0, 1);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.01f, 0.01f, 0.03f);
        glVertex2f(0, 0);
        glVertex2f(1, 0);
        glVertex2f(1, 1);
        glVertex2f(0, 1);
        glEnd();
        float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        glPointSize(1.6f);
        glBegin(GL_POINTS);
        for (const Star& s : stars) {
            float yy = s.y - std::fmod(t * s.speed, 1.0f);
            if (yy < 0.0f) yy += 1.0f;
            float drift = std::sin(t * 0.15f + s.twinkle) * 0.01f;
            float xx = s.x + drift;
            if (xx < 0.0f) xx += 1.0f;
            if (xx > 1.0f) xx -= 1.0f;
            float tw = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(t * 2.0f + s.twinkle));
            float b = s.brightness * tw;
            glColor3f(b * 0.85f, b * 0.90f, b);
            glVertex2f(xx, yy);
        }
        glEnd();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

/*
 * Avança o índice da cor do background para a próxima no vetor, com ciclo
 * (módulo pelo tamanho do vetor).
 */
void Background::nextColor() {
    currentColorIndex = (currentColorIndex + 1) % (int)colors.size();
}

/*
 * Retrocede o índice da cor do background para a anterior no vetor, com
 * ciclo reverso (soma size antes do módulo para evitar negativo).
 */
void Background::previousColor() {
    currentColorIndex = (currentColorIndex - 1 + (int)colors.size()) % (int)colors.size();
}

/*
 * Cicla o modo de desenho: 0 -> 1 -> 2 -> 0 (sólido, gradiente, matemático).
 */
void Background::nextModel() {
    currentModel = (currentModel + 1) % 2;
}

/*
 * Define o índice da cor atual se estiver dentro do intervalo [0, size).
 * Caso contrário não altera o estado.
 */
void Background::setColorIndex(int index) {
    if (index >= 0 && index < (int)colors.size()) {
        currentColorIndex = index;
    }
}

/*
 * Define o modo de desenho (0, 1 ou 2). Valores fora desse intervalo são
 * ignorados.
 */
void Background::setModel(int model) {
    if (model >= 0 && model <= 1) {
        currentModel = model;
    }
}
