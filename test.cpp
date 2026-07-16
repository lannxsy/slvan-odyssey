// =====================================================================================
// g++ main.cpp glad.c -o game -DWEBVIEW_WINAPI -Iinclude -Iinclude/glm -lglfw3 -lopengl32 -lgdi32 -lole32 -lcomctl32 -loleaut32 -luuid

//  SYLVAN ODYSSEY - PENJELASAN KODE (versi gampang dipahami buat UAS)
//  Game ini intinya: game platformer 3D sederhana pake OpenGL.
//  Pemain lompat-lompat naik ke atas lewat "wall" (platform) yang bergerak,
//  sambil hindari platform yang nabrak dari samping & serangan roket.
//  Menu utama dibikin pake HTML (webview), gamenya sendiri pake OpenGL (GLFW+GLAD).
// =====================================================================================

#define WEBVIEW_IMPLEMENTATION      // aktifin kode implementasi library webview (biar fungsi2 webview beneran ke-compile, bukan cuma deklarasi)
#include "webview.h"                 // library buat bikin jendela HTML (dipakai buat tampilan MENU)
#if defined(_WIN32)||defined(_WIN64) // kalau lagi di-compile di Windows...
  #define WIN32_LEAN_AND_MEAN        // biar header windows.h gak import semua fitur (biar ringan & gak bentrok)
  #include <windows.h>               // header khusus windows (dipakai buat Sleep, cari window, dll)
#else                                // kalau bukan windows (linux/mac)...
  #include <unistd.h>                // dipakai buat fungsi usleep (delay/jeda waktu)
#endif
#include <glad/glad.h>               // GLAD = loader fungsi-fungsi OpenGL (wajib sebelum pakai OpenGL)
#include <GLFW/glfw3.h>              // GLFW = library buat bikin window & handle input keyboard
#include <glm/glm.hpp>               // GLM = library matematika buat grafis 3D (vector, matrix)
#include <glm/gtc/matrix_transform.hpp> // fungsi transformasi matrix (translate, scale, rotate, dll)
#include <glm/gtc/type_ptr.hpp>      // buat convert glm::mat4/vec3 jadi pointer float (dibutuhin OpenGL)
#include <iostream>                  // buat print ke console (cout, cerr)
#include <vector>                    // buat pakai std::vector (array dinamis)
#include <cmath>                     // fungsi matematika (sinf, fabs, fmodf, dll)
#include <string>                    // buat pakai std::string
#include <algorithm>                 // fungsi std::min, std::max, std::remove_if
#include <atomic>                    // buat variabel std::atomic (aman dipakai lintas "thread")
#include <random>                    // buat bikin angka acak (posisi roket random)
              // library buat load file gambar (walau di kode ini kayaknya gak kepake langsung)

