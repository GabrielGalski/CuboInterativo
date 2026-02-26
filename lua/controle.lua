--[[
  controle.lua
mapeia WASD para incrementos de rotação do cubo. lidarComEntrada(tecla) recebe
o código ASCII da tecla e devolve (dx, dy, dz) em graus 
W/S inclinam em X, A/D giram em Y e passo fixo de 15 graus por tecla.
]]

function lidarComEntrada(tecla)
    local deltaX, deltaY, deltaZ = 0, 0, 0
    local passo = 15.0

    local caractere = string.char(tecla):lower()

    if caractere == 'w' then
        deltaX = -passo
    elseif caractere == 's' then
        deltaX = passo
    elseif caractere == 'a' then
        deltaY = -passo
    elseif caractere == 'd' then
        deltaY = passo
    end

    return deltaX, deltaY, deltaZ
end
