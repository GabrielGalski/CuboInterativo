/*
 * lua_bridge.h
 *
 * declara a classe LuaBridge, que mantém um estado lua (lua_State) e expõe
 * métodos para inicializar o interpretador, carregar scripts e invocar funções
 * lua a partir do c++. usado para: estado e padrão do background (getBackgroundState,
 * getBackgroundPattern), mistura de cores (mixColors) e processamento de entrada
 * para rotação (processInput). a implementação usa um ponteiro opaco (PIMPL) para
 * não exigir os headers lua neste ficheiro, evitando dependência de include path
 * do lua em ides. declarações forward de Cubo e Background evitam incluir os
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
    void misturarCor(float r, float g, float b, float ar, float ag, float ab,
                  float& newR, float& newG, float& newB);
    void lidarComEntrada(Cubo& cube, unsigned char key);
    void definirFotoFace(int faceIndex, const std::string& path);

    // Partículas de estrela delegadas ao Lua
    void inicializarEstrelas(int count);
    // Preenche 'out' com pacotes de 5 floats por estrela: x, y, r, g, b
    void obterPosicoesEstrelas(float t, std::vector<float>& out);
};

#endif
