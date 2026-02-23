/*
 * cubo.cpp
 *
 * Implementação do cubo 3D: seis faces com vértices em (-1,-1,-1) a (1,1,1),
 * cada uma com cor própria. render() aplica as rotações acumuladas (rotX, rotY,
 * rotZ) e desenha os quads. selectCurrentFace realiza color picking: desenha o
 * cubo com uma cor de identificação por face (R=1..6), lê o pixel na posição
 * do clique (convertendo y de janela para OpenGL), e define selectedFace pelo
 * valor lido. A mistura de cores na face atual pode ser feita por addColorToCurrentFace
 * (lógica local) ou externamente via getFaceColor/setFaceColor e o mixer em Lua.
 */

#include "cubo.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>

// stb_image: suporte a PNG, JPG, BMP, TGA, GIF, etc.
// Baixe em: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
#if __has_include("stb_image.h")
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
    #define HAS_STB_IMAGE 1
#else
    #define HAS_STB_IMAGE 0
#endif

namespace {
struct Vertex3 {
    float x;
    float y;
    float z;
};

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>

struct ComInit {
    HRESULT hr;
    ComInit() : hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComInit() {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }
};

bool loadImageWicRgb(const std::string& path, int& width, int& height, std::vector<unsigned char>& dataRgb) {
    ComInit com;
    if (FAILED(com.hr)) {
        return false;
    }

    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) {
        return false;
    }

    std::wstring wpath(path.begin(), path.end());
    IWICBitmapDecoder* decoder = nullptr;
    hr = factory->CreateDecoderFromFilename(wpath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr) || !decoder) {
        factory->Release();
        return false;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) {
        decoder->Release();
        factory->Release();
        return false;
    }

    UINT w = 0;
    UINT h = 0;
    hr = frame->GetSize(&w, &h);
    if (FAILED(hr) || w == 0 || h == 0) {
        frame->Release();
        decoder->Release();
        factory->Release();
        return false;
    }

    IWICFormatConverter* converter = nullptr;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) {
        frame->Release();
        decoder->Release();
        factory->Release();
        return false;
    }

    hr = converter->Initialize(frame, GUID_WICPixelFormat24bppRGB, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return false;
    }

    width = static_cast<int>(w);
    height = static_cast<int>(h);
    dataRgb.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
    UINT stride = static_cast<UINT>(width * 3);
    UINT bufferSize = static_cast<UINT>(dataRgb.size());
    hr = converter->CopyPixels(nullptr, stride, bufferSize, dataRgb.data());

    converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();

    return SUCCEEDED(hr);
}
#endif

void faceVertices(int face, Vertex3& v0, Vertex3& v1, Vertex3& v2, Vertex3& v3) {
    if (face == 0) {
        v0 = {-1.0f, -1.0f,  1.0f};
        v1 = { 1.0f, -1.0f,  1.0f};
        v2 = { 1.0f,  1.0f,  1.0f};
        v3 = {-1.0f,  1.0f,  1.0f};
    } else if (face == 1) {
        v0 = {-1.0f, -1.0f, -1.0f};
        v1 = {-1.0f,  1.0f, -1.0f};
        v2 = { 1.0f,  1.0f, -1.0f};
        v3 = { 1.0f, -1.0f, -1.0f};
    } else if (face == 2) {
        v0 = {-1.0f,  1.0f, -1.0f};
        v1 = {-1.0f,  1.0f,  1.0f};
        v2 = { 1.0f,  1.0f,  1.0f};
        v3 = { 1.0f,  1.0f, -1.0f};
    } else if (face == 3) {
        v0 = {-1.0f, -1.0f, -1.0f};
        v1 = { 1.0f, -1.0f, -1.0f};
        v2 = { 1.0f, -1.0f,  1.0f};
        v3 = {-1.0f, -1.0f,  1.0f};
    } else if (face == 4) {
        v0 = { 1.0f, -1.0f, -1.0f};
        v1 = { 1.0f,  1.0f, -1.0f};
        v2 = { 1.0f,  1.0f,  1.0f};
        v3 = { 1.0f, -1.0f,  1.0f};
    } else {
        v0 = {-1.0f, -1.0f, -1.0f};
        v1 = {-1.0f, -1.0f,  1.0f};
        v2 = {-1.0f,  1.0f,  1.0f};
        v3 = {-1.0f,  1.0f, -1.0f};
    }
}