// =====================================================================================
// ///////////////////////////// TAMPILAN MENU (HTML) /////////////////////////////////
// Bagian ini isinya cuma teks HTML+CSS mentah yang nanti dirender sama webview
// jadi tampilan MENU AWAL (Start Journey, How To Play, Exit)
// =====================================================================================
static const char* MENU_HTML = R"HTML(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>Sylvan Odyssey</title><style>
*{margin:0;padding:0;box-sizing:border-box;} html,body{width:100%;height:100%;overflow:hidden;background:#0a1a0f;font-family:serif;}
#m{width:100%;height:100vh;position:relative;overflow:hidden;}
.sky{position:absolute;inset:0;background:linear-gradient(to bottom,#050d08,#0e2616 40%,#1a3d24 70%,#0d2010);}
.vgn{position:absolute;inset:0;background:radial-gradient(circle,transparent 30%,rgba(3,8,5,.8) 100%);pointer-events:none;}
.box{position:relative;z-index:10;width:100%;height:100%;display:flex;flex-direction:column;justify-content:center;align-items:center;}
h1{font-size:4rem;font-weight:900;color:#e4f5e8;text-transform:uppercase;letter-spacing:10px;text-shadow:0 0 20px rgba(93,214,104,.4);}
p{color:#86b390;letter-spacing:5px;text-transform:uppercase;margin-bottom:60px;} .wrap{display:flex;flex-direction:column;gap:18px;min-width:280px;}
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
// note: tombol2 di HTML di atas kalau diklik bakal manggil window.external.invoke('startGame'/'options'/'exit')
// nah, panggilan itu bakal ditangkap sama fungsi onMenuInvoke() di bawah nanti.


// =====================================================================================
// ///////////////////////// VARIABEL GLOBAL & KONSTANTA GAME /////////////////////////
// =====================================================================================
const unsigned int SCR_W=900, SCR_H=600;              // ukuran layar/jendela game (lebar x tinggi)
glm::vec3 camPos(0,2.5f,13), camFront(0,-0.15f,-1), camUp(0,1,0); // posisi kamera, arah hadap kamera, arah "atas" kamera
float camTargetY=2.5f;                                // target ketinggian kamera (biar kamera ngikutin pemain naik pelan2/smooth)
glm::vec3 lightPos(5,10,6), lightColor(1,0.95f,0.85f); // posisi & warna cahaya buat pencahayaan 3D
enum GameState{PLAYING,PAUSED,GAME_OVER};             // status game: lagi main / lagi pause / udah game over
GameState gameState=PLAYING;                          // status game sekarang, defaultnya lagi main
float dt=0, lastFrame=0, gameTime=0, hitCooldown=0;   // dt = delta time (jeda antar frame), lastFrame = waktu frame sebelumnya, gameTime = total waktu main, hitCooldown = jeda kebal setelah kena hit
int wallsPassed=0, playerLives=5;                     // wallsPassed = skor (jumlah platform yang berhasil dilewati), playerLives = nyawa pemain
const float GROUND_Y=0, GRAVITY=-28, JUMP_FORCE=10.5f; // GROUND_Y = ketinggian lantai, GRAVITY = gaya gravitasi (nariknya ke bawah), JUMP_FORCE = kekuatan lompatan
const float PHW=0.36f, PH=1.9f, PSPEED=6, PBOUND=6.8f; // PHW = setengah lebar badan pemain (buat deteksi tabrakan), PH = tinggi pemain, PSPEED = kecepatan gerak kiri/kanan, PBOUND = batas kiri-kanan area main

// ///////////////////////////////// WARNA-WARNA OBJEK ////////////////////////////////
// semua warna disimpen dalem bentuk RGB (0.0 - 1.0), dipake pas gambar objek 3D-nya
const glm::vec3 COL_SKY(0.55f,0.78f,0.98f);            // warna langit
const glm::vec3 COL_SUN(1.0f,0.95f,0.35f);             // warna matahari
const glm::vec3 COL_GROUND(0.32f,0.58f,0.24f);         // warna tanah/lantai
const glm::vec3 COL_GROUND_TOP(0.42f,0.72f,0.32f);     // warna rumput di atas tanah
const glm::vec3 COL_WALL(0.2f,0.75f,0.35f);            // warna platform/wall biasa
const glm::vec3 COL_WALL_EDGE(0.35f,0.95f,0.5f);       // warna pinggiran/tepi platform (biar keliatan garis atasnya)
const glm::vec3 COL_PLAYER_BODY(0.2f,0.55f,0.95f);     // warna badan pemain
const glm::vec3 COL_PLAYER_SKIN(0.96f,0.8f,0.62f);     // warna kulit (kepala) pemain
const glm::vec3 COL_PLAYER_LIMB(0.12f,0.22f,0.6f);     // warna tangan/kaki pemain
const glm::vec3 COL_EYE_WHITE(0.95f,0.95f,0.95f);      // warna putih mata
const glm::vec3 COL_EYE_BLACK(0.05f,0.05f,0.05f);      // warna hitam bola mata

// ///////////////////////////////////// ROKET /////////////////////////////////////
// warna-warna buat bagian-bagian roket yang jadi rintangan/serangan
const glm::vec3 COL_ROCKET_BODY(0.85f,0.85f,0.9f);     // warna badan roket
const glm::vec3 COL_ROCKET_NOSE(0.85f,0.1f,0.05f);     // warna ujung/hidung roket (runcing merah)
const glm::vec3 COL_ROCKET_FIN(0.5f,0.5f,0.6f);        // warna sirip roket
const glm::vec3 COL_FLAME(1.0f,0.5f,0.05f);            // warna api/semburan roket

// ///////////////////////////////// KARAKTER (PLAYER) /////////////////////////////////
struct Player{ glm::vec3 pos{0,0,0}; float velY=0, standingY=0; bool onGround=true; } player;
// pos = posisi pemain (x,y,z), velY = kecepatan vertikal (buat lompat/jatuh),
// standingY = ketinggian platform tempat pemain lagi berdiri, onGround = lagi nginjek tanah/platform atau nggak
// "player" di belakang struct itu langsung bikin 1 variabel global bernama player

// ///////////////////////////////////// WALL (PLATFORM) ///////////////////////////////
struct Wall{ float x,y,width,height,speedSign,delayTimer; int id; bool passed,alive,playerOn,isFrozen,waitingDelay; };
// x,y = posisi wall, width/height = ukuran wall, speedSign = arah gerak (+1 kanan / -1 kiri)
// delayTimer = hitungan mundur sebelum wall ini muncul/gerak, id = nomor urut wall
// passed = udah pernah diinjek/dilewatin pemain?, alive = wall ini masih aktif/belum dihapus?
// playerOn = lagi ada pemain di atasnya?, isFrozen = wall berhenti gerak (biasanya abis ketabrak/dipake buat pijakan)
// waitingDelay = wall ini lagi nunggu delay sebelum muncul di layar
std::vector<Wall> walls;                               // kumpulan semua wall yang lagi aktif di game
float wallSpeed=5.5f;                                  // kecepatan gerak wall (makin lama makin cepet, bikin susah)
bool spawnFromRight=false;                             // penanda wall berikutnya nongol dari kanan atau kiri (biar gantian)

// ///////////////////////////////////// ROKET (DATA) //////////////////////////////////
struct Rocket{ float x,y,velY,trail; bool alive; };
// x,y = posisi roket, velY = kecepatan vertikal roket (biasanya minus = ke bawah/nyamber ke pemain)
// trail = penghitung waktu buat animasi api di ekor roket, alive = roket ini masih aktif/belum dihapus
std::vector<Rocket> rockets;                           // kumpulan semua roket yang lagi meluncur
int lastMilestone=0;                                   // nyimpen milestone/skor kelipatan terakhir (buat munculin roket tiap kelipatan skor tertentu)

// =====================================================================================
// ///////////////////////////////// SHADER (KODE GPU) /////////////////////////////////
// Shader itu program kecil yang jalan di GPU buat ngegambar objek 3D + pencahayaannya
// =====================================================================================
const char* vertSrc=R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal; out vec3 FragPos, Normal; uniform mat4 model, view, projection;
void main(){ FragPos=vec3(model*vec4(aPos,1)); Normal=mat3(transpose(inverse(model)))*aNormal; gl_Position=projection*view*vec4(FragPos,1); })";
// ^ VERTEX SHADER: ngitung posisi tiap titik/vertex objek 3D di layar (dikonversi dari posisi lokal ke posisi layar)

const char* fragSrc=R"(#version 330 core
out vec4 FragColor; in vec3 FragPos, Normal; uniform vec3 lightPos, lightColor, objectColor; uniform float alpha;
void main(){ float diff=max(dot(normalize(Normal),normalize(lightPos-FragPos)),0.0); FragColor=vec4((0.38+diff*0.62)*lightColor*objectColor,alpha); })";
// ^ FRAGMENT SHADER: nentuin warna akhir tiap pixel objek, termasuk efek cahaya (makin ngadep ke cahaya, makin terang)

const char* shadowFragSrc=R"(#version 330 core
out vec4 FragColor; in vec3 FragPos; uniform float alpha; uniform vec3 shadowCenter; uniform vec2 shadowRadius;
void main(){
    float dx=(FragPos.x-shadowCenter.x)/shadowRadius.x, dz=(FragPos.z-shadowCenter.z)/shadowRadius.y, d=dx*dx+dz*dz;
    if(d>1.0) discard;
    FragColor=vec4(0,0,0,alpha*(1.0-d));
})";
// ^ shader khusus buat gambar BAYANGAN pemain di lantai/platform (bentuknya oval, makin ke pinggir makin transparan)

// =====================================================================================
// //////////////////////////////// FONT 8x8 (TEKS/HUD) ////////////////////////////////
// FONT8 ini tabel data bitmap huruf ukuran 8x8 pixel (dipakai buat nampilin teks di HUD/skor)
// tiap huruf disimpen sebagai 8 angka byte (tiap byte = 1 baris pixel, 8 bit = 8 pixel on/off)
// =====================================================================================
static const unsigned char FONT8[96][8]={
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},{0x6C,0x6C,0x28,0x00,0x00,0x00,0x00,0x00},{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},{0x18,0x7C,0xD0,0x7C,0x16,0xFC,0x18,0x00},{0xC2,0xC6,0x0C,0x18,0x30,0x66,0xC6,0x00},{0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},{0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
{0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},{0x7C,0xC6,0x06,0x1C,0x70,0xC6,0xFE,0x00},{0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},{0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00},{0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},{0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},{0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00},{0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},{0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},{0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},{0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},{0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
{0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00},{0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0x00},{0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},{0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},{0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},{0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},{0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00},{0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0x00},{0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},{0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00},{0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00},{0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},{0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},{0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00},
{0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00},{0x38,0x6C,0xC6,0xC6,0xCE,0x6C,0x3A,0x00},{0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00},{0x78,0xCC,0xC0,0x70,0x18,0xCC,0x78,0x00},{0xFC,0xB4,0x30,0x30,0x30,0x30,0x78,0x00},{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00},{0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},{0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00},{0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00},{0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},{0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},{0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00},
{0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00},{0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00},{0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00},{0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00},{0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00},{0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8},{0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00},{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},{0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C},{0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00},{0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x00,0x00,0xCC,0xFE,0xD6,0xC6,0xC6,0x00},{0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00},{0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00},
{0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E},{0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00},{0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00},{0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8},{0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00},{0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00},{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},{0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00},{0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00},
};
// (isi tabelnya angka2 hex doang, gak perlu dihafal — intinya cuma "gambar" tiap huruf dalam bentuk angka)

unsigned int fontTex,fontVAO,fontVBO2,fontProg,whiteTex; // ID2 buat resource OpenGL: texture font, VAO/VBO buat gambar teks, program shader font, texture polos putih (buat gambar kotak/rect)
unsigned int VAO,VBO,shaderProg,shadowProg;              // ID2 buat resource OpenGL objek 3D utama (kubus), shader utama, shader bayangan
const char* fontV=R"(#version 330 core
layout(location=0) in vec2 aPos; layout(location=1) in vec2 aUV; out vec2 vUV; uniform mat4 proj;
void main(){ vUV=aUV; gl_Position=proj*vec4(aPos,0,1); })";
// ^ vertex shader khusus buat gambar teks 2D di layar (HUD)

const char* fontF=R"(#version 330 core
in vec2 vUV; out vec4 C; uniform sampler2D tex; uniform vec4 color;
void main(){ C=vec4(color.rgb, color.a*texture(tex,vUV).r); })";
// ^ fragment shader teks: warna teks diambil dari uniform "color", transparansinya ikut bentuk huruf di texture font

// =====================================================================================
void initFont(){                                          // fungsi buat nyiapin texture font & buffer gambar teks (dipanggil sekali di awal)
    static unsigned char atlas[48][128]={};                // "kanvas" besar tempat nyusun semua huruf jadi satu gambar (atlas)
    for(int c=0;c<96;c++){                                 // loop semua karakter (96 karakter ASCII yang dipakai)
        int col=c%16, row=c/16;                            // hitung posisi huruf ini di dalam grid atlas (16 kolom)
        for(int y=0;y<8;y++){                              // loop tiap baris pixel huruf (8 baris)
            unsigned char b=FONT8[c][y];                   // ambil data 1 baris pixel huruf dari tabel FONT8
            for(int x=0;x<8;x++) if(b&(0x80>>x)) atlas[row*8+y][col*8+x]=255; // cek tiap bit, kalau nyala (1) taruh pixel putih (255) di atlas
        }
    }
    glGenTextures(1,&fontTex); glBindTexture(GL_TEXTURE_2D,fontTex); // bikin & aktifin texture buat font
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,128,48,0,GL_RED,GL_UNSIGNED_BYTE,atlas); // upload data atlas huruf ke GPU sebagai texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); // filter NEAREST biar teks tetap tajam/gak blur (pixel-art style)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); // biar texture gak "ngulang" di tepi
    unsigned char w=255;                                   // siapin 1 pixel putih polos
    glGenTextures(1,&whiteTex); glBindTexture(GL_TEXTURE_2D,whiteTex); // bikin texture putih 1x1 (dipakai buat gambar kotak/rectangle polos, misal HUD background)
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&w); // upload si 1 pixel putih tadi
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); // filter nearest juga
    glGenVertexArrays(1,&fontVAO); glGenBuffers(1,&fontVBO2); // bikin VAO+VBO khusus buat gambar quad (persegi) teks/rect
    glBindVertexArray(fontVAO); glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*24,NULL,GL_DYNAMIC_DRAW); // siapin buffer kosong (isinya bakal diupdate tiap gambar quad baru -> makanya DYNAMIC)
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0); glEnableVertexAttribArray(0); // atribut posisi (x,y)
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float))); glEnableVertexAttribArray(1); // atribut UV (koordinat texture)
    glBindVertexArray(0);                                  // lepas binding VAO biar gak ke-utak-atik gak sengaja
}

