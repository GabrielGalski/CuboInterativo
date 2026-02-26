/*
 * main.cpp
 * Programa principal: inicializa e gerencia a janela GLUT, chamadas de eventos,
 * renderização da cena 3D e interface de usuário.
 */

#include <GL/glut.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include "cubo.h"
#include "background.h"
#include "lua_bridge.h"

#ifdef BENCH_MODE
#include "bench.h"
#endif

Cubo cube;
Background background;
LuaBridge bridge;

int  larguraJanela  = 800;
int  alturaJanela = 600;
bool mostrarControles = false;

static int janelaPrincipal  = 0;
#ifdef BENCH_MODE
static int janelaBenchmark = 0;

static long estimateTexKb(const Cubo& c) {
    long kb = 0;
    for (int i = 0; i < 6; ++i)
        if (c.faceTemTextura(i))
            kb += (512 * 512 * 4) / 1024;
    return kb;
}
static int countTex(const Cubo& c) {
    int n = 0;
    for (int i = 0; i < 6; ++i)
        if (c.faceTemTextura(i)) ++n;
    return n;
}
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

std::string openImageFileDialog() {
    char fileName[MAX_PATH] = {0};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = nullptr;
    ofn.lpstrFile    = fileName;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  =
        "Imagens\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.ppm\0Todos\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return std::string(fileName);
    return "";
}
#else
static std::string openImageFileDialog() {
    auto tryPipe = [](const char* cmd) -> std::string {
        FILE* f = popen(cmd, "r");
        if (!f) return "";
        char buf[4096] = {0};
        fgets(buf, sizeof(buf), f);
        pclose(f);
        std::string s(buf);
        while (!s.empty() && (s.back()=='\n'||s.back()=='\r'||s.back()==' '))
            s.pop_back();
        return s;
    };
    std::string r;
    r = tryPipe("zenity --file-selection --title='Selecionar imagem'"
                " --file-filter='Imagens | *.png *.jpg *.jpeg *.bmp *.ppm *.tga'"
                " 2>/dev/null");
    if (!r.empty()) return r;
    r = tryPipe("kdialog --getopenfilename . 'Images (*.png *.jpg *.bmp)' 2>/dev/null");
    if (!r.empty()) return r;
    r = tryPipe("osascript -e 'POSIX path of (choose file)' 2>/dev/null");
    if (!r.empty()) return r;
    return "";
}
#endif

// Renderiza texto 2D na tela usando bitmap do GLUT.
void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char ch : text) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ch);
}

// Renderiza a janela de benchmark com informações de desempenho.
#ifdef BENCH_MODE
void displayBench() {
    int bw = glutGet(GLUT_WINDOW_WIDTH);
    int bh = glutGet(GLUT_WINDOW_HEIGHT);

    glClearColor(0.04f, 0.04f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gBench.draw(bw, bh);

    glutSwapBuffers();
}
#endif

// Renderiza a cena principal: background, cubo, botão de controles e painel.
void display() {
#ifdef BENCH_MODE
    gBench.frameBegin();
#endif

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef BENCH_MODE
    gBench.cppRenderBegin();
#endif

    background.renderizar();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);
    cube.renderizar();

#ifdef BENCH_MODE
    gBench.cppRenderEnd();
#endif

    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix(); glLoadIdentity();
        glOrtho(0, larguraJanela, 0, alturaJanela, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix(); glLoadIdentity();

        float margin = 12.0f, btnW = 150.0f, btnH = 26.0f;
        float x1 = larguraJanela  - margin - btnW;
        float y1 = alturaJanela - margin - btnH;
        float x2 = larguraJanela  - margin;
        float y2 = alturaJanela - margin;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.10f, 0.10f, 0.12f, 0.65f);
        glBegin(GL_QUADS);
        glVertex2f(x1,y1); glVertex2f(x2,y1);
        glVertex2f(x2,y2); glVertex2f(x1,y2);
        glEnd();

        glColor4f(0.85f, 0.85f, 0.88f, 0.9f);
        drawText(x1 + 10.0f, y1 + 9.0f, "Controles");

        if (mostrarControles) {
            float panelW = 300.0f, panelH = 200.0f;
            float px2 = x2, py2 = y1 - 8.0f;
            float px1 = px2 - panelW, py1 = py2 - panelH;

            glColor4f(0.08f, 0.08f, 0.10f, 0.78f);
            glBegin(GL_QUADS);
            glVertex2f(px1,py1); glVertex2f(px2,py1);
            glVertex2f(px2,py2); glVertex2f(px1,py2);
            glEnd();

            std::vector<LinhaUI> linhas;
            bridge.obterLinhasControles(linhas);
            float tx = px1 + 10.0f, ty = py2 - 18.0f;
            for (const auto& linha : linhas) {
                glColor4f(linha.r, linha.g, linha.b, 0.92f);
                drawText(tx, ty, linha.texto);
                ty -= linha.passo;
            }
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    glutSwapBuffers();

#ifdef BENCH_MODE
    gBench.setTexInfo(countTex(cube), estimateTexKb(cube));
    gBench.frameEnd();
    if (janelaBenchmark) glutPostWindowRedisplay(janelaBenchmark);
#endif
}

// Controla o loop de renderização com intervalo de 16ms (60 FPS).
void timer(int) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Processa entrada de teclado: rotação, cores, reset, imagem e painel.
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'w': case 'W':
        case 's': case 'S':
        case 'a': case 'A':
        case 'd': case 'D':
#ifdef BENCH_MODE
            gBench.luaBegin();
#endif
            bridge.lidarComEntrada(cube, key);
#ifdef BENCH_MODE
            gBench.luaEnd();
#endif
            break;

        case '1': {
            Cor c = cube.obterCorFace(cube.obterFaceSelecionada());
            float nr, ng, nb;
#ifdef BENCH_MODE
            gBench.luaBegin();
#endif
            bridge.misturarCor(c.vermelho, c.verde, c.azul, 1.0f, 0.0f, 0.0f, nr, ng, nb);
#ifdef BENCH_MODE
            gBench.luaEnd();
#endif
            cube.definirCorFace(cube.obterFaceSelecionada(), nr, ng, nb);
            break;
        }
        case '2': {
            Cor c = cube.obterCorFace(cube.obterFaceSelecionada());
            float nr, ng, nb;
#ifdef BENCH_MODE
            gBench.luaBegin();
#endif
            bridge.misturarCor(c.vermelho, c.verde, c.azul, 0.0f, 0.0f, 1.0f, nr, ng, nb);
#ifdef BENCH_MODE
            gBench.luaEnd();
#endif
            cube.definirCorFace(cube.obterFaceSelecionada(), nr, ng, nb);
            break;
        }
        case '3': {
            Cor c = cube.obterCorFace(cube.obterFaceSelecionada());
            float nr, ng, nb;
#ifdef BENCH_MODE
            gBench.luaBegin();
#endif
            bridge.misturarCor(c.vermelho, c.verde, c.azul, 0.0f, 1.0f, 0.0f, nr, ng, nb);
#ifdef BENCH_MODE
            gBench.luaEnd();
#endif
            cube.definirCorFace(cube.obterFaceSelecionada(), nr, ng, nb);
            break;
        }
        case '4':
            cube.definirCorFace(cube.obterFaceSelecionada(), 0.0f, 0.0f, 0.0f);
            break;

        case 8: case 127: {
            int face = cube.obterFaceSelecionada();
            std::string path = openImageFileDialog();
            if (path.empty()) break;
            if (cube.definirFotoFaceDeArquivo(face, path)) {
                bridge.definirFotoFace(face, path);
            }
            break;
        }

        case 'r': case 'R':
            cube.limparFaceSelecionada();
            break;

        case 'h': case 'H':
            mostrarControles = !mostrarControles;
            break;

        case 27:
            exit(0);
    }
    glutPostRedisplay();
}

