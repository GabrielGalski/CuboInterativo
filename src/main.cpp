/*
 * main.cpp
 *
 * Ponto de entrada da aplicação Cube Editor. Responsável por criar a janela GLUT,
 * registrar callbacks de teclado, mouse e redimensionamento, e pelo loop principal.
 * O fluxo de desenho a cada frame: limpa os buffers, atualiza o padrão do background
 * em modo matemático (via Lua), desenha o fundo, aplica a transformação de câmera
 * e desenha o cubo. A lógica de rotação (WASD) e mistura de cores é delegada ao
 * LuaBridge, que chama scripts Lua; a seleção de faces e a cor do background são
 * controladas localmente com feedback no console.
 */

#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <string>
#include "cubo.h"
#include "background.h"
#include "lua_bridge.h"

Cubo cube;
Background background;
LuaBridge bridge;
int windowWidth = 800;
int windowHeight = 600;
bool showControls = false;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

std::string openImageFileDialog() {
    char fileName[MAX_PATH] = {0};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        "Imagens\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff;*.ppm\0"
        "Todos\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        return std::string(fileName);
    }
    return "";
}
#endif

void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char ch : text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ch);
    }
}

/*
 * Desenha um frame completo: limpa cor e profundidade, obtém parâmetros do padrão
 * dinâmico em Lua quando o modo de background é matemático, renderiza o background
 * (sólido, gradiente ou curvas matemáticas), aplica identidade e translação da câmera
 * e renderiza o cubo com suas rotações e cores. Finaliza com glutSwapBuffers.
 */
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    background.render();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    cube.render();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float margin = 12.0f;
    float btnW = 150.0f;
    float btnH = 26.0f;
    float x1 = windowWidth - margin - btnW;
    float y1 = windowHeight - margin - btnH;
    float x2 = windowWidth - margin;
    float y2 = windowHeight - margin;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.10f, 0.10f, 0.12f, 0.65f);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();

    glColor4f(0.85f, 0.85f, 0.88f, 0.9f);
    drawText(x1 + 10.0f, y1 + 9.0f, "Controles");

    if (showControls) {
        float panelW = 320.0f;
        float panelH = 170.0f;
        float px2 = x2;
        float py2 = y1 - 8.0f;
        float px1 = px2 - panelW;
        float py1 = py2 - panelH;

        glColor4f(0.08f, 0.08f, 0.10f, 0.78f);
        glBegin(GL_QUADS);
        glVertex2f(px1, py1);
        glVertex2f(px2, py1);
        glVertex2f(px2, py2);
        glVertex2f(px1, py2);
        glEnd();

        glColor4f(0.85f, 0.85f, 0.88f, 0.92f);
        float tx = px1 + 10.0f;
        float ty = py2 - 18.0f;
        drawText(tx, ty, "Mouse: clique esq seleciona / dir limpa");
        ty -= 16.0f;
        drawText(tx, ty, "WASD: rotacionar cubo");
        ty -= 16.0f;
        drawText(tx, ty, "1-6: aplicar cor na face");
        ty -= 16.0f;
        drawText(tx, ty, "M: trocar modo de mistura");
        ty -= 16.0f;
        drawText(tx, ty, "F: trocar padrao da face");
        ty -= 16.0f;
        drawText(tx, ty, "P: colocar foto na face");
        ty -= 16.0f;
        drawText(tx, ty, "Setas: cor do fundo / modo fundo");
        ty -= 16.0f;
        drawText(tx, ty, "H: mostrar/ocultar esta ajuda");
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

/*
 * Trata teclas ASCII. W/A/S/D repassam ao LuaBridge para processInput em Lua e
 * aplicam a rotação retornada no cubo. Teclas 1/2/3 obtêm a cor atual da face
 * selecionada, chamam mixColor no bridge (mixer.lua) e aplicam a nova cor à face.
 * R limpa a face selecionada para branco. ESC encerra a aplicação. glutPostRedisplay
 * é chamado ao final para atualizar a tela.
 */
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'w': case 'W':
        case 's': case 'S':
        case 'a': case 'A':
        case 'd': case 'D':
            bridge.handleInput(cube, key);
            break;

        case '1': {
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 1.0f, 0.0f, 0.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            std::cout << "Adicionado vermelho à face " << cube.getSelectedFace() << std::endl;
            break;
        }

        case '2': {
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 0.0f, 1.0f, 0.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            std::cout << "Adicionado verde à face " << cube.getSelectedFace() << std::endl;
            break;
        }

        case '3': {
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 0.0f, 0.0f, 1.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            std::cout << "Adicionado azul à face " << cube.getSelectedFace() << std::endl;
            break;
        }

        case '4': {
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 0.0f, 1.0f, 1.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            std::cout << "Adicionado ciano à face " << cube.getSelectedFace() << std::endl;
            break;
        }

        case '5': {
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 1.0f, 0.0f, 1.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            std::cout << "Adicionado magenta à face " << cube.getSelectedFace() << std::endl;
            break;
        }

        case '6': {
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 1.0f, 1.0f, 0.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            std::cout << "Adicionado amarelo à face " << cube.getSelectedFace() << std::endl;
            break;
        }

        case 'm': case 'M': {
            std::string name = bridge.cycleMixMode();
            if (!name.empty()) {
                std::cout << "Modo de mistura: " << name << std::endl;
            }
            break;
        }

        case 'f': case 'F':
            bridge.toggleFacePattern(cube);
            std::cout << "Padrão da face " << cube.getSelectedFace() << " alterado para " << cube.getFacePattern(cube.getSelectedFace()) << std::endl;
            break;

        case 'p': case 'P': {
            int face = cube.getSelectedFace();
            std::string path;
#ifdef _WIN32
            path = openImageFileDialog();
            if (path.empty()) {
                break;
            }
#else
            std::cout << "Caminho da imagem PPM para a face " << face << ": ";
            std::cin >> path;
#endif
            if (cube.setFacePhotoFromFile(face, path)) {
                bridge.setFacePhoto(face, path);
                std::cout << "Foto aplicada na face " << face << std::endl;
            } else {
                std::cout << "Não foi possível carregar a imagem" << std::endl;
            }
            break;
        }

        case 'r': case 'R':
            cube.clearSelectedFace();
            std::cout << "Face resetada para branco" << std::endl;
            break;

        case 'h': case 'H':
            showControls = !showControls;
            break;

        case 27:
            exit(0);
            break;
    }

    glutPostRedisplay();
}