static void quadDraw(float x,float y,float w,float h,float u0,float v0,float u1,float v1){ // fungsi buat gambar 1 kotak (quad) di posisi x,y ukuran w,h, dengan koordinat texture u0v0-u1v1
    float v[6][4]={{x,y+h,u0,v1},{x,y,u0,v0},{x+w,y,u1,v0},           // 6 titik (2 segitiga) buat bentuk 1 persegi
                   {x,y+h,u0,v1},{x+w,y,u1,v0},{x+w,y+h,u1,v1}};
    glBindVertexArray(fontVAO); glBindBuffer(GL_ARRAY_BUFFER,fontVBO2); // pakai VAO/VBO font
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v); glDrawArrays(GL_TRIANGLES,0,6); // update data quad ke GPU terus gambar 2 segitiga (jadi 1 kotak)
}

void renderRect(float x,float y,float w,float h,float r,float g,float b,float a){ // gambar kotak polos warna solid (dipake buat background HUD, dsb)
    glUseProgram(fontProg); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,whiteTex); // pakai shader font + texture putih polos
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);          // set warna kotaknya
    quadDraw(x,y,w,h,0,0,1,1);                                            // gambar kotaknya
}

void renderText(const std::string& s,float x,float y,float sc,float r=1,float g=1,float b=1,float a=1){ // fungsi gambar teks di layar (HUD)
    glUseProgram(fontProg); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,fontTex); // pakai shader font + texture atlas huruf
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);          // warna teksnya
    for(char c:s){                                                       // loop tiap karakter di string
        if(c<32||c>127) c=32;                                            // kalau karakter aneh/di luar range, ganti jadi spasi
        int idx=c-32, col=idx%16, row=idx/16;                            // cari posisi huruf ini di dalam atlas (grid 16 kolom)
        quadDraw(x,y,8*sc,8*sc, col*8/128.f, row*8/48.f, (col+1)*8/128.f, (row+1)*8/48.f); // gambar 1 huruf sesuai posisi di atlas & skala ukurannya
        x+=8*sc;                                                         // geser posisi x buat huruf berikutnya
    }
}

void renderHeart(float x,float y,float s,bool filled){                  // gambar 1 icon hati (buat nunjukin nyawa/lives) pake kotak2 kecil
    float r=filled?.95f:.28f, g=filled?.15f:.22f, b=filled?.18f:.26f;    // kalau "filled"=true warnanya merah cerah (nyawa masih ada), kalau false warnanya gelap/pudar (nyawa hilang)
    renderRect(x+s,y,s,s,r,g,b,1); renderRect(x+4*s,y,s,s,r,g,b,1);      // gambar 2 "puncak" hati
    renderRect(x,y+s,6*s,s,r,g,b,1); renderRect(x,y+2*s,6*s,s,r,g,b,1);  // gambar badan tengah hati
    renderRect(x+s,y+3*s,4*s,s,r,g,b,1); renderRect(x+2*s,y+4*s,2*s,s,r,g,b,1); renderRect(x+3*s,y+5*s,s,s,r,g,b,1); // gambar bagian bawah hati yang meruncing
}

// =====================================================================================
// ///////////////////////////////// KUBUS (MODEL 3D DASAR) ////////////////////////////
// Semua objek di game ini (tanah, wall, pemain, roket) dibentuk dari kubus2 yang di-scale
// =====================================================================================
float cubeVerts[]={-1,-1,-1,0,0,-1, 1,-1,-1,0,0,-1, 1,1,-1,0,0,-1, 1,1,-1,0,0,-1,-1,1,-1,0,0,-1,-1,-1,-1,0,0,-1, -1,-1,1,0,0,1,   1,-1,1,0,0,1,   1,1,1,0,0,1,   1,1,1,0,0,1,  -1,1,1,0,0,1,  -1,-1,1,0,0,1, -1,1,1,-1,0,0, -1,1,-1,-1,0,0, -1,-1,-1,-1,0,0,-1,-1,-1,-1,0,0,-1,-1,1,-1,0,0,-1,1,1,-1,0,0, 1,1,1,1,0,0,  1,1,-1,1,0,0,   1,-1,-1,1,0,0, 1,-1,-1,1,0,0, 1,-1,1,1,0,0,   1,1,1,1,0,0, -1,-1,-1,0,-1,0, 1,-1,-1,0,-1,0, 1,-1,1,0,-1,0, 1,-1,1,0,-1,0, -1,-1,1,0,-1,0, -1,-1,-1,0,-1,0, -1,1,-1,0,1,0,  1,1,-1,0,1,0,   1,1,1,0,1,0,   1,1,1,0,1,0, -1,1,1,0,1,0,  -1,1,-1,0,1,0,};
// ^ data mentah 36 titik (6 sisi x 2 segitiga x 3 titik) buat bentuk 1 kubus, tiap titik = posisi (x,y,z) + arah normal (buat pencahayaan)

unsigned int makeShader(const char* vs,const char* fs){                  // fungsi buat compile & link shader jadi 1 "program" yang siap dipakai
    auto compile=[](unsigned int t,const char* src){                     // fungsi kecil di dalam buat compile 1 shader (vertex/fragment)
        unsigned int s=glCreateShader(t);                                // bikin objek shader kosong
        glShaderSource(s,1,&src,NULL); glCompileShader(s);               // masukin source code-nya terus di-compile
        int ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);                  // cek apakah compile-nya sukses
        if(!ok){char log[512];glGetShaderInfoLog(s,512,NULL,log);std::cerr<<log;} // kalau gagal, print error-nya ke console
        return s;
    };
    unsigned int v=compile(GL_VERTEX_SHADER,vs), f=compile(GL_FRAGMENT_SHADER,fs), p=glCreateProgram(); // compile vertex & fragment shader, bikin program kosong
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);          // gabungin dua shader itu jadi 1 program & link
    glDeleteShader(v); glDeleteShader(f);                                // hapus shader individual (udah gak kepake abis di-link)
    return p;                                                           // balikin ID program shader yang siap dipakai
}

