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

function definirFotoFace(indiceFace, caminho)
    local indice = limitarIndice(indiceFace)
    local face = faces[indice]
    if not face then
        return
    end
    face.caminhoFoto = caminho
end