bool loadPpm(const std::string& path, int& width, int& height, std::vector<unsigned char>& data) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }
    std::string magic;
    file >> magic;
    if (magic != "P3") {
        return false;
    }
    file >> width >> height;
    int maxVal = 0;
    file >> maxVal;
    if (!file.good() || width <= 0 || height <= 0 || maxVal <= 0) {
        return false;
    }
    data.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
    for (int i = 0; i < width * height; ++i) {
        int r = 0;
        int g = 0;
        int b = 0;
        file >> r >> g >> b;
        if (!file.good()) {
            return false;
        }
        data[static_cast<size_t>(i) * 3u + 0u] = static_cast<unsigned char>(r * 255 / maxVal);
        data[static_cast<size_t>(i) * 3u + 1u] = static_cast<unsigned char>(g * 255 / maxVal);
        data[static_cast<size_t>(i) * 3u + 2u] = static_cast<unsigned char>(b * 255 / maxVal);
    }
    return true;
}

void cropCenterSquare(int& width, int& height, std::vector<unsigned char>& dataRgb) {
    if (width <= 0 || height <= 0) return;
    if (width == height) return;

    int side = (width < height) ? width : height;
    int x0 = (width - side) / 2;
    int y0 = (height - side) / 2;

    std::vector<unsigned char> cropped(static_cast<size_t>(side) * static_cast<size_t>(side) * 3u);
    for (int y = 0; y < side; ++y) {
        int srcY = y0 + y;
        for (int x = 0; x < side; ++x) {
            int srcX = x0 + x;
            size_t src = (static_cast<size_t>(srcY) * static_cast<size_t>(width) + static_cast<size_t>(srcX)) * 3u;
            size_t dst = (static_cast<size_t>(y) * static_cast<size_t>(side) + static_cast<size_t>(x)) * 3u;
            cropped[dst + 0u] = dataRgb[src + 0u];
            cropped[dst + 1u] = dataRgb[src + 1u];
            cropped[dst + 2u] = dataRgb[src + 2u];
        }
    }

    width = side;
    height = side;
    dataRgb.swap(cropped);
}

GLuint createTextureFromData(int width, int height, const std::vector<unsigned char>& data) {
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Garante alinhamento de 1 byte para imagens RGB com largura arbitrária
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restaura padrão
    return texId;
}

GLuint createStripeTexture() {
    const int size = 64;
    std::vector<unsigned char> data(static_cast<size_t>(size) * static_cast<size_t>(size) * 3u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int stripe = (x / 8) % 2;
            float v = stripe == 0 ? 0.3f : 0.7f;
            unsigned char c = static_cast<unsigned char>(v * 255.0f);
            size_t index = static_cast<size_t>(y) * static_cast<size_t>(size) * 3u + static_cast<size_t>(x) * 3u;
            data[index + 0u] = c;
            data[index + 1u] = c;
            data[index + 2u] = c;
        }
    }
    return createTextureFromData(size, size, data);
}

GLuint createDotsTexture() {
    const int size = 64;
    std::vector<unsigned char> data(static_cast<size_t>(size) * static_cast<size_t>(size) * 3u);
    float center = (size - 1) * 0.5f;
    float radius = size * 0.12f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float dist = std::sqrt(dx * dx + dy * dy);
            float v = dist < radius ? 0.9f : 0.3f;
            unsigned char c = static_cast<unsigned char>(v * 255.0f);
            size_t index = static_cast<size_t>(y) * static_cast<size_t>(size) * 3u + static_cast<size_t>(x) * 3u;
            data[index + 0u] = c;
            data[index + 1u] = c;
            data[index + 2u] = c;
        }
    }
    return createTextureFromData(size, size, data);
}
}

