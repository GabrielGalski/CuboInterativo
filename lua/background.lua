--[[
background.lua:
geração e animação do campo de estrelas. inicializarEstrelas(n) cria a tabela
com posições e propriedades usando um LCG com seed fixa.
obterPosicoesEstrelas recalcula posição e cor para o instante t e
devolve uma tabela flat com cinco valores por estrela que o C++
lê diretamente para glVertex2f e glColor3f.
]]

local estrelas = {}
local TWO_PI = 2.0 * math.pi

local estadoLCG = 1337

local function geradorLCG()
    estadoLCG = (estadoLCG * 1664525 + 1013904223) & 0xFFFFFFFF
    return estadoLCG
end

local function aleatorio01()
    return (geradorLCG() & 0x00FFFFFF) / 16777215.0
end

function inicializarEstrelas(quantidade)
    estadoLCG = 1337
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
calcula posição e cor de todas as estrelas para o instante t. Cada estrela
deriva para baixo em loop, oscila horizontalmente com seno e rotaciona ao
redor do centro da tela a cada 120 segundos. O brilho cintila com seno
individual por estrela.
]]
function obterPosicoesEstrelas(tempo)
    local angulo = tempo * (TWO_PI / 120.0)
    local cossenoAngulo  = math.cos(angulo)
    local senoAngulo  = math.sin(angulo)

    local resultado = {}
    local indice = 0

    for _, estrela in ipairs(estrelas) do
        local yDerivado = estrela.y - (tempo * estrela.velocidade) % 1.0
        if yDerivado < 0.0 then yDerivado = yDerivado + 1.0 end

        local desvio = math.sin(tempo * 0.15 + estrela.cintilar) * 0.01
        local xDerivado = estrela.x + desvio

        local relativoX = xDerivado - 0.5
        local relativoY = yDerivado - 0.5
        local rotacionadoX = relativoX * cossenoAngulo - relativoY * senoAngulo + 0.5
        local rotacionadoY = relativoX * senoAngulo + relativoY * cossenoAngulo + 0.5

        rotacionadoX = rotacionadoX - math.floor(rotacionadoX)
        rotacionadoY = rotacionadoY - math.floor(rotacionadoY)

        local cintilacao = 0.35 + 0.65 * (0.5 + 0.5 * math.sin(tempo * 2.0 + estrela.cintilar))
        local brilhoFinal  = estrela.brilho * cintilacao

        resultado[indice + 1] = rotacionadoX
        resultado[indice + 2] = rotacionadoY
        resultado[indice + 3] = brilhoFinal * 0.85
        resultado[indice + 4] = brilhoFinal * 0.90
        resultado[indice + 5] = brilhoFinal
        indice = indice + 5
    end

    return resultado
end

local bgState = { colorIndex = 0, model = 1 }
function getBackgroundState() return bgState.colorIndex, bgState.model end
function setBackgroundState(i, m) bgState.colorIndex = i; bgState.model = m end
