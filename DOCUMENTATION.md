# Documentação do Cube Editor

Este documento descreve a arquitetura, o fluxo de execução e o funcionamento de cada módulo e função do projeto Cube Editor (cubo 3D em C++/OpenGL com Lua para lógica de negócio e background dinâmico).

---

## Visão geral do sistema

A aplicação é uma janela OpenGL/GLUT que exibe um cubo 3D colorível e um background configurável. O utilizador pode rotacionar o cubo (WASD), selecionar uma face com o rato (color picking), alterar a cor da face com as teclas 1/2/3 (mistura em Lua) e mudar o fundo (setas: cor e modo). O modo de fundo pode ser sólido, gradiente ou formas matemáticas dinâmicas (Lissajous, espiral, ondas), cuja escolha e parâmetros são calculados em Lua a cada frame.

**Fluxo principal:** `main` inicia GLUT, regista callbacks e chama `init()`, que inicializa o LuaBridge (carrega `background.lua`, `mixer.lua`, `controle.lua`), define rotação inicial do cubo e estado do background. O loop GLUT chama `display()` a cada frame: limpa o buffer, atualiza o padrão do background via Lua quando o modo é matemático, desenha o background e o cubo. Teclado e rato disparam `keyboard`, `specialKeys` e `mouse`, que atualizam o cubo ou o background e pedem redesenho.

---

## Módulos C++

### main.cpp

**Responsabilidade:** Ponto de entrada, criação da janela, registo de callbacks e ciclo de desenho.

- **Variáveis globais:** `Cubo cube`, `Background background`, `LuaBridge bridge` — instâncias únicas usadas nos callbacks.

- **display()**  
  Limpa cor e profundidade. Se o modo do background for 2 (matemático), chama `bridge.updateBackgroundPattern(background, glutGet(GLUT_ELAPSED_TIME))` para obter tipo e parâmetros do padrão em Lua. Chama `background.render()` (fundo em 2D orto), depois aplica identidade e translação da câmera e `cube.render()`. Finaliza com `glutSwapBuffers()`.

- **keyboard(key, x, y)**  
  W/A/S/D: delega para `bridge.handleInput(cube, key)` (Lua devolve dx, dy, dz e o cubo roda). 1/2/3: lê a cor da face selecionada, chama `bridge.mixColor` com o incremento (vermelho/verde/azul) e aplica a nova cor com `cube.setFaceColor`. R: `cube.clearSelectedFace()`. ESC: `exit(0)`. Sempre `glutPostRedisplay()`.

- **specialKeys(key, x, y)**  
  Seta esquerda/direita: `background.previousColor()` / `nextColor()`. Seta cima: `background.nextModel()` (cicla sólido → gradiente → matemático). `glutPostRedisplay()`.

- **mouse(button, state, x, y)**  
  Só reage a `GLUT_DOWN`. Botão esquerdo: `cube.selectCurrentFace(x, y)` (color picking). Botão direito: `cube.clearSelectedFace()`. `glutPostRedisplay()`.

- **init()**  
  Ativa `GL_DEPTH_TEST` e `GL_LESS`. Chama `bridge.init()`; em falha, imprime erro e sai. Define `cube.setRotation(0, 25, 0)`, `background.setDefault()`, `glClearColor(0,0,0,1)` e imprime no console o guia de controles (incluindo modo “formas matemáticas”).

- **reshape(w, h)**  
  Evita h=0, define viewport e projeção perspectiva (45°, aspect w/h, near 1, far 100).

- **main(argc, argv)**  
  Inicializa GLUT (duplo buffer, RGB, profundidade), janela 800×600, chama `init()`, regista `display`, `reshape`, `keyboard`, `specialKeys`, `mouse` e entra em `glutMainLoop()`.

---

### lua_bridge.h / lua_bridge.cpp

**Responsabilidade:** Manter um estado Lua, carregar scripts e expor chamadas C++ → Lua para background, mistura de cores e entrada.

- **LuaBridge()**  
  Construtor: `L` = nullptr.

- **~LuaBridge()**  
  Se `L` existir, `lua_close(L)`.

