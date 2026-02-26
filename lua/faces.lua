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
  Resolve a face clicada a partir do valor do canal R lido pelo glReadPixels.
  O cubo renderiza cada face com glColor3ub(indice+1, 0, 0), então o R lido
  vale 1–6 para as faces 0–5, e 0 para o fundo.
  Retorna o índice da face (0–5) ou -1 se nenhuma face foi atingida.
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

