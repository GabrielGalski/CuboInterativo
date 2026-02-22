/*
 * background.h
 *
 * Background sempre estrelado. A geração e animação das estrelas vivem em
 * background.lua (via LuaBridge). Background::render() chama
 * bridge->getStarPositions(t) e desenha os pontos com GL_POINTS.
 * Nenhuma lógica de partícula existe em C++ — apenas o desenho OpenGL.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <vector>

class LuaBridge;   // forward declaration — evita include circular

class Background {
private:
    int         currentColorIndex;
    int         currentModel;
    LuaBridge*  bridge;            // ponteiro para o bridge Lua (não owned)
    std::vector<float> starCache;  // buffer reutilizável para os dados de estrela

public:
    explicit Background(LuaBridge* b = nullptr);
    void setDefault();
    void setBridge(LuaBridge* b) { bridge = b; }
    void render();
    void nextColor();
    void previousColor();
    void nextModel();
    void setColorIndex(int index);
    void setModel(int model);
    int  getModel() const { return currentModel; }
};

#endif