- **init()**  
  Cria estado com `luaL_newstate()`, abre bibliotecas com `luaL_openlibs(L)`. Para cada ficheiro em `{"background.lua","mixer.lua","controle.lua"}`, tenta carregar em `lua/` e no diretório atual com `luaL_dofile`. Se algum falhar, imprime o erro (lua_tostring(L,-1)), faz `lua_pop(L,1)` e retorna false. Caso contrário retorna true.

- **updateBackground(bg)**  
  Obtém a função global `getBackgroundState`, chama com 0 argumentos e 2 retornos. Converte os retornos em número (colorIndex, model) e chama `bg.setColorIndex` e `bg.setModel`. Em erro de pcall, imprime e faz pop do valor de erro.

- **updateBackgroundPattern(bg, timeMs)**  
  Obtém `getBackgroundPattern`, faz push de `timeMs`, pcall com 1 arg e 3 retornos. Lê tipo (inteiro) e dois parâmetros (número), chama `bg.setPattern(ptype, p1, p2)` e faz pop dos três. Se a função não existir, apenas faz pop e retorna.

- **mixColor(r, g, b, ar, ag, ab, newR, newG, newB)**  
  Chama a função global `mixColors` com os seis números. Espera três retornos e escreve em newR, newG, newB (ordem na pilha: -3, -2, -1). Se a função não existir ou pcall falhar, usa fallback aditivo: newR = min(1, r+ar), idem para G e B.

- **handleInput(cube, key)**  
  Chama `processInput` com um argumento (código da tecla). Espera três retornos numéricos (dx, dy, dz) e chama `cube.rotate(dx, dy, dz)`. Em erro, imprime e faz pop.

---

### background.h / background.cpp

**Responsabilidade:** Desenhar o fundo da cena em três modos (sólido, gradiente, matemático) e manter estado de cor e padrão.

- **BgColor:** struct com `r, g, b` (float).

- **Background()**  
  Inicializa índices e parâmetros; preenche o vetor `colors` com sete cores (branco, preto, laranja, rosa, azul escuro, vermelho escuro, verde escuro).

- **setDefault()**  
  `currentColorIndex = 1` (preto), `currentModel = 0` (sólido).

- **render()**  
  Desativa depth test, empilha projeção orto (0–1) e modelo identidade. Consoante `currentModel`: 0 — um quad com a cor atual; 1 — quad com gradiente vertical (metade escura em baixo); 2 — quad escuro (cor×0.2) e depois uma GL_LINE_STRIP com 401 vértices. No modo 2, usa `glutGet(GLUT_ELAPSED_TIME)` e `patternType`/`patternParam1`/`patternParam2`: tipo 1 = Lissajous (sin(f1*u), sin(f2*u)), tipo 2 = espiral (raio crescente com ângulo), tipo 3 = onda ao longo de x (produto de senos). Restaura matrizes e reativa depth test.

- **nextColor() / previousColor()**  
  Incrementa ou decrementa `currentColorIndex` com módulo pelo tamanho de `colors`.

- **nextModel()**  
  `currentModel = (currentModel + 1) % 3`.

- **setColorIndex(index)**  
  Atualiza `currentColorIndex` se index estiver em [0, size).

- **setModel(model)**  
  Atualiza `currentModel` se model estiver em [0, 2].

- **setPattern(type, param1, param2)**  
  Atualiza `patternType`, `patternParam1` e `patternParam2` para o desenho do modo 2.

- **getModel()**  
  Retorna `currentModel` (inline no .h).

---

### cubo.h / cubo.cpp

**Responsabilidade:** Representar o cubo 3D (seis faces coloridas, rotação) e implementar seleção de face por color picking.

- **Color:** struct com `r, g, b` (float).

- **Cubo()**  
  Rotações e `selectedFace` a zero; as seis `faceColors` inicializadas a branco.

- **render()**  
  Empilha matriz, aplica `glRotatef` para rotX, rotY, rotZ, desenha seis GL_QUADS com as cores de `faceColors` (vértices do cubo -1 a 1), desempilha matriz.

