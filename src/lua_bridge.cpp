/*
 * lua_bridge.cpp
 *
 * Implementação do bridge C++/Lua. Cria e mantém o estado Lua, carrega os
 * arquivos background.lua, mixer.lua e controle.lua (procurando em lua/ e no
 * diretório atual). Expõe chamadas às funções globais getBackgroundState,
 * getBackgroundPattern, mixColors e processInput, convertendo parâmetros e
 * retornos entre os tipos Lua e C++. Em erro de pcall, a mensagem é impressa
 * em stderr e a pilha Lua é corrigida com lua_pop; onde aplicável, fallback
 * em C++ (ex.: mistura aditiva simples quando mixColors falha).
 */

#include "lua_bridge.h"
#include "background.h"
#include "cubo.h"
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

/*
 * Destrói o estado Lua se existir, libertando todos os recursos associados
 * ao interpretador (tabelas, funções carregadas, etc.).
 */
LuaBridge::~LuaBridge() {
    if (impl) {
        if (impl->L) lua_close(impl->L);
        delete impl;
        impl = nullptr;
    }
}

/*
 * Cria um novo estado Lua com luaL_newstate, abre as bibliotecas padrão e
 * carrega na ordem os scripts background.lua, mixer.lua e controle.lua,
 * tentando cada um primeiro em lua/ e depois no diretório atual. Se qualquer
 * arquivo não for encontrado ou gerar erro de execução, imprime o erro e
 * retorna false; caso contrário retorna true.
 */
bool LuaBridge::init() {
    if (!impl) return false;
    impl->L = luaL_newstate();
    if (!impl->L) {
        std::cerr << "Erro ao criar estado Lua!" << std::endl;
        return false;
    }

    luaL_openlibs(impl->L);

    const char* files[] = {"background.lua", "mixer.lua", "controle.lua"};
    const char* prefixes[] = {"lua/", ""};

    for (int i = 0; i < 3; i++) {
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
 * Obtém o estado atual do background em Lua chamando getBackgroundState() sem
 * argumentos. Espera dois retornos: índice da cor e modo (0 sólido, 1 gradiente,
 * 2 matemático). Aplica-os em bg via setColorIndex e setModel. Em função
 * inexistente ou erro de pcall, imprime em stderr e remove valores da pilha.
 */
void LuaBridge::updateBackground(Background& bg) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "getBackgroundState");
    if (!lua_isfunction(impl->L, -1)) {
        std::cerr << "Função getBackgroundState não encontrada!" << std::endl;
        lua_pop(impl->L, 1);
        return;
    }

    if (lua_pcall(impl->L, 0, 2, 0) == LUA_OK) {
        int model = (int)lua_tonumber(impl->L, -1);
        int colorIdx = (int)lua_tonumber(impl->L, -2);
        bg.setColorIndex(colorIdx);
        bg.setModel(model);
        lua_pop(impl->L, 2);
    } else {
        std::cerr << "Erro ao chamar getBackgroundState: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
    }
}

/*
 * Chama getBackgroundPattern(timeMs) em Lua com o tempo em milissegundos,
 * esperando três retornos: tipo do padrão (1 Lissajous, 2 espiral, 3 ondas),
 * param1 e param2 (frequências ou parâmetros). Atualiza o background com
 * setPattern. Se a função não existir, apenas remove o valor da pilha e retorna.
 */
void LuaBridge::updateBackgroundPattern(Background& bg, int timeMs) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "getBackgroundPattern");
    if (!lua_isfunction(impl->L, -1)) {
        lua_pop(impl->L, 1);
        return;
    }
    lua_pushinteger(impl->L, timeMs);
    if (lua_pcall(impl->L, 1, 3, 0) == LUA_OK) {
        float p2 = (float)lua_tonumber(impl->L, -1);
        float p1 = (float)lua_tonumber(impl->L, -2);
        int ptype = (int)lua_tonumber(impl->L, -3);
        bg.setPattern(ptype, p1, p2);
        lua_pop(impl->L, 3);
    } else {
        std::cerr << "Erro ao chamar getBackgroundPattern: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
    }
}

/*
 * Invoca mixColors(r, g, b, ar, ag, ab) em Lua com a cor atual e o incremento
 * (aditivo). Os três retornos são escritos em newR, newG, newB. Se a função
 * não existir ou pcall falhar, usa fallback: componente a componente min(1, c + ac).
 */
void LuaBridge::mixColor(float r, float g, float b, float ar, float ag, float ab,
                         float& newR, float& newG, float& newB) {
    if (!impl || !impl->L) {
        newR = std::min(1.0f, r + ar);
        newG = std::min(1.0f, g + ag);
        newB = std::min(1.0f, b + ab);
        return;
    }
    lua_getglobal(impl->L, "mixColors");
    if (!lua_isfunction(impl->L, -1)) {
        std::cerr << "Função mixColors não encontrada!" << std::endl;
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
        std::cerr << "Erro ao chamar mixColors: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
        newR = std::min(1.0f, r + ar);
        newG = std::min(1.0f, g + ag);
        newB = std::min(1.0f, b + ab);
    }
}

/*
 * Passa o código da tecla (unsigned char) para processInput(key) em Lua.
 * Espera três valores de retorno (dx, dy, dz) em graus e aplica-os na rotação
 * do cubo com cube.rotate. Em função inexistente ou erro, imprime e limpa a pilha.
 */
void LuaBridge::handleInput(Cubo& cube, unsigned char key) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "processInput");
    if (!lua_isfunction(impl->L, -1)) {
        std::cerr << "Função processInput não encontrada!" << std::endl;
        lua_pop(impl->L, 1);
        return;
    }

    lua_pushinteger(impl->L, key);
    if (lua_pcall(impl->L, 1, 3, 0) == LUA_OK) {
        float dx = (float)lua_tonumber(impl->L, -3);
        float dy = (float)lua_tonumber(impl->L, -2);
        float dz = (float)lua_tonumber(impl->L, -1);
        cube.rotate(dx, dy, dz);
        lua_pop(impl->L, 3);
    } else {
        std::cerr << "Erro ao chamar processInput: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
    }
}
