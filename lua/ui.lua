--[[
  ui.lua

  Conteúdo textual da interface do Cube Editor.
  Mantém os textos, cores e espaçamentos das duas telas de UI fora do C++,
  permitindo editar mensagens e layout sem recompilar.

  Cada entrada da tabela tem:
    texto      : string a exibir
    r, g, b    : cor RGB (0.0–1.0)
    passo      : espaçamento vertical em pixels até a próxima linha
    centralizar: se verdadeiro, o C++ centraliza horizontalmente no painel
]]

--[[
  Retorna as linhas da tela de splash inicial.
  O C++ desenha o painel de fundo e itera sobre essa tabela para renderizar
  cada linha com glColor4f + glutBitmapCharacter.
]]
function obterLinhasSplash()
    return {
        { texto = "CUBO 3D",
          r = 0.9,  g = 0.9,  b = 1.0,  passo = 26, centralizar = false },

        { texto = "-------------------------------",
          r = 0.6,  g = 0.6,  b = 0.75, passo = 22, centralizar = false },

        { texto = "WASD         Rotacionar cubo",
          r = 0.85, g = 0.85, b = 0.9,  passo = 18, centralizar = false },

        { texto = "Click esq    Selecionar face",
          r = 0.85, g = 0.85, b = 0.9,  passo = 18, centralizar = false },

        { texto = "1-Verm  2-Azul  3-Verde  4-Preto",
          r = 0.85, g = 0.85, b = 0.9,  passo = 18, centralizar = false },

        { texto = "R            Limpar face",
          r = 0.85, g = 0.85, b = 0.9,  passo = 18, centralizar = false },

        { texto = "Backspace    Imagem na face",
          r = 0.85, g = 0.85, b = 0.9,  passo = 18, centralizar = false },

        { texto = "Seta Cima/Baixo  Zoom imagem",
          r = 0.85, g = 0.85, b = 0.9,  passo = 18, centralizar = false },

        { texto = "Seta Esq/Dir     Girar imagem",
          r = 0.85, g = 0.85, b = 0.9,  passo = 26, centralizar = false },

        { texto = "Pressione para comecar",
          r = 0.5,  g = 0.8,  b = 0.5,  passo = 18, centralizar = true  },
    }
end

--[[
  Retorna as linhas do painel flutuante de controles (botão "Controles").
  O C++ desenha o painel de fundo e itera sobre essa tabela.
]]
function obterLinhasControles()
    return {
        { texto = "Mouse esq: seleciona face",         r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Mouse dir: remove cor",             r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "WASD: rodar cubo",                  r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "1-Verm  2-Azul  3-Verde  4-Preto",  r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Seta Cima/Baixo: zoom imagem",      r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Seta esq/dir: girar imagem",        r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "R: resetar face",                   r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "Backspace: adicionar foto",         r = 0.85, g = 0.85, b = 0.88, passo = 16 },
        { texto = "ESC: sair",                         r = 0.85, g = 0.85, b = 0.88, passo = 16 },
    }
end
