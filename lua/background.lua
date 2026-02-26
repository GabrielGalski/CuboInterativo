--[[
  background.lua

  Geração e animação das estrelas em Lua.
  initStars(count) cria a tabela com posições e propriedades aleatórias,
  usando um LCG determinístico (mesma seed = mesmo céu toda vez).
  getStarPositions(t) calcula, para o tempo t (segundos), a posição
  e cor de cada estrela com a mesma lógica que estava em C++:
    - Deriva vertical com loop
    - Oscilação horizontal suave (seno)
    - Rotação global ao redor do centro (0.5, 0.5)
    - Cintilamento via seno com fase individual
  Retorna uma tabela flat: { x1,y1,r1,g1,b1, x2,y2,r2,g2,b2, ... }
]]

local estrelas = {}
local TWO_PI = 2.0 * math.pi

-- LCG idêntico ao usado em C++: mesmos parâmetros de Knuth
local estadoLCG = 1337

local function geradorLCG()
    estadoLCG = (estadoLCG * 1664525 + 1013904223) & 0xFFFFFFFF
    return estadoLCG
end

local function aleatorio01()
    return (geradorLCG() & 0x00FFFFFF) / 16777215.0
end

--[[
  Gera 'quantidade' estrelas com posição, velocidade, brilho e fase aleatórios.
  Deve ser chamada uma vez pelo C++ logo após carregar o script.
]]
function inicializarEstrelas(quantidade)
    estadoLCG = 1337   -- garante reprodutibilidade
    estrelas = {}
    for i = 1, quantidade do
        local profundidade = aleatorio01()
        estrelas[i] = {
            x          = aleatorio01(),
            y          = aleatorio01(),
            velocidade      = 0.02 + profundidade * 0.10,
            brilho = 0.45 + (1.0 - profundidade) * 0.55,
            cintilar    = aleatorio01() * 6.2831853,
        }
    end
end

--[[
  Calcula a posição e cor de todas as estrelas para o instante t (segundos).
  Retorna tabela flat com 5 valores por estrela: x, y, r, g, b.
  O C++ lê essa tabela e chama glVertex2f / glColor3f diretamente.
]]
function obterPosicoesEstrelas(tempo)
    -- Ângulo de rotação global: uma volta completa a cada 120 s
    local angulo = tempo * (TWO_PI / 120.0)
    local cossenoAngulo  = math.cos(angulo)
    local senoAngulo  = math.sin(angulo)

    local resultado = {}
    local indice = 0

    for _, estrela in ipairs(estrelas) do
        -- 1. Deriva vertical com loop
        local yDerivado = estrela.y - (tempo * estrela.velocidade) % 1.0
        if yDerivado < 0.0 then yDerivado = yDerivado + 1.0 end

        -- 2. Oscilação horizontal suave
        local desvio = math.sin(tempo * 0.15 + estrela.cintilar) * 0.01
        local xDerivado = estrela.x + desvio

        -- 3. Rotação ao redor de (0.5, 0.5)
        local relativoX = xDerivado - 0.5
        local relativoY = yDerivado - 0.5
        local rotacionadoX = relativoX * cossenoAngulo - relativoY * senoAngulo + 0.5
        local rotacionadoY = relativoX * senoAngulo + relativoY * cossenoAngulo + 0.5

        -- Wrap para manter dentro de [0, 1]
        rotacionadoX = rotacionadoX - math.floor(rotacionadoX)
        rotacionadoY = rotacionadoY - math.floor(rotacionadoY)

        -- 4. Cintilamento
        local cintilacao = 0.35 + 0.65 * (0.5 + 0.5 * math.sin(tempo * 2.0 + estrela.cintilar))
        local brilhoFinal  = estrela.brilho * cintilacao

        -- Empacota: x, y, r, g, b (leve dominante azul, igual ao C++)
        resultado[indice + 1] = rotacionadoX
        resultado[indice + 2] = rotacionadoY
        resultado[indice + 3] = brilhoFinal * 0.85
        resultado[indice + 4] = brilhoFinal * 0.90
        resultado[indice + 5] = brilhoFinal
        indice = indice + 5
    end

    return resultado
end

-- Estado legado (mantido para compatibilidade com updateBackground no bridge)
local bgState = { colorIndex = 0, model = 1 }
function getBackgroundState() return bgState.colorIndex, bgState.model end
function setBackgroundState(i, m) bgState.colorIndex = i; bgState.model = m end
