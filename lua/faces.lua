--[[
  faces.lua
Mantém o estado das seis faces do cubo no lado Lua: por enquanto só o caminho
da foto carregada em cada face. resolverFacePicking converte o byte R lido pelo
glReadPixels no índice da face clicada.
]]

local faces = {}

for i = 0, 5 do
    faces[i] = {
        caminhoFoto = nil
    }
end

local function limitarIndice(indice)
    if indice < 0 then return 0 end
    if indice > 5 then return 5 end
    return indice
end

--[[
o cubo renderiza cada face com glColor3ub durante o picking,
então o canal R lido vale 1–6 para as faces 0–5 e 0 para o fundo.
Retorna o índice da face ou -1 se o clique não acertou nenhuma.
]]
function resolverFacePicking(valorPixelR)
    if valorPixelR >= 1 and valorPixelR <= 6 then
        return valorPixelR - 1
    end
    return -1
end

function definirFotoFace(indiceFace, caminho)
    local indice = limitarIndice(indiceFace)
    local face = faces[indice]
    if not face then
        return
    end
    face.caminhoFoto = caminho
end