void setUniforms(unsigned int prog,glm::mat4& proj,glm::mat4& view){    // fungsi buat kirim matrix proyeksi & kamera ke shader
    glUseProgram(prog); glBindVertexArray(VAO);                          // aktifin shader program & VAO kubus
    glUniformMatrix4fv(glGetUniformLocation(prog,"projection"),1,GL_FALSE,glm::value_ptr(proj)); // kirim matrix projection (buat efek perspektif 3D)
    glUniformMatrix4fv(glGetUniformLocation(prog,"view"),1,GL_FALSE,glm::value_ptr(view));       // kirim matrix view (posisi & arah kamera)
}

void drawCube(unsigned int prog,glm::vec3 pos,glm::vec3 sc,glm::vec3 col,float a=1){ // fungsi utama buat gambar 1 kubus di posisi "pos", ukuran "sc", warna "col", transparansi "a"
    glm::mat4 m=glm::scale(glm::translate(glm::mat4(1),pos),sc);          // bikin matrix model: pindahin (translate) ke posisi lalu di-scale sesuai ukuran
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m)); // kirim matrix model ke shader
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(col));        // kirim warna objek ke shader
    glUniform1f(glGetUniformLocation(prog,"alpha"),a);                    // kirim nilai transparansi
    glDrawArrays(GL_TRIANGLES,0,36);                                      // gambar kubusnya (36 titik = 12 segitiga = 6 sisi kubus)
}

// ///////////////////////////////// KARAKTER (PLAYER) - GAMBAR //////////////////////////
void drawPlayer(unsigned int prog,glm::vec3 base,float s){                // fungsi buat gambar karakter pemain, disusun dari beberapa kubus kecil (kayak lego)
    drawCube(prog,base+glm::vec3(-s*.2f,s*.15f,0),{s*.16f,s*.3f,s*.16f},COL_PLAYER_LIMB);   // kaki kiri
    drawCube(prog,base+glm::vec3( s*.2f,s*.15f,0),{s*.16f,s*.3f,s*.16f},COL_PLAYER_LIMB);   // kaki kanan
    drawCube(prog,base+glm::vec3(0,s*.85f,0),{s*.36f,s*.38f,s*.2f},COL_PLAYER_BODY);        // badan
    drawCube(prog,base+glm::vec3(-s*.52f,s*.71f,0),{s*.14f,s*.26f,s*.14f},COL_PLAYER_LIMB); // tangan kiri
    drawCube(prog,base+glm::vec3( s*.52f,s*.71f,0),{s*.14f,s*.26f,s*.14f},COL_PLAYER_LIMB); // tangan kanan
    drawCube(prog,base+glm::vec3(0,s*1.52f,0),{s*.33f,s*.3f,s*.26f},COL_PLAYER_SKIN);       // kepala
    drawCube(prog,base+glm::vec3(-s*.13f,s*1.59f,s*.30f),{s*.07f,s*.07f,s*.04f},COL_EYE_WHITE); // putih mata kiri
    drawCube(prog,base+glm::vec3(-s*.13f,s*1.59f,s*.35f),{s*.04f,s*.04f,s*.03f},COL_EYE_BLACK); // bola mata kiri
    drawCube(prog,base+glm::vec3( s*.13f,s*1.59f,s*.30f),{s*.07f,s*.07f,s*.04f},COL_EYE_WHITE); // putih mata kanan
    drawCube(prog,base+glm::vec3( s*.13f,s*1.59f,s*.35f),{s*.04f,s*.04f,s*.03f},COL_EYE_BLACK); // bola mata kanan
}
// intinya: karakter cuma tumpukan kubus kecil (kayak Minecraft mini), posisinya relatif dari "base" (posisi kaki/pijakan)

// ///////////////////////////////// WALL (PLATFORM) - LOGIKA ///////////////////////////
void spawnWall(bool forceSame=false,float delay=0){                       // fungsi buat munculin 1 wall/platform baru
    Wall w;                                                               // bikin data wall baru
    w.id=wallsPassed+1; w.height=0.16f;                                   // id-nya berdasarkan skor sekarang, tinggi wall tetap
    w.passed=w.playerOn=w.isFrozen=false; w.alive=true;                   // status awal: belum dilewatin, belum ada yang injek, belum beku, dan aktif
    w.waitingDelay=(delay>0); w.delayTimer=delay;                         // kalau ada delay, wall nunggu dulu sebelum muncul beneran
    w.width=std::max(6.f-(wallsPassed/5)*0.55f,1.5f);                     // lebar wall makin lama makin kecil (makin susah) tapi ada batas minimal
    bool right=forceSame?!spawnFromRight:spawnFromRight;                  // tentuin wall muncul dari kanan atau kiri
    w.speedSign=right?-1.f:1.f;                                           // arah gerak: kalau dari kanan berarti geraknya ke kiri (negatif), dst
    w.x=w.waitingDelay?999.f:(right?16.f:-16.f);                          // posisi awal x: kalau masih nunggu delay taruh jauh (999=gak keliatan), kalau enggak taruh di tepi layar
    if(!forceSame) spawnFromRight=!spawnFromRight;                        // gantian arah munculnya wall berikutnya (biar variatif kiri-kanan)
    w.y=player.standingY+0.75f;                                          // ketinggian wall = posisi pijakan pemain sekarang + jarak lompat standar
    walls.push_back(w);                                                  // masukin wall baru ini ke daftar walls
}

void resetGame(){                                                        // fungsi buat reset semua data game ke kondisi awal (dipanggil pas mulai/restart)
    player={}; walls.clear(); rockets.clear();                           // reset posisi pemain, kosongin semua wall & roket
    wallsPassed=0; wallSpeed=5.5f; gameTime=0;                           // reset skor, kecepatan wall, & waktu main
    gameState=PLAYING; playerLives=5;                                    // set status main lagi & nyawa full (5)
    hitCooldown=0; spawnFromRight=false; lastMilestone=0;                // reset cooldown kena hit, arah spawn, & milestone skor
    camPos.y=2.5f; camTargetY=2.5f;                                      // reset posisi kamera ke awal
    spawnWall(false,1.2f);                                               // munculin wall pertama (dengan delay dikit biar gak instan)
}

// =====================================================================================
// //////////////////////////////////// INPUT & KONTROL /////////////////////////////////
// =====================================================================================
static bool g_backToMenu=false;                                          // penanda global: pemain minta balik ke menu (tekan M)
static bool keys[512]={};                                                // array status semua tombol keyboard (true=lagi ditekan)
void key_callback(GLFWwindow* win,int key,int,int action,int){          // fungsi ini dipanggil otomatis tiap ada event keyboard
    if(key>=0&&key<512){                                                 // pastiin kode key-nya valid
        if(action==GLFW_PRESS)   keys[key]=true;                         // tombol ditekan -> set true
        if(action==GLFW_RELEASE) keys[key]=false;                        // tombol dilepas -> set false
    }
    if(action!=GLFW_PRESS) return;                                       // sisa kode di bawah cuma jalan pas tombol BARU ditekan (bukan ditahan terus atau dilepas)
    if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W)&&player.onGround&&gameState==PLAYING){ // kalau tekan SPACE/UP/W, lagi berdiri di tanah, & lagi main
        player.velY=JUMP_FORCE; player.onGround=false;                   // kasih kecepatan lompat ke atas & tandain lagi di udara
    }
    if(key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P) gameState=gameState==PLAYING?PAUSED:(gameState==PAUSED?PLAYING:gameState); // ESC/P buat toggle pause <-> main (kalau lagi game over, gak ngaruh)
    if(key==GLFW_KEY_R) resetGame();                                     // tombol R buat restart game dari awal
    if(key==GLFW_KEY_M){ g_backToMenu=true; glfwSetWindowShouldClose(win,GLFW_TRUE); } // tombol M buat balik ke menu (nutup window game)
}
void framebuffer_size_callback(GLFWwindow*,int w,int h){ glViewport(0,0,w,h); } // dipanggil kalau ukuran window berubah, biar area gambar OpenGL ikut nyesuain

