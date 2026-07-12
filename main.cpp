// =============================================================================
// Sylvan Odyssey - main.cpp
// Game 3D Platformer Endless menggunakan OpenGL + GLFW
// Menu ditampilkan pakai webview (HTML), game pakai OpenGL
// =============================================================================

// ================================ INCLUDE =====================================
#define WEBVIEW_IMPLEMENTATION   // aktifkan implementasi webview di file ini
#include "webview.h"             // library window HTML untuk menu
#if defined(_WIN32)||defined(_WIN64)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>           // Windows API: Sleep, ShowWindow, dll
#else
  #include <unistd.h>            // POSIX: usleep untuk Linux/macOS
#endif
#include <glad/glad.h>                   // loader fungsi-fungsi OpenGL
#include <GLFW/glfw3.h>                  // library window & input keyboard/mouse
#include <glm/glm.hpp>                   // matematika vektor & matriks (vec3, mat4)
#include <glm/gtc/matrix_transform.hpp>  // fungsi translate, rotate, scale, perspective
#include <glm/gtc/type_ptr.hpp>          // value_ptr untuk kirim matriks ke shader
#include <iostream>    // output error ke console
#include <vector>      // std::vector untuk list platform & roket
#include <cmath>       // sinf, fabs, fmodf
#include <string>      // std::string & std::to_string
#include <algorithm>   // std::min, std::max, std::remove_if
#include <atomic>      // std::atomic untuk flag antar webview & game
#include <random>      // std::mt19937, distribusi random
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // load gambar (dipakai jika ada texture)

