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
#include <vector>
#include <fstream>
#include <sstream>

namespace {
struct Vertex3 {
    float x;
    float y;
    float z;
};

void faceVertices(int face, Vertex3& v0, Vertex3& v1, Vertex3& v2, Vertex3& v3) {
    if (face == 0) {
        v0 = {-1.0f, -1.0f,  1.0f};
        v1 = { 1.0f, -1.0f,  1.0f};
        v2 = { 1.0f,  1.0f,  1.0f};
        v3 = {-1.0f,  1.0f,  1.0f};
    } else if (face == 1) {
        v0 = {-1.0f, -1.0f, -1.0f};
        v1 = {-1.0f,  1.0f, -1.0f};
        v2 = { 1.0f,  1.0f, -1.0f};
        v3 = { 1.0f, -1.0f, -1.0f};
    } else if (face == 2) {
        v0 = {-1.0f,  1.0f, -1.0f};
        v1 = {-1.0f,  1.0f,  1.0f};
        v2 = { 1.0f,  1.0f,  1.0f};
        v3 = { 1.0f,  1.0f, -1.0f};
    } else if (face == 3) {
        v0 = {-1.0f, -1.0f, -1.0f};
        v1 = { 1.0f, -1.0f, -1.0f};
        v2 = { 1.0f, -1.0f,  1.0f};
        v3 = {-1.0f, -1.0f,  1.0f};
    } else if (face == 4) {
        v0 = { 1.0f, -1.0f, -1.0f};
        v1 = { 1.0f,  1.0f, -1.0f};
        v2 = { 1.0f,  1.0f,  1.0f};
        v3 = { 1.0f, -1.0f,  1.0f};
    } else {
        v0 = {-1.0f, -1.0f, -1.0f};
        v1 = {-1.0f, -1.0f,  1.0f};
        v2 = {-1.0f,  1.0f,  1.0f};
        v3 = {-1.0f,  1.0f, -1.0f};
    }
}

bool loadPpm(const std::string& path, int& width, int& height, std::vector<unsigned char>& data) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }
    std::string magic;
    file >> magic;
    if (magic != "P3") {
        return false;
    }
    file >> width >> height;
    int maxVal = 0;
    file >> maxVal;
    if (!file.good() || width <= 0 || height <= 0 || maxVal <= 0) {
        return false;
    }
    data.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
    for (int i = 0; i < width * height; ++i) {
        int r = 0;
        int g = 0;
        int b = 0;
        file >> r >> g >> b;
        if (!file.good()) {
            return false;
        }
        data[static_cast<size_t>(i) * 3u + 0u] = static_cast<unsigned char>(r * 255 / maxVal);
        data[static_cast<size_t>(i) * 3u + 1u] = static_cast<unsigned char>(g * 255 / maxVal);
        data[static_cast<size_t>(i) * 3u + 2u] = static_cast<unsigned char>(b * 255 / maxVal);
    }
    return true;
}

GLuint createTextureFromData(int width, int height, const std::vector<unsigned char>& data) {
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return texId;
}

GLuint createStripeTexture() {
    const int size = 64;
    std::vector<unsigned char> data(static_cast<size_t>(size) * static_cast<size_t>(size) * 3u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int stripe = (x / 8) % 2;
            float v = stripe == 0 ? 0.3f : 0.7f;
            unsigned char c = static_cast<unsigned char>(v * 255.0f);
            size_t index = static_cast<size_t>(y) * static_cast<size_t>(size) * 3u + static_cast<size_t>(x) * 3u;
            data[index + 0u] = c;
            data[index + 1u] = c;
            data[index + 2u] = c;
        }
    }
    return createTextureFromData(size, size, data);
}

GLuint createDotsTexture() {
    const int size = 64;
    std::vector<unsigned char> data(static_cast<size_t>(size) * static_cast<size_t>(size) * 3u);
    float center = (size - 1) * 0.5f;
    float radius = size * 0.12f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float dist = std::sqrt(dx * dx + dy * dy);
            float v = dist < radius ? 0.9f : 0.3f;
            unsigned char c = static_cast<unsigned char>(v * 255.0f);
            size_t index = static_cast<size_t>(y) * static_cast<size_t>(size) * 3u + static_cast<size_t>(x) * 3u;
            data[index + 0u] = c;
            data[index + 1u] = c;
            data[index + 2u] = c;
        }
    }
    return createTextureFromData(size, size, data);
}
}

