--[[
  mixer.lua

  Lógica de mistura de cores usada pelo Cube Editor. mixColors é chamada pelo
  LuaBridge quando o utilizador pressiona 1 (vermelho), 2 (verde) ou 3 (azul):
  recebe a cor atual da face (r,g,b) e o incremento (ar,ag,ab) e devolve a nova
  cor. Se a face estiver branca (todos >= 0.99), substitui pela cor do incremento;
  caso contrário faz mistura aditiva limitada a 1.0. Inclui ainda funções
  auxiliares RGB/HSV, mistura subtrativa e descrição textual de cores para
  expansões futuras.
]]

--[[
  Mistura aditiva: se (r,g,b) for branco, devolve (ar,ag,ab). Senão devolve
  (min(1, r+ar), min(1, g+ag), min(1, b+ab)). Assim, vermelho + verde tende a
  amarelo e as cores não ultrapassam o canal máximo.
]]
function mixColors(r, g, b, ar, ag, ab)
    if r >= 0.99 and g >= 0.99 and b >= 0.99 then
        return ar, ag, ab
    end

    local newR = math.min(1.0, r + ar)
    local newG = math.min(1.0, g + ag)
    local newB = math.min(1.0, b + ab)

    return newR, newG, newB
end

--[[
  Converte RGB (0–1) em HSV: H em graus (0–360), S e V em 0–1. Usado para
  manipulações no espaço HSV em futuras implementações (paletas, saturação).
]]
function rgbToHsv(r, g, b)
    local max = math.max(r, g, b)
    local min = math.min(r, g, b)
    local delta = max - min

    local h, s, v = 0, 0, max

    if delta > 0 then
        s = delta / max

        if max == r then
            h = ((g - b) / delta) % 6
        elseif max == g then
            h = (b - r) / delta + 2
        else
            h = (r - g) / delta + 4
        end

        h = h * 60
        if h < 0 then h = h + 360 end
    end

    return h, s, v
end

--[[
  Converte HSV (H em graus, S e V em 0–1) em RGB (0–1). Implementação padrão
  por setores de 60° para o matiz.
]]
function hsvToRgb(h, s, v)
    local c = v * s
    local x = c * (1 - math.abs((h / 60) % 2 - 1))
    local m = v - c

    local r, g, b = 0, 0, 0

    if h >= 0 and h < 60 then
        r, g, b = c, x, 0
    elseif h >= 60 and h < 120 then
        r, g, b = x, c, 0
    elseif h >= 120 and h < 180 then
        r, g, b = 0, c, x
    elseif h >= 180 and h < 240 then
        r, g, b = 0, x, c
    elseif h >= 240 and h < 300 then
        r, g, b = x, 0, c
    else
        r, g, b = c, 0, x
    end

    return r + m, g + m, b + m
end

--[[
  Mistura subtrativa (modelo simplificado CMYK): converte RGB em complementos,
  soma os complementos limitando a 1, e converte de volta para RGB. Útil para
  simular tintas ou impressão.
]]
function mixColorsSubtractive(r, g, b, ar, ag, ab)
    if r >= 0.99 and g >= 0.99 and b >= 0.99 then
        return ar, ag, ab
    end

    local c1 = 1.0 - r
    local m1 = 1.0 - g
    local y1 = 1.0 - b

    local c2 = 1.0 - ar
    local m2 = 1.0 - ag
    local y2 = 1.0 - ab

    local c = math.min(1.0, c1 + c2)
    local m = math.min(1.0, m1 + m2)
    local y = math.min(1.0, y1 + y2)

    return 1.0 - c, 1.0 - m, 1.0 - y
end

--[[
  Devolve uma string descritiva da cor (Branco, Preto, Vermelho, etc.) com base
  em limiares RGB. Usado para debug ou futura interface textual.
]]
function describeColor(r, g, b)
    if r >= 0.99 and g >= 0.99 and b >= 0.99 then
        return "Branco"
    elseif r < 0.01 and g < 0.01 and b < 0.01 then
        return "Preto"
    elseif r > 0.8 and g < 0.2 and b < 0.2 then
        return "Vermelho"
    elseif r < 0.2 and g > 0.8 and b < 0.2 then
        return "Verde"
    elseif r < 0.2 and g < 0.2 and b > 0.8 then
        return "Azul"
    elseif r > 0.8 and g > 0.8 and b < 0.2 then
        return "Amarelo"
    elseif r > 0.8 and g < 0.2 and b > 0.8 then
        return "Magenta/Roxo"
    elseif r < 0.2 and g > 0.8 and b > 0.8 then
        return "Ciano"
    else
        return "Misto"
    end
end
