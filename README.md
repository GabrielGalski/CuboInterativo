# Cube Editor - C++ & Lua Integration

Um editor de cubo 3D interativo desenvolvido com C++ (OpenGL/GLUT) e Lua, demonstrando integração entre as duas linguagens.

## 🎯 Funcionalidades

### Controles do Cubo
- **W/A/S/D**: Rotacionar o cubo nos eixos X e Y
- **Click Esquerdo**: Selecionar face do cubo (funciona em qualquer rotação!)
- **1, 2, 3**: Adicionar cores (Vermelho, Verde, Azul) à face selecionada
- **Click Direito**: Limpar face selecionada (volta para branco)
- **R**: Reset rápido da face atual

### Controles do Background
- **Seta Esquerda/Direita**: Navegar entre cores
- **Seta Cima**: Alternar entre modo liso, gradiente e formas matemáticas dinâmicas
- **Cores disponíveis**: Branco, Preto, Laranja, Rosa, Azul Escuro, Vermelho Escuro, Verde Escuro
- **Modo matemático**: Lissajous, espiral e ondas (lógica e parâmetros em Lua)

### Mistura de Cores
- Clique em uma face branca e pressione **1** → Fica vermelho
- Pressione **3** → Fica roxo (vermelho + azul)
- Continue adicionando cores para criar misturas complexas!

## 🔧 Melhorias Implementadas

### 1. **Seleção de Faces Melhorada (Color Picking)**
**Problema original**: A seleção de faces não funcionava corretamente dependendo da rotação do cubo.

**Solução**: Implementei técnica de **Color Picking**:
- Cada face recebe uma cor única (ID) em um buffer invisível
- Ao clicar, o programa lê o pixel exato onde o mouse clicou
- Identifica a face pela cor do pixel
- Funciona perfeitamente em qualquer rotação!

```cpp
// Exemplo: Face 0 = ID 1 (vermelho puro no buffer)
glColor3ub(1, 0, 0);  // R=1, G=0, B=0
// ... desenha face ...
unsigned char pixel[3];
glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
// Se pixel[0] == 1, selecionou face 0!
```

### 2. **Sistema de Mistura de Cores Aprimorado**
**Problema original**: Lógica de mistura limitada e duplicada entre C++ e Lua.

**Solução**:
- Lógica centralizada em `mixer.lua`
- Suporta mistura aditiva (RGB)
- Detecção inteligente: se a face é branca, substitui; senão, adiciona
- Preparado para futuras expansões (mistura subtrativa, HSV, etc.)

```lua
function mixColors(r, g, b, ar, ag, ab)
    -- Se branco, substitui pela nova cor
    if r >= 0.99 and g >= 0.99 and b >= 0.99 then
        return ar, ag, ab
    end
    
    -- Senão, mistura aditiva
    return math.min(1.0, r + ar),
           math.min(1.0, g + ag),
           math.min(1.0, b + ab)
end
```

### 3. **Melhor Integração C++ ↔ Lua**
- Tratamento de erros robusto
- Busca automática de arquivos Lua em múltiplas pastas
- Mensagens de debug claras
- Fallbacks para quando Lua não está disponível

### 4. **Interface Melhorada**
- Mensagens de console informativas
- Guia de controles na inicialização
- Feedback visual ao selecionar faces
- Nome descritivo das faces (Front, Back, Top, etc.)

## 📁 Estrutura do Projeto

```
cube_editor/
├── src/                    # Arquivos C++
│   ├── main.cpp            # Ponto de entrada
│   ├── cubo.cpp/h          # Classe do cubo
│   ├── background.cpp/h    # Sistema de background
│   └── lua_bridge.cpp/h    # Bridge C++ ↔ Lua
├── lua/                    # Scripts Lua
│   ├── controle.lua        # Lógica de rotação WASD
│   ├── mixer.lua           # Lógica de mistura de cores
│   └── background.lua      # Estado e padrões matemáticos do background
├── Makefile
├── README.md
└── DOCUMENTATION.md        # Documentação detalhada do código e funções
```

