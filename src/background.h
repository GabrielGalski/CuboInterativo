/*
 * background.h
 *
 * declara a classe background. o background sempre é estrelado. a geração e animação das estrelas
 * vive em background.lua (via LuaBridge). Background::render() chama
 * bridge->getStarPositions(t) e desenha os pontos com GL_POINTS.
 * nenhuma lógica de partícula existe em c++ — apenas o desenho OpenGL.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <vector>

class LuaBridge;   //evita include circular

class Background {
private:
    LuaBridge*  ponteiroBridge;            // ponteiro para o bridge Lua (não owned)
    std::vector<float> cacheEstrelas;  // buffer reutilizável para os dados de estrela

public:
    explicit Background(LuaBridge* b = nullptr);
    void definirPadrao();
    void definirBridge(LuaBridge* b) { ponteiroBridge = b; }
    void renderizar();
};

#endif