// ================================ HTML MENU ===================================
// Menu utama ditampilkan sebagai halaman HTML di dalam window webview
// Tombol memanggil window.external.invoke() yang ditangkap oleh onMenuInvoke()
static const char* MENU_HTML = R"HTML(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>Sylvan Odyssey</title><style>
*{margin:0;padding:0;box-sizing:border-box;}
html,body{width:100%;height:100%;overflow:hidden;background:#0a1a0f;font-family:serif;}
#m{width:100%;height:100vh;position:relative;overflow:hidden;}
.sky{position:absolute;inset:0;background:linear-gradient(to bottom,#050d08,#0e2616 40%,#1a3d24 70%,#0d2010);}
.vgn{position:absolute;inset:0;background:radial-gradient(circle,transparent 30%,rgba(3,8,5,.8) 100%);pointer-events:none;}
.box{position:relative;z-index:10;width:100%;height:100%;display:flex;flex-direction:column;justify-content:center;align-items:center;}
h1{font-size:4rem;font-weight:900;color:#e4f5e8;text-transform:uppercase;letter-spacing:10px;text-shadow:0 0 20px rgba(93,214,104,.4);}
p{color:#86b390;letter-spacing:5px;text-transform:uppercase;margin-bottom:60px;}
.wrap{display:flex;flex-direction:column;gap:18px;min-width:280px;}
.btn{background:linear-gradient(135deg,rgba(22,54,31,.6),rgba(13,34,19,.8));border:1px solid #336e43;border-radius:4px;color:#cce3d2;padding:15px 30px;font-family:serif;font-size:1rem;font-weight:700;letter-spacing:3px;text-transform:uppercase;cursor:pointer;transition:all .3s;}
.btn:hover{color:#fff;border-color:#5dd668;box-shadow:0 0 25px rgba(93,214,104,.3);transform:translateY(-2px);}
.prim{background:linear-gradient(135deg,rgba(37,92,51,.8),rgba(16,43,23,.9));border-color:#4a9e5e;color:#fff;}
.foot{position:absolute;bottom:25px;left:50%;transform:translateX(-50%);color:#45694e;font-size:.8rem;letter-spacing:2px;text-transform:uppercase;}
</style></head><body>
<div id="m"><div class="sky"></div><div class="vgn"></div>
<div class="box"><h1>Sylvan Odyssey</h1><p>The Rising Canopy</p>
<div class="wrap">
  <button class="btn prim" onclick="this.textContent='Loading...';this.disabled=true;window.external.invoke('startGame');">Start Journey</button>
  <button class="btn" onclick="window.external.invoke('options');">HOW TO PLAY</button>
  <button class="btn" onclick="window.external.invoke('exit');">Exit</button>
</div></div>
<div class="foot">STMIK AMIK Bandung &copy; 2026</div>
</div></body></html>)HTML";

// ================================ KONSTANTA ==================================
const unsigned int SCR_W = 900; // lebar window game (piksel)
const unsigned int SCR_H = 600; // tinggi window game (piksel)

// ================================ KAMERA =====================================
// Kamera side-view: melihat dari depan, sedikit dari atas
glm::vec3 camPos(0,2.5f,13);    // posisi kamera di dunia 3D
glm::vec3 camFront(0,-0.15f,-1);// arah pandang kamera (sedikit ke bawah)
glm::vec3 camUp(0,1,0);         // arah "atas" dunia
float camTargetY = 2.5f;        // target Y kamera untuk smooth follow

// ================================ PENCAHAYAAN ================================
glm::vec3 lightPos(5,10,6);         // posisi sumber cahaya
glm::vec3 lightColor(1,0.95f,0.85f);// warna cahaya (putih hangat)

// ================================ STATE GAME =================================
enum GameState { PLAYING, PAUSED, GAME_OVER }; // tiga kemungkinan state
GameState gameState = PLAYING;
float dt=0;          // deltaTime: waktu antar frame (detik)
float lastFrame=0;   // waktu frame terakhir
float gameTime=0;    // total waktu bermain
float overlayTimer=0;// timer animasi overlay pause/gameover
float hitCooldown=0; // waktu kebal setelah kena hit (invincibility)
int wallsPassed=0;   // skor = jumlah platform berhasil diinjak
int playerLives=5;   // nyawa pemain

// ================================ FISIKA =====================================
const float GROUND_Y  = 0;      // ketinggian lantai dasar
const float GRAVITY   = -28;    // percepatan gravitasi (negatif = ke bawah)
const float JUMP_FORCE= 10.5f;  // kecepatan awal saat lompat
const float PHW       = 0.36f;  // setengah lebar player (untuk collision)
const float PH        = 1.9f;   // tinggi player (untuk collision)
const float PSPEED    = 6;      // kecepatan gerak kiri-kanan
const float PBOUND    = 6.8f;   // batas kiri-kanan layar

// ================================ PLAYER =====================================
struct Player {
    glm::vec3 pos{0,0,0}; // posisi player di dunia 3D
    float velY=0;          // kecepatan vertikal (+ naik, - turun)
    float animTimer=0;     // timer untuk animasi kaki/tangan berjalan
    float standingY=0;     // ketinggian lantai yang sedang dipijak
    bool  onGround=true;   // apakah sedang berdiri di lantai/platform
} player;

// ================================ PLATFORM ===================================
// Platform (Wall) bergerak dari kiri/kanan, player harus melompat ke atasnya
struct Wall {
    float x,y;         // posisi tengah platform
    float width;       // setengah lebar (collision = player.x ± width)
    float height;      // setengah tinggi platform
    float speedSign;   // arah gerak: -1=ke kiri, +1=ke kanan
    float delayTimer;  // sisa waktu sebelum platform mulai masuk layar
    int   id;          // nomor urut platform (untuk warna gradasi)
    bool  passed;      // sudah diinjak player? (skor sudah dihitung)
    bool  alive;       // masih aktif di layar?
    bool  playerOn;    // player sedang berdiri di atasnya?
    bool  isFrozen;    // berhenti bergerak (sudah diinjak atau kena player)
    bool  waitingDelay;// sedang menunggu delay sebelum masuk layar
};
std::vector<Wall> walls;  // daftar semua platform aktif
float wallSpeed=5.5f;     // kecepatan platform (bertambah seiring skor)
bool spawnFromRight=false; // giliran spawn: kiri atau kanan (bergantian)

// ================================ ROKET ======================================
// Roket jatuh dari atas, spawn setiap kelipatan 10 tangga (1 atau 2 acak)
struct Rocket {
    float x,y;   // posisi roket
    float velY;  // kecepatan jatuh (negatif)
    float trail; // timer animasi api ekor
    bool  alive; // masih aktif?
};
std::vector<Rocket> rockets; // daftar roket aktif
int lastMilestone=0;         // milestone terakhir yang sudah spawn roket

// ================================ SHADER 3D ==================================
// Vertex Shader: transformasi posisi vertex dari dunia 3D ke layar 2D
const char* vertSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;    // posisi vertex (dari cubeVerts)
layout(location=1) in vec3 aNormal; // normal permukaan (untuk pencahayaan)
out vec3 FragPos, Normal;            // diteruskan ke fragment shader
uniform mat4 model, view, projection;// transformasi objek, kamera, perspektif
void main(){
    FragPos = vec3(model * vec4(aPos,1));                          // posisi di dunia
    Normal  = mat3(transpose(inverse(model))) * aNormal;           // normal di dunia
    gl_Position = projection * view * vec4(FragPos,1);             // posisi di layar
})";

// Fragment Shader: hitung warna piksel pakai model Phong (ambient + diffuse)
const char* fragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos, Normal;
uniform vec3 lightPos, lightColor, objectColor;
uniform float alpha; // transparansi (1=opak, 0=invisible)
void main(){
    // diffuse: makin tegak lurus ke cahaya, makin terang
    float diff = max(dot(normalize(Normal), normalize(lightPos-FragPos)), 0.0);
    // 0.38 = ambient (cahaya merata), 0.62 = porsi diffuse
    FragColor = vec4((0.38 + diff*0.62) * lightColor * objectColor, alpha);
})";

// Shadow Shader: gambar elips hitam memudar dari tengah ke tepi
const char* shadowFragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
uniform float alpha;        // intensitas bayangan di tengah
uniform vec3  shadowCenter; // posisi pusat elips di dunia
uniform vec2  shadowRadius; // radius elips (x=lebar, y=dalam)
void main(){
    float dx = (FragPos.x - shadowCenter.x) / shadowRadius.x;
    float dz = (FragPos.z - shadowCenter.z) / shadowRadius.y;
    float d  = dx*dx + dz*dz; // jarak kuadrat dari pusat
    if(d > 1.0) discard;       // buang piksel di luar elips
    FragColor = vec4(0,0,0, alpha*(1.0-d)); // makin pinggir makin transparan
})";

// ================================ FONT BITMAP ================================
// Font 8x8 piksel untuk ASCII 32-127 (total 96 karakter)
// Tiap karakter = 8 byte = 8 baris piksel, tiap bit = 1 piksel (1=nyala)
static const unsigned char FONT8[96][8]={
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
{0x6C,0x6C,0x28,0x00,0x00,0x00,0x00,0x00},{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
{0x18,0x7C,0xD0,0x7C,0x16,0xFC,0x18,0x00},{0xC2,0xC6,0x0C,0x18,0x30,0x66,0xC6,0x00},
{0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
{0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
{0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x7C,0xC6,0x06,0x1C,0x70,0xC6,0xFE,0x00},{0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},
{0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00},{0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},
{0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},{0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00},
{0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},{0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},
{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
{0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
{0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},{0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
{0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00},{0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0x00},
{0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},{0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},
{0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},{0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},
{0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00},{0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0x00},
{0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},{0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
{0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00},{0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00},
{0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},{0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},{0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00},
{0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00},{0x38,0x6C,0xC6,0xC6,0xCE,0x6C,0x3A,0x00},
{0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00},{0x78,0xCC,0xC0,0x70,0x18,0xCC,0x78,0x00},
{0xFC,0xB4,0x30,0x30,0x30,0x30,0x78,0x00},{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00},
{0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
{0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00},{0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00},
{0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
{0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
{0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00},
{0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00},
{0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00},{0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00},
{0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00},{0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00},
{0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8},
{0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00},{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
{0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C},{0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00},
{0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x00,0x00,0xCC,0xFE,0xD6,0xC6,0xC6,0x00},
{0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00},{0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00},
{0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E},
{0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00},{0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00},
{0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00},
{0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},
{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8},
{0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00},{0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00},
{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},{0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00},
{0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00},
};

// ================================ GPU HANDLES ================================
unsigned int fontTex,fontVAO,fontVBO2,fontProg,whiteTex; // handle untuk render teks/UI 2D
unsigned int VAO,VBO,shaderProg,shadowProg;               // handle untuk render objek 3D

// Vertex shader font 2D: posisi piksel + UV koordinat -> clip space
const char* fontV = R"(#version 330 core
layout(location=0) in vec2 aPos; layout(location=1) in vec2 aUV;
out vec2 vUV; uniform mat4 proj;
void main(){ vUV=aUV; gl_Position=proj*vec4(aPos,0,1); })";

// Fragment shader font: ambil channel R dari atlas sebagai alpha, warna dari uniform
const char* fontF = R"(#version 330 core
in vec2 vUV; out vec4 C; uniform sampler2D tex; uniform vec4 color;
void main(){ C=vec4(color.rgb, color.a*texture(tex,vUV).r); })";

// ================================ INIT FONT ==================================
// Bangun atlas tekstur 128x48 dari data FONT8 lalu upload ke GPU
void initFont(){
    static unsigned char atlas[48][128]={};
    for(int c=0;c<96;c++){                         // loop 96 karakter
        int col=c%16, row=c/16;                    // posisi di grid atlas (16 per baris)
        for(int y=0;y<8;y++){
            unsigned char b=FONT8[c][y];           // 1 byte = 8 piksel baris
            for(int x=0;x<8;x++)
                if(b&(0x80>>x)) atlas[row*8+y][col*8+x]=255; // bit 1 = piksel putih
        }
    }
    glGenTextures(1,&fontTex); glBindTexture(GL_TEXTURE_2D,fontTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,128,48,0,GL_RED,GL_UNSIGNED_BYTE,atlas);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); // piksel font tajam
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    unsigned char w=255; // tekstur putih 1x1 untuk kotak solid
    glGenTextures(1,&whiteTex); glBindTexture(GL_TEXTURE_2D,whiteTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&w);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glGenVertexArrays(1,&fontVAO); glGenBuffers(1,&fontVBO2); // VAO/VBO quad 2D
    glBindVertexArray(fontVAO); glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*24,NULL,GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// ================================ RENDER 2D ==================================
// Gambar satu quad 2D (2 segitiga) dengan koordinat UV
static void quadDraw(float x,float y,float w,float h,float u0,float v0,float u1,float v1){
    float v[6][4]={{x,y+h,u0,v1},{x,y,u0,v0},{x+w,y,u1,v0},
                   {x,y+h,u0,v1},{x+w,y,u1,v0},{x+w,y+h,u1,v1}};
    glBindVertexArray(fontVAO); glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v); glDrawArrays(GL_TRIANGLES,0,6);
}

// Gambar kotak warna solid (HUD, overlay, panel)
void renderRect(float x,float y,float w,float h,float r,float g,float b,float a){
    glUseProgram(fontProg); glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,whiteTex); // tekstur putih = warna solid
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);
    quadDraw(x,y,w,h,0,0,1,1);
}

// Gambar string teks di layar dengan posisi, skala, dan warna
void renderText(const std::string& s,float x,float y,float sc,float r=1,float g=1,float b=1,float a=1){
    glUseProgram(fontProg); glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,fontTex); // tekstur atlas font
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);
    for(char c:s){
        if(c<32||c>127) c=32;             // karakter tidak valid = spasi
        int idx=c-32, col=idx%16, row=idx/16; // posisi karakter di grid atlas
        // UV koordinat karakter di atlas 128x48
        quadDraw(x,y,8*sc,8*sc, col*8/128.f, row*8/48.f, (col+1)*8/128.f, (row+1)*8/48.f);
        x+=8*sc; // geser ke karakter berikutnya
    }
}

// Gambar ikon hati pixel-art (nyawa)
void renderHeart(float x,float y,float s,bool filled){
    float r=filled?.95f:.28f, g=filled?.15f:.22f, b=filled?.18f:.26f;
    // 7 kotak kecil membentuk shape hati pixel-art
    renderRect(x+s,y,s,s,r,g,b,1);    renderRect(x+4*s,y,s,s,r,g,b,1);
    renderRect(x,y+s,6*s,s,r,g,b,1);  renderRect(x,y+2*s,6*s,s,r,g,b,1);
    renderRect(x+s,y+3*s,4*s,s,r,g,b,1);
    renderRect(x+2*s,y+4*s,2*s,s,r,g,b,1);
    renderRect(x+3*s,y+5*s,s,s,r,g,b,1);
}

// ================================ DATA KUBUS =================================
// 36 vertex (6 sisi x 2 segitiga x 3 vertex), format: posXYZ + normalXYZ
float cubeVerts[]={
    // Sisi belakang (-Z)
    -1,-1,-1,0,0,-1, 1,-1,-1,0,0,-1, 1,1,-1,0,0,-1,
     1, 1,-1,0,0,-1,-1, 1,-1,0,0,-1,-1,-1,-1,0,0,-1,
    // Sisi depan (+Z)
    -1,-1,1,0,0,1,   1,-1,1,0,0,1,   1,1,1,0,0,1,
     1, 1,1,0,0,1,  -1, 1,1,0,0,1,  -1,-1,1,0,0,1,
    // Sisi kiri (-X)
    -1,1,1,-1,0,0, -1,1,-1,-1,0,0, -1,-1,-1,-1,0,0,
    -1,-1,-1,-1,0,0,-1,-1,1,-1,0,0,-1,1,1,-1,0,0,
    // Sisi kanan (+X)
     1,1,1,1,0,0,  1,1,-1,1,0,0,   1,-1,-1,1,0,0,
     1,-1,-1,1,0,0, 1,-1,1,1,0,0,   1,1,1,1,0,0,
    // Sisi bawah (-Y)
    -1,-1,-1,0,-1,0, 1,-1,-1,0,-1,0, 1,-1,1,0,-1,0,
     1,-1,1,0,-1,0, -1,-1,1,0,-1,0, -1,-1,-1,0,-1,0,
    // Sisi atas (+Y)
    -1,1,-1,0,1,0,  1,1,-1,0,1,0,   1,1,1,0,1,0,
     1, 1,1,0,1,0, -1, 1,1,0,1,0,  -1, 1,-1,0,1,0,
};

// ================================ SHADER HELPER ==============================
// Compile vertex + fragment shader, link jadi satu program GPU
unsigned int makeShader(const char* vs,const char* fs){
    auto compile=[](unsigned int t,const char* src){
        unsigned int s=glCreateShader(t);
        glShaderSource(s,1,&src,NULL); glCompileShader(s);
        int ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[512];glGetShaderInfoLog(s,512,NULL,log);std::cerr<<log;}
        return s;
    };
    unsigned int v=compile(GL_VERTEX_SHADER,vs);
    unsigned int f=compile(GL_FRAGMENT_SHADER,fs);
    unsigned int p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f); // sudah di-link, bisa dihapus
    return p;
}

// Kirim matrix view & projection ke shader (dipanggil sebelum render 3D)
void setUniforms(unsigned int prog,glm::mat4& proj,glm::mat4& view){
    glUseProgram(prog); glBindVertexArray(VAO);
    glUniformMatrix4fv(glGetUniformLocation(prog,"projection"),1,GL_FALSE,glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(prog,"view"),1,GL_FALSE,glm::value_ptr(view));
}

// Gambar satu kubus 3D di posisi dengan skala dan warna tertentu
void drawCube(unsigned int prog,glm::vec3 pos,glm::vec3 sc,glm::vec3 col,float a=1){
    glm::mat4 m=glm::scale(glm::translate(glm::mat4(1),pos),sc); // translate lalu scale
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(col));
    glUniform1f(glGetUniformLocation(prog,"alpha"),a);
    glDrawArrays(GL_TRIANGLES,0,36); // 36 vertex = 1 kubus penuh
}

// ================================ KARAKTER ===================================
// Gambar satu anggota gerak (kaki/tangan) dengan rotasi dari titik pivot
// Pivot = titik putar (pangkal kaki/tangan), angle = derajat ayunan
void drawLimb(unsigned int prog,glm::vec3 pivot,float angle,glm::vec3 offset,glm::vec3 sc,glm::vec3 col){
    glm::mat4 m=glm::translate(glm::mat4(1),pivot);         // pindah ke pivot
    m=glm::rotate(m,glm::radians(angle),glm::vec3(1,0,0));  // rotasi di pivot (sumbu X)
    m=glm::translate(m,offset);                              // geser ke posisi anggota
    m=glm::scale(m,sc);                                      // atur ukuran
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(col));
    glUniform1f(glGetUniformLocation(prog,"alpha"),1);
    glDrawArrays(GL_TRIANGLES,0,36);
}

// Gambar karakter player lengkap: kaki, badan, tangan, kepala+mata
// leg & arm = sudut ayunan (berubah saat berjalan via animTimer)
void drawPlayer(unsigned int prog,glm::vec3 base,float s,float leg,float arm){
    glm::vec3 bC(0.2f,0.55f,0.95f);  // warna badan: biru
    glm::vec3 hC(0.96f,0.8f,0.62f);  // warna kepala: skin tone
    glm::vec3 lC(0.12f,0.22f,0.6f);  // warna kaki/tangan: biru tua
    glm::vec3 W(0.95f,0.95f,0.95f);  // warna mata: putih
    glm::vec3 B(0.05f,0.05f,0.05f);  // warna pupil: hitam

    // Kaki kiri dan kanan berayun berlawanan (leg vs -leg)
    drawLimb(prog,base+glm::vec3(-s*.2f,s*.3f,0), leg,{0,-s*.3f,0},{s*.16f,s*.3f,s*.16f},lC);
    drawLimb(prog,base+glm::vec3( s*.2f,s*.3f,0),-leg,{0,-s*.3f,0},{s*.16f,s*.3f,s*.16f},lC);
    // Badan
    drawCube(prog,base+glm::vec3(0,s*.85f,0),{s*.36f,s*.38f,s*.2f},bC);
    // Tangan kiri dan kanan berlawanan dengan kaki (agar natural)
    drawLimb(prog,base+glm::vec3(-s*.52f,s*.95f,0),-arm,{0,-s*.24f,0},{s*.14f,s*.26f,s*.14f},lC);
    drawLimb(prog,base+glm::vec3( s*.52f,s*.95f,0), arm,{0,-s*.24f,0},{s*.14f,s*.26f,s*.14f},lC);
    // Kepala
    drawCube(prog,base+glm::vec3(0,s*1.52f,0),{s*.33f,s*.3f,s*.26f},hC);
    // Mata kiri (bola putih + pupil hitam)
    drawCube(prog,base+glm::vec3(-s*.13f,s*1.59f,s*.30f),{s*.07f,s*.07f,s*.04f},W);
    drawCube(prog,base+glm::vec3(-s*.13f,s*1.59f,s*.35f),{s*.04f,s*.04f,s*.03f},B);
    // Mata kanan
    drawCube(prog,base+glm::vec3( s*.13f,s*1.59f,s*.30f),{s*.07f,s*.07f,s*.04f},W);
    drawCube(prog,base+glm::vec3( s*.13f,s*1.59f,s*.35f),{s*.04f,s*.04f,s*.03f},B);
}

// ================================ SPAWN PLATFORM =============================
// Buat platform baru dan masukkan ke vector walls
// forceSame: paksa arah sama dengan sebelumnya | delay: tunggu sebelum masuk layar
void spawnWall(bool forceSame=false,float delay=0){
    Wall w;
    w.id=wallsPassed+1;
    w.height=0.16f;
    w.passed=w.playerOn=w.isFrozen=false;
    w.alive=true;
    w.waitingDelay=(delay>0);
    w.delayTimer=delay;
    // Lebar berkurang 0.55 setiap 10 tangga, minimum 1.5
    w.width=std::max(6.f-(wallsPassed/10)*0.55f,1.5f);
    // Arah datang bergantian kiri-kanan (kecuali forceSame)
    bool right=forceSame?!spawnFromRight:spawnFromRight;
    w.speedSign=right?-1.f:1.f;  // -1=bergerak ke kiri, +1=ke kanan
    w.x=w.waitingDelay?999.f:(right?16.f:-16.f); // posisi awal di luar layar
    if(!forceSame) spawnFromRight=!spawnFromRight; // ganti giliran
    w.y=player.standingY+0.75f;  // satu langkah di atas lantai player
    walls.push_back(w);
}

// ================================ RESET GAME =================================
void resetGame(){
    player={};           // reset semua field player ke default
    walls.clear();
    rockets.clear();
    wallsPassed=0; wallSpeed=5.5f; gameTime=0;
    gameState=PLAYING; playerLives=5; overlayTimer=0;
    hitCooldown=0; spawnFromRight=false; lastMilestone=0;
    camPos.y=2.5f; camTargetY=2.5f;
    spawnWall(false,1.2f); // platform pertama muncul setelah 1.2 detik
}

// ================================ INPUT ======================================
static bool g_backToMenu=false; // flag: player tekan M untuk ke menu
static bool keys[512]={};       // status tombol keyboard (true=ditekan)

void key_callback(GLFWwindow* win,int key,int,int action,int){
    // Catat tombol ditekan/dilepas
    if(key>=0&&key<512){
        if(action==GLFW_PRESS)   keys[key]=true;
        if(action==GLFW_RELEASE) keys[key]=false;
    }
    if(action==GLFW_PRESS){
        // Lompat: SPACE / W / panah atas (hanya saat di lantai & PLAYING)
        if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W)
           &&player.onGround&&gameState==PLAYING){
            player.velY=JUMP_FORCE; player.onGround=false;
        }
        // ESC atau P: toggle pause
        if(key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P){
            if(gameState==PLAYING)      gameState=PAUSED;
            else if(gameState==PAUSED)  gameState=PLAYING;
        }
        if(key==GLFW_KEY_R) resetGame(); // restart
        if(key==GLFW_KEY_M){ g_backToMenu=true; glfwSetWindowShouldClose(win,GLFW_TRUE); }
    }
}
// Callback resize window: update viewport OpenGL
void framebuffer_size_callback(GLFWwindow*,int w,int h){ glViewport(0,0,w,h); }

// ================================ DAMAGE =====================================
// Kurangi nyawa, aktifkan invincibility, respawn atau game over
void takeDamage(){
    playerLives--; hitCooldown=1.8f;
    if(playerLives<=0){ gameState=GAME_OVER; overlayTimer=0; return; }
    player.pos={0,player.standingY,0}; // respawn ke lantai terakhir
    player.velY=0; player.onGround=true;
}

// ================================ MAIN GAME ==================================
bool runGame(){
    g_backToMenu=false; lastFrame=0;
    // Setup GLFW + window OpenGL 3.3 Core
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(SCR_W,SCR_H,"Sylvan Odyssey",NULL,NULL);
#if defined(_WIN32)||defined(_WIN64)
    { int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
      glfwSetWindowPos(win,(sw-SCR_W)/2,(sh-SCR_H)/2); } // tengahkan window
#endif
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback);
    glfwSetKeyCallback(win,key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); // load fungsi OpenGL
    glEnable(GL_DEPTH_TEST);  // objek dekat menutupi yang jauh
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); // transparansi standar

    // Compile 3 shader program yang dibutuhkan
    shaderProg=makeShader(vertSrc,fragSrc);         // render objek 3D
    shadowProg=makeShader(vertSrc,shadowFragSrc);   // render bayangan
    fontProg  =makeShader(fontV,fontF);             // render teks & UI
    initFont();

    // Upload data kubus ke GPU (VAO+VBO, dipakai untuk SEMUA objek 3D)
    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); // posisi
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); // normal
    glEnableVertexAttribArray(1);

    resetGame();

    // ============================== GAME LOOP ================================
    while(!glfwWindowShouldClose(win)){
        // DeltaTime: waktu antar frame, penting agar physics konsisten di semua FPS
        float now=(float)glfwGetTime();
        dt=std::min(now-lastFrame,0.05f); // cap 50ms agar tidak spike
        lastFrame=now;
        overlayTimer+=dt;
        if(hitCooldown>0) hitCooldown-=dt; // hitung mundur invincibility

        // ============================ UPDATE =================================
        if(gameState==PLAYING){
            gameTime+=dt;
            player.animTimer+=dt; // timer untuk animasi kaki/tangan

            // Fisika vertikal: gravitasi + gerak player
            player.velY+=GRAVITY*dt;
            player.pos.y+=player.velY*dt;

            // Gerak horizontal kiri/kanan
            if(keys[GLFW_KEY_A]||keys[GLFW_KEY_LEFT])  player.pos.x-=PSPEED*dt;
            if(keys[GLFW_KEY_D]||keys[GLFW_KEY_RIGHT]) player.pos.x+=PSPEED*dt;
            player.pos.x=std::max(-PBOUND,std::min(PBOUND,player.pos.x)); // clamp batas layar

            // Cek lantai dasar
            bool onGnd=false; float floor=GROUND_Y;
            if(player.pos.y<=GROUND_Y){ player.pos.y=GROUND_Y; player.velY=0; onGnd=true; }

            bool nextSpawn=false;
            std::vector<Wall> pending; // platform pengganti (ditambah setelah loop)

            // =================== COLLISION PLATFORM ========================
            for(auto& w:walls){
                if(!w.alive||w.waitingDelay) continue;
                if(fabs(player.pos.x-w.x)>=PHW+w.width) continue; // tidak overlap X

                float foot=player.pos.y, head=player.pos.y+PH;
                float top=w.y+w.height, bot=w.y-w.height;

                // --- KASUS 1: Player mendarat di atas platform ---
                if(player.velY<=0&&foot>=top-0.35f&&foot<=top+0.15f){
                    if(!w.isFrozen&&!w.passed){
                        // Platform baru: freeze, tambah skor, spawn berikutnya
                        w.isFrozen=w.passed=true;
                        wallsPassed++; nextSpawn=true;
                        player.pos.y=top; player.velY=0;
                        onGnd=true; floor=top; w.playerOn=true;
                    }
                    else if(w.passed&&top<player.standingY-0.1f&&hitCooldown<=0){
                        // Turun ke platform LAMA (lebih rendah) = damage
                        takeDamage();
                    }
                    else{
                        // Platform lama setingkat = landing normal (tanpa skor)
                        player.pos.y=top; player.velY=0;
                        onGnd=true; floor=top; w.playerOn=true;
                    }
                }
                // --- KASUS 2: Platform menabrak player dari samping ---
                else if(!w.isFrozen&&!w.passed&&hitCooldown<=0
                        &&foot<top-0.15f&&head>bot+0.15f){
                    w.isFrozen=true; w.alive=false;
                    // Buat platform pengganti dengan delay 1 detik
                    Wall rep=w;
                    rep.passed=rep.playerOn=rep.isFrozen=false;
                    rep.alive=true; rep.waitingDelay=true;
                    rep.delayTimer=1.f; rep.x=999.f;
                    pending.push_back(rep);
                    // Freeze semua platform yang bergerak saat ini
                    for(auto& fw:walls)
                        if(fw.alive&&!fw.isFrozen&&!fw.waitingDelay) fw.isFrozen=true;
                    takeDamage();
                }
            }
            for(auto& r:pending) walls.push_back(r); // tambah platform pengganti

            // Update status grounded player
            player.onGround=onGnd;
            if(onGnd){ player.standingY=floor; player.pos.y=floor; player.velY=0; }
            if(nextSpawn&&gameState==PLAYING) spawnWall(false,0.15f);
            wallSpeed=std::min(5.5f+(float)wallsPassed*0.45f,22.f); // makin cepat seiring skor

            // =================== SISTEM ROKET ==============================
            // Spawn wave roket tiap kelipatan 10 tangga (1-2 roket acak)
            int ms=wallsPassed/10;
            if(ms>0&&ms>lastMilestone){
                lastMilestone=ms;
                std::mt19937 rng(std::random_device{}());
                int cnt=1+(rng()%2); // 1 atau 2 roket
                std::uniform_real_distribution<float> xd(-5.5f,5.5f);
                for(int k=0;k<cnt;k++)
                    rockets.push_back({xd(rng), player.pos.y+8.f, -(7.f+ms*.3f), 0, true});
            }
            // Update roket & cek collision dengan player
            for(auto& r:rockets){
                if(!r.alive) continue;
                r.y+=r.velY*dt; r.trail+=dt;
                if(r.y<player.standingY-2){ r.alive=false; continue; } // lewat lantai
                if(hitCooldown<=0
                   &&fabs(r.x-player.pos.x)<PHW+0.25f
                   &&r.y>=player.pos.y-.1f&&r.y<=player.pos.y+PH+.2f){
                    r.alive=false; takeDamage(); // kena roket
                }
            }
            rockets.erase(std::remove_if(rockets.begin(),rockets.end(),
                [](const Rocket& r){return !r.alive;}),rockets.end());

            // =================== UPDATE PLATFORM ===========================
            for(size_t i=0;i<walls.size();i++){
                if(!walls[i].alive) continue;
                if(walls[i].waitingDelay){ // masih menunggu delay
                    walls[i].delayTimer-=dt;
                    if(walls[i].delayTimer<=0){
                        walls[i].waitingDelay=false;
                        walls[i].x=walls[i].speedSign<0?16.f:-16.f; // masuk dari tepi
                    }
                    continue;
                }
                if(!walls[i].isFrozen) walls[i].x+=walls[i].speedSign*wallSpeed*dt;
                // Platform keluar layar: matikan dan spawn baru dari arah sama
                bool out=(walls[i].speedSign<0&&walls[i].x<-16)||(walls[i].speedSign>0&&walls[i].x>16);
                if(out&&!walls[i].isFrozen){ walls[i].alive=false; spawnWall(true,0.5f); }
            }

            // Kamera smooth follow player (interpolasi linear)
            camTargetY=player.pos.y+2.5f;
            camPos.y+=(camTargetY-camPos.y)*8.f*dt;
        }

        // Update judul window
        std::string title;
        if(gameState==PLAYING)
            title="Sylvan Odyssey | Score:"+std::to_string(wallsPassed)+" | Lives:"+std::to_string(playerLives)+" | ESC=Pause";
        else if(gameState==PAUSED) title="PAUSED | ESC=Resume | R=Restart";
        else title="GAME OVER | Score:"+std::to_string(wallsPassed)+" | R=Restart";
        glfwSetWindowTitle(win,title.c_str());

        // ============================== RENDER ===============================
        float camOff=camPos.y-2.5f; // offset kamera untuk background ikut naik
        glClearColor(0.38f,0.62f,0.88f,1); // warna langit biru
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        // Matrix perspektif & kamera
        glm::mat4 proj=glm::perspective(glm::radians(62.f),(float)SCR_W/SCR_H,0.1f,100.f);
        glm::mat4 view=glm::lookAt(camPos,camPos+camFront,camUp);
        setUniforms(shaderProg,proj,view);
        glUniform3fv(glGetUniformLocation(shaderProg,"lightPos"),1,glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProg,"lightColor"),1,glm::value_ptr(lightColor));

        // --- BACKGROUND ---
        // Panel langit biru besar (ikut offset kamera)
        drawCube(shaderProg,{0,3.5f+camOff,-9},{22,20,0.3f},{0.55f,0.78f,0.98f});
        // Matahari kuning di tengah atas
        drawCube(shaderProg,{0,6.5f+camOff,-7},{0.6f,0.6f,0.2f},{1,0.95f,0.35f});

        // --- LANTAI ---
        drawCube(shaderProg,{0,-0.3f,0},{15,0.3f,2.5f},{0.32f,0.58f,0.24f}); // tanah hijau
        drawCube(shaderProg,{0, 0.02f,0},{15,0.05f,2.5f},{0.42f,0.72f,0.32f}); // highlight tepi atas

        // --- PLATFORM (MOVING SHAPE) ---
        setUniforms(shaderProg,proj,view);
        for(auto& w:walls){
            if(!w.alive||w.waitingDelay) continue;
            // Warna gradasi merah->hijau berdasarkan nomor urut platform (mod 10)
            float f=(float)((w.id-1)%10+1)/10.f;
            glm::vec3 wc=glm::mix(glm::vec3(0.92f,0.25f,0.2f),glm::vec3(0.15f,0.85f,0.4f),f);
            if(w.isFrozen) wc*=0.82f; // platform beku sedikit gelap
            drawCube(shaderProg,{w.x,w.y,0},{w.width,w.height,0.55f},wc);
            // Highlight tipis di tepi atas platform
            drawCube(shaderProg,{w.x,w.y+w.height,0},{w.width+0.02f,0.02f,0.58f},
                     glm::clamp(wc*1.25f,glm::vec3(0),glm::vec3(1)));
        }

        // --- SHADOW (bayangan player di lantai) ---
        {
            setUniforms(shadowProg,proj,view);
            // Cari lantai terdekat di bawah player
            float sf=GROUND_Y;
            for(auto& w:walls)
                if(w.alive&&!w.waitingDelay&&w.y+w.height<=player.pos.y+0.02f
                   &&fabs(player.pos.x-w.x)<PHW+w.width)
                    sf=std::max(sf,w.y+w.height);
            float h=std::max(player.pos.y-sf,0.f);
            float rX=std::max(0.55f-h*.045f,.18f); // radius elips X (makin tinggi, makin kecil)
            float rZ=std::max(0.32f-h*.025f,.10f); // radius elips Z
            float al=std::max(0.85f-h*.07f,.15f);  // alpha (makin tinggi, makin pudar)
            float sy=sf+0.08f; // offset Y agar tidak Z-fight dengan lantai
            glUniform3f(glGetUniformLocation(shadowProg,"shadowCenter"),player.pos.x,sy,player.pos.z);
            glUniform2f(glGetUniformLocation(shadowProg,"shadowRadius"),rX,rZ);
            glUniform1f(glGetUniformLocation(shadowProg,"alpha"),al);
            glDepthMask(GL_FALSE); // jangan tulis ke depth buffer
            glm::mat4 sm=glm::scale(glm::translate(glm::mat4(1),{player.pos.x,sy,player.pos.z}),
                                    {rX*2.2f,0.005f,rZ*2.2f});
            glUniformMatrix4fv(glGetUniformLocation(shadowProg,"model"),1,GL_FALSE,glm::value_ptr(sm));
            glDrawArrays(GL_TRIANGLES,0,36);
            glDepthMask(GL_TRUE);
            setUniforms(shaderProg,proj,view);
        }

        // --- ROKET ---
        for(auto& r:rockets){
            if(!r.alive) continue;
            float t=(float)glfwGetTime();
            float fl=0.12f+0.1f*sinf(t*22+r.x*3); // api bergoyang dengan sinf
            drawCube(shaderProg,{r.x,r.y,0},     {0.18f,0.70f,0.18f},{0.85f,0.85f,0.90f}); // badan
            drawCube(shaderProg,{r.x,r.y-.55f,0},{0.14f,0.28f,0.14f},{0.90f,0.12f,0.08f}); // hidung merah
            drawCube(shaderProg,{r.x,r.y-.75f,0},{0.08f,0.14f,0.08f},{0.75f,0.08f,0.05f}); // ujung runcing
            drawCube(shaderProg,{r.x,r.y+.05f,0},{0.20f,0.06f,0.20f},{0.30f,0.40f,0.55f}); // jendela
            drawCube(shaderProg,{r.x-.28f,r.y+.38f,0},{0.18f,0.20f,0.12f},{0.55f,0.55f,0.65f}); // sirip kiri
            drawCube(shaderProg,{r.x+.28f,r.y+.38f,0},{0.18f,0.20f,0.12f},{0.55f,0.55f,0.65f}); // sirip kanan
            drawCube(shaderProg,{r.x-.20f,r.y-.30f,0},{0.10f,0.12f,0.10f},{0.50f,0.50f,0.60f}); // stabil kiri
            drawCube(shaderProg,{r.x+.20f,r.y-.30f,0},{0.10f,0.12f,0.10f},{0.50f,0.50f,0.60f}); // stabil kanan
            drawCube(shaderProg,{r.x,r.y+.62f,0},{fl*.7f,.22f,fl*.7f},{1,1,.6f},1);         // api putih
            drawCube(shaderProg,{r.x,r.y+.80f,0},{fl+.06f,.3f,fl+.06f},{1,.5f,.05f},.85f); // api oranye
            drawCube(shaderProg,{r.x,r.y+1.02f,0},{fl+.1f,.28f,fl+.1f},{.95f,.18f,.02f},.65f); // api merah
        }

        // --- KARAKTER PLAYER (3D SHAPE) ---
        // leg & arm: sudut ayunan kaki/tangan (sinf saat jalan, fixed saat lompat)
        float leg=player.onGround?sinf(player.animTimer*3)*8.f:-22.f;
        float arm=player.onGround?sinf(player.animTimer*3)*8.f:-30.f;
        drawPlayer(shaderProg,player.pos,0.58f,leg,arm);

        // ============================== HUD ==================================
        {
            glDisable(GL_DEPTH_TEST); // UI 2D tidak perlu depth test
            // Proyeksi ortografis: koordinat = piksel layar langsung
            glm::mat4 hudP=glm::ortho(0.f,(float)SCR_W,(float)SCR_H,0.f,-1.f,1.f);
            glUseProgram(fontProg);
            glUniformMatrix4fv(glGetUniformLocation(fontProg,"proj"),1,GL_FALSE,glm::value_ptr(hudP));

            float sc=2, cw=16, ch=16; // skala teks, lebar & tinggi 1 karakter

            // Panel header gelap di atas
            renderRect(0,0,(float)SCR_W,ch+14,0.03f,0.08f,0.04f,0.88f);
            renderRect(0,ch+12,(float)SCR_W,2,0.22f,0.68f,0.3f,0.6f); // garis hijau

            // Ikon hati nyawa (5 hati, merah=ada, abu=hilang)
            for(int i=0;i<5;i++) renderHeart(10+i*32,5,sc*.9f,i<playerLives);

            // Skor di tengah
            std::string sc_str="SCORE: "+std::to_string(wallsPassed);
            renderText(sc_str,SCR_W/2.f-sc_str.size()*cw/2,5,sc,0.7f,0.98f,0.72f);

            // Hint kontrol di kanan
            renderText("A/D=MOVE  ESC=PAUSE",SCR_W-10-20*cw,5,sc,0.4f,0.6f,0.42f,0.8f);

            // Bar kecepatan (hijau->merah seiring bertambah)
            float bx=SCR_W-170,by=ch+2,spd=std::min(wallSpeed/22.f,1.f);
            renderRect(bx,by,160,6,0.08f,0.18f,0.09f,0.9f); // background bar
            if(spd>0) renderRect(bx,by,160*spd,6,0.2f+0.75f*spd,0.9f-0.65f*spd,0.35f,1);

            // Warning roket (berkedip saat ada roket aktif)
            if(!rockets.empty()){
                float p=0.65f+0.35f*sinf((float)glfwGetTime()*6);
                std::string w2="!! ROCKET INCOMING !!";
                renderText(w2,SCR_W/2.f-w2.size()*cw*.8f/2,ch+20,.8f*sc,1,.3f*p,.05f,p);
            }

            // Flash merah seluruh layar saat kena hit (berkedip)
            if(hitCooldown>0){
                float fl2=fmodf(hitCooldown*6,1)>.5f?.35f:0;
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.9f,0.1f,0.1f,fl2);
            }

            // ===================== PAUSE OVERLAY ===========================
            if(gameState==PAUSED){
                float ox=SCR_W/2.f, oy=SCR_H/2.f;
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.02f,0.06f,0.03f,0.72f); // layar gelap
                renderRect(ox-155,oy-95,310,190,0.04f,0.11f,0.06f,0.97f);           // panel
                renderRect(ox-155,oy-95,310,3,0.22f,0.72f,0.32f,0.85f);             // border atas
                renderRect(ox-155,oy+92,310,3,0.22f,0.72f,0.32f,0.85f);             // border bawah
                renderRect(ox-155,oy-95,3,195,0.22f,0.72f,0.32f,0.85f);             // border kiri
                renderRect(ox+152,oy-95,3,195,0.22f,0.72f,0.32f,0.85f);             // border kanan
                renderText("PAUSED",     ox-6*8*3.f/2, oy-78,3,  0.32f,0.98f,0.48f);
                renderText("ESC/P - RESUME", ox-14*8*1.5f/2,oy-12,1.5f,0.72f,0.92f,0.74f);
                renderText("R     - RESTART",ox-15*8*1.5f/2,oy+18,1.5f,0.72f,0.92f,0.74f);
                renderText("SCORE "+std::to_string(wallsPassed),ox-6*8*1.8f/2,oy+52,1.8f,0.5f,0.72f,0.52f);
                renderText("LIVES "+std::to_string(playerLives),ox-6*8*1.8f/2,oy+72,1.8f,0.5f,0.72f,0.52f);
            }

            // ==================== GAME OVER OVERLAY ========================
            if(gameState==GAME_OVER){
                float t  =std::min(overlayTimer*2.f,1.f);      // fade-in 0->1
                float pul=0.6f+0.4f*sinf(overlayTimer*4);      // border berkedip
                float ox=SCR_W/2.f, oy=SCR_H/2.f;
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.05f,0.01f,0.01f,0.75f*t); // dim
                float pw=360*t, ph=230*t;
                renderRect(ox-pw/2,oy-ph/2,pw,ph,0.1f,0.02f,0.02f,0.97f); // panel
                float bc=0.8f*pul; // warna border berkedip
                renderRect(ox-pw/2,oy-ph/2,pw,3,bc,0.08f,0.08f,0.9f);
                renderRect(ox-pw/2,oy+ph/2-3,pw,3,bc,0.08f,0.08f,0.9f);
                renderRect(ox-pw/2,oy-ph/2,3,ph,bc,0.08f,0.08f,0.9f);
                renderRect(ox+pw/2-3,oy-ph/2,3,ph,bc,0.08f,0.08f,0.9f);
                if(t>.5f){ // teks muncul setelah panel terbentuk
                    float ft=(t-.5f)*2;
                    renderText("GAME  OVER",ox-10*8*3.f/2,oy-90,3,0.98f,0.18f,0.18f,ft);
                    renderText("THE CANOPY CLAIMED YOU",ox-22*8*1.6f/2,oy-45,1.6f,0.75f,0.4f,0.4f,ft*.85f);
                    renderText("SCORE: "+std::to_string(wallsPassed),ox-7*8*1.8f/2,oy-10,1.8f,.88f,.62f,.62f,ft);
                    // Hiasan kotak kuning (jumlah = min(skor, 10))
                    int dc=std::min(wallsPassed,10);
                    float dx=ox-dc*6.f;
                    for(int s=0;s<dc;s++){
                        float p2=0.85f+0.15f*sinf(overlayTimer*4+s*.4f);
                        renderRect(dx+s*12,oy+15,9,9,0.98f*p2,0.82f*p2,0.1f,ft);
                    }
                    float pa=0.55f+0.45f*sinf(overlayTimer*3);
                    renderText("PRESS R TO RESTART",ox-18*8*1.5f/2,oy+44,1.5f,1,.92f,.92f,pa*ft);
                    renderText("PRESS M FOR MENU",  ox-16*8*1.5f/2,oy+64,1.5f,.8f,.72f,.72f,pa*ft);
                }
            }
            glEnable(GL_DEPTH_TEST); // aktifkan kembali depth test
        }

        glfwSwapBuffers(win); // tampilkan frame ke layar
        glfwPollEvents();     // proses event input
    }

    glDeleteVertexArrays(1,&VAO); glDeleteBuffers(1,&VBO);
    glfwTerminate();
    return g_backToMenu; // true = kembali ke menu
}

