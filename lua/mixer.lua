--[[
  mixer.lua

  Sistema de cores aditivo simples para o Cube Editor.
  Três primárias: Vermelho (1), Azul (2), Verde (3).
  Mistura:
    Vermelho + Azul  = Roxo   (magenta)
    Vermelho + Verde = Amarelo
    Azul + Verde     = Ciano
    Roxo/Amarelo/Ciano + a cor complementar = Branco (R+G+B=1,1,1)

  Se a face estiver branca (todos >= 0.99), a nova cor substitui.
  Caso contrário, mistura aditiva limitada a 1.0.
]]

function mixColors(r, g, b, ar, ag, ab)
    -- Face branca: começa com a cor aplicada
    if r >= 0.99 and g >= 0.99 and b >= 0.99 then
        return ar, ag, ab
    end

    local newR = math.min(1.0, r + ar)
    local newG = math.min(1.0, g + ag)
    local newB = math.min(1.0, b + ab)

    return newR, newG, newB
end

-- Alias usado pelo lua_bridge
function mixColorsCurrent(r, g, b, ar, ag, ab)
    return mixColors(r, g, b, ar, ag, ab)
end

function describeColor(r, g, b)
    if r >= 0.99 and g >= 0.99 and b >= 0.99 then return "Branco"
    elseif r < 0.01 and g < 0.01 and b < 0.01 then return "Preto"
    elseif r > 0.8 and g < 0.2 and b < 0.2 then return "Vermelho"
    elseif r < 0.2 and g < 0.2 and b > 0.8 then return "Azul"
    elseif r < 0.2 and g > 0.8 and b < 0.2 then return "Verde"
    elseif r > 0.8 and g > 0.8 and b < 0.2 then return "Amarelo"
    elseif r > 0.8 and g < 0.2 and b > 0.8 then return "Roxo"
    elseif r < 0.2 and g > 0.8 and b > 0.8 then return "Ciano"
    else return "Misto"
    end
end
