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

local stars = {}
local TWO_PI = 2.0 * math.pi

-- LCG idêntico ao usado em C++: mesmos parâmetros de Knuth
local lcgState = 1337

local function lcg()
    lcgState = (lcgState * 1664525 + 1013904223) & 0xFFFFFFFF
    return lcgState
end

local function rand01()
    return (lcg() & 0x00FFFFFF) / 16777215.0
end

--[[
  Gera 'count' estrelas com posição, velocidade, brilho e fase aleatórios.
  Deve ser chamada uma vez pelo C++ logo após carregar o script.
]]
function initStars(count)
    lcgState = 1337   -- garante reprodutibilidade
    stars = {}
    for i = 1, count do
        local depth = rand01()
        stars[i] = {
            x          = rand01(),
            y          = rand01(),
            speed      = 0.02 + depth * 0.10,
            brightness = 0.45 + (1.0 - depth) * 0.55,
            twinkle    = rand01() * 6.2831853,
        }
    end
end

--[[
  Calcula a posição e cor de todas as estrelas para o instante t (segundos).
  Retorna tabela flat com 5 valores por estrela: x, y, r, g, b.
  O C++ lê essa tabela e chama glVertex2f / glColor3f diretamente.
]]
function getStarPositions(t)
    -- Ângulo de rotação global: uma volta completa a cada 120 s
    local angle = t * (TWO_PI / 120.0)
    local cosA  = math.cos(angle)
    local sinA  = math.sin(angle)

    local result = {}
    local idx = 0

    for _, s in ipairs(stars) do
        -- 1. Deriva vertical com loop
        local yy = s.y - (t * s.speed) % 1.0
        if yy < 0.0 then yy = yy + 1.0 end

        -- 2. Oscilação horizontal suave
        local drift = math.sin(t * 0.15 + s.twinkle) * 0.01
        local xx = s.x + drift

        -- 3. Rotação ao redor de (0.5, 0.5)
        local rx = xx - 0.5
        local ry = yy - 0.5
        local rxR = rx * cosA - ry * sinA + 0.5
        local ryR = rx * sinA + ry * cosA + 0.5

        -- Wrap para manter dentro de [0, 1]
        rxR = rxR - math.floor(rxR)
        ryR = ryR - math.floor(ryR)

        -- 4. Cintilamento
        local tw = 0.35 + 0.65 * (0.5 + 0.5 * math.sin(t * 2.0 + s.twinkle))
        local b  = s.brightness * tw

        -- Empacota: x, y, r, g, b (leve dominante azul, igual ao C++)
        result[idx + 1] = rxR
        result[idx + 2] = ryR
        result[idx + 3] = b * 0.85
        result[idx + 4] = b * 0.90
        result[idx + 5] = b
        idx = idx + 5
    end

    return result
end

-- Estado legado (mantido para compatibilidade com updateBackground no bridge)
local bgState = { colorIndex = 0, model = 1 }
function getBackgroundState() return bgState.colorIndex, bgState.model end
function setBackgroundState(i, m) bgState.colorIndex = i; bgState.model = m end