/*
 * Construtor: rotações e face selecionada em zero; todas as seis faces
 * começam com cor branca (1, 1, 1).
 */
Cubo::Cubo() : rotX(0), rotY(0), rotZ(0), selectedFace(0), stripesTexture(0), dotsTexture(0) {
    for (int i = 0; i < 6; ++i) {
        faceColors[i]           = {1.0f, 1.0f, 1.0f};
        facePatterns[i]         = 0;
        faceTextures[i]         = 0;
        faceTextureHasAlpha[i]  = false;
        faceTextureScale[i]     = 1.0f;
        faceTextureRotation[i]  = 0;
    }
}

/*
 * Salva a matriz de modelo, aplica rotações em graus nos eixos X, Y e Z
 * (na ordem fixa glRotatef X, Y, Z), desenha seis GL_QUADS com as cores
 * de faceColors e restaura a matriz. A ordem dos vértices segue a orientação
 * frontal de cada face para que a normal apontar para fora.
 */
void Cubo::render() {
    glPushMatrix();
    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotZ, 0.0f, 0.0f, 1.0f);
    for (int face = 0; face < 6; ++face) {
        Vertex3 v0;
        Vertex3 v1;
        Vertex3 v2;
        Vertex3 v3;
        faceVertices(face, v0, v1, v2, v3);
        Color c = faceColors[face];
        bool hasTexture = faceTextures[face] != 0;

        if (hasTexture) {
            float scale = faceTextureScale[face];
            int   rot   = faceTextureRotation[face]; // 0-3: passos de 90° CW

            // Tabela de UVs por rotação para os 4 vértices (v0=BL,v1=BR,v2=TR,v3=TL).
            // Cada linha é {u0,v0, u1,v1, u2,v2, u3,v3}.
            // rot 0=normal, 1=90°CW, 2=180°, 3=270°CW
            static const float uvTable[4][8] = {
                {0,0, 1,0, 1,1, 0,1},  // 0°
                {1,0, 1,1, 0,1, 0,0},  // 90° CW
                {1,1, 0,1, 0,0, 1,0},  // 180°
                {0,1, 0,0, 1,0, 1,1},  // 270° CW
            };
            const float* uv = uvTable[rot & 3];

            // Ajusta UVs para zoom-in: [0,1] → [uvMin, uvMax]
            auto mapUV = [&](float u, float v, float lo, float hi) -> std::pair<float,float> {
                return { lo + u * (hi - lo), lo + v * (hi - lo) };
            };

            // ── Passo 1: cor sólida de fundo ─────────────────────────────────
            glDisable(GL_TEXTURE_2D);
            glColor3f(c.r, c.g, c.b);
            glBegin(GL_QUADS);
            glVertex3f(v0.x, v0.y, v0.z);
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
            glVertex3f(v3.x, v3.y, v3.z);
            glEnd();

            // ── Passo 2: textura com rotação + escala ────────────────────────
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, faceTextures[face]);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

            if (scale >= 1.0f) {
                // Zoom in: UV range encolhe ao redor do centro
                float lo = 0.5f - 0.5f / scale;
                float hi = 0.5f + 0.5f / scale;
                auto [u0,v0uv] = mapUV(uv[0], uv[1], lo, hi);
                auto [u1,v1uv] = mapUV(uv[2], uv[3], lo, hi);
                auto [u2,v2uv] = mapUV(uv[4], uv[5], lo, hi);
                auto [u3,v3uv] = mapUV(uv[6], uv[7], lo, hi);
                glBegin(GL_QUADS);
                glTexCoord2f(u0,v0uv); glVertex3f(v0.x, v0.y, v0.z);
                glTexCoord2f(u1,v1uv); glVertex3f(v1.x, v1.y, v1.z);
                glTexCoord2f(u2,v2uv); glVertex3f(v2.x, v2.y, v2.z);
                glTexCoord2f(u3,v3uv); glVertex3f(v3.x, v3.y, v3.z);
                glEnd();
            } else {
                // Zoom out: vértices encolhem, UV normal (com rotação)
                float cx2 = (v0.x+v1.x+v2.x+v3.x)*0.25f;
                float cy2 = (v0.y+v1.y+v2.y+v3.y)*0.25f;
                float cz2 = (v0.z+v1.z+v2.z+v3.z)*0.25f;
                auto sv = [&](const Vertex3& v) -> Vertex3 {
                    return { cx2+(v.x-cx2)*scale, cy2+(v.y-cy2)*scale, cz2+(v.z-cz2)*scale };
                };
                Vertex3 sv0=sv(v0), sv1=sv(v1), sv2=sv(v2), sv3=sv(v3);
                glBegin(GL_QUADS);
                glTexCoord2f(uv[0],uv[1]); glVertex3f(sv0.x,sv0.y,sv0.z);
                glTexCoord2f(uv[2],uv[3]); glVertex3f(sv1.x,sv1.y,sv1.z);
                glTexCoord2f(uv[4],uv[5]); glVertex3f(sv2.x,sv2.y,sv2.z);
                glTexCoord2f(uv[6],uv[7]); glVertex3f(sv3.x,sv3.y,sv3.z);
                glEnd();
            }

            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthFunc(GL_LESS);
        } else {
            int pattern = facePatterns[face];
            if (pattern == 0) {
                glDisable(GL_TEXTURE_2D);
                glColor3f(c.r, c.g, c.b);
                glBegin(GL_QUADS);
                glVertex3f(v0.x, v0.y, v0.z);
                glVertex3f(v1.x, v1.y, v1.z);
                glVertex3f(v2.x, v2.y, v2.z);
                glVertex3f(v3.x, v3.y, v3.z);
                glEnd();
            } else {
                GLuint tex = 0;
                if (pattern == 1) {
                    tex = stripesTexture;
                } else if (pattern == 2) {
                    tex = dotsTexture;
                }
                if (tex != 0) {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glColor3f(c.r, c.g, c.b);
                    glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex3f(v0.x, v0.y, v0.z);
                    glTexCoord2f(1.0f, 0.0f);
                    glVertex3f(v1.x, v1.y, v1.z);
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex3f(v2.x, v2.y, v2.z);
                    glTexCoord2f(0.0f, 1.0f);
                    glVertex3f(v3.x, v3.y, v3.z);
                    glEnd();
                    glDisable(GL_TEXTURE_2D);
                } else {
                    glDisable(GL_TEXTURE_2D);
                    glColor3f(c.r, c.g, c.b);
                    glBegin(GL_QUADS);
                    glVertex3f(v0.x, v0.y, v0.z);
                    glVertex3f(v1.x, v1.y, v1.z);
                    glVertex3f(v2.x, v2.y, v2.z);
                    glVertex3f(v3.x, v3.y, v3.z);
                    glEnd();
                }
            }
        }
    }
    glPopMatrix();
}

