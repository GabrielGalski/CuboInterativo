/*
 * lua_bridge.h
 *
 * Declaração da classe LuaBridge, que mantém um estado Lua (lua_State) e expõe
 * métodos para inicializar o interpretador, carregar scripts e invocar funções
 * Lua a partir do C++. Usado para: estado e padrão do background (getBackgroundState,
 * getBackgroundPattern), mistura de cores (mixColors) e processamento de entrada
 * para rotação (processInput). A implementação usa um ponteiro opaco (PIMPL) para
 * não exigir os headers Lua neste ficheiro, evitando dependência de include path
 * do Lua em IDEs. Declarações forward de Cubo e Background evitam incluir os
 * respetivos headers.
 */

#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include <string>
#include <vector>

struct LuaBridgeImpl;

class Cubo;
class Background;

class LuaBridge {
private:
    LuaBridgeImpl* impl;

public:
    LuaBridge();
    ~LuaBridge();
    bool init();
    void updateBackground(Background& bg);
    void mixColor(float r, float g, float b, float ar, float ag, float ab,
                  float& newR, float& newG, float& newB);
    void handleInput(Cubo& cube, unsigned char key);
    void toggleFacePattern(Cubo& cube);
    void setFacePhoto(int faceIndex, const std::string& path);

    // Partículas de estrela delegadas ao Lua
    void initStars(int count);
    // Preenche 'out' com pacotes de 5 floats por estrela: x, y, r, g, b
    void getStarPositions(float t, std::vector<float>& out);
};

#endif
