--[[
  ui.lua

  Textos do painel de controles. Manter aqui evita strings hardcoded no C++ e
  permite ajustar mensagens e cores sem recompilar.
]]

function obterLinhasControles()
    return {
        { texto = "WASD: rodar cubo", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Mouse esq: seleciona face", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Mouse dir: remove cor", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "1-Verm  2-Azul  3-Verde  4-Preto", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Backspace: adicionar foto", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Seta esq/dir: girar imagem", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Seta Cima/Baixo: zoom imagem", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "R: resetar face", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "ESC: sair", r = 0.85, g = 0.85, b = 0.88, passo = 16 },
    }
end
