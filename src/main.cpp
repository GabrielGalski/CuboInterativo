#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <string>
#include "cubo.h"
#include "background.h"
#include "lua_bridge.h"

Cubo       cube;
Background background;
LuaBridge  bridge;

int  windowWidth  = 800;
int  windowHeight = 600;
bool showControls = false;
bool showSplash   = true;   // tela inicial

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
    std::cout << "Digite o caminho da imagem: ";
    std::getline(std::cin, r);
    return r;
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
    float px1 = (windowWidth  - pw) * 0.5f;
    float py1 = (windowHeight - ph) * 0.5f;
    float px2 = px1 + pw;
    float py2 = py1 + ph;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1);
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

    // Texto
    float tx = px1 + 20.0f;
    float ty = py2 - 30.0f;
    glColor4f(0.9f, 0.9f, 1.0f, 0.95f);
    drawText(tx, ty, "CUBE EDITOR  -  C++ & Lua");
    ty -= 26.0f;
    glColor4f(0.6f, 0.6f, 0.75f, 0.9f);
    drawText(tx, ty, "-------------------------------");
    ty -= 22.0f;
    glColor4f(0.85f, 0.85f, 0.9f, 0.9f);
    drawText(tx, ty, "WASD         Rotacionar cubo");
    ty -= 18.0f;
    drawText(tx, ty, "Click esq    Selecionar face");
    ty -= 18.0f;
    drawText(tx, ty, "1            Vermelho");
    ty -= 18.0f;
    drawText(tx, ty, "2            Azul");
    ty -= 18.0f;
    drawText(tx, ty, "3            Verde");
    ty -= 18.0f;
    drawText(tx, ty, "R            Limpar face");
    ty -= 18.0f;
    drawText(tx, ty, "Backspace    Imagem na face");
    ty -= 26.0f;
    glColor4f(0.5f, 0.8f, 0.5f, 0.85f);
    drawText(px1 + pw * 0.5f - 80.0f, ty, "Pressione qualquer tecla para comecar");

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ── Display ──────────────────────────────────────────────────────────────────
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    background.render();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);
    cube.render();

    // Botão "Controles" (canto superior direito)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix(); glLoadIdentity();
        glOrtho(0, windowWidth, 0, windowHeight, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix(); glLoadIdentity();

        float margin = 12.0f, btnW = 150.0f, btnH = 26.0f;
        float x1 = windowWidth  - margin - btnW;
        float y1 = windowHeight - margin - btnH;
        float x2 = windowWidth  - margin;
        float y2 = windowHeight - margin;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.10f, 0.10f, 0.12f, 0.65f);
        glBegin(GL_QUADS);
        glVertex2f(x1,y1); glVertex2f(x2,y1);
        glVertex2f(x2,y2); glVertex2f(x1,y2);
        glEnd();

        glColor4f(0.85f, 0.85f, 0.88f, 0.9f);
        drawText(x1 + 10.0f, y1 + 9.0f, "Controles [H]");

        if (showControls) {
            float panelW = 300.0f, panelH = 170.0f;
            float px2 = x2, py2 = y1 - 8.0f;
            float px1 = px2 - panelW, py1 = py2 - panelH;

            glColor4f(0.08f, 0.08f, 0.10f, 0.78f);
            glBegin(GL_QUADS);
            glVertex2f(px1,py1); glVertex2f(px2,py1);
            glVertex2f(px2,py2); glVertex2f(px1,py2);
            glEnd();

            glColor4f(0.85f, 0.85f, 0.88f, 0.92f);
            float tx = px1 + 10.0f, ty = py2 - 18.0f;
            drawText(tx, ty, "Click esq: seleciona face");   ty -= 16.0f;
            drawText(tx, ty, "Click dir: limpa face");       ty -= 16.0f;
            drawText(tx, ty, "WASD: rodar cubo");            ty -= 16.0f;
            drawText(tx, ty, "1 - Vermelho  2 - Azul  3 - Verde"); ty -= 16.0f;
            drawText(tx, ty, "R: resetar face");             ty -= 16.0f;
            drawText(tx, ty, "Backspace: foto na face");     ty -= 16.0f;
            drawText(tx, ty, "ESC: sair");
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    if (showSplash) drawSplash();

    glutSwapBuffers();
}

