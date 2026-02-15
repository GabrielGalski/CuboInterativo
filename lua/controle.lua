--[[
  controle.lua

  Mapeia a entrada de teclado (WASD) para incrementos de rotação do cubo.
  processInput(key) é chamada pelo LuaBridge com o código ASCII da tecla;
  devolve três valores (dx, dy, dz) em graus para serem aplicados às rotações
  em X, Y e Z. W/S alteram rotX (inclinação para cima/baixo), A/D alteram rotY
  (rotação horizontal). O passo fixo é 15 graus por tecla.
]]

--[[
  Converte o código da tecla para minúscula e devolve (dx, dy, dz): W -15 em X,
  S +15 em X, A -15 em Y, D +15 em Y. Teclas não mapeadas devolvem (0, 0, 0).
]]
function processInput(key)
    local dx, dy, dz = 0, 0, 0
    local step = 15.0

    local char = string.char(key):lower()

    if char == 'w' then
        dx = -step
    elseif char == 's' then
        dx = step
    elseif char == 'a' then
        dy = -step
    elseif char == 'd' then
        dy = step
    end

    return dx, dy, dz
end
