/*
 * cubo.cpp
 *
 * Implementação do cubo 3D: seis faces com vértices em (-1,-1,-1) a (1,1,1),
 * cada uma com cor própria. render() aplica as rotações acumuladas (rotX, rotY,
 * rotZ) e desenha os quads. selectCurrentFace realiza color picking: desenha o
 * cubo com uma cor de identificação por face (R=1..6), lê o pixel na posição
 * do clique (convertendo y de janela para OpenGL), e define selectedFace pelo
 * valor lido. A mistura de cores na face atual pode ser feita por addColorToCurrentFace
 * (lógica local) ou externamente via getFaceColor/setFaceColor e o mixer em Lua.
 */

#include "cubo.h"
#include <iostream>
#include <cmath>

/*
 * Construtor: rotações e face selecionada em zero; todas as seis faces
 * começam com cor branca (1, 1, 1).
 */
Cubo::Cubo() : rotX(0), rotY(0), rotZ(0), selectedFace(0) {
    for (int i = 0; i < 6; ++i) {
        faceColors[i] = {1.0f, 1.0f, 1.0f};
    }
}

/*
 * Salva a matriz de modelo, aplica rotações em graus nos eixos X, Y e Z
 * (na ordem fixa glRotatef X, Y, Z), desenha seis GL_QUADS com as cores
 * de faceColors e restaura a matriz. A ordem dos vértices segue a orientação
 * frontal de cada face para que a normal apontar para fora.
 */
void Cubo::render() {
    glPushMatrix();
    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotZ, 0.0f, 0.0f, 1.0f);

    glBegin(GL_QUADS);

    glColor3f(faceColors[0].r, faceColors[0].g, faceColors[0].b);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);

    glColor3f(faceColors[1].r, faceColors[1].g, faceColors[1].b);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);

    glColor3f(faceColors[2].r, faceColors[2].g, faceColors[2].b);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);

    glColor3f(faceColors[3].r, faceColors[3].g, faceColors[3].b);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);

    glColor3f(faceColors[4].r, faceColors[4].g, faceColors[4].b);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);

    glColor3f(faceColors[5].r, faceColors[5].g, faceColors[5].b);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);

    glEnd();
    glPopMatrix();
}

/*
 * Soma os incrementos dx, dy, dz às rotações internas (em graus). Usado
 * após processInput em Lua retornar os deltas para a tecla pressionada.
 */
void Cubo::rotate(float dx, float dy, float dz) {
    rotX += dx;
    rotY += dy;
    rotZ += dz;
}

/*
 * Substitui as rotações atuais pelos valores absolutos x, y, z (graus).
 * Usado na inicialização para definir a pose inicial do cubo.
 */
void Cubo::setRotation(float x, float y, float z) {
    rotX = x;
    rotY = y;
    rotZ = z;
}

/*
 * Se a face selecionada estiver praticamente branca (todos os canais >= 0.99),
 * substitui a cor pela nova (r, g, b). Caso contrário, soma aditivamente cada
 * canal e limita a 1.0. Usado quando a mistura não é feita via Lua.
 */
void Cubo::addColorToCurrentFace(float r, float g, float b) {
    Color& c = faceColors[selectedFace];
    if (c.r >= 0.99f && c.g >= 0.99f && c.b >= 0.99f) {
        c.r = r;
        c.g = g;
        c.b = b;
    } else {
        c.r = std::min(1.0f, c.r + r);
        c.g = std::min(1.0f, c.g + g);
        c.b = std::min(1.0f, c.b + b);
    }
}

/*
 * Define a cor da face atualmente selecionada para branco (1, 1, 1).
 */
void Cubo::clearSelectedFace() {
    faceColors[selectedFace] = {1.0f, 1.0f, 1.0f};
}

/*
 * Atualiza a cor da face indicada pelo índice (0–5) para (r, g, b). Índices
 * fora do intervalo são ignorados.
 */
void Cubo::setFaceColor(int face, float r, float g, float b) {
    if (face >= 0 && face < 6) {
        faceColors[face].r = r;
        faceColors[face].g = g;
        faceColors[face].b = b;
    }
}

/*
 * Usa color picking para definir selectedFace a partir da posição (x, y) em
 * coordenadas de janela. Obtém o viewport, limpa os buffers e desativa
 * iluminação/textura; mantém a projeção atual, empilha a matriz de modelo,
 * aplica a mesma translação e rotações do cubo e desenha as seis faces com
 * glColor3ub(faceId, 0, 0) onde faceId é 1 a 6. Lê o pixel em (x, viewport[3]-y-1)
 * com glReadPixels; o componente R do pixel (1–6) determina a face (selectedFace = R - 1).
 * Restaura as matrizes e limpa o buffer para o desenho normal. Imprime no
 * console o índice e o nome da face ou que nenhuma face foi clicada.
 */
void Cubo::selectCurrentFace(int x, int y) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotZ, 0.0f, 0.0f, 1.0f);

    glBegin(GL_QUADS);

    glColor3ub(1, 0, 0);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);

    glColor3ub(2, 0, 0);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);

    glColor3ub(3, 0, 0);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);

    glColor3ub(4, 0, 0);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);

    glColor3ub(5, 0, 0);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);

    glColor3ub(6, 0, 0);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);

    glEnd();

    glFlush();
    glFinish();

    unsigned char pixel[3];
    glReadPixels(x, viewport[3] - y - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    if (pixel[0] >= 1 && pixel[0] <= 6) {
        selectedFace = pixel[0] - 1;
        std::cout << "Face selecionada: " << selectedFace << " (";
        switch(selectedFace) {
            case 0: std::cout << "Front"; break;
            case 1: std::cout << "Back"; break;
            case 2: std::cout << "Top"; break;
            case 3: std::cout << "Bottom"; break;
            case 4: std::cout << "Right"; break;
            case 5: std::cout << "Left"; break;
        }
        std::cout << ")" << std::endl;
    } else {
        std::cout << "Nenhuma face clicada (pixel: " << (int)pixel[0] << ")" << std::endl;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