// ///////////////////////////////// SISTEM NYAWA (DAMAGE) //////////////////////////////
void takeDamage(){                                                       // fungsi dipanggil pas pemain kena serangan (ketabrak wall/roket)
    playerLives--; hitCooldown=1.8f;                                     // kurangin 1 nyawa, kasih jeda kebal 1.8 detik biar gak kena damage beruntun
    if(playerLives<=0){ gameState=GAME_OVER; return; }                   // kalau nyawa habis, langsung ganti status jadi GAME_OVER
    player.pos={0,player.standingY,0};                                   // kalau masih ada nyawa, taruh ulang pemain di tengah pada ketinggian terakhir dia berdiri
    player.velY=0; player.onGround=true;                                 // reset kecepatan jatuh & anggap lagi berdiri
}

// =====================================================================================
// //////////////////////////////////// FUNGSI UTAMA GAME ///////////////////////////////
// runGame() ini isinya: setup window OpenGL, terus GAME LOOP (jalan terus tiap frame)
// sampai window ditutup atau pemain balik ke menu
// =====================================================================================
bool runGame(){
    g_backToMenu=false; lastFrame=0;                                     // reset penanda balik menu & waktu frame terakhir
    glfwInit();                                                          // inisialisasi library GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3); // minta versi OpenGL 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);        // pakai profile "core" (modern OpenGL, gak pake fungsi lama/deprecated)
    GLFWwindow* win=glfwCreateWindow(SCR_W,SCR_H,"Sylvan Odyssey",NULL,NULL); // bikin window game-nya
#if defined(_WIN32)||defined(_WIN64)
    { int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN); // (khusus windows) ambil ukuran layar
      glfwSetWindowPos(win,(sw-SCR_W)/2,(sh-SCR_H)/2); }                 // posisiin window di tengah layar
