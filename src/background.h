/*
 * background.h
 *
 * Declaração da classe Background e da estrutura BgColor. O background pode ser
 * desenhado em três modos: sólido (um quad com a cor atual), gradiente vertical
 * (escuro na base, cor no topo) ou formas matemáticas dinâmicas (Lissajous,
 * espiral, ondas), com parâmetros fornecidos pelo Lua. Mantém um vetor de cores
 * fixas e índices para cor atual, modo e parâmetros do padrão (tipo e dois floats).
 * Os getters/setters permitem integração com o LuaBridge e controle por teclado.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <vector>

struct BgColor {
    float r, g, b;
};

class Background {
private:
    struct Star {
        float x;
        float y;
        float speed;
        float brightness;
        float twinkle;
    };

    int currentColorIndex;
    int currentModel;
    std::vector<BgColor> colors;
    std::vector<Star> stars;

public:
    Background();
    void setDefault();
    void render();
    void nextColor();
    void previousColor();
    void nextModel();
    void setColorIndex(int index);
    void setModel(int model);
    int getModel() const { return currentModel; }
};

#endif