/*
 * Trata teclas especiais (setas). Esquerda/Direita alteram a cor do background
 * (anterior/próxima no vetor de cores). Seta para cima avança o modo de desenho
 * do background (sólido, gradiente, formas matemáticas). glutPostRedisplay ao final.
 */
void specialKeys(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_LEFT:
            background.previousColor();
            std::cout << "Background: cor anterior" << std::endl;
            break;

        case GLUT_KEY_RIGHT:
            background.nextColor();
            std::cout << "Background: próxima cor" << std::endl;
            break;

        case GLUT_KEY_UP:
            background.nextModel();
            std::cout << "Background: modo alterado" << std::endl;
            break;
    }

    glutPostRedisplay();
}

/*
 * Callback de mouse. Em GLUT_DOWN do botão esquerdo, chama selectCurrentFace com
 * as coordenadas da janela (x, y), que usa color picking para identificar a face
 * do cubo sob o cursor. Botão direito limpa a face selecionada para branco.
 * glutPostRedisplay ao final.
 */
void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN)
        return;

    if (button == GLUT_LEFT_BUTTON) {
        float margin = 12.0f;
        float btnW = 150.0f;
        float btnH = 26.0f;
        float x1 = windowWidth - margin - btnW;
        float y1 = windowHeight - margin - btnH;
        float x2 = windowWidth - margin;
        float y2 = windowHeight - margin;
        float mx = (float)x;
        float my = (float)(windowHeight - y);
        if (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) {
            showControls = !showControls;
            glutPostRedisplay();
            return;
        }
        cube.selectCurrentFace(x, y);
        std::cout << "Click esquerdo em (" << x << ", " << y << ")" << std::endl;
    }

    if (button == GLUT_RIGHT_BUTTON) {
        cube.clearSelectedFace();
        std::cout << "Face limpa (botão direito)" << std::endl;
    }

    glutPostRedisplay();
}

/*
 * Inicialização do OpenGL e da aplicação: ativa teste de profundidade, inicializa
 * o LuaBridge (carrega scripts Lua), define rotação inicial do cubo e estado
 * padrão do background, cor de clear e imprime no console o guia de controles.
 * Em falha na inicialização do Lua, encerra com código 1.
 */
void init() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if (!bridge.init()) {
        std::cerr << "Erro ao inicializar Lua!" << std::endl;
        exit(1);
    }
    std::cout << "Lua inicializado com sucesso!" << std::endl;

    cube.setRotation(0.0f, 25.0f, 0.0f);
    cube.initPatterns();

    background.setDefault();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "\n========================================" << std::endl;
    std::cout << "   CUBO EDITOR - Controles" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "ROTAÇÃO:" << std::endl;
    std::cout << "  W/A/S/D - Rotacionar cubo" << std::endl;
    std::cout << "\nCORES:" << std::endl;
    std::cout << "  Click Esquerdo - Selecionar face" << std::endl;
    std::cout << "  1 - Adicionar vermelho" << std::endl;
    std::cout << "  2 - Adicionar verde" << std::endl;
    std::cout << "  3 - Adicionar azul" << std::endl;
    std::cout << "  4 - Adicionar ciano" << std::endl;
    std::cout << "  5 - Adicionar magenta" << std::endl;
    std::cout << "  6 - Adicionar amarelo" << std::endl;
    std::cout << "  M - Trocar modo de mistura" << std::endl;
    std::cout << "  Click Direito - Limpar face (branco)" << std::endl;
    std::cout << "  R - Reset face atual" << std::endl;
    std::cout << "  F - Trocar padrão da face" << std::endl;
    std::cout << "  P - Colocar foto na face" << std::endl;
    std::cout << "\nBACKGROUND:" << std::endl;
    std::cout << "  Seta Esquerda/Direita - Mudar cor" << std::endl;
    std::cout << "  Seta Cima - Alternar modo (sólido / espaço)" << std::endl;
    std::cout << "\nOUTRO:" << std::endl;
    std::cout << "  H - Mostrar/ocultar ajuda" << std::endl;
    std::cout << "  ESC - Sair" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/*
 * Ajusta viewport e projeção perspectiva ao redimensionar a janela. Evita divisão
 * por zero na altura. Projeção: 45° FOV, aspect ratio da janela, near 1, far 100.
 */
void reshape(int w, int h) {
    if (h == 0) h = 1;

    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

/*
 * Configura GLUT (modo duplo buffer, RGB, profundidade), cria janela, registra
 * callbacks e entra no loop principal. Não retorna até o fim da aplicação.
 */
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Cube Editor - C++ & Lua Integration");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);

    glutMainLoop();

    return 0;
}