/*
 * Soma os incrementos dx, dy, dz às rotações internas (em graus). Usado
 * após processInput em Lua retornar os deltas para a tecla pressionada.
 */
void Cubo::rotate(float dx, float dy, float dz) {
    rotX += dx;
    rotY += dy;
    rotZ += dz;
}

/*
 * Substitui as rotações atuais pelos valores absolutos x, y, z (graus).
 * Usado na inicialização para definir a pose inicial do cubo.
 */
void Cubo::setRotation(float x, float y, float z) {
    rotX = x;
    rotY = y;
    rotZ = z;
}

/*
 * Se a face selecionada estiver praticamente branca (todos os canais >= 0.99),
 * substitui a cor pela nova (r, g, b). Caso contrário, soma aditivamente cada
 * canal e limita a 1.0. Usado quando a mistura não é feita via Lua.
 */
void Cubo::addColorToCurrentFace(float r, float g, float b) {
    Color& c = faceColors[selectedFace];
    if (c.r >= 0.99f && c.g >= 0.99f && c.b >= 0.99f) {
        c.r = r;
        c.g = g;
        c.b = b;
    } else {
        c.r = std::min(1.0f, c.r + r);
        c.g = std::min(1.0f, c.g + g);
        c.b = std::min(1.0f, c.b + b);
    }
}

