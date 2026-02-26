/*
 * cubo.cpp
 *
 * implementa o cubo 3d: seis faces com vértices em (-1,-1,-1) a (1,1,1),
 * cada uma com cor própria. render() aplica as rotações acumuladas (rotX, rotY,
 * rotZ) e desenha os quads. selectCurrentFace realiza color picking: desenha o
 * cubo com uma cor de identificação por face (R=1..6), lê o pixel na posição
 * do clique (convertendo y de janela para OpenGL), e define faceSelecionada pelo
 * valor lido. a mistura de cores na face atual pode ser feita por addColorToCurrentFace
 * (lógica local) ou externamente via getFaceColor/setFaceColor e o mixer em Lua.
 */

#include "cubo.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>


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
}

/*
 * Construtor: rotações e face selecionada em zero; todas as seis faces
 * começam com cor branca (1, 1, 1).
 */
Cubo::Cubo() : rotacaoX(0), rotacaoY(0), rotacaoZ(0), faceSelecionada(0) {
    for (int i = 0; i < 6; ++i) {
        coresFaces[i]           = {1.0f, 1.0f, 1.0f};
        texturasFaces[i]         = 0;
        texturasFacesTemAlfa[i]  = false;
        escalasTexturasFaces[i]     = 1.0f;
        rotacoesTexturasFaces[i]  = 0;
    }
}

/*
 * Salva a matriz de modelo, aplica rotações em graus nos eixos X, Y e Z
 * (na ordem fixa glRotatef X, Y, Z), desenha seis GL_QUADS com as cores
 * de coresFaces e restaura a matriz. A ordem dos vértices segue a orientação
 * frontal de cada face para que a normal apontar para fora.
 */
void Cubo::renderizar() {
    glPushMatrix();
    glRotatef(rotacaoX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotacaoY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotacaoZ, 0.0f, 0.0f, 1.0f);
    for (int face = 0; face < 6; ++face) {
        Vertex3 v0;
        Vertex3 v1;
        Vertex3 v2;
        Vertex3 v3;
        faceVertices(face, v0, v1, v2, v3);
        Cor c = coresFaces[face];
        bool hasTexture = texturasFaces[face] != 0;

        if (hasTexture) {
            float scale = escalasTexturasFaces[face];
            int   rot   = rotacoesTexturasFaces[face]; // 0-3: passos de 90° CW

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
            glColor3f(c.vermelho, c.verde, c.azul);
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
            glBindTexture(GL_TEXTURE_2D, texturasFaces[face]);
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
            glDisable(GL_TEXTURE_2D);
            glColor3f(c.vermelho, c.verde, c.azul);
            glBegin(GL_QUADS);
            glVertex3f(v0.x, v0.y, v0.z);
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
            glVertex3f(v3.x, v3.y, v3.z);
            glEnd();
        }
    }
    glPopMatrix();
}

/*
 * Soma os incrementos dx, dy, dz às rotações internas (em graus). Usado
 * após processInput em Lua retornar os deltas para a tecla pressionada.
 */
void Cubo::rotacionar(float dx, float dy, float dz) {
    rotacaoX += dx;
    rotacaoY += dy;
    rotacaoZ += dz;
}

/*
 * Substitui as rotações atuais pelos valores absolutos x, y, z (graus).
 * Usado na inicialização para definir a pose inicial do cubo.
 */
void Cubo::definirRotacao(float x, float y, float z) {
    rotacaoX = x;
    rotacaoY = y;
    rotacaoZ = z;
}

/*
 * limpa completamente a face selecionada: cor para branco e remove textura
 */
void Cubo::limparFaceSelecionada() {
    coresFaces[faceSelecionada] = {1.0f, 1.0f, 1.0f};
    texturasFaces[faceSelecionada] = 0;
    texturasFacesTemAlfa[faceSelecionada] = false;
    escalasTexturasFaces[faceSelecionada] = 1.0f;
    rotacoesTexturasFaces[faceSelecionada] = 0;
}

/*
 * define a cor da face atualmente selecionada para branco
 */
void Cubo::limparCorFaceSelecionada() {
    coresFaces[faceSelecionada] = {1.0f, 1.0f, 1.0f};
}

/*
 * Atualiza a cor da face indicada pelo índice (0–5) para (r, g, b). Índices
 * fora do intervalo são ignorados.
 */
void Cubo::definirCorFace(int face, float r, float g, float b) {
    if (face >= 0 && face < 6) {
        coresFaces[face].vermelho = r;
        coresFaces[face].verde = g;
        coresFaces[face].azul = b;
    }
}

/*
 * Renderiza a cena de color picking (cada face com glColor3ub(i+1, 0, 0)),
 * lê o pixel em (x, y) com glReadPixels e retorna o byte R (0–255).
 * Valores 1–6 correspondem às faces 0–5; 0 indica fundo.
 * A resolução R → índice de face é feita pelo Lua (bridge.resolverFacePicking).
 */
int Cubo::lerPixelPicking(int x, int y) {
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

    glRotatef(rotacaoX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotacaoY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotacaoZ, 0.0f, 0.0f, 1.0f);

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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return (int)pixel[0];
}

/*
 * Define a face selecionada diretamente.
 * Chamada pelo main.cpp após resolução do índice via bridge.resolverFacePicking().
 * Valores fora de [0, 5] são ignorados (mantém seleção atual).
 */
void Cubo::definirFaceSelecionada(int face) {
    if (face >= 0 && face < 6)
        faceSelecionada = face;
}

bool Cubo::definirFotoFaceDeArquivo(int face, const std::string& path) {
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
        loaded   = loadPpm(path, width, height, data);
        hasAlpha = false;
    }

    if (!loaded) {
        return false;
    }

    if (texturasFaces[face] != 0) {
        glDeleteTextures(1, &texturasFaces[face]);
        texturasFaces[face] = 0;
    }

    if (hasAlpha) {
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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (hasAlpha) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height, 0, GL_RGB,  GL_UNSIGNED_BYTE, data.data());
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restaura padrão

    texturasFaces[face]        = texId;
    texturasFacesTemAlfa[face] = hasAlpha;
    escalasTexturasFaces[face]    = 1.0f;   // reseta zoom ao carregar nova imagem
    rotacoesTexturasFaces[face] = 0;      // reseta rotação ao carregar nova imagem
    return true;
}

/*
 * Define o fator de escala da textura de uma face.
 * Clampado a [0.1, 4.0].
 */
void Cubo::definirEscalaTexturaFace(int face, float s) {
    if (face < 0 || face >= 6) return;
    if (s < 0.1f) s = 0.1f;
    if (s > 4.0f) s = 4.0f;
    escalasTexturasFaces[face] = s;
}

/*
 * Rotaciona a textura da face em passos de 90°.
 * delta = +1 → 90° no sentido horário; delta = -1 → sentido anti-horário.
 * O valor é mantido no intervalo [0, 3] com módulo circular.
 */
void Cubo::rotacionarTexturaFace(int face, int delta) {
    if (face < 0 || face >= 6) return;
    rotacoesTexturasFaces[face] = ((rotacoesTexturasFaces[face] + delta) % 4 + 4) % 4;
}