// Processa teclas especiais para controle de escala e rotação da textura.
void specialKeys(int key, int x, int y) {
    int face = cube.obterFaceSelecionada();

    switch (key) {
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN: {
            if (!cube.faceTemTextura(face)) break;
            float s = cube.obterEscalaTexturaFace(face);
            s += (key == GLUT_KEY_UP) ? 0.1f : -0.1f;
            cube.definirEscalaTexturaFace(face, s);
            break;
        }
        case GLUT_KEY_LEFT:
            if (!cube.faceTemTextura(face)) break;
            cube.rotacionarTexturaFace(face, -1);
            break;
        case GLUT_KEY_RIGHT:
            if (!cube.faceTemTextura(face)) break;
            cube.rotacionarTexturaFace(face, +1);
            break;
    }
    glutPostRedisplay();
}

// Processa cliques do mouse para seleção de face, limpeza de cor e alternância do painel de controles.
void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    if (button == GLUT_LEFT_BUTTON) {
        float margin = 12.0f, btnW = 150.0f, btnH = 26.0f;
        float x1 = larguraJanela  - margin - btnW;
        float y1 = alturaJanela - margin - btnH;
        float x2 = larguraJanela  - margin;
        float y2 = alturaJanela - margin;
        float mx = (float)x, my = (float)(alturaJanela - y);
        if (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) {
            mostrarControles = !mostrarControles;
            glutPostRedisplay();
            return;
        }
        int pixelR = cube.lerPixelPicking(x, y);
        int face   = bridge.resolverFacePicking(pixelR);
        cube.definirFaceSelecionada(face);
    }
    if (button == GLUT_RIGHT_BUTTON) {
        cube.limparCorFaceSelecionada();
    }
    glutPostRedisplay();
}

// Inicializa o estado do programa: OpenGL, Lua, background e cubo.
void init() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if (!bridge.init()) {
        exit(1);
    }

    background.definirBridge(&bridge);

#ifdef BENCH_MODE
    gBench.luaBegin();
#endif
    bridge.inicializarEstrelas(420);
#ifdef BENCH_MODE
    gBench.luaEnd();
#endif

    cube.definirRotacao(15.0f, 25.0f, 0.0f);
    background.definirPadrao();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

// Ajusta a projeção e viewport quando a janela é redimensionada.
void reshape(int w, int h) {
    if (h == 0) h = 1;
    larguraJanela = w; alturaJanela = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w/(double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Função principal: inicializa GLUT e inicia o loop de eventos.
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    janelaPrincipal = glutCreateWindow("Cubo 3d");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(16, timer, 0);

#ifdef BENCH_MODE
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(400, 260);
    glutInitWindowPosition(920, 100);
    janelaBenchmark = glutCreateWindow("Benchmark");
    glutDisplayFunc(displayBench);
    glutSetWindow(janelaPrincipal);
#endif

    glutMainLoop();
    return 0;
}