/*
 * Define a cor da face atualmente selecionada para branco (1, 1, 1).
 */
void Cubo::clearSelectedFace() {
    faceColors[selectedFace] = {1.0f, 1.0f, 1.0f};
}

/*
 * Atualiza a cor da face indicada pelo índice (0–5) para (r, g, b). Índices
 * fora do intervalo são ignorados.
 */
void Cubo::setFaceColor(int face, float r, float g, float b) {
    if (face >= 0 && face < 6) {
        faceColors[face].r = r;
        faceColors[face].g = g;
        faceColors[face].b = b;
    }
}

/*
 * Usa color picking para definir selectedFace a partir da posição (x, y) em
 * coordenadas de janela. Obtém o viewport, limpa os buffers e desativa
 * iluminação/textura; mantém a projeção atual, empilha a matriz de modelo,
 * aplica a mesma translação e rotações do cubo e desenha as seis faces com
 * glColor3ub(faceId, 0, 0) onde faceId é 1 a 6. Lê o pixel em (x, viewport[3]-y-1)
 * com glReadPixels; o componente R do pixel (1–6) determina a face (selectedFace = R - 1).
 * Restaura as matrizes e limpa o buffer para o desenho normal. Imprime no
 * console o índice e o nome da face ou que nenhuma face foi clicada.
 */
