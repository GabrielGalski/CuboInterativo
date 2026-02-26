--[[
  mixer.lua

  Mistura de cores aditiva para as faces do cubo. Três primárias: Vermelho,
  Azul, Verde, cores secundárias surgem naturalmente da mistura. Se a face já
  estiver branca a nova cor substitui em vez de somar.
]]

function misturarCores(vermelho, verde, azul, adicionarVermelho, adicionarVerde, adicionarAzul)
    if vermelho >= 0.99 and verde >= 0.99 and azul >= 0.99 then
        return adicionarVermelho, adicionarVerde, adicionarAzul
    end

    local novoVermelho = math.min(1.0, vermelho + adicionarVermelho)
    local novoVerde = math.min(1.0, verde + adicionarVerde)
    local novoAzul = math.min(1.0, azul + adicionarAzul)

    return novoVermelho, novoVerde, novoAzul
end

-- Alias chamado pelo lua_bridge
function mixColorsCurrent(vermelho, verde, azul, adicionarVermelho, adicionarVerde, adicionarAzul)
    return misturarCores(vermelho, verde, azul, adicionarVermelho, adicionarVerde, adicionarAzul)
end

function descreverCor(vermelho, verde, azul)
    if vermelho >= 0.99 and verde >= 0.99 and azul >= 0.99 then return "Branco"
    elseif vermelho < 0.01 and verde < 0.01 and azul < 0.01 then return "Preto"
    elseif vermelho > 0.8 and verde < 0.2 and azul < 0.2 then return "Vermelho"
    elseif vermelho < 0.2 and verde < 0.2 and azul > 0.8 then return "Azul"
    elseif vermelho < 0.2 and verde > 0.8 and azul < 0.2 then return "Verde"
    elseif vermelho > 0.8 and verde > 0.8 and azul < 0.2 then return "Amarelo"
    elseif vermelho > 0.8 and verde < 0.2 and azul > 0.8 then return "Roxo"
    elseif vermelho < 0.2 and verde > 0.8 and azul > 0.8 then return "Ciano"
    else return "Misto"
    end
end
