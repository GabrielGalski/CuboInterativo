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

/*
 * Construtor: inicializa índice de cor e modo em 0, tipo de padrão 1 e
 * parâmetros padrão; preenche o vetor de cores com branco, preto, laranja,
 * rosa, azul escuro, vermelho escuro e verde escuro (RGB normalizado).
 */
Background::Background() : currentColorIndex(0), currentModel(0), patternType(1), patternParam1(2.0f), patternParam2(3.0f) {
    colors.push_back({1.0f, 1.0f, 1.0f});
    colors.push_back({0.0f, 0.0f, 0.0f});
    colors.push_back({1.0f, 0.5f, 0.0f});
    colors.push_back({1.0f, 0.75f, 0.8f});
    colors.push_back({0.0f, 0.0f, 0.5f});
    colors.push_back({0.5f, 0.0f, 0.0f});
    colors.push_back({0.0f, 0.5f, 0.0f});
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
    } else if (currentModel == 1) {
        glBegin(GL_QUADS);
        glColor3f(c.r * 0.5f, c.g * 0.5f, c.b * 0.5f);
        glVertex2f(0, 0);
        glVertex2f(1, 0);
        glColor3f(c.r, c.g, c.b);
        glVertex2f(1, 1);
        glVertex2f(0, 1);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glColor3f(c.r * 0.2f, c.g * 0.2f, c.b * 0.2f);
        glVertex2f(0, 0);
        glVertex2f(1, 0);
        glVertex2f(1, 1);
        glVertex2f(0, 1);
        glEnd();
        float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float cx = 0.5f, cy = 0.5f, scale = 0.4f;
        int segments = 400;
        glColor3f(c.r, c.g, c.b);
        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);

        if (patternType == 1) {
            for (int i = 0; i <= segments; ++i) {
                float u = (float)i / (float)segments * 2.0f * 3.14159265f + t;
                float x = cx + scale * std::sin(patternParam1 * u);
                float y = cy + scale * std::sin(patternParam2 * u);
                glVertex2f(x, y);
            }
        } else if (patternType == 2) {
            for (int i = 0; i <= segments; ++i) {
                float u = (float)i / (float)segments * 4.0f * 3.14159265f + t * 0.5f;
                float r = 0.15f + 0.25f * (u / (4.0f * 3.14159265f));
                float x = cx + scale * r * std::cos(patternParam1 * u);
                float y = cy + scale * r * std::sin(patternParam2 * u);
                glVertex2f(x, y);
            }
        } else {
            for (int i = 0; i <= segments; ++i) {
                float u = (float)i / (float)segments * 2.0f * 3.14159265f;
                float x = (float)i / (float)segments;
                float y = 0.5f + 0.35f * std::sin(patternParam1 * (u + t)) * std::sin(patternParam2 * (u * 2.0f + t * 0.7f));
                glVertex2f(x, y);
            }
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
    currentModel = (currentModel + 1) % 3;
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
    if (model >= 0 && model <= 2) {
        currentModel = model;
    }
}

/*
 * Atualiza o tipo e os dois parâmetros do padrão matemático (modo 2).
 * Tipo 1: Lissajous; 2: espiral; 3: onda. Os parâmetros são usados como
 * frequências ou escalas nas fórmulas de render.
 */
void Background::setPattern(int type, float param1, float param2) {
    patternType = type;
    patternParam1 = param1;
    patternParam2 = param2;
}
