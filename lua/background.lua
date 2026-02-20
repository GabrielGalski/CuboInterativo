--[[
  background.lua

  Mantém o estado do background (índice de cor e modo).
]]

local bgState = {
    colorIndex = 1,
    model = 0
}

--[[
  Retorna o índice da cor atual e o modo de desenho:
  0 = sólido, 1 = espaço.
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