## 🚀 Como Compilar e Executar

### Requisitos
- g++ (C++11 ou superior)
- OpenGL/GLUT
- Lua 5.3 (ou 5.1, 5.2, 5.4 - ajuste o Makefile)

### Ubuntu/Debian
```bash
sudo apt-get install build-essential freeglut3-dev liblua5.3-dev
```

### Compilação
```bash
make              # Compila o projeto
make install-lua  # Cria pasta lua/ e copia arquivos
make run          # Compila e executa
```

### Limpeza
```bash
make clean       # Remove arquivos objeto e executável
make distclean   # Limpeza completa
```

## 🎨 Exemplos de Uso

### Criar um Cubo Colorido
1. Rode o programa: `./cube_editor`
2. Clique na face frontal (front face)
3. Pressione `1` para vermelho
4. Rotacione com `W` para ver outra face
5. Clique nessa face e pressione `2` para verde
6. Continue colorindo!

### Misturar Cores
1. Clique em uma face branca
2. Pressione `1` (vermelho)
3. Pressione `2` (verde)
4. Resultado: Amarelo! (vermelho + verde)

### Cores Secundárias
- Vermelho (1) + Verde (2) = Amarelo
- Vermelho (1) + Azul (3) = Magenta/Roxo
- Verde (2) + Azul (3) = Ciano

## 📖 Documentação do Código

Para uma descrição completa do funcionamento de cada ficheiro e função (fluxo, parâmetros, integração C++/Lua), consulte **DOCUMENTATION.md**. Cada ficheiro fonte inclui ainda no próprio código um bloco de documentação no início e antes de cada função.

## 🔍 Detalhes Técnicos

### Color Picking
O método `selectCurrentFace()` em `cubo.cpp` implementa:
1. Renderiza cubo off-screen com cores ID
2. Lê pixel na posição do mouse
3. Mapeia cor → índice da face
4. Atualiza `selectedFace`

### Lua Bridge
A classe `LuaBridge` em `lua_bridge.cpp` permite:
- Chamar funções Lua do C++
- Passar parâmetros entre as linguagens
- Receber valores de retorno
- Tratamento de erros robusto

### Arquitetura
- **C++**: Renderização, OpenGL, estrutura principal
- **Lua**: Lógica de jogo, regras de negócio, configurações
- **Vantagem**: Modificar comportamento sem recompilar!

## 🐛 Troubleshooting

### "Erro ao carregar *.lua"
- Verifique se os arquivos `.lua` estão no diretório correto
- Execute `make install-lua` para criar a pasta lua/
- Ou coloque os arquivos `.lua` no mesmo diretório do executável

### "Função X não encontrada"
- Verifique se os arquivos Lua foram carregados corretamente
- Veja mensagens de inicialização no console

### Faces não selecionam corretamente
- Certifique-se de que está usando o método `selectCurrentFace(x, y)`
- Os parâmetros x e y devem ser as coordenadas do mouse

## 📝 Notas de Desenvolvimento

### Próximas Melhorias Possíveis
- [ ] Salvar/carregar estado do cubo
- [ ] Modo de animação automática
- [ ] Mais cores primárias (ciano, magenta, amarelo)
- [ ] Undo/Redo
- [ ] Mistura subtrativa (como tintas reais)
- [ ] Exportar como imagem
- [ ] Interface GUI com botões

### Expansões de Lua
O arquivo `mixer.lua` já contém funções preparadas para:
- Conversão RGB ↔ HSV
- Mistura subtrativa (CMYK)
- Descrição textual de cores
- Futuras implementações de paletas

## 📄 Licença

Este projeto é fornecido como exemplo educacional de integração C++/Lua.

## 👨‍💻 Autor

Desenvolvido como demonstração de integração entre linguagens de programação.

---

**Divirta-se criando seu cubo colorido! 🎨🎲**
