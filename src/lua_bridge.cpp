/*
 * lua_bridge.cpp
 *
 * toda a comunicação entre c++ e lua vem daqui. 
 * Lua expõe uma API C baseada em pilha. Todo valor que vai de C++ para Lua
 * ou de Lua de volta para C++ passa por essa pilha. 
 * para chamar uma função Lua a partir daqui, o padrão é sempre o mesmo:
 *
 *   lua_getglobal(L, "nomeDaFuncao") → empilha a função
 *   lua_push*(L, valor) → empilha cada argumento
 *   lua_pcall(L, nArgs, nRets, 0) → chama, substitui args+fn pelos retornos
 *   lua_to*(L, -N) → lê os retornos pelo índice negativo
 *   lua_pop(L, nRets) → limpa a pilha
 *
 * se lua_pcall retorna algo diferente de LUA_OK, o topo da pilha contém a
 * mensagem de erro como string precisando fazer lua_pop para não vazar a pilha.
 *
 * Cada método desta classe implementa exatamente essa sequência para a função
 * Lua que corresponde e onde a função não existe ou falha, há um fallback
 * em C++ para não travar o programa
 */

#include "lua_bridge.h"
#include "background.h"
#include "cubo.h"
#include <algorithm>
#include <iostream>

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

struct LuaBridgeImpl {
    lua_State* L;
};

LuaBridge::LuaBridge() : impl(new LuaBridgeImpl{nullptr}) {}

LuaBridge::~LuaBridge() {
    if (impl) {
        if (impl->L) lua_close(impl->L);
        delete impl;
        impl = nullptr;
    }
}

/*
 * cria um novo lua_State com todas as bibliotecas padrão,
 * depois carrega cada script do projeto em ordem. Para cada arquivo, tenta
 * primeiro o prefixo "lua/" depois o diretório atual e o primeiro que
 * carregar sem erro vence e se algum script falhar, imprime a mensagem que
 * Lua deixou no topo da pilha e retorna false
 */
bool LuaBridge::init() {
    if (!impl) return false;
    impl->L = luaL_newstate();
    if (!impl->L) {
        std::cerr << "Erro ao criar estado Lua!" << std::endl;
        return false;
    }

    luaL_openlibs(impl->L);

    const char* files[] = {"background.lua", "mixer.lua", "controle.lua", "faces.lua", "ui.lua"};
    const char* prefixes[] = {"lua/", ""};

    for (int i = 0; i < 5; i++) {
        bool loaded = false;
        for (int p = 0; p < 2; p++) {
            std::string path = std::string(prefixes[p]) + files[i];
            if (luaL_dofile(impl->L, path.c_str()) == LUA_OK) {
                std::cout << "Carregado: " << path << std::endl;
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            std::cerr << "Erro ao carregar " << files[i] << ": "
                      << lua_tostring(impl->L, -1) << std::endl;
            lua_pop(impl->L, 1);
            return false;
        }
    }

    return true;
}

/*
 * Mistura a cor atual de uma face com um incremento de cor chamando
 * mixColorsCurrent em Lua ou mixcolors como fallback. Empilhamos os seis
 * floats de entrada, chamamos com lua_pcall e lemos os três retornos da pilha
 * em ordem inversa, lua empilha da esquerda para a direita, então o último
 * retorno fica no topo da pilha. 
 * se a função não existir ou falhar, é feita a mistura simples.
 */
void LuaBridge::misturarCor(float r, float g, float b, float ar, float ag, float ab,
                         float& newR, float& newG, float& newB) {
    if (!impl || !impl->L) {
        newR = std::min(1.0f, r + ar);
        newG = std::min(1.0f, g + ag);
        newB = std::min(1.0f, b + ab);
        return;
    }
    lua_getglobal(impl->L, "mixColorsCurrent");
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        lua_getglobal(impl->L, "mixColors");
    }
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        newR = std::min(1.0f, r + ar);
        newG = std::min(1.0f, g + ag);
        newB = std::min(1.0f, b + ab);
        return;
    }

    lua_pushnumber(impl->L, r);
    lua_pushnumber(impl->L, g);
    lua_pushnumber(impl->L, b);
    lua_pushnumber(impl->L, ar);
    lua_pushnumber(impl->L, ag);
    lua_pushnumber(impl->L, ab);

    if (lua_pcall(impl->L, 6, 3, 0) == LUA_OK) {
        newB = (float)lua_tonumber(impl->L, -1);
        newG = (float)lua_tonumber(impl->L, -2);
        newR = (float)lua_tonumber(impl->L, -3);
        lua_pop(impl->L, 3);
    } else {
        lua_pop(impl->L, 1);
        newR = std::min(1.0f, r + ar);
        newG = std::min(1.0f, g + ag);
        newB = std::min(1.0f, b + ab);
    }
}

/*
 * Passa o código ASCII da tecla para Lua (controle.lua),
 * que mapeia WASD para deltas de rotação e devolve três números x,y,z.
 * esses valores são aplicados diretamente na rotação do cubo.
 */
void LuaBridge::lidarComEntrada(Cubo& cube, unsigned char key) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "lidarComEntrada");
    if (!lua_isfunction(impl->L, -1)) {
        std::cerr << "Função lidarComEntrada não encontrada!" << std::endl;
        lua_pop(impl->L, 1);
        return;
    }

    lua_pushinteger(impl->L, key);
    if (lua_pcall(impl->L, 1, 3, 0) == LUA_OK) {
        float dx = (float)lua_tonumber(impl->L, -3);
        float dy = (float)lua_tonumber(impl->L, -2);
        float dz = (float)lua_tonumber(impl->L, -1);
        cube.rotacionar(dx, dy, dz);
        lua_pop(impl->L, 3);
    } else {
        std::cerr << "Erro ao chamar lidarComEntrada: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
    }
}

