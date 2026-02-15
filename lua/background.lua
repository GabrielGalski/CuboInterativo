--[[
  background.lua

  Mantém o estado do background (índice de cor e modo) e calcula os parâmetros
  dos padrões matemáticos dinâmicos. getBackgroundState() devolve o estado
  atual para o C++. getBackgroundPattern(timeMs) recebe o tempo em milissegundos
  e devolve tipo do padrão (1 Lissajous, 2 espiral, 3 ondas) e dois parâmetros
  (frequências) que variam suavemente com o tempo e ciclam a cada 12 segundos
  entre os três tipos, gerando transição contínua entre as formas.
]]

local bgState = {
    colorIndex = 1,
    model = 0
}

--[[
  Retorna o índice da cor atual e o modo de desenho (0 sólido, 1 gradiente,
  2 matemático) para o C++ sincronizar o objeto Background.
]]
function getBackgroundState()
    return bgState.colorIndex, bgState.model
end

--[[
  Atualiza o estado interno do background com o índice de cor e o modo.
  Usado quando o C++ quiser forçar um estado (ex.: após carregar configuração).
]]
function setBackgroundState(index, model)
    bgState.colorIndex = index
    bgState.model = model
end

--[[
  Calcula o padrão matemático a ser desenhado no frame atual. timeMs é o
  tempo desde o início da aplicação. Divide o tempo em ciclos de 12 segundos
  e escolhe patternType 1, 2 ou 3 conforme a fase (0–33%, 33–66%, 66–100%).
  freq1 e freq2 variam com seno/cosseno do tempo para dar movimento orgânico
  às curvas (Lissajous, espiral e onda). Retorna patternType, freq1, freq2.
]]
function getBackgroundPattern(timeMs)
    local t = timeMs / 1000.0
    local cycle = 12.0
    local phase = (t % cycle) / cycle
    local patternType = 1
    if phase < 0.33 then
        patternType = 1
    elseif phase < 0.66 then
        patternType = 2
    else
        patternType = 3
    end
    local freq1 = 2.0 + math.sin(t * 0.5) * 0.5
    local freq2 = 3.0 + math.cos(t * 0.3) * 0.5
    return patternType, freq1, freq2
end