// ── Timer para animação contínua ─────────────────────────────────────────────
void timer(int) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);   // ~60 fps
}

// ── Teclado ───────────────────────────────────────────────────────────────────
void keyboard(unsigned char key, int x, int y) {
    if (showSplash) { showSplash = false; glutPostRedisplay(); return; }

    switch(key) {
        case 'w': case 'W':
        case 's': case 'S':
        case 'a': case 'A':
        case 'd': case 'D':
            bridge.handleInput(cube, key);
            break;

        case '1': {   // Vermelho
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 1.0f, 0.0f, 0.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            break;
        }
        case '2': {   // Azul
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 0.0f, 0.0f, 1.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            break;
        }
        case '3': {   // Verde
            Color c = cube.getFaceColor(cube.getSelectedFace());
            float nr, ng, nb;
            bridge.mixColor(c.r, c.g, c.b, 0.0f, 1.0f, 0.0f, nr, ng, nb);
            cube.setFaceColor(cube.getSelectedFace(), nr, ng, nb);
            break;
        }

        case 8: case 127: {   // Backspace / Delete
            int face = cube.getSelectedFace();
            std::string path = openImageFileDialog();
            if (path.empty()) break;
            if (cube.setFacePhotoFromFile(face, path)) {
                bridge.setFacePhoto(face, path);
                cube.setFacePattern(face, 0);
                std::cout << "Foto aplicada na face " << face << std::endl;
            }
            break;
        }

        case 'r': case 'R':
            cube.clearSelectedFace();
            break;

        case 'h': case 'H':
            showControls = !showControls;
            break;

        case 27:
            exit(0);
    }
    glutPostRedisplay();
}

// ── Teclas especiais ─────────────────────────────────────────────────────────
void specialKeys(int key, int x, int y) {
    if (showSplash) { showSplash = false; glutPostRedisplay(); return; }
    // Arrow keys removidos (sem controle de cor de fundo)
    (void)key; (void)x; (void)y;
}

// ── Mouse ─────────────────────────────────────────────────────────────────────
void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    if (showSplash) { showSplash = false; glutPostRedisplay(); return; }

    if (button == GLUT_LEFT_BUTTON) {
        float margin = 12.0f, btnW = 150.0f, btnH = 26.0f;
        float x1 = windowWidth  - margin - btnW;
        float y1 = windowHeight - margin - btnH;
        float x2 = windowWidth  - margin;
        float y2 = windowHeight - margin;
        float mx = (float)x, my = (float)(windowHeight - y);
        if (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) {
            showControls = !showControls;
            glutPostRedisplay();
            return;
        }
        cube.selectCurrentFace(x, y);
    }
    if (button == GLUT_RIGHT_BUTTON) {
        cube.clearSelectedFace();
    }
    glutPostRedisplay();
}

// ── Init ──────────────────────────────────────────────────────────────────────
void init() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if (!bridge.init()) {
        std::cerr << "Erro ao inicializar Lua!" << std::endl;
        exit(1);
    }

    // Liga o bridge ao background e gera as 420 estrelas via Lua
    background.setBridge(&bridge);
    bridge.initStars(420);

    cube.setRotation(15.0f, 25.0f, 0.0f);
    cube.initPatterns();
    background.setDefault();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "\n=== CUBE EDITOR ===\n"
              << "1 Vermelho  2 Azul  3 Verde\n"
              << "R: limpar face   Backspace: foto\n"
              << "H: ajuda   ESC: sair\n"
              << "Vermelho+Azul=Roxo | Vermelho+Verde=Amarelo | Azul+Verde=Ciano\n"
              << "Roxo/Amarelo/Ciano + complementar = Branco\n";
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    windowWidth = w; windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w/(double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Cube Editor - C++ & Lua");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(16, timer, 0);   // inicia animação contínua

    glutMainLoop();
    return 0;
}