void Cubo::selectCurrentFace(int x, int y) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotZ, 0.0f, 0.0f, 1.0f);

    glBegin(GL_QUADS);

    glColor3ub(1, 0, 0);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);

    glColor3ub(2, 0, 0);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);

    glColor3ub(3, 0, 0);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);

    glColor3ub(4, 0, 0);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);

    glColor3ub(5, 0, 0);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);

    glColor3ub(6, 0, 0);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);

    glEnd();

    glFlush();
    glFinish();

    unsigned char pixel[3];
    glReadPixels(x, viewport[3] - y - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    if (pixel[0] >= 1 && pixel[0] <= 6) {
        selectedFace = pixel[0] - 1;
        std::cout << "Face selecionada: " << selectedFace << " (";
        switch(selectedFace) {
            case 0: std::cout << "Front"; break;
            case 1: std::cout << "Back"; break;
            case 2: std::cout << "Top"; break;
            case 3: std::cout << "Bottom"; break;
            case 4: std::cout << "Right"; break;
            case 5: std::cout << "Left"; break;
        }
        std::cout << ")" << std::endl;
    } else {
        std::cout << "Nenhuma face clicada (pixel: " << (int)pixel[0] << ")" << std::endl;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Cubo::initPatterns() {
    if (stripesTexture == 0) {
        stripesTexture = createStripeTexture();
    }
    if (dotsTexture == 0) {
        dotsTexture = createDotsTexture();
    }
}

void Cubo::setFacePattern(int face, int pattern) {
    if (face < 0 || face >= 6) {
        return;
    }
    if (pattern < 0) {
        pattern = 0;
    }
    if (pattern > 2) {
        pattern = 2;
    }
    facePatterns[face] = pattern;
}

int Cubo::getFacePattern(int face) const {
    if (face < 0 || face >= 6) {
        return 0;
    }
    return facePatterns[face];
}

bool Cubo::setFacePhotoFromFile(int face, const std::string& path) {
    if (face < 0 || face >= 6) {
        return false;
    }
    int width = 0;
    int height = 0;
    std::vector<unsigned char> data;
    bool hasAlpha = false;
    bool loaded   = false;

#if HAS_STB_IMAGE
    {
        int channels = 0;
        // Sempre carrega RGBA para preservar canal alpha de PNGs transparentes
        unsigned char* raw = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (raw) {
            size_t pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
            if (channels == 4) {
                // Verifica se há pixels semi-transparentes
                hasAlpha = false;
                for (size_t i = 0; i < pixels; ++i) {
                    if (raw[i * 4 + 3] < 255) {
                        hasAlpha = true;
                        break;
                    }
                }
                data.assign(raw, raw + pixels * 4);
            } else if (channels == 3) {
                data.assign(raw, raw + pixels * 3);
                hasAlpha = false;
            } else {
                // Grayscale ou outros: converte para RGB
                data.resize(pixels * 3);
                for (size_t i = 0; i < pixels; ++i) {
                    unsigned char v = raw[i * static_cast<size_t>(channels)];
                    data[i * 3 + 0] = v;
                    data[i * 3 + 1] = v;
                    data[i * 3 + 2] = v;
                }
            }
            stbi_image_free(raw);
            loaded = true;
        } else {
            std::cerr << "stb_image falhou: " << stbi_failure_reason() << std::endl;
        }
    }
#endif

    if (!loaded) {
        // Fallback PPM (sem alpha)
        loaded   = loadPpm(path, width, height, data);
        hasAlpha = false;
    }

    if (!loaded) {
        std::cerr << "Falha ao carregar imagem: " << path << std::endl;
        return false;
    }

    // Remove textura anterior se existir
    if (faceTextures[face] != 0) {
        glDeleteTextures(1, &faceTextures[face]);
        faceTextures[face] = 0;
    }

    // Crop quadrado central (operamos em RGB ou RGBA)
    if (hasAlpha) {
        // cropCenterSquare opera em RGB (3 bytes), adaptar para RGBA
        if (width != height) {
            int side = (width < height) ? width : height;
            int x0 = (width - side) / 2;
            int y0 = (height - side) / 2;
            std::vector<unsigned char> cropped(static_cast<size_t>(side) * static_cast<size_t>(side) * 4u);
            for (int y = 0; y < side; ++y) {
                for (int x = 0; x < side; ++x) {
                    size_t src = (static_cast<size_t>(y0 + y) * static_cast<size_t>(width) + static_cast<size_t>(x0 + x)) * 4u;
                    size_t dst = (static_cast<size_t>(y) * static_cast<size_t>(side) + static_cast<size_t>(x)) * 4u;
                    cropped[dst+0] = data[src+0];
                    cropped[dst+1] = data[src+1];
                    cropped[dst+2] = data[src+2];
                    cropped[dst+3] = data[src+3];
                }
            }
            width = side; height = side;
            data.swap(cropped);
        }
    } else {
        cropCenterSquare(width, height, data);
    }

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Alinhamento de 1 byte obrigatório para imagens com stride arbitrário
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (hasAlpha) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height, 0, GL_RGB,  GL_UNSIGNED_BYTE, data.data());
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restaura padrão

    faceTextures[face]        = texId;
    faceTextureHasAlpha[face] = hasAlpha;
    faceTextureScale[face]    = 1.0f;   // reseta zoom ao carregar nova imagem
    faceTextureRotation[face] = 0;      // reseta rotação ao carregar nova imagem
    return true;
}

/*
 * Define o fator de escala da textura de uma face.
 * Clampado a [0.1, 4.0].
 */
void Cubo::setFaceTextureScale(int face, float s) {
    if (face < 0 || face >= 6) return;
    if (s < 0.1f) s = 0.1f;
    if (s > 4.0f) s = 4.0f;
    faceTextureScale[face] = s;
}

/*
 * Rotaciona a textura da face em passos de 90°.
 * delta = +1 → 90° no sentido horário; delta = -1 → sentido anti-horário.
 * O valor é mantido no intervalo [0, 3] com módulo circular.
 */
void Cubo::rotateFaceTexture(int face, int delta) {
    if (face < 0 || face >= 6) return;
    faceTextureRotation[face] = ((faceTextureRotation[face] + delta) % 4 + 4) % 4;
}