/*
 * Construtor: rotações e face selecionada em zero; todas as seis faces
 * começam com cor branca (1, 1, 1).
 */
Cubo::Cubo() : rotX(0), rotY(0), rotZ(0), selectedFace(0), stripesTexture(0), dotsTexture(0) {
    for (int i = 0; i < 6; ++i) {
        faceColors[i] = {1.0f, 1.0f, 1.0f};
        facePatterns[i] = 0;
        faceTextures[i] = 0;
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
    for (int face = 0; face < 6; ++face) {
        Vertex3 v0;
        Vertex3 v1;
        Vertex3 v2;
        Vertex3 v3;
        faceVertices(face, v0, v1, v2, v3);
        Color c = faceColors[face];
        bool hasTexture = faceTextures[face] != 0;
        if (hasTexture) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, faceTextures[face]);
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(v0.x, v0.y, v0.z);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(v1.x, v1.y, v1.z);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(v2.x, v2.y, v2.z);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(v3.x, v3.y, v3.z);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        } else {
            int pattern = facePatterns[face];
            if (pattern == 0) {
                glDisable(GL_TEXTURE_2D);
                glColor3f(c.r, c.g, c.b);
                glBegin(GL_QUADS);
                glVertex3f(v0.x, v0.y, v0.z);
                glVertex3f(v1.x, v1.y, v1.z);
                glVertex3f(v2.x, v2.y, v2.z);
                glVertex3f(v3.x, v3.y, v3.z);
                glEnd();
            } else {
                GLuint tex = 0;
                if (pattern == 1) {
                    tex = stripesTexture;
                } else if (pattern == 2) {
                    tex = dotsTexture;
                }
                if (tex != 0) {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glColor3f(c.r, c.g, c.b);
                    glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex3f(v0.x, v0.y, v0.z);
                    glTexCoord2f(1.0f, 0.0f);
                    glVertex3f(v1.x, v1.y, v1.z);
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex3f(v2.x, v2.y, v2.z);
                    glTexCoord2f(0.0f, 1.0f);
                    glVertex3f(v3.x, v3.y, v3.z);
                    glEnd();
                    glDisable(GL_TEXTURE_2D);
                } else {
                    glDisable(GL_TEXTURE_2D);
                    glColor3f(c.r, c.g, c.b);
                    glBegin(GL_QUADS);
                    glVertex3f(v0.x, v0.y, v0.z);
                    glVertex3f(v1.x, v1.y, v1.z);
                    glVertex3f(v2.x, v2.y, v2.z);
                    glVertex3f(v3.x, v3.y, v3.z);
                    glEnd();
                }
            }
        }
    }
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

void Cubo::initPatterns() {
    if (stripesTexture == 0) {
        stripesTexture = createStripeTexture();
    }
    if (dotsTexture == 0) {
        dotsTexture = createDotsTexture();
    }
}

void Cubo::setFacePattern(int face, int pattern) {
    if (face < 0 || face >= 6) {
        return;
    }
    if (pattern < 0) {
        pattern = 0;
    }
    if (pattern > 2) {
        pattern = 2;
    }
    facePatterns[face] = pattern;
}

int Cubo::getFacePattern(int face) const {
    if (face < 0 || face >= 6) {
        return 0;
    }
    return facePatterns[face];
}

bool Cubo::setFacePhotoFromFile(int face, const std::string& path) {
    if (face < 0 || face >= 6) {
        return false;
    }
    int width = 0;
    int height = 0;
    std::vector<unsigned char> data;
    if (!loadPpm(path, width, height, data)) {
        std::cerr << "Falha ao carregar imagem PPM: " << path << std::endl;
        return false;
    }
    GLuint texId = createTextureFromData(width, height, data);
    faceTextures[face] = texId;
    return true;
}
