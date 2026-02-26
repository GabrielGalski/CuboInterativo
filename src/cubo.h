/*
 * cubo.h
 *
 * declara a classe cubo e a estrutura cor. o cubo é um sólido 3d
 * centrado na origem com aresta 2, desenhado como seis quads coloridos. cada
 * face tem uma cor rgb independente; a rotação é armazenada em graus nos
 * eixos x, y e z. a face selecionada (para pintar com teclas 1/2/3) é
 * determinada por color picking no método selectCurrentFace(x, y). inline
 * getters expõem a face selecionada e a cor de uma face por índice.
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
    float escalasTexturasFaces[6];   // 1.0=normal; >1=zoom in; <1=imagem menor
    int   rotacoesTexturasFaces[6]; // 0-3: passos de 90° no sentido horário

public:
    Cubo();
    void renderizar();
    void rotacionar(float dx, float dy, float dz);
    void definirRotacao(float x, float y, float z);
    void limparFaceSelecionada();
    void limparCorFaceSelecionada();
    // Renderiza cena de picking e retorna o byte R lido (0–255).
    // O caller passa o valor a bridge.resolverFacePicking() para obter o índice da face.
    int  lerPixelPicking(int x, int y);
    // Define diretamente a face selecionada (usado após resolução pelo Lua).
    void definirFaceSelecionada(int face);
    int obterFaceSelecionada() const { return faceSelecionada; }
    Cor obterCorFace(int face) const { return coresFaces[face]; }
    void definirCorFace(int face, float r, float g, float b);
    bool definirFotoFaceDeArquivo(int face, const std::string& path);
    bool  faceTemTextura(int face) const { return face>=0&&face<6&&texturasFaces[face]!=0; }
    float obterEscalaTexturaFace(int face) const { return (face>=0&&face<6)?escalasTexturasFaces[face]:1.0f; }
    void  definirEscalaTexturaFace(int face, float s);
    int   obterRotacaoTexturaFace(int face) const { return (face>=0&&face<6)?rotacoesTexturasFaces[face]:0; }
    void  rotacionarTexturaFace(int face, int delta); // delta = +1 ou -1
};

#endif