#endif
    glfwMakeContextCurrent(win);                                        // aktifin context OpenGL buat window ini
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback);      // daftarin fungsi callback kalau window di-resize
    glfwSetKeyCallback(win,key_callback);                               // daftarin fungsi callback buat input keyboard
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);                 // load semua fungsi OpenGL lewat GLAD
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND);                        // aktifin depth test (objek dekat nutupin objek jauh) & blending (buat transparansi)
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);                   // atur cara blending transparansi standar

    // ------------------- SETUP SHADER & BUFFER (sekali di awal) -------------------
    shaderProg=makeShader(vertSrc,fragSrc);                              // compile shader utama (buat objek 3D biasa)
    shadowProg=makeShader(vertSrc,shadowFragSrc);                        // compile shader bayangan
    fontProg=makeShader(fontV,fontF);                                    // compile shader font/teks
    initFont();                                                         // siapin texture font & buffer teks
    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO);                    // bikin VAO+VBO buat data kubus
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW); // upload data kubus ke GPU (STATIC karena datanya gak berubah2)
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0); // atribut posisi vertex (x,y,z)
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1); // atribut normal (buat pencahayaan)
    resetGame();                                                        // reset/siapin kondisi awal game (posisi pemain, wall pertama, dll)

    // ============================= GAME LOOP (INTI GAME) =============================
    while(!glfwWindowShouldClose(win)){                                  // ulang terus selama window belum ditutup
        float now=(float)glfwGetTime();                                  // ambil waktu sekarang
        dt=std::min(now-lastFrame,0.05f); lastFrame=now;                 // hitung delta time (jeda antar frame), dibatasin max 0.05 detik biar gak lompat jauh kalau lag
        if(hitCooldown>0) hitCooldown-=dt;                               // kurangin cooldown kebal tiap frame

        // ------------------------- LOGIKA GAME (cuma jalan pas PLAYING) -------------------------
        if(gameState==PLAYING){
            gameTime+=dt;                                                // tambahin total waktu main
            player.velY+=GRAVITY*dt; player.pos.y+=player.velY*dt;       // terapin gravitasi ke kecepatan vertikal, terus update posisi y pemain (ini rumus fisika dasar gerak jatuh)

            // ///////////////////////// GERAK KIRI/KANAN PEMAIN /////////////////////////
            if(keys[GLFW_KEY_A]||keys[GLFW_KEY_LEFT])  player.pos.x-=PSPEED*dt; // tombol A/kiri -> gerak ke kiri
            if(keys[GLFW_KEY_D]||keys[GLFW_KEY_RIGHT]) player.pos.x+=PSPEED*dt; // tombol D/kanan -> gerak ke kanan
            player.pos.x=std::max(-PBOUND,std::min(PBOUND,player.pos.x));       // batesin posisi x biar pemain gak keluar area main

            bool onGnd=false; float floor=GROUND_Y;                     // penanda sementara: apakah pemain lagi di atas sesuatu, dan ketinggian pijakannya
            if(player.pos.y<=GROUND_Y){ player.pos.y=GROUND_Y; player.velY=0; onGnd=true; } // kalau pemain nyampe lantai dasar, berhentiin jatuhnya di situ

            // ///////////////////////// CEK TABRAKAN DENGAN WALL /////////////////////////
            bool nextSpawn=false;                                        // penanda: perlu munculin wall baru gak abis ini?
            std::vector<Wall> pending;                                   // wall2 baru yang bakal ditambahin setelah loop (gak boleh nambah pas lagi looping walls)
            for(auto& w:walls){                                          // cek satu-satu wall yang ada
                if(!w.alive||w.waitingDelay) continue;                   // skip kalau wall udah gak aktif atau masih nunggu delay
                if(fabs(player.pos.x-w.x)>=PHW+w.width) continue;        // skip kalau posisi x pemain masih jauh dari wall (belum mungkin nabrak)
                float foot=player.pos.y, head=player.pos.y+PH;           // hitung posisi kaki & kepala pemain
                float top=w.y+w.height, bot=w.y-w.height;                // hitung sisi atas & bawah wall
                if(player.velY<=0&&foot>=top-0.35f&&foot<=top+0.15f){    // kalau pemain lagi turun/jatuh & kakinya pas ketemu sisi atas wall (mendarat)
                    if(!w.isFrozen&&!w.passed){                          // kalau wall ini belum pernah diinjek sebelumnya -> INI PENDARATAN PERTAMA (dapat skor!)
                        w.isFrozen=w.passed=true; wallsPassed++; nextSpawn=true; // tandain wall beku & udah dilewatin, skor nambah 1, tandain perlu spawn wall baru
                        player.pos.y=top; player.velY=0; onGnd=true; floor=top; w.playerOn=true; // taruh pemain berdiri pas di atas wall
                    }
                    else if(w.passed&&top<player.standingY-0.1f&&hitCooldown<=0) takeDamage(); // kalau wall ini di BAWAH posisi berdiri terakhir pemain (berarti ketabrak dari atas turun ke wall lama) -> kena damage
                    else{ player.pos.y=top; player.velY=0; onGnd=true; floor=top; w.playerOn=true; } // selain itu ya pemain berdiri normal di atas wall
                }
                else if(!w.isFrozen&&!w.passed&&hitCooldown<=0&&foot<top-0.15f&&head>bot+0.15f){ // kalau pemain nabrak wall dari SAMPING (badan pemain nyenggol sisi wall, bukan mendarat di atasnya)
                    w.isFrozen=true; w.alive=false;                       // wall ini dianggap "meledak"/berhenti & dihapus
                    Wall rep=w;                                          // bikin wall pengganti (respawn) berdasarkan data wall yang tadi
                    rep.passed=rep.playerOn=rep.isFrozen=false;
                    rep.alive=true; rep.waitingDelay=true; rep.delayTimer=1.f; rep.x=999.f; // wall penggantinya nunggu delay 1 detik dulu sebelum muncul lagi
                    pending.push_back(rep);                              // simpen dulu di "pending" (baru ditambah ke walls abis loop ini selesai)
                    for(auto& fw:walls) if(fw.alive&&!fw.isFrozen&&!fw.waitingDelay) fw.isFrozen=true; // bekuin semua wall lain yang masih gerak (biar gak tambah runyam pas kena damage)
                    takeDamage();                                        // pemain kena damage karena ketabrak dari samping
                }
            }
            for(auto& r:pending) walls.push_back(r);                     // baru sekarang masukin wall pengganti yang tadi ke daftar walls
            player.onGround=onGnd;                                      // update status "lagi di tanah/platform" pemain
            if(onGnd){ player.standingY=floor; player.pos.y=floor; player.velY=0; } // kalau lagi berpijak, update ketinggian pijakan terakhir
            if(nextSpawn&&gameState==PLAYING) spawnWall(false,0.15f);    // kalau abis dapet skor, munculin wall baru berikutnya (dengan delay dikit)

            // ///////////////////////// TAMBAH KESULITAN SEIRING SKOR /////////////////////////
            wallSpeed=std::min(5.5f+(float)wallsPassed*0.45f,22.f);      // makin banyak skor, wall makin cepet gerak (tapi dibatesin max 22)

            // ///////////////////// ROKET (MUNCUL TIAP KELIPATAN SKOR TERTENTU) ////////////////
            int ms=wallsPassed/5;                                      // hitung "level milestone" (tiap 10 skor naik 1 level)
            if(ms>0&&ms>lastMilestone){                                  // kalau level milestone-nya baru (belum pernah dicapai sebelumnya)
                lastMilestone=ms;                                        // update milestone terakhir
                std::mt19937 rng(std::random_device{}());               // siapin generator angka acak
                std::uniform_real_distribution<float> xd(-5.5f,5.5f);    // rentang posisi x acak buat roket
                for(int k=0;k<2;k++) rockets.push_back({xd(rng), player.pos.y+8.f, -(7.f+ms*.3f), 0, true}); // munculin 2 roket di atas pemain, meluncur turun makin cepet seiring level makin tinggi
            }
            for(auto& r:rockets){                                       // update posisi & cek tabrakan tiap roket
                if(!r.alive) continue;                                  // skip roket yang udah gak aktif
                r.y+=r.velY*dt; r.trail+=dt;                             // gerakin roket turun, & update timer buat animasi api
                if(r.y<player.standingY-2){ r.alive=false; continue; }   // kalau roket udah kelewat jauh ke bawah, matiin (gak kepake lagi)
                if(hitCooldown<=0&&fabs(r.x-player.pos.x)<PHW+0.25f&&r.y>=player.pos.y-.1f&&r.y<=player.pos.y+PH+.2f){ // cek apakah roket nabrak badan pemain
                    r.alive=false; takeDamage();                         // kalau kena, roket ilang & pemain kena damage
                }
            }
            rockets.erase(std::remove_if(rockets.begin(),rockets.end(),[](const Rocket& r){return !r.alive;}),rockets.end()); // beneran hapus semua roket yang udah gak aktif dari list (biar list-nya gak numpuk)

            // ///////////////////////// UPDATE GERAK SEMUA WALL /////////////////////////
            for(size_t i=0;i<walls.size();i++){
                if(!walls[i].alive) continue;                            // skip wall yang udah gak aktif
                if(walls[i].waitingDelay){                               // kalau wall ini masih dalam masa nunggu delay
                    walls[i].delayTimer-=dt;                             // kurangin timer delay-nya
                    if(walls[i].delayTimer<=0){ walls[i].waitingDelay=false; walls[i].x=walls[i].speedSign<0?16.f:-16.f; } // kalau delay abis, munculin wall-nya di tepi layar sesuai arah geraknya
                    continue;
                }
                if(!walls[i].isFrozen) walls[i].x+=walls[i].speedSign*wallSpeed*dt; // kalau wall belum beku, gerakin sesuai arah & kecepatannya
                bool out=(walls[i].speedSign<0&&walls[i].x<-16)||(walls[i].speedSign>0&&walls[i].x>16); // cek apakah wall udah keluar layar
                if(out&&!walls[i].isFrozen){ walls[i].alive=false; spawnWall(true,0.5f); } // kalau keluar layar & belum beku, matiin wall ini & munculin wall baru pengganti (arah sama)
            }

            // ///////////////////////// KAMERA NGIKUTIN PEMAIN /////////////////////////
            camTargetY=player.pos.y+2.5f;                                // target ketinggian kamera = posisi pemain + offset
            camPos.y+=(camTargetY-camPos.y)*8.f*dt;                      // gerakin kamera pelan2 (smooth/lerp) menuju target, biar gerakannya halus gak kaku
        }

        // ------------------------- UPDATE JUDUL WINDOW (info skor/status) -------------------------
        std::string title;
        if(gameState==PLAYING) title="Sylvan Odyssey | Score:"+std::to_string(wallsPassed)+" | Lives:"+std::to_string(playerLives)+" | ESC=Pause";
        else if(gameState==PAUSED) title="PAUSED | ESC=Resume | R=Restart";
        else title="GAME OVER | Score:"+std::to_string(wallsPassed)+" | R=Restart";
        glfwSetWindowTitle(win,title.c_str());                          // judul window diupdate sesuai status game sekarang

        // =============================== BAGIAN GAMBAR (RENDER) ===============================
        float camOff=camPos.y-2.5f;                                     // hitung offset kamera dari posisi awal (buat geser objek statis biar ikut "naik" seolah kamera diem)
        glClearColor(0.38f,0.62f,0.88f,1);                              // warna background dibersihin ke warna biru langit
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);               // bersihin layar & depth buffer sebelum gambar frame baru
        glm::mat4 proj=glm::perspective(glm::radians(62.f),(float)SCR_W/SCR_H,0.1f,100.f); // bikin matrix proyeksi perspektif (biar keliatan 3D, objek jauh keliatan kecil)
        glm::mat4 view=glm::lookAt(camPos,camPos+camFront,camUp);       // bikin matrix kamera (posisi kamera, arah liat, arah atas)
        setUniforms(shaderProg,proj,view);                              // kirim kedua matrix itu ke shader utama
        glUniform3fv(glGetUniformLocation(shaderProg,"lightPos"),1,glm::value_ptr(lightPos));     // kirim posisi cahaya
        glUniform3fv(glGetUniformLocation(shaderProg,"lightColor"),1,glm::value_ptr(lightColor)); // kirim warna cahaya

        // //////////////////////// GAMBAR LATAR (LANGIT, MATAHARI, TANAH) ////////////////////////
        drawCube(shaderProg,{0,3.5f+camOff,-9},{22,20,0.3f},COL_SKY);        // panel langit besar di belakang
        drawCube(shaderProg,{0,6.5f+camOff,-7},{0.6f,0.6f,0.2f},COL_SUN);    // matahari
        drawCube(shaderProg,{0,-0.3f,0},{15,0.3f,2.5f},COL_GROUND);         // lantai/tanah
        drawCube(shaderProg,{0,0.02f,0},{15,0.05f,2.5f},COL_GROUND_TOP);    // lapisan rumput tipis di atas tanah
        setUniforms(shaderProg,proj,view);                                  // set ulang uniform (karena abis drawCube uniform "model" dll udah keganti)

        // //////////////////////////////// GAMBAR SEMUA WALL ////////////////////////////////
        for(auto& w:walls){
            if(!w.alive||w.waitingDelay) continue;                        // skip wall yang gak aktif / masih nunggu delay
            glm::vec3 wc=w.isFrozen?COL_WALL*0.82f:COL_WALL;                // kalau wall lagi beku, warnanya sedikit lebih gelap (biar keliatan beda)
            glm::vec3 edge=w.isFrozen?COL_WALL_EDGE*0.82f:COL_WALL_EDGE;
            drawCube(shaderProg,{w.x,w.y,0},{w.width,w.height,0.55f},wc);              // badan wall
            drawCube(shaderProg,{w.x,w.y+w.height,0},{w.width+0.02f,0.02f,0.58f},edge); // garis tepi/edge di atas wall
        }

        // ///////////////////////////////// BAYANGAN PEMAIN /////////////////////////////////
        {
            setUniforms(shadowProg,proj,view);                              // pindah pakai shader khusus bayangan
            float sf=GROUND_Y;                                              // sf = ketinggian permukaan tempat bayangan bakal digambar (default lantai)
            for(auto& w:walls)                                              // cari wall tertinggi yang ada tepat di bawah pemain (buat bayangan nempel di situ, bukan tembus wall)
                if(w.alive&&!w.waitingDelay&&w.y+w.height<=player.pos.y+0.02f&&fabs(player.pos.x-w.x)<PHW+w.width) sf=std::max(sf,w.y+w.height);
            float h=std::max(player.pos.y-sf,0.f);                          // hitung seberapa tinggi pemain dari permukaan bayangan
            float rX=std::max(0.55f-h*.045f,.18f), rZ=std::max(0.32f-h*.025f,.10f), al=std::max(0.85f-h*.07f,.15f), sy=sf+0.08f; // makin tinggi pemain, bayangan makin kecil & makin transparan (efek realistis)
            glUniform3f(glGetUniformLocation(shadowProg,"shadowCenter"),player.pos.x,sy,player.pos.z); // kirim posisi tengah bayangan
            glUniform2f(glGetUniformLocation(shadowProg,"shadowRadius"),rX,rZ);   // kirim ukuran radius bayangan
            glUniform1f(glGetUniformLocation(shadowProg,"alpha"),al);            // kirim transparansi bayangan
            glDepthMask(GL_FALSE);                                               // matiin depth write sementara biar bayangan nempel rata di permukaan (gak "ketiban" masalah z-fighting)
            glm::mat4 sm=glm::scale(glm::translate(glm::mat4(1),{player.pos.x,sy,player.pos.z}),{rX*2.2f,0.005f,rZ*2.2f}); // bikin matrix bayangan (pipih banget di sumbu y)
            glUniformMatrix4fv(glGetUniformLocation(shadowProg,"model"),1,GL_FALSE,glm::value_ptr(sm));
            glDrawArrays(GL_TRIANGLES,0,36);                                     // gambar bayangannya
            glDepthMask(GL_TRUE);                                               // nyalain lagi depth write buat gambar objek selanjutnya
            setUniforms(shaderProg,proj,view);                                  // balik lagi pakai shader utama
        }

        // //////////////////////////////////// GAMBAR ROKET ////////////////////////////////////
        for(auto& r:rockets){
            if(!r.alive) continue;                                        // skip roket yang gak aktif
            float t=(float)glfwGetTime();                                 // ambil waktu sekarang buat animasi
            float fl=0.15f+0.1f*sinf(t*20+r.x*3);                        // hitung ukuran api yang "berkedip"/bergetar pakai fungsi sin (biar keliatan hidup nyalanya)
            drawCube(shaderProg,{r.x,r.y,0},      {0.18f,0.75f,0.18f},COL_ROCKET_BODY); // badan roket
            drawCube(shaderProg,{r.x,r.y-0.6f,0}, {0.12f,0.25f,0.12f},COL_ROCKET_NOSE); // hidung/ujung roket
            drawCube(shaderProg,{r.x,r.y+0.4f,0}, {0.34f,0.14f,0.14f},COL_ROCKET_FIN);  // sirip roket
            drawCube(shaderProg,{r.x,r.y+0.85f,0},{fl,0.3f,fl},COL_FLAME,0.85f);        // api ekor roket (ukurannya berubah2/animasi)
        }
        drawPlayer(shaderProg,player.pos,0.58f);                         // gambar karakter pemain di posisinya sekarang

        // =============================== HUD (TEKS & INFO DI LAYAR) ===============================
        {
            glDisable(GL_DEPTH_TEST);                                    // matiin depth test biar HUD selalu di depan (gak ketiban objek 3D)
            glm::mat4 hudP=glm::ortho(0.f,(float)SCR_W,(float)SCR_H,0.f,-1.f,1.f); // proyeksi 2D biasa (bukan perspektif) khusus buat HUD
            glUseProgram(fontProg);
            glUniformMatrix4fv(glGetUniformLocation(fontProg,"proj"),1,GL_FALSE,glm::value_ptr(hudP)); // kirim proyeksi 2D ke shader font
            float sc=2, cw=16, ch=16;                                    // sc = skala ukuran teks, cw/ch = lebar/tinggi per karakter di layar
            renderRect(0,0,(float)SCR_W,ch+14,0.03f,0.08f,0.04f,0.88f);  // gambar background bar atas (buat taruh skor & info)
            renderRect(0,ch+12,(float)SCR_W,2,0.22f,0.68f,0.3f,0.6f);    // garis tipis pembatas bar atas
            for(int i=0;i<5;i++) renderHeart(10+i*32,5,sc*.9f,i<playerLives); // gambar 5 icon hati, yang aktif sebanyak sisa nyawa pemain
            std::string sc_str="SCORE: "+std::to_string(wallsPassed);
            renderText(sc_str,SCR_W/2.f-sc_str.size()*cw/2,5,sc,0.7f,0.98f,0.72f); // tampilin skor di tengah atas
            renderText("A/D=MOVE  ESC=PAUSE",SCR_W-10-20*cw,5,sc,0.4f,0.6f,0.42f,0.8f); // tampilin petunjuk kontrol di pojok kanan atas
            if(!rockets.empty()){                                       // kalau lagi ada roket yang mengincar
                std::string w2="!! ROCKET INCOMING !!";
                renderText(w2,SCR_W/2.f-w2.size()*cw*.8f/2,ch+20,.8f*sc,1,.3f,.05f); // munculin peringatan roket di layar
            }
            if(hitCooldown>0){                                           // kalau lagi masa kebal abis kena hit
                float fl2=fmodf(hitCooldown*6,1)>.5f?.35f:0;             // bikin layar berkedip merah (efek "kena damage") pake modulo waktu
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.9f,0.1f,0.1f,fl2);
            }
            // ///////////////////////////// TAMPILAN PAUSE /////////////////////////////
            if(gameState==PAUSED){
                float ox=SCR_W/2.f, oy=SCR_H/2.f;                        // titik tengah layar
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.02f,0.06f,0.03f,0.72f); // overlay gelap penuh layar
                renderRect(ox-155,oy-95,310,190,0.04f,0.11f,0.06f,0.97f);          // kotak panel pause di tengah
                renderText("PAUSED",ox-6*8*3.f/2,oy-60,3,0.32f,0.98f,0.48f);       // judul "PAUSED"
                renderText("ESC/P - RESUME",ox-14*8*1.5f/2,oy,1.5f,0.72f,0.92f,0.74f); // petunjuk resume
                renderText("R - RESTART",ox-11*8*1.5f/2,oy+30,1.5f,0.72f,0.92f,0.74f); // petunjuk restart
                renderText("SCORE "+std::to_string(wallsPassed),ox-8*8*1.5f/2,oy+60,1.5f,0.5f,0.72f,0.52f); // skor saat pause
            }
            // ///////////////////////////// TAMPILAN GAME OVER /////////////////////////////
            if(gameState==GAME_OVER){
                float ox=SCR_W/2.f, oy=SCR_H/2.f;
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.05f,0.01f,0.01f,0.75f); // overlay merah gelap penuh layar
                renderRect(ox-180,oy-115,360,230,0.1f,0.02f,0.02f,0.97f);          // kotak panel game over
                renderText("GAME  OVER",ox-10*8*3.f/2,oy-90,3,0.98f,0.18f,0.18f);
                renderText("THE CANOPY CLAIMED YOU",ox-22*8*1.6f/2,oy-45,1.6f,0.75f,0.4f,0.4f);
                renderText("SCORE: "+std::to_string(wallsPassed),ox-7*8*1.8f/2,oy-10,1.8f,.88f,.62f,.62f);
                renderText("PRESS R TO RESTART",ox-18*8*1.5f/2,oy+44,1.5f,1,.92f,.92f);
                renderText("PRESS M FOR MENU",ox-16*8*1.5f/2,oy+64,1.5f,.8f,.72f,.72f);
            }
            glEnable(GL_DEPTH_TEST);                                     // nyalain lagi depth test buat frame 3D berikutnya
        }
        glfwSwapBuffers(win);                                            // tampilin hasil gambar frame ini ke layar (double buffering)
        glfwPollEvents();                                                // proses event2 window (keyboard, close, dll)
    }
    // ---- keluar dari game loop (window ditutup atau pindah menu) ----
    glDeleteVertexArrays(1,&VAO); glDeleteBuffers(1,&VBO);               // bersihin resource OpenGL yang tadi dibikin
    glfwTerminate();                                                     // matiin GLFW
    return g_backToMenu;                                                 // balikin true kalau pemain minta balik ke menu (false kalau window ditutup paksa/exit total)
}