// ================================ WEBVIEW ====================================
static std::atomic<bool> g_startGame{false}, g_exit{false};

// Callback tombol menu HTML
void onMenuInvoke(struct webview* w,const char* arg){
    std::string cmd(arg);
    if(cmd=="startGame"){ g_startGame=true; webview_terminate(w); }
    else if(cmd=="options")
        webview_eval(w,"alert('HOW TO PLAY\\n\\nSPACE/W/UP = Jump\\nA/D = Move\\n\\nLompat ke platform bergerak untuk naik.\\nHindari platform yang menabrak dari samping!\\n\\nESC/P = Pause  |  R = Restart  |  M = Menu');");
    else if(cmd=="exit"){ g_exit=true; webview_terminate(w); }
}

// Encode HTML ke data URI (%XX) agar bisa dibuka oleh webview lama
static std::string makeDataUrl(const char* html){
    std::string out="data:text/html,";
    for(const char* p=html;*p;++p){
        unsigned char c=(unsigned char)*p;
        if(isalnum(c)||strchr("-_.!~*'()/:@,;?=&#+ ",c)) out+=(char)c;
        else{ char b[4]; snprintf(b,4,"%%%02X",c); out+=b; }
    }
    return out;
}

// Tengahkan window webview di layar (Windows only)
#if defined(_WIN32)||defined(_WIN64)
static void centerWebview(struct webview* wv){
    HWND h=(HWND)wv->priv.hwnd; if(!h) return;
    int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
    RECT r; GetWindowRect(h,&r);
    SetWindowPos(h,NULL,(sw-(r.right-r.left))/2,(sh-(r.bottom-r.top))/2,0,0,SWP_NOSIZE|SWP_NOZORDER);
}
#else
static void centerWebview(struct webview*){}
#endif

