/*
 * cubo.h
 *
 * Declaração da classe Cubo e da estrutura Color. O cubo é um sólido 3D
 * centrado na origem com aresta 2, desenhado como seis quads coloridos. Cada
 * face tem uma cor RGB independente; a rotação é armazenada em graus nos
 * eixos X, Y e Z. A face selecionada (para pintar com teclas 1/2/3) é
 * determinada por color picking no método selectCurrentFace(x, y). Inline
 * getters expõem a face selecionada e a cor de uma face por índice.
 */

#ifndef CUBO_H
#define CUBO_H

#include <GL/glut.h>
#include <string>

struct Color {
    float r, g, b;
};

class Cubo {
private:
    float rotX, rotY, rotZ;
    Color faceColors[6];
    int selectedFace;
    int facePatterns[6];
    GLuint faceTextures[6];
    bool faceTextureHasAlpha[6];
    float faceTextureScale[6];   // 1.0=normal; >1=zoom in; <1=imagem menor
    int   faceTextureRotation[6]; // 0-3: passos de 90° no sentido horário
    GLuint stripesTexture;
    GLuint dotsTexture;

public:
    Cubo();
    void render();
    void rotate(float dx, float dy, float dz);
    void setRotation(float x, float y, float z);
    void addColorToCurrentFace(float r, float g, float b);
    void clearSelectedFace();
    void selectCurrentFace(int x, int y);
    int getSelectedFace() const { return selectedFace; }
    Color getFaceColor(int face) const { return faceColors[face]; }
    void setFaceColor(int face, float r, float g, float b);
    void initPatterns();
    void setFacePattern(int face, int pattern);
    int getFacePattern(int face) const;
    bool setFacePhotoFromFile(int face, const std::string& path);
    bool  faceHasTexture(int face) const { return face>=0&&face<6&&faceTextures[face]!=0; }
    float getFaceTextureScale(int face) const { return (face>=0&&face<6)?faceTextureScale[face]:1.0f; }
    void  setFaceTextureScale(int face, float s);
    int   getFaceTextureRotation(int face) const { return (face>=0&&face<6)?faceTextureRotation[face]:0; }
    void  rotateFaceTexture(int face, int delta); // delta = +1 ou -1
};

#endif
