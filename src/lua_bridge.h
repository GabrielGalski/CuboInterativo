/*
 * lua_bridge.h
 * Interface para integração entre C++ e Lua.
 * Encapsula o lua_State e fornece métodos para as operações
 * envolvendo cores, entrada do usuário, partículas e painel de controles.
 */

#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include <string>
#include <vector>

struct LuaBridgeImpl;

class Cubo;
class Background;

/*
 * Representa uma linha de texto do painel de controles, com cor e espaçamento.
 * os dados vêm do Lua e o C++ os usa para renderizar sem nenhuma string ou cor no código.
 */
struct LinhaUI {
    std::string texto;
    float r, g, b;
    float passo;
    bool  centralizar;
};

class LuaBridge {
private:
    LuaBridgeImpl* impl;

public:
    LuaBridge();
    ~LuaBridge();

    /*
     * Cria o lua_State, carrega as bibliotecas padrão e executa os scripts do projeto. 
     */
    bool init();

    /*
     * Chama mixColorsCurrent em Lua para misturar a cor atual de uma face
     * com um incremento de cor. Escreve o resultado nos três floats de saída.
     */
    void misturarCor(float r, float g, float b, float ar, float ag, float ab,
                  float& newR, float& newG, float& newB);

    /*
     * Passa o código ASCII da tecla para lidarComEntrada em Lua, que devolve
     * três deltas (dx, dy, dz) em graus e os aplica na rotação do cubo.
     */
    void lidarComEntrada(Cubo& cube, unsigned char key);

    /*
     * Notifica o Lua sobre uma foto carregada em uma face, para que
     * o estado interno do script fique sincronizado.
     */
    void definirFotoFace(int faceIndex, const std::string& path);

    /*
     * Chama inicializarEstrelas em Lua para gerar a tabela de partículas
     * com um LCG determinístico.
     */
    void inicializarEstrelas(int count);

    /*
     * Chama obterPosicoesEstrelas(t) em Lua e preenche 'out' com pacotes
     * de 5 floats por estrela: x, y, r, g, b.
     */
    void obterPosicoesEstrelas(float t, std::vector<float>& out);

    /*
     * Passa o valor do canal R lido por glReadPixels para resolverFacePicking
     * em Lua, que converte R=1..6 no índice de face 0..5.
     */
    int resolverFacePicking(int pixelR);

    /*
     * Chama obterLinhasControles em Lua e popula 'out' com as LinhaUI para
     * o painel de controles flutuante.
     */
    void obterLinhasControles(std::vector<LinhaUI>& out);
};

#endif
