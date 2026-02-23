# Cubo 3D — C++ & Lua

Aplicação de um cubo 3D interativo desenvolvido em C++ com OpenGL/GLUT e lógica de scripting em Lua. 

C++: cuida da renderização gráfica e da estrutura da aplicação. 
Lua: controla a movimentação do cubo, animação das estrelas, processamento de entrada, mistura de cores e os padrões das faces do cubo.

## Estrutura do repositório

O projeto tem todos os fontes C++ e os scripts Lua separados por diretório.

src/main.cpp é o ponto de entrada da aplicação. Inicializa o GLUT, cria a janela principal (e a janela de benchmark quando compilado em modo bench) e registra os callbacks de teclado, mouse, reshape e display.

src/cubo.cpp e src/cubo.h definem a classe Cubo, responsável por todo o desenho 3D, color picking por face, carregamento de texturas via stb_image e controle de rotação.

src/background.cpp e src/background.h implementam o fundo estrelado. Toda a matemática das estrelas vive em Lua; o C++ apenas solicita as posições calculadas ao bridge e as desenha com GL_POINTS.

src/lua_bridge.cpp e src/lua_bridge.h são o bridge entre C++ e Lua. Mantém o estado lua_State, carrega os scripts e expõe chamadas como initStars, getStarPositions, mixColor, handleInput e toggleFacePattern.

src/bench.cpp e src/bench.h implementam o monitor de performance que abre como segunda janela quando o programa é compilado com make bench. Mede FPS, tempo de frame, tempo isolado de Lua vs C++, uso de memória RSS e cache de texturas.

stb_image.h é uma biblioteca single-header de domínio público (autor: Sean Barrett) usada para decodificar imagens PNG, JPG, BMP e outros formatos e aplicá-las como texturas nas faces do cubo. Não faz parte do apt — deve ser baixada manualmente antes de compilar.

lua/ contém os quatro scripts Lua do projeto. background.lua gera e anima as estrelas com um LCG determinístico. controle.lua mapeia teclas WASD a incrementos de rotação. mixer.lua implementa mistura de cores aditiva. faces.lua armazena o padrão e o caminho de foto de cada face.

## Dependências (Linux/Ubuntu)

Antes de compilar, instale os pacotes necessários com:

    sudo apt update
    sudo apt install build-essential libglu1-mesa-dev freeglut3-dev liblua5.4-dev zenity

### stb_image.h 

O suporte a imagens depende da biblioteca stb_image.h.

    curl -L "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
         -o "stb_image.h" \
         && echo "stb_image.h baixado com sucesso!" \
         || echo "Falha no download. Acesse: https://github.com/nothings/stb"

sem esse arquivo a funcionalidade de aplicar fotos nas faces do cubo ficará desativada.

## Como compilar

Para o build normal:

    make

Para o build com o monitor de performance:

    make bench

Para compilar e já executar em um único comando:

    make run
    ou
    make bench run

## Como executar

Após compilar, rode diretamente:

    ./cubo          # build normal
    ./cubo_bench    # build com monitor de performance

## Controles

* WASD rotaciona o cubo. 
* Clique esquerdo seleciona uma face. 
* As teclas 1, 2, 3 e 4 aplicam vermelho, azul, verde e preto à face selecionada (mistura aditiva via Lua). 
* R limpa a face selecionada. 
* Delete abre um seletor de arquivo para aplicar uma foto na face; 
* as setas para cima e para baixo ajustam o zoom da imagem e as setas laterais a giram. 
* H exibe o painel de controles.
* ESC encerra o programa.
