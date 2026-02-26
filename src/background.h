/*
 * background.h
 *
 * A lógica das estrelas vive inteiramente em background.lua — posição, velocidade,
 * cintilamento. Background::renderizar() pede ao Lua as posições para o instante
 * atual e desenha os pontos com GL_POINTS. Sem nenhuma matemática de partícula
 * em C++, só o desenho OpenGL.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <vector>

class LuaBridge;

class Background {
private:
    LuaBridge*  ponteiroBridge;
    std::vector<float> cacheEstrelas;

public:
    explicit Background(LuaBridge* b = nullptr);
    void definirPadrao();
    void definirBridge(LuaBridge* b) { ponteiroBridge = b; }
    void renderizar();
};

#endif