// =====================================================================================
// //////////////////////////////////// MENU & WEBVIEW /////////////////////////////////
// Bagian ini ngatur jendela MENU (pakai webview/HTML), bukan game 3D-nya
// =====================================================================================
static std::atomic<bool> g_startGame{false}, g_exit{false};              // penanda global: pemain klik "Start Journey" atau "Exit" di menu
void onMenuInvoke(struct webview* w,const char* arg){                    // fungsi ini dipanggil pas ada tombol HTML yang diklik (dari window.external.invoke di HTML)
    std::string cmd(arg);                                                // arg = perintah yang dikirim dari HTML ("startGame"/"options"/"exit")
    if(cmd=="startGame"){ g_startGame=true; webview_terminate(w); }       // kalau start, set penanda & tutup webview (biar lanjut ke game)
    else if(cmd=="options") webview_eval(w,"alert('HOW TO PLAY\\n\\nSPACE/W/UP = Jump\\nA/D = Move\\n\\nLompat ke platform bergerak untuk naik.\\nHindari platform yang menabrak dari samping!\\n\\nESC/P = Pause  |  R = Restart  |  M = Menu');"); // kalau klik "how to play", munculin alert JS berisi cara main
    else if(cmd=="exit"){ g_exit=true; webview_terminate(w); }            // kalau exit, set penanda & tutup webview (program berhenti total)
}