// ================================ MAIN =======================================
// Alur: tampilkan menu webview -> jalankan game -> kembali ke menu (atau exit)
int main(){
    std::string url=makeDataUrl(MENU_HTML); // konversi HTML ke data URI
    while(true){
        // Setup dan tampilkan menu webview
        g_startGame=false; g_exit=false;
        struct webview menu={};
        menu.title="Sylvan Odyssey"; menu.url=url.c_str();
        menu.width=SCR_W; menu.height=SCR_H;
        menu.resizable=0; menu.external_invoke_cb=onMenuInvoke;
        if(webview_init(&menu)!=0) return 1;
        centerWebview(&menu);
        while(webview_loop(&menu,1)==0){} // event loop menu
        webview_exit(&menu);

        if(g_exit) return 0;       // user klik Exit
        if(!g_startGame) break;    // window ditutup manual

        // Delay & sembunyikan webview sebelum window OpenGL muncul
#if defined(_WIN32)||defined(_WIN64)
        Sleep(120);
        { HWND h=FindWindowA(NULL,"Sylvan Odyssey"); if(h) ShowWindow(h,SW_HIDE); Sleep(150); }
#else
        usleep(270000);
#endif
        if(!runGame()) break; // jalankan game; false = exit (bukan kembali ke menu)
    }
    return 0;
}