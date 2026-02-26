/*
 * lua_bridge.cpp
 *
 * cria e mantém o estado lua, carrega os arquivos background, mixer e controle 
 * tentando primeiro em lua/ e depois no diretório atual.
 * expõe chamadas às funções globais getBackgroundState,
 * getBackgroundPattern, mixColors e processInput, convertendo parâmetros e
 * retornos entre os tipos lua e c++. em erro de pcall, a mensagem é impressa
 * em stderr e a pilha lua é corrigida com lua_pop; onde aplicável, fallback em c++.
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

/*
 * destroi o estado lua se existir, libertando todos os recursos associados
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

    const char* files[] = {"background.lua", "mixer.lua", "controle.lua", "faces.lua"};
    const char* prefixes[] = {"lua/", ""};

    for (int i = 0; i < 4; i++) {
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
 * Invoca mixColors em Lua com a cor atual e o incremento
 * Os três retornos são escritos em newR, newG, newB. Se a função
 * não existir ou pcall falhar, usa fallback: componente a componente min(1, c + ac).
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
 * Passa o código da tecla (unsigned char) para processInput(key) em Lua.
 * Espera três valores de retorno (dx, dy, dz) em graus e aplica-os na rotação
 * do cubo com cube.rotate. Em função inexistente ou erro, imprime e limpa a pilha.
 */
void LuaBridge::lidarComEntrada(Cubo& cube, unsigned char key) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "lidarComEntrada");
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
        cube.rotacionar(dx, dy, dz);
        lua_pop(impl->L, 3);
    } else {
        std::cerr << "Erro ao chamar processInput: "
                  << lua_tostring(impl->L, -1) << std::endl;
        lua_pop(impl->L, 1);
    }
}

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
        // Erro ao chamar setFacePhoto: " << lua_tostring(impl->L, -1)
        lua_pop(impl->L, 1);
    }
}

// cycleMixMode removed — single additive mixing mode only

/*
 * Chama initStars(count) em Lua para gerar a tabela de estrelas com o LCG
 * determinístico. Deve ser chamada uma vez após bridge.init().
 */
void LuaBridge::inicializarEstrelas(int count) {
    if (!impl || !impl->L) return;
    lua_getglobal(impl->L, "inicializarEstrelas");
    if (!lua_isfunction(impl->L, -1)) {
        // Função initStars não encontrada!
        lua_pop(impl->L, 1);
        return;
    }
    lua_pushinteger(impl->L, count);
    if (lua_pcall(impl->L, 1, 0, 0) != LUA_OK) {
        // Erro em initStars: " << lua_tostring(impl->L, -1)
        lua_pop(impl->L, 1);
    }
}

/*
 * Chama getStarPositions(t) em Lua e lê a tabela flat retornada.
 * Cada estrela ocupa 5 entradas consecutivas: x, y, r, g, b.
 * O vetor 'out' é redimensionado e preenchido com esses valores.
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

    lua_pop(impl->L, 1);   // remove a tabela da pilha
}