static std::string makeDataUrl(const char* html){                        // fungsi buat ubah string HTML mentah jadi URL "data:text/html,..." biar bisa dibuka webview tanpa file terpisah
    std::string out="data:text/html,";
    for(const char* p=html;*p;++p){                                      // loop tiap karakter di HTML
        unsigned char c=(unsigned char)*p;
        if(isalnum(c)||strchr("-_.!~*'()/:@,;?=&#+ ",c)) out+=(char)c;   // kalau karakternya aman (huruf/angka/simbol umum), langsung tempel apa adanya
        else{ char b[4]; snprintf(b,4,"%%%02X",c); out+=b; }             // kalau karakternya "berbahaya" buat URL (misal <, >, "), diubah jadi kode %XX (URL encoding)
    }
    return out;                                                          // hasil akhirnya string URL yang siap dipakai webview
}

// ///////////////////////// POSISIIN WINDOW MENU DI TENGAH LAYAR /////////////////////////
#if defined(_WIN32)||defined(_WIN64)
static void centerWebview(struct webview* wv){                           // khusus windows: pindahin window webview ke tengah layar
    HWND h=(HWND)wv->priv.hwnd; if(!h) return;                           // ambil handle window-nya, kalau gak ada ya keluar
    int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN); // ukuran layar
    RECT r; GetWindowRect(h,&r);                                        // ukuran window sekarang
    SetWindowPos(h,NULL,(sw-(r.right-r.left))/2,(sh-(r.bottom-r.top))/2,0,0,SWP_NOSIZE|SWP_NOZORDER); // geser window ke tengah
}
#else
static void centerWebview(struct webview*){}                            // di OS selain windows, fungsi ini gak ngapa-ngapain (kosong)
#endif

// =====================================================================================
// //////////////////////////////////// FUNGSI MAIN() ///////////////////////////////////
// Alur program: tampilin MENU -> kalau start, sembunyiin menu & jalanin GAME ->
// kalau dari game balik ke menu (M), ulang lagi ke MENU -> kalau exit, program berhenti
// =====================================================================================
int main(){
    std::string url=makeDataUrl(MENU_HTML);                              // siapin URL data buat tampilan menu HTML
    while(true){                                                         // loop utama program: bolak-balik antara MENU <-> GAME
        g_startGame=false; g_exit=false;                                 // reset penanda tiap kali balik ke menu
        struct webview menu={};                                          // bikin objek webview kosong buat window menu
        menu.title="Sylvan Odyssey"; menu.url=url.c_str();               // set judul & isi (HTML) window menu
        menu.width=SCR_W; menu.height=SCR_H; menu.resizable=0; menu.external_invoke_cb=onMenuInvoke; // atur ukuran, gak bisa di-resize, & daftarin fungsi callback tombol
        if(webview_init(&menu)!=0) return 1;                             // buka window menu-nya, kalau gagal langsung keluar program
        centerWebview(&menu);                                            // posisiin window menu ke tengah layar
        while(webview_loop(&menu,1)==0){}                                // loop khusus buat jendela menu (jalan terus sampai ditutup/di-terminate)
        webview_exit(&menu);                                             // bersihin resource webview menu
        if(g_exit) return 0;                                             // kalau tadi klik "Exit", program berhenti total
        if(!g_startGame) break;                                          // kalau window ditutup tanpa klik "Start" (misal user klik silang), keluar dari loop juga

        // ------------- delay dikit + sembunyiin window menu sebelum masuk game -------------
#if defined(_WIN32)||defined(_WIN64)
        Sleep(120);                                                      // jeda dikit (windows)
        { HWND h=FindWindowA(NULL,"Sylvan Odyssey"); if(h) ShowWindow(h,SW_HIDE); Sleep(150); } // cari window menu & sembunyiin biar gak numpuk sama window game
#else
        usleep(270000);                                                  // jeda dikit (linux/mac)
#endif
        if(!runGame()) break;                                            // jalanin game-nya! kalau runGame() balikin false (bukan minta balik menu), keluar dari loop -> program selesai
        // kalau runGame() balikin true (pemain tekan M), loop while(true) ini ngulang lagi -> balik nampilin MENU
    }
    return 0;                                                            // program selesai dengan normal
}