/*
 * Notifica o faces.lua sobre uma foto carregada, para que o caminho fique
 * registrado no estado interno do script.
 */
void LuaBridge::definirFotoFace(int faceIndex, const std::string& path) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "definirFotoFace");
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        return;
    }
    lua_pushinteger(impl->L, faceIndex);
    lua_pushstring(impl->L, path.c_str());
    if (lua_pcall(impl->L, 2, 0, 0) != LUA_OK) {
        lua_pop(impl->L, 1);
    }
}

/*
 * Chama inicializarEstrelas em background.lua, que gera a tabela de estrelas
 * com posições, velocidades e fases determinísticas via LCG com seed fixa.
 * é chamada uma única vez após init().
 */
void LuaBridge::inicializarEstrelas(int count) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "inicializarEstrelas");
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        return;
    }
    lua_pushinteger(impl->L, count);
    if (lua_pcall(impl->L, 1, 0, 0) != LUA_OK) {
        lua_pop(impl->L, 1);
    }
}

/*
 * Chama obterPosicoesEstrelas em background.lua e lê a tabela flat
 * retornada. Lua retorna uma table indexada de 1 a N, onde cada grupo de
 * cinco valores consecutivos é x, y, r, g, b de uma estrela. Esses valores
 * são lidos com lua_rawgeti e empilhados no vetor de saída para serem usados
 * diretamente em glVertex2f e glColor3f. se a função falhar o vetor fica vazio.
 */
void LuaBridge::obterPosicoesEstrelas(float t, std::vector<float>& out) {
    out.clear();
    if (!impl || !impl->L) return;

    lua_getglobal(impl->L, "obterPosicoesEstrelas");
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        return;
    }
    lua_pushnumber(impl->L, t);
    if (lua_pcall(impl->L, 1, 1, 0) != LUA_OK) {
        lua_pop(impl->L, 1);
        return;
    }

    if (!lua_istable(impl->L, -1)) {
        lua_pop(impl->L, 1);
        return;
    }

    int n = (int)lua_rawlen(impl->L, -1);
    out.reserve(n);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(impl->L, -1, i);
        out.push_back((float)lua_tonumber(impl->L, -1));
        lua_pop(impl->L, 1);
    }

    lua_pop(impl->L, 1);
}

/*
 * Converte o byte R lido por glReadPixels em índice de face chamando
 * resolverFacePicking em faces.lua. 
 * o cubo pinta cada face com glColor3ub durante o picking.
 * se a chamada falhar faz a mesma conta em C++ como fallback.
 */
int LuaBridge::resolverFacePicking(int pixelR) {
    if (!impl || !impl->L) {
        return (pixelR >= 1 && pixelR <= 6) ? pixelR - 1 : -1;
    }
    lua_getglobal(impl->L, "resolverFacePicking");
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        return (pixelR >= 1 && pixelR <= 6) ? pixelR - 1 : -1;
    }
    lua_pushinteger(impl->L, pixelR);
    if (lua_pcall(impl->L, 1, 1, 0) == LUA_OK) {
        int face = (int)lua_tonumber(impl->L, -1);
        lua_pop(impl->L, 1);
        return face;
    }
    std::cerr << "Erro em resolverFacePicking: "
              << lua_tostring(impl->L, -1) << std::endl;
    lua_pop(impl->L, 1);
    return (pixelR >= 1 && pixelR <= 6) ? pixelR - 1 : -1;
}

/*
 * lê uma table Lua de LinhaUI da pilha no indice -1 e popula out.
 * cada entrada da table tem campos texto, r, g, b, passo e centralizar,
 * que são lidos com lua_getfield e convertidos para o C++.
 */
static void lerTabelaLinhasUI(lua_State* L, std::vector<LinhaUI>& out) {
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }
    int n = (int)lua_rawlen(L, -1);
    out.reserve(n);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, -1, i);
        if (!lua_istable(L, -1)) { lua_pop(L, 1); continue; }

        LinhaUI linha;

        lua_getfield(L, -1, "texto");
        linha.texto = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
        lua_pop(L, 1);

        lua_getfield(L, -1, "r"); linha.r = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_getfield(L, -1, "g"); linha.g = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_getfield(L, -1, "b"); linha.b = (float)lua_tonumber(L, -1); lua_pop(L, 1);

        lua_getfield(L, -1, "passo");
        linha.passo = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "centralizar");
        linha.centralizar = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);

        out.push_back(linha);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/*
 * Chama obterLinhasControles em ui.lua e usa lerTabelaLinhasUI para
 * converter a tabela retornada em um vetor para o C++.
 */
void LuaBridge::obterLinhasControles(std::vector<LinhaUI>& out) {
    out.clear();
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "obterLinhasControles");
    if (!lua_isfunction(impl->L, -1)) { lua_pop(impl->L, 1); return; }
    if (lua_pcall(impl->L, 0, 1, 0) != LUA_OK) {
        std::cerr << "Erro em obterLinhasControles: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
        return;
    }
    lerTabelaLinhasUI(impl->L, out);
}
