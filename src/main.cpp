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

Cubo       cube;
Background background;
LuaBridge  bridge;

int  larguraJanela  = 800;
int  alturaJanela = 600;
bool mostrarControles = false;
bool mostrarSplash   = true;   // tela inicial

static int janelaPrincipal  = 0;   // ID da janela principal (sempre usado)
#ifdef BENCH_MODE
static int janelaBenchmark = 0;

/* Estima memória de textura: assume RGBA 8bpp, escala típica 512×512 por face */
static long estimateTexKb(const Cubo& c) {
    long kb = 0;
    for (int i = 0; i < 6; ++i)
        if (c.faceTemTextura(i))
            kb += (512 * 512 * 4) / 1024;   // ~1024 kB por textura 512×512 RGBA
    return kb;
}
static int countTex(const Cubo& c) {
    int n = 0;
    for (int i = 0; i < 6; ++i)
        if (c.faceTemTextura(i)) ++n;
    return n;
}
#endif

// ── Diálogo de arquivo ───────────────────────────────────────────────────────
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

// ── Texto 2D ─────────────────────────────────────────────────────────────────
void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char ch : text) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ch);
}

// ── Tela de splash ────────────────────────────────────────────────────────────
void drawSplash() {
    float pw = 400.0f, ph = 260.0f;
    float px1 = (larguraJanela  - pw) * 0.5f;
    float py1 = (alturaJanela - ph) * 0.5f;
    float px2 = px1 + pw;
    float py2 = py1 + ph;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0, larguraJanela, 0, alturaJanela, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Painel semi-transparente
    glColor4f(0.04f, 0.04f, 0.08f, 0.82f);
    glBegin(GL_QUADS);
    glVertex2f(px1, py1); glVertex2f(px2, py1);
    glVertex2f(px2, py2); glVertex2f(px1, py2);
    glEnd();

    // Borda fina
    glColor4f(0.4f, 0.5f, 0.9f, 0.6f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(px1, py1); glVertex2f(px2, py1);
    glVertex2f(px2, py2); glVertex2f(px1, py2);
    glEnd();

    // Linhas de texto fornecidas pelo Lua (ui.lua → obterLinhasSplash)
    std::vector<LinhaUI> linhas;
    bridge.obterLinhasSplash(linhas);

    float tx = px1 + 20.0f;
    float ty = py2 - 30.0f;
    for (const auto& linha : linhas) {
        glColor4f(linha.r, linha.g, linha.b, 0.92f);
        if (linha.centralizar) {
            float cx = px1 + pw * 0.5f - (float)(linha.texto.size() * 4);
            drawText(cx, ty, linha.texto);
        } else {
            drawText(tx, ty, linha.texto);
        }
        ty -= linha.passo;
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // ── Indicador de zoom/rotação (se face tem imagem) ───────────────────────
    {
        int face = cube.obterFaceSelecionada();
        if (cube.faceTemTextura(face)) {
            float s   = cube.obterEscalaTexturaFace(face);
            int   deg = cube.obterRotacaoTexturaFace(face) * 90;
            char buf[80];
            std::snprintf(buf, sizeof(buf),
                "Face %d | Zoom: %.1fx | Rot: %d°  [↑/↓ zoom  ←/→ girar]", face, s, deg);

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            float tw2 = (float)(std::strlen(buf) * 8 + 20);
            float bx1 = 10.0f, by1 = 10.0f;
            float bx2 = bx1 + tw2, by2 = by1 + 22.0f;
            glColor4f(0.05f, 0.05f, 0.10f, 0.72f);
            glBegin(GL_QUADS);
            glVertex2f(bx1,by1); glVertex2f(bx2,by1);
            glVertex2f(bx2,by2); glVertex2f(bx1,by2);
            glEnd();

            glColor4f(0.85f, 0.95f, 0.65f, 0.95f);
            drawText(bx1 + 10.0f, by1 + 6.0f, buf);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ── Display da janela de benchmark ───────────────────────────────────────────
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

// ── Display ──────────────────────────────────────────────────────────────────
void display() {
#ifdef BENCH_MODE
    gBench.frameBegin();
#endif

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* ── Render C++ (background + cubo) ── */
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

    /* ── Botão "Controles" (canto superior direito) ── */
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

            // Linhas fornecidas pelo Lua (ui.lua → obterLinhasControles)
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

    if (mostrarSplash) drawSplash();

    glutSwapBuffers();

    /* ── Fecha o frame de bench e pede redraw na janela de monitor ── */
#ifdef BENCH_MODE
    gBench.setTexInfo(countTex(cube), estimateTexKb(cube));
    gBench.frameEnd();
    if (janelaBenchmark) glutPostWindowRedisplay(janelaBenchmark);
#endif
}

// ── Timer para animação contínua ─────────────────────────────────────────────
void timer(int) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);   // ~60 fps
}

// ── Teclado ───────────────────────────────────────────────────────────────────
void keyboard(unsigned char key, int x, int y) {
    if (mostrarSplash) { mostrarSplash = false; glutPostRedisplay(); return; }

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

        case '1': {   // Vermelho
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
        case '2': {   // Azul
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
        case '3': {   // Verde
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
        case '4':   // Preto
            cube.definirCorFace(cube.obterFaceSelecionada(), 0.0f, 0.0f, 0.0f);
            break;

        case 8: case 127: {   // Backspace / Delete
            int face = cube.obterFaceSelecionada();
            std::string path = openImageFileDialog();
            if (path.empty()) break;
            if (cube.definirFotoFaceDeArquivo(face, path)) {
                bridge.definirFotoFace(face, path);
            }
            break;
        }

        case 'r': case 'R':
            cube.limparFaceSelecionada();  // limpa cor e remove imagem
            break;

        case 'h': case 'H':
            mostrarControles = !mostrarControles;
            break;

        case 27:
            exit(0);
    }
    glutPostRedisplay();
}

// ── Teclas especiais ─────────────────────────────────────────────────────────
void specialKeys(int key, int x, int y) {
    if (mostrarSplash) { mostrarSplash = false; glutPostRedisplay(); return; }

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

// ── Mouse ─────────────────────────────────────────────────────────────────────
void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    if (mostrarSplash) { mostrarSplash = false; glutPostRedisplay(); return; }

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
        // Picking: C++ lê o pixel, Lua resolve o índice da face
        int pixelR = cube.lerPixelPicking(x, y);
        int face   = bridge.resolverFacePicking(pixelR);
        cube.definirFaceSelecionada(face);
    }
    if (button == GLUT_RIGHT_BUTTON) {
        cube.limparCorFaceSelecionada();  // só remove a cor, mantém imagem
    }
    glutPostRedisplay();
}

// ── Init ──────────────────────────────────────────────────────────────────────
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

void reshape(int w, int h) {
    if (h == 0) h = 1;
    larguraJanela = w; alturaJanela = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w/(double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    /* ── Janela principal ── */
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
    /* ── Janela de benchmark ── */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(400, 260);
    glutInitWindowPosition(920, 100);   /* 100 + 800 + 20 px de gap */
    janelaBenchmark = glutCreateWindow("Benchmark");
    glutDisplayFunc(displayBench);
    /* Volta ao contexto da janela principal antes do loop */
    glutSetWindow(janelaPrincipal);
#endif

    glutMainLoop();
    return 0;
}
