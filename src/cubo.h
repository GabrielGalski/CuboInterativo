/*
 * cubo.h
 *
 * Define o cubo 3D e a struct Cor. O cubo tem aresta 2, centrado na origem,
 * com seis faces independentes — cada uma com sua própria cor RGB, textura
 * opcional, escala de textura e rotação de textura. A rotação do cubo inteiro
 * é acumulada em graus nos três eixos.
 *
 * A seleção de face é feita por color picking: o cubo é desenhado com uma cor
 * única por face em lerPixelPicking, e a resolução do pixel para índice de face
 * fica com o Lua (bridge.resolverFacePicking).
 */

#ifndef CUBO_H
#define CUBO_H

#include <GL/glut.h>
#include <string>

struct Cor {
    float vermelho, verde, azul;
};

class Cubo {
private:
    float rotacaoX, rotacaoY, rotacaoZ;
    Cor coresFaces[6];
    int faceSelecionada;
    GLuint texturasFaces[6];
    bool texturasFacesTemAlfa[6];
    float escalasTexturasFaces[6];
    int   rotacoesTexturasFaces[6];

public:
    Cubo();
    void renderizar();
    void rotacionar(float dx, float dy, float dz);
    void definirRotacao(float x, float y, float z);
    void limparFaceSelecionada();
    void limparCorFaceSelecionada();
    int  lerPixelPicking(int x, int y);
    void definirFaceSelecionada(int face);
    int obterFaceSelecionada() const { return faceSelecionada; }
    Cor obterCorFace(int face) const { return coresFaces[face]; }
    void definirCorFace(int face, float r, float g, float b);
    bool definirFotoFaceDeArquivo(int face, const std::string& path);
    bool  faceTemTextura(int face) const { return face>=0&&face<6&&texturasFaces[face]!=0; }
    float obterEscalaTexturaFace(int face) const { return (face>=0&&face<6)?escalasTexturasFaces[face]:1.0f; }
    void  definirEscalaTexturaFace(int face, float s);
    int   obterRotacaoTexturaFace(int face) const { return (face>=0&&face<6)?rotacoesTexturasFaces[face]:0; }
    void  rotacionarTexturaFace(int face, int delta);
};

#endif