- **rotate(dx, dy, dz)**  
  Soma dx, dy, dz a rotX, rotY, rotZ.

- **setRotation(x, y, z)**  
  Substitui rotX, rotY, rotZ por x, y, z.

- **addColorToCurrentFace(r, g, b)**  
  Se a face selecionada for branca (todos >= 0.99), substitui pela (r,g,b); senão soma aditivamente e limita a 1.0 por canal.

- **clearSelectedFace()**  
  Coloca a face selecionada em branco (1,1,1).

- **setFaceColor(face, r, g, b)**  
  Se face em [0,5], atualiza `faceColors[face]`.

- **getSelectedFace() / getFaceColor(face)**  
  Getters inline.

- **selectCurrentFace(x, y)**  
  Obtém viewport; limpa buffers e desativa iluminação/textura; mantém projeção, empilha modelo, aplica translação -5 em Z e as mesmas rotações do cubo; desenha as seis faces com `glColor3ub(faceId, 0, 0)` (faceId 1–6). `glReadPixels(x, viewport[3]-y-1, ...)` lê o pixel; o componente R (1–6) dá a face (selectedFace = R - 1). Restaura matrizes, imprime no console o resultado e limpa o buffer para o desenho normal.

---

## Módulos Lua

### background.lua

- **bgState:** tabela com `colorIndex` e `model`.

- **getBackgroundState()**  
  Retorna `colorIndex` e `model` para o C++.

- **setBackgroundState(index, model)**  
  Atualiza `bgState`.

- **getBackgroundPattern(timeMs)**  
  Converte tempo em segundos; usa ciclo de 12 s para alternar entre patternType 1, 2 e 3 (0–33%, 33–66%, 66–100% do ciclo). Calcula freq1 e freq2 com pequenas variações sinusoidais no tempo. Retorna patternType, freq1, freq2.

### mixer.lua

- **mixColors(r, g, b, ar, ag, ab)**  
  Se (r,g,b) for branco, retorna (ar,ag,ab); senão retorna (min(1,r+ar), min(1,g+ag), min(1,b+ab)).

- **rgbToHsv / hsvToRgb / mixColorsSubtractive / describeColor**  
  Funções auxiliares para conversão e descrição de cores (uso futuro ou debug).

### controle.lua

- **processInput(key)**  
  Converte key em carácter minúsculo; W/S devolvem dx = -15/15, A/D devolvem dy = -15/15 (dz = 0). Retorna (dx, dy, dz).

---

## Build e execução

- **Makefile:** compila todos os `.cpp` em `src/` para `obj/*.o` e liga com OpenGL, GLU, GLUT e Lua (pkg-config lua5.4 ou lua5.3). O executável é `cube_editor`. Os scripts Lua devem estar em `lua/` ou no diretório de execução.

- **Documentação no código:** Cada ficheiro fonte (.cpp, .h) e cada script Lua contém no início um bloco de comentário que descreve o papel do ficheiro e o funcionamento; antes de cada função há um bloco que descreve o comportamento e o uso dos parâmetros/retornos. Não são usadas anotações de uma linha no código.

---

## Resumo das alterações realizadas

1. **Renomeação:** `ligacao.cpp` / `ligacao.h` → `lua_bridge.cpp` / `lua_bridge.h`; referências em `main.cpp` e documentação atualizadas.

2. **Background dinâmico:** Modo 2 (matemático) com padrões Lissajous, espiral e ondas; lógica de escolha e parâmetros em `background.lua` (`getBackgroundPattern(timeMs)`); C++ desenha as curvas em `Background::render()` usando `setPattern` chamado por `updateBackgroundPattern`.

3. **Documentação:** Comentários de bloco no início de cada ficheiro e antes de cada função, descrevendo funcionamento e fluxo; remoção de comentários de uma linha no código.

4. **Correções:** Validação de `setModel` e `setColorIndex` com intervalos; uso de `(int)colors.size()` em módulos para evitar problemas de tipo; ciclo de modos do background em 3 (sólido, gradiente, matemático) e texto de ajuda no console atualizado.
