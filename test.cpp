// ============================================================================
// SYLVAN ODYSSEY - main.cpp (VERSI BERKOMENTAR)
// Game platformer 3D sederhana pakai OpenGL: pemain melompat dari platform
// (Wall) ke platform sambil menghindari ROKET yang terbang dari bawah ke atas.
// File ini dijelaskan baris-per-baris dalam bahasa Indonesia, dengan fokus
// khusus pada bagian ROKET (struct Rocket, spawn, update, gambar, tabrakan).
// ============================================================================

#define WEBVIEW_IMPLEMENTATION      // Aktifkan implementasi library webview.h (bukan cuma deklarasi)
#include "webview.h"                 // Library untuk menampilkan jendela HTML (dipakai buat menu utama)
#if defined(_WIN32)||defined(_WIN64) // Jika compile di Windows...
  #define WIN32_LEAN_AND_MEAN        // Kurangi header Windows yang jarang dipakai, biar compile lebih cepat
  #include <windows.h>                // API Windows (dipakai untuk pindah posisi jendela, sleep, dll)
#else                                 // Jika bukan Windows (Linux/Mac)...
  #include <unistd.h>                 // Untuk fungsi usleep() (jeda waktu)
#endif
#include <glad/glad.h>               // Loader fungsi-fungsi OpenGL modern
#include <GLFW/glfw3.h>              // Library untuk membuat window, keyboard input, dsb
#include <glm/glm.hpp>               // Library matematika vektor/matriks (vec3, mat4, dll)
#include <glm/gtc/matrix_transform.hpp> // Fungsi transformasi: translate, rotate, scale, perspective
#include <glm/gtc/type_ptr.hpp>      // Konversi glm::mat4/vec3 ke pointer float untuk OpenGL
#include <iostream>                  // std::cerr untuk print error
#include <vector>                    // std::vector, dipakai untuk list Wall & Rocket
#include <cmath>                     // fabs, sinf, fmodf, dll
#include <string>                    // std::string
#include <algorithm>                 // std::min, std::max, std::remove_if
#include <atomic>                    // std::atomic<bool>, untuk flag antar-thread yang aman
#include <random>                    // std::mt19937, untuk posisi acak roket
#define STB_IMAGE_IMPLEMENTATION     // Aktifkan implementasi stb_image (load gambar), tidak dipakai berat di sini
#include "stb_image.h"

// ----------------------------------------------------------------------------
// HTML untuk MENU UTAMA (dibuka lewat webview, sebelum masuk ke game 3D)
// Ini string HTML+CSS+JS mentah. Isinya TIDAK diberi komentar per baris
// karena itu teks HTML/CSS, bukan kode C++ — kalau disisipi komentar C++
// (//) akan merusak tampilan menu. Ringkasannya:
//   - Tombol "Start Journey"  -> memanggil startGame()  -> masuk ke game
//   - Tombol "HOW TO PLAY"    -> memanggil options()    -> munculkan alert petunjuk
//   - Tombol "Exit"           -> memanggil exit()       -> keluar aplikasi
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// KONSTANTA & VARIABEL GLOBAL DASAR
// ----------------------------------------------------------------------------
const unsigned int SCR_W=900, SCR_H=600;        // Ukuran layar window game (lebar, tinggi) dalam piksel
glm::vec3 camPos(0,2.5f,13);                    // Posisi kamera di dunia 3D (x, y, z)
glm::vec3 camFront(0,-0.15f,-1);                // Arah kamera menghadap (sedikit menunduk, ke arah -z)
glm::vec3 camUp(0,1,0);                         // Arah "atas" kamera, dipakai untuk hitung matriks view
float camTargetY=2.5f;                          // Target tinggi kamera yang mau dikejar (ikut naik saat pemain naik)
glm::vec3 lightPos(5,10,6);                     // Posisi sumber cahaya (matahari) di dunia
glm::vec3 lightColor(1,0.95f,0.85f);            // Warna cahaya (agak kekuningan hangat)

enum GameState{PLAYING,PAUSED,GAME_OVER};       // Kemungkinan status permainan
GameState gameState=PLAYING;                    // Status game saat ini, mulai dari PLAYING

float dt=0;            // delta time: berapa detik berlalu sejak frame sebelumnya
float lastFrame=0;     // waktu (glfwGetTime) saat frame terakhir dihitung
float gameTime=0;      // total waktu berjalan sejak game dimulai/reset
float hitCooldown=0;   // waktu "kebal" setelah pemain kena hit, supaya tidak damage berkali-kali

int wallsPassed=0;     // skor: jumlah platform (wall) yang berhasil dilewati/diinjak
int playerLives=5;     // nyawa pemain, mulai dari 5

const float GROUND_Y=0;       // ketinggian tanah/lantai dasar
const float GRAVITY=-28;      // percepatan gravitasi (negatif = menarik ke bawah)
const float JUMP_FORCE=10.5f; // kecepatan awal ke atas saat pemain lompat

const float PHW=0.36f;   // Player Half-Width: setengah lebar hitbox pemain (untuk deteksi tabrakan)
const float PH=1.9f;     // Player Height: tinggi hitbox pemain
const float PSPEED=6;    // kecepatan gerak kiri-kanan pemain
const float PBOUND=6.8f; // batas kiri/kanan area gerak pemain (tidak boleh keluar dari sini)

// ----------------------------------------------------------------------------
// WARNA-WARNA OBJEK (RGB, masing-masing 0.0 - 1.0)
// ----------------------------------------------------------------------------
const glm::vec3 COL_SKY(0.55f,0.78f,0.98f);         // warna panel langit
const glm::vec3 COL_SUN(1.0f,0.95f,0.35f);          // warna matahari
const glm::vec3 COL_GROUND(0.32f,0.58f,0.24f);      // warna tanah
const glm::vec3 COL_GROUND_TOP(0.42f,0.72f,0.32f);  // warna lapisan rumput di atas tanah
const glm::vec3 COL_WALL(0.2f,0.75f,0.35f);         // warna badan platform (wall)
const glm::vec3 COL_WALL_EDGE(0.35f,0.95f,0.5f);    // warna tepi/garis atas platform
const glm::vec3 COL_PLAYER_BODY(0.2f,0.55f,0.95f);  // warna badan pemain
const glm::vec3 COL_PLAYER_SKIN(0.96f,0.8f,0.62f);  // warna kepala/kulit pemain
const glm::vec3 COL_PLAYER_LIMB(0.12f,0.22f,0.6f);  // warna tangan & kaki pemain
const glm::vec3 COL_EYE_WHITE(0.95f,0.95f,0.95f);   // warna putih mata
const glm::vec3 COL_EYE_BLACK(0.05f,0.05f,0.05f);   // warna bola mata (pupil)

// --- Warna khusus ROKET ---
const glm::vec3 COL_ROCKET_BODY(0.85f,0.85f,0.9f);  // warna badan roket (abu-abu terang, mirip logam)
const glm::vec3 COL_ROCKET_NOSE(0.85f,0.1f,0.05f);  // warna ujung/hidung roket (merah)
const glm::vec3 COL_ROCKET_FIN(0.5f,0.5f,0.6f);     // warna sirip roket (abu-abu gelap)
const glm::vec3 COL_FLAME(1.0f,0.5f,0.05f);         // warna api/knalpot roket (oranye)

// ----------------------------------------------------------------------------
// STRUCT PLAYER: data pemain
// ----------------------------------------------------------------------------
struct Player{
    glm::vec3 pos{0,0,0}; // posisi pemain di dunia (x,y,z)
    float velY=0;         // kecepatan vertikal saat ini (untuk gravitasi & lompat)
    float animTimer=0;    // timer untuk animasi jalan (ayunan tangan/kaki)
    float standingY=0;    // ketinggian platform tempat pemain berdiri terakhir
    bool onGround=true;   // apakah pemain sedang menapak di sesuatu (tanah/platform)
} player;

// ----------------------------------------------------------------------------
// STRUCT WALL: platform yang bergerak dari kiri/kanan, tempat pemain melompat
// ----------------------------------------------------------------------------
struct Wall{
    float x,y;          // posisi platform
    float width,height;  // ukuran platform (lebar & tinggi setengah, karena dipakai sebagai scale cube)
    float speedSign;     // arah gerak: -1 (ke kiri) atau +1 (ke kanan)
    float delayTimer;    // sisa waktu tunda sebelum platform ini mulai muncul/bergerak
    int id;               // nomor urut platform
    bool passed;          // apakah pemain sudah pernah menginjak platform ini
    bool alive;           // apakah platform ini masih aktif (belum dihapus)
    bool playerOn;        // apakah pemain sedang berdiri di atas platform ini (frame ini)
    bool isFrozen;        // apakah platform berhenti bergerak (setelah diinjak atau menabrak pemain)
    bool waitingDelay;    // apakah platform masih menunggu delayTimer sebelum aktif
};
std::vector<Wall> walls;   // daftar semua platform yang sedang ada di layar
float wallSpeed=5.5f;      // kecepatan gerak platform saat ini (makin lama makin cepat)
bool spawnFromRight=false; // menentukan platform berikutnya muncul dari kanan atau kiri (bergantian)

// ----------------------------------------------------------------------------
// ★★★ STRUCT ROCKET ★★★  <-- INI BAGIAN UTAMA YANG KAMU TANYAKAN
// Roket adalah RINTANGAN yang terbang vertikal (dari bawah ke atas) dan bisa
// mengenai/melukai pemain kalau posisinya bersinggungan.
// ----------------------------------------------------------------------------
struct Rocket{
    float x;      // posisi horizontal roket (tetap, roket tidak bergerak ke kiri/kanan)
    float y;      // posisi vertikal roket (ini yang berubah tiap frame, roket "naik")
    float velY;   // kecepatan vertikal roket. Perhatikan: nilainya NEGATIF saat di-spawn
                  // (lihat kode spawn di bawah: -(7.f+ms*.3f)), artinya y akan BERKURANG,
                  // jadi sebenarnya roket bergerak TURUN, bukan naik. Ini "hujan roket dari atas".
    float trail;  // timer kecil untuk animasi kedipan api di ekor roket
    bool alive;   // apakah roket ini masih aktif (belum kena hapus)
};
std::vector<Rocket> rockets; // daftar semua roket yang sedang aktif di layar

int lastMilestone=0; // menyimpan "milestone" skor terakhir (kelipatan 10) yang sudah memicu roket

// ----------------------------------------------------------------------------
// SHADER SOURCE (kode GLSL untuk GPU) - komentar GLSL pakai // juga aman
// ----------------------------------------------------------------------------
// Vertex shader utama: menghitung posisi akhir tiap titik (vertex) di layar,
// dan mengirim posisi dunia + normal ke fragment shader untuk pencahayaan.
const char* vertSrc=R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal; out vec3 FragPos, Normal; uniform mat4 model, view, projection;
void main(){ FragPos=vec3(model*vec4(aPos,1)); Normal=mat3(transpose(inverse(model)))*aNormal; gl_Position=projection*view*vec4(FragPos,1); })";

// Fragment shader utama: menghitung warna akhir tiap piksel berdasarkan
// arah cahaya sederhana (diffuse lighting) dan warna objek.
const char* fragSrc=R"(#version 330 core
out vec4 FragColor; in vec3 FragPos, Normal; uniform vec3 lightPos, lightColor, objectColor; uniform float alpha;
void main(){ float diff=max(dot(normalize(Normal),normalize(lightPos-FragPos)),0.0); FragColor=vec4((0.38+diff*0.62)*lightColor*objectColor,alpha); })";

// Fragment shader untuk BAYANGAN (shadow) elips di bawah pemain: menggambar
// lingkaran/elips gelap yang memudar di tepinya.
const char* shadowFragSrc=R"(#version 330 core
out vec4 FragColor; in vec3 FragPos; uniform float alpha; uniform vec3 shadowCenter; uniform vec2 shadowRadius;
void main(){
    float dx=(FragPos.x-shadowCenter.x)/shadowRadius.x, dz=(FragPos.z-shadowCenter.z)/shadowRadius.y, d=dx*dx+dz*dz;
    if(d>1.0) discard;
    FragColor=vec4(0,0,0,alpha*(1.0-d));
})";

// ----------------------------------------------------------------------------
// FONT BITMAP 8x8 untuk menampilkan teks (skor, HUD, dsb) tanpa file gambar.
// Setiap karakter (96 karakter ASCII 32-127) direpresentasikan 8 byte,
// tiap byte = 1 baris piksel (bit 1 = piksel menyala).
// ----------------------------------------------------------------------------
static const unsigned char FONT8[96][8]={
// (data font, tidak diberi komentar per-angka karena ini murni data bitmap
// piksel huruf, bukan logika program)
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},{0x6C,0x6C,0x28,0x00,0x00,0x00,0x00,0x00},{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},{0x18,0x7C,0xD0,0x7C,0x16,0xFC,0x18,0x00},{0xC2,0xC6,0x0C,0x18,0x30,0x66,0xC6,0x00},{0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},{0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
{0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},{0x7C,0xC6,0x06,0x1C,0x70,0xC6,0xFE,0x00},{0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},{0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00},{0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},{0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},{0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00},{0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},{0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},{0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},{0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},{0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
{0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00},{0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0x00},{0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},{0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},{0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},{0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},{0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00},{0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0x00},{0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},{0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00},{0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00},{0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},{0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},{0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00},
{0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00},{0x38,0x6C,0xC6,0xC6,0xCE,0x6C,0x3A,0x00},{0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00},{0x78,0xCC,0xC0,0x70,0x18,0xCC,0x78,0x00},{0xFC,0xB4,0x30,0x30,0x30,0x30,0x78,0x00},{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00},{0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},{0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00},{0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00},{0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},{0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},{0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00},
{0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00},{0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00},{0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00},{0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00},{0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00},{0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8},{0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00},{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},{0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C},{0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00},{0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x00,0x00,0xCC,0xFE,0xD6,0xC6,0xC6,0x00},{0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00},{0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00},
{0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E},{0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00},{0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00},{0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8},{0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00},{0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00},{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},{0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00},{0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00},
};

// Handle-handle (ID) resource OpenGL global (texture, VAO/VBO, shader program)
unsigned int fontTex,fontVAO,fontVBO2,fontProg,whiteTex; // resource untuk render teks & kotak polos
unsigned int VAO,VBO,shaderProg,shadowProg;               // resource untuk render objek 3D (cube) & bayangan

// Vertex/fragment shader khusus untuk render teks 2D (HUD) di atas layar
const char* fontV=R"(#version 330 core
layout(location=0) in vec2 aPos; layout(location=1) in vec2 aUV; out vec2 vUV; uniform mat4 proj;
void main(){ vUV=aUV; gl_Position=proj*vec4(aPos,0,1); })";
const char* fontF=R"(#version 330 core
in vec2 vUV; out vec4 C; uniform sampler2D tex; uniform vec4 color;
void main(){ C=vec4(color.rgb, color.a*texture(tex,vUV).r); })";

// ----------------------------------------------------------------------------
// initFont(): membangun texture atlas font dari data bitmap FONT8 di atas,
// lalu menyiapkan VAO/VBO untuk menggambar quad (persegi) yang menampilkan
// potongan texture tersebut sebagai huruf.
// ----------------------------------------------------------------------------
void initFont(){
    static unsigned char atlas[48][128]={};       // buffer gambar atlas font (128x48 piksel, 1 channel)
    for(int c=0;c<96;c++){                          // untuk tiap 96 karakter...
        int col=c%16, row=c/16;                     // hitung posisi kolom & baris karakter di atlas (grid 16 kolom)
        for(int y=0;y<8;y++){                        // untuk tiap baris piksel (8 baris per karakter)...
            unsigned char b=FONT8[c][y];             // ambil 1 byte data baris piksel karakter ini
            for(int x=0;x<8;x++)                     // untuk tiap kolom piksel (8 kolom)...
                if(b&(0x80>>x))                       // jika bit ke-x menyala...
                    atlas[row*8+y][col*8+x]=255;       // set piksel jadi putih penuh di atlas
        }
    }
    glGenTextures(1,&fontTex); glBindTexture(GL_TEXTURE_2D,fontTex);           // buat & aktifkan texture font
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,128,48,0,GL_RED,GL_UNSIGNED_BYTE,atlas); // upload data atlas ke GPU (1 channel merah)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); // filter nearest = tegas, tidak blur (cocok untuk font pixel-art)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); // jangan diulang di tepi
    unsigned char w=255;                             // 1 piksel putih polos...
    glGenTextures(1,&whiteTex); glBindTexture(GL_TEXTURE_2D,whiteTex);          // ...dipakai sebagai texture untuk menggambar kotak/HUD polos
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&w);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glGenVertexArrays(1,&fontVAO); glGenBuffers(1,&fontVBO2);                   // siapkan VAO/VBO untuk quad teks
    glBindVertexArray(fontVAO); glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*24,NULL,GL_DYNAMIC_DRAW);        // alokasikan buffer kosong (akan diisi ulang tiap gambar quad)
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0); glEnableVertexAttribArray(0); // atribut posisi (x,y)
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float))); glEnableVertexAttribArray(1); // atribut UV (koordinat texture)
    glBindVertexArray(0);
}

// quadDraw(): mengisi 6 vertex (2 segitiga = 1 persegi) dengan posisi & UV
// tertentu, lalu menggambarnya. Dipakai untuk teks maupun kotak HUD.
static void quadDraw(float x,float y,float w,float h,float u0,float v0,float u1,float v1){
    float v[6][4]={{x,y+h,u0,v1},{x,y,u0,v0},{x+w,y,u1,v0},
                   {x,y+h,u0,v1},{x+w,y,u1,v0},{x+w,y+h,u1,v1}};
    glBindVertexArray(fontVAO); glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);   // update isi buffer dengan 6 vertex quad
    glDrawArrays(GL_TRIANGLES,0,6);                    // gambar 6 vertex sebagai 2 segitiga
}

// renderRect(): menggambar kotak polos berwarna (dipakai untuk latar HUD,
// layar merah saat kena hit, kotak pause/game over, dll)
void renderRect(float x,float y,float w,float h,float r,float g,float b,float a){
    glUseProgram(fontProg); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,whiteTex); // pakai texture putih polos
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);   // set warna & transparansi kotak
    quadDraw(x,y,w,h,0,0,1,1);                                       // gambar kotak di posisi (x,y) ukuran (w,h)
}

// renderText(): menggambar string teks memakai font bitmap, karakter demi karakter
void renderText(const std::string& s,float x,float y,float sc,float r=1,float g=1,float b=1,float a=1){
    glUseProgram(fontProg); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,fontTex); // pakai texture atlas font
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);   // warna teks
    for(char c:s){                                                  // untuk tiap karakter dalam string...
        if(c<32||c>127) c=32;                                        // karakter di luar jangkauan font -> jadikan spasi
        int idx=c-32, col=idx%16, row=idx/16;                        // cari posisi karakter ini di atlas
        quadDraw(x,y,8*sc,8*sc, col*8/128.f, row*8/48.f, (col+1)*8/128.f, (row+1)*8/48.f); // gambar 1 huruf
        x+=8*sc;                                                      // geser posisi x untuk huruf berikutnya
    }
}

// renderHeart(): menggambar 1 ikon hati (untuk menampilkan nyawa pemain di HUD)
// dari beberapa kotak kecil yang disusun membentuk bentuk hati piksel.
void renderHeart(float x,float y,float s,bool filled){
    float r=filled?.95f:.28f, g=filled?.15f:.22f, b=filled?.18f:.26f; // hati terisi = merah terang, kosong = abu gelap
    renderRect(x+s,y,s,s,r,g,b,1); renderRect(x+4*s,y,s,s,r,g,b,1);
    renderRect(x,y+s,6*s,s,r,g,b,1); renderRect(x,y+2*s,6*s,s,r,g,b,1);
    renderRect(x+s,y+3*s,4*s,s,r,g,b,1); renderRect(x+2*s,y+4*s,2*s,s,r,g,b,1); renderRect(x+3*s,y+5*s,s,s,r,g,b,1);
}

// Data mentah 36 vertex kubus (6 sisi x 2 segitiga x 3 titik), tiap vertex
// berisi (posisi x,y,z) + (normal x,y,z). Ini dipakai untuk MENGGAMBAR SEMUA
// objek 3D di game (pemain, platform, roket, tanah, dll) — semuanya sebenarnya
// cuma kubus yang di-scale/geser dengan ukuran & posisi berbeda.
float cubeVerts[]={-1,-1,-1,0,0,-1, 1,-1,-1,0,0,-1, 1,1,-1,0,0,-1, 1,1,-1,0,0,-1,-1,1,-1,0,0,-1,-1,-1,-1,0,0,-1, -1,-1,1,0,0,1,   1,-1,1,0,0,1,   1,1,1,0,0,1,   1,1,1,0,0,1,  -1,1,1,0,0,1,  -1,-1,1,0,0,1, -1,1,1,-1,0,0, -1,1,-1,-1,0,0, -1,-1,-1,-1,0,0,-1,-1,-1,-1,0,0,-1,-1,1,-1,0,0,-1,1,1,-1,0,0, 1,1,1,1,0,0,  1,1,-1,1,0,0,   1,-1,-1,1,0,0, 1,-1,-1,1,0,0, 1,-1,1,1,0,0,   1,1,1,1,0,0, -1,-1,-1,0,-1,0, 1,-1,-1,0,-1,0, 1,-1,1,0,-1,0, 1,-1,1,0,-1,0, -1,-1,1,0,-1,0, -1,-1,-1,0,-1,0, -1,1,-1,0,1,0,  1,1,-1,0,1,0,   1,1,1,0,1,0,   1,1,1,0,1,0, -1,1,1,0,1,0,  -1,1,-1,0,1,0,};

// makeShader(): compile vertex shader + fragment shader, lalu link jadi
// satu "program" shader yang siap dipakai untuk menggambar.
unsigned int makeShader(const char* vs,const char* fs){
    auto compile=[](unsigned int t,const char* src){
        unsigned int s=glCreateShader(t);            // buat objek shader kosong bertipe t (vertex/fragment)
        glShaderSource(s,1,&src,NULL); glCompileShader(s); // masukkan source code & compile
        int ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);    // cek apakah compile berhasil
        if(!ok){char log[512];glGetShaderInfoLog(s,512,NULL,log);std::cerr<<log;} // kalau gagal, print pesan error
        return s;
    };
    unsigned int v=compile(GL_VERTEX_SHADER,vs), f=compile(GL_FRAGMENT_SHADER,fs), p=glCreateProgram(); // compile keduanya, buat program
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p); // gabungkan vertex+fragment shader jadi 1 program
    glDeleteShader(v); glDeleteShader(f);                        // shader individu sudah tidak perlu lagi setelah di-link
    return p;                                                     // kembalikan ID program shader
}

// setUniforms(): kirim matriks projection & view (kamera) ke shader tertentu,
// dan aktifkan VAO kubus supaya siap digambar.
void setUniforms(unsigned int prog,glm::mat4& proj,glm::mat4& view){
    glUseProgram(prog); glBindVertexArray(VAO);
    glUniformMatrix4fv(glGetUniformLocation(prog,"projection"),1,GL_FALSE,glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(prog,"view"),1,GL_FALSE,glm::value_ptr(view));
}

// drawCube(): menggambar SATU kubus di posisi `pos`, dengan ukuran `sc`
// (scale per-sumbu), warna `col`, dan transparansi `a`.
// Fungsi inilah yang dipakai berulang-ulang untuk membentuk roket, pemain,
// platform, tanah, dsb — masing-masing bagian tubuh/objek = 1 pemanggilan drawCube.
void drawCube(unsigned int prog,glm::vec3 pos,glm::vec3 sc,glm::vec3 col,float a=1){
    glm::mat4 m=glm::scale(glm::translate(glm::mat4(1),pos),sc); // matriks model = geser ke `pos`, lalu skala sebesar `sc`
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m)); // kirim matriks model ke shader
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(col));         // kirim warna objek
    glUniform1f(glGetUniformLocation(prog,"alpha"),a);                                     // kirim transparansi
    glDrawArrays(GL_TRIANGLES,0,36);                                                        // gambar 36 vertex (kubus) sebagai segitiga
}

// drawLimb(): mirip drawCube, tapi menambahkan ROTASI di sekitar titik pivot
// (dipakai untuk menggambar tangan/kaki pemain yang bisa berayun/berputar)
void drawLimb(unsigned int prog,glm::vec3 pivot,float angle,glm::vec3 offset,glm::vec3 sc,glm::vec3 col){
    glm::mat4 m=glm::translate(glm::mat4(1),pivot);                 // pindah ke titik pivot (sendi, misal bahu/pangkal paha)
    m=glm::rotate(m,glm::radians(angle),glm::vec3(1,0,0));          // putar sebesar `angle` derajat di sumbu X
    m=glm::translate(m,offset);                                     // geser lagi sesuai offset (supaya limb menjuntai dari pivot)
    m=glm::scale(m,sc);                                             // skalakan ukuran limb
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(col));
    glUniform1f(glGetUniformLocation(prog,"alpha"),1);
    glDrawArrays(GL_TRIANGLES,0,36);
}

// drawPlayer(): menggambar karakter pemain lengkap dari beberapa kubus:
// 2 kaki, badan, 2 tangan, kepala, dan 2 pasang mata (putih+pupil).
void drawPlayer(unsigned int prog,glm::vec3 base,float s,float leg,float arm){
    drawLimb(prog,base+glm::vec3(-s*.2f,s*.3f,0), leg,{0,-s*.3f,0},{s*.16f,s*.3f,s*.16f},COL_PLAYER_LIMB); // kaki kiri
    drawLimb(prog,base+glm::vec3( s*.2f,s*.3f,0),-leg,{0,-s*.3f,0},{s*.16f,s*.3f,s*.16f},COL_PLAYER_LIMB); // kaki kanan (ayunan berlawanan)
    drawCube(prog,base+glm::vec3(0,s*.85f,0),{s*.36f,s*.38f,s*.2f},COL_PLAYER_BODY);                        // badan
    drawLimb(prog,base+glm::vec3(-s*.52f,s*.95f,0),-arm,{0,-s*.24f,0},{s*.14f,s*.26f,s*.14f},COL_PLAYER_LIMB); // tangan kiri
    drawLimb(prog,base+glm::vec3( s*.52f,s*.95f,0), arm,{0,-s*.24f,0},{s*.14f,s*.26f,s*.14f},COL_PLAYER_LIMB); // tangan kanan
    drawCube(prog,base+glm::vec3(0,s*1.52f,0),{s*.33f,s*.3f,s*.26f},COL_PLAYER_SKIN);                        // kepala
    drawCube(prog,base+glm::vec3(-s*.13f,s*1.59f,s*.30f),{s*.07f,s*.07f,s*.04f},COL_EYE_WHITE);              // mata kiri (putih)
    drawCube(prog,base+glm::vec3(-s*.13f,s*1.59f,s*.35f),{s*.04f,s*.04f,s*.03f},COL_EYE_BLACK);              // mata kiri (pupil, sedikit lebih depan)
    drawCube(prog,base+glm::vec3( s*.13f,s*1.59f,s*.30f),{s*.07f,s*.07f,s*.04f},COL_EYE_WHITE);              // mata kanan (putih)
    drawCube(prog,base+glm::vec3( s*.13f,s*1.59f,s*.35f),{s*.04f,s*.04f,s*.03f},COL_EYE_BLACK);              // mata kanan (pupil)
}

// spawnWall(): membuat 1 platform baru dan memasukkannya ke `walls`.
void spawnWall(bool forceSame=false,float delay=0){
    Wall w;
    w.id=wallsPassed+1; w.height=0.16f;                                   // nomor urut & tinggi tetap
    w.passed=w.playerOn=w.isFrozen=false; w.alive=true;                   // status awal: belum diinjak, belum beku, aktif
    w.waitingDelay=(delay>0); w.delayTimer=delay;                          // jika delay>0, platform menunggu dulu sebelum tampil
    w.width=std::max(6.f-(wallsPassed/10)*0.55f,1.5f);                     // makin tinggi skor, platform makin sempit (lebih sulit)
    bool right=forceSame?!spawnFromRight:spawnFromRight;                   // tentukan arah datang platform ini
    w.speedSign=right?-1.f:1.f;                                            // jika datang dari kanan, bergerak ke kiri (-1), dan sebaliknya
    w.x=w.waitingDelay?999.f:(right?16.f:-16.f);                           // posisi awal: di luar layar (kanan/kiri), atau "parkir" di 999 kalau masih delay
    if(!forceSame) spawnFromRight=!spawnFromRight;                         // bergantian kiri-kanan untuk spawn normal
    w.y=player.standingY+0.75f;                                            // tinggi platform = tinggi pijakan pemain saat ini + naik sedikit
    walls.push_back(w);                                                    // masukkan ke daftar platform aktif
}

// resetGame(): mengembalikan semua variabel game ke kondisi awal (dipanggil
// saat mulai main / tekan tombol R restart).
void resetGame(){
    player={}; walls.clear(); rockets.clear();     // reset posisi pemain, kosongkan semua platform & ROKET
    wallsPassed=0; wallSpeed=5.5f; gameTime=0;      // reset skor, kecepatan platform, waktu main
    gameState=PLAYING; playerLives=5;               // kembali ke status bermain, nyawa penuh
    hitCooldown=0; spawnFromRight=false; lastMilestone=0; // reset cooldown, arah spawn, milestone roket
    camPos.y=2.5f; camTargetY=2.5f;                 // reset posisi kamera
    spawnWall(false,1.2f);                          // buat platform pertama (muncul setelah delay 1.2 detik)
}

static bool g_backToMenu=false;   // flag: apakah pemain menekan M untuk kembali ke menu
static bool keys[512]={};         // status tiap tombol keyboard: true = sedang ditekan

// key_callback(): dipanggil GLFW setiap ada event keyboard (tekan/lepas tombol)
void key_callback(GLFWwindow* win,int key,int,int action,int){
    if(key>=0&&key<512){                              // pastikan kode tombol valid (dalam batas array `keys`)
        if(action==GLFW_PRESS)   keys[key]=true;        // tombol ditekan -> tandai true
        if(action==GLFW_RELEASE) keys[key]=false;        // tombol dilepas -> tandai false
    }
    if(action!=GLFW_PRESS) return;                     // sisa kode di bawah hanya untuk event "baru ditekan" (bukan tahan/lepas)
    if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W)&&player.onGround&&gameState==PLAYING){
        player.velY=JUMP_FORCE; player.onGround=false;   // SPACE/UP/W saat menapak & sedang main -> lompat
    }
    if(key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P) gameState=gameState==PLAYING?PAUSED:(gameState==PAUSED?PLAYING:gameState); // ESC/P -> toggle pause
    if(key==GLFW_KEY_R) resetGame();                    // R -> restart game
    if(key==GLFW_KEY_M){ g_backToMenu=true; glfwSetWindowShouldClose(win,GLFW_TRUE); } // M -> tutup window game, kembali ke menu
}

// framebuffer_size_callback(): dipanggil saat ukuran window berubah, supaya
// area render OpenGL (viewport) disesuaikan.
void framebuffer_size_callback(GLFWwindow*,int w,int h){ glViewport(0,0,w,h); }

// takeDamage(): dipanggil setiap pemain terkena tabrakan (dari platform yang
// menabrak dari samping, ATAU dari roket). Mengurangi nyawa, memberi waktu
// kebal (hitCooldown), dan memindahkan pemain kembali ke titik aman.
void takeDamage(){
    playerLives--; hitCooldown=1.8f;                   // kurangi nyawa, beri 1.8 detik waktu kebal (tidak bisa kena damage lagi)
    if(playerLives<=0){ gameState=GAME_OVER; return; }  // nyawa habis -> game over, berhenti di sini
    player.pos={0,player.standingY,0};                  // pemain "dilempar balik" ke tengah, di ketinggian pijakan terakhir
    player.velY=0; player.onGround=true;                // reset kecepatan jatuh & anggap kembali menapak
}

// ----------------------------------------------------------------------------
// runGame(): FUNGSI UTAMA GAME LOOP. Menyiapkan window OpenGL, shader, lalu
// menjalankan loop update+render sampai window ditutup / kembali ke menu.
// ----------------------------------------------------------------------------
bool runGame(){
    g_backToMenu=false; lastFrame=0;                    // reset flag & timer sebelum mulai
    glfwInit();                                          // inisialisasi GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3); // minta OpenGL versi 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);  // pakai core profile (tanpa fitur lama/deprecated)
    GLFWwindow* win=glfwCreateWindow(SCR_W,SCR_H,"Sylvan Odyssey",NULL,NULL); // buat window game
#if defined(_WIN32)||defined(_WIN64)
    { int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN); // di Windows, hitung ukuran layar
      glfwSetWindowPos(win,(sw-SCR_W)/2,(sh-SCR_H)/2); }                     // lalu posisikan window di tengah layar
#endif
    glfwMakeContextCurrent(win);                          // aktifkan context OpenGL window ini
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback); // daftarkan callback resize
    glfwSetKeyCallback(win,key_callback);                  // daftarkan callback keyboard
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);    // load semua fungsi OpenGL lewat GLAD
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND);           // aktifkan depth test (objek dekat menutupi jauh) & transparansi
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);      // aturan pencampuran warna transparan standar

    shaderProg=makeShader(vertSrc,fragSrc);                // compile shader utama (untuk objek 3D bertekstur cahaya)
    shadowProg=makeShader(vertSrc,shadowFragSrc);          // compile shader bayangan
    fontProg=makeShader(fontV,fontF);                      // compile shader teks/HUD
    initFont();                                             // siapkan texture font & buffer quad

    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO);        // buat VAO/VBO untuk data kubus
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW); // upload data 36 vertex kubus ke GPU (statis, tidak berubah)
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0); // atribut posisi (3 float)
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1); // atribut normal (3 float)

    resetGame();                                            // mulai kondisi game dari awal

    // ============================= GAME LOOP =============================
    while(!glfwWindowShouldClose(win)){
        float now=(float)glfwGetTime();                     // waktu sekarang sejak GLFW dimulai
        dt=std::min(now-lastFrame,0.05f); lastFrame=now;     // delta time, dibatasi maksimal 0.05 detik (anti lonjakan lag)
        if(hitCooldown>0) hitCooldown-=dt;                   // kurangi waktu kebal tiap frame

        if(gameState==PLAYING){
            // --- UPDATE PEMAIN ---
            gameTime+=dt; player.animTimer+=dt;               // tambah waktu main & timer animasi
            player.velY+=GRAVITY*dt; player.pos.y+=player.velY*dt; // terapkan gravitasi lalu update posisi Y
            if(keys[GLFW_KEY_A]||keys[GLFW_KEY_LEFT])  player.pos.x-=PSPEED*dt; // gerak kiri
            if(keys[GLFW_KEY_D]||keys[GLFW_KEY_RIGHT]) player.pos.x+=PSPEED*dt; // gerak kanan
            player.pos.x=std::max(-PBOUND,std::min(PBOUND,player.pos.x)); // batasi posisi X agar tidak keluar arena

            bool onGnd=false; float floor=GROUND_Y;            // asumsikan belum menapak, lantai default = tanah
            if(player.pos.y<=GROUND_Y){ player.pos.y=GROUND_Y; player.velY=0; onGnd=true; } // kalau jatuh ke tanah, berhenti di situ

            // --- CEK TABRAKAN DENGAN PLATFORM (WALL) ---
            bool nextSpawn=false;
            std::vector<Wall> pending;                          // platform baru yang akan ditambahkan setelah loop (spawn ulang setelah menabrak)
            for(auto& w:walls){
                if(!w.alive||w.waitingDelay) continue;            // lewati platform yang tidak aktif/masih menunggu
                if(fabs(player.pos.x-w.x)>=PHW+w.width) continue; // lewati kalau posisi X jauh (tidak mungkin bersentuhan)
                float foot=player.pos.y, head=player.pos.y+PH;    // titik kaki & kepala pemain
                float top=w.y+w.height, bot=w.y-w.height;         // sisi atas & bawah platform
                if(player.velY<=0&&foot>=top-0.35f&&foot<=top+0.15f){ // pemain sedang turun & kaki dekat sisi atas platform -> "mendarat"
                    if(!w.isFrozen&&!w.passed){
                        w.isFrozen=w.passed=true; wallsPassed++; nextSpawn=true; // platform baru diinjak pertama kali -> tambah skor, beku, minta spawn platform baru
                        player.pos.y=top; player.velY=0; onGnd=true; floor=top; w.playerOn=true;
                    }
                    else if(w.passed&&top<player.standingY-0.1f&&hitCooldown<=0) takeDamage(); // platform lama yang lebih rendah dari pijakan -> dianggap tabrakan, kena damage
                    else{ player.pos.y=top; player.velY=0; onGnd=true; floor=top; w.playerOn=true; } // kondisi lain -> tetap berdiri di atasnya
                }
                else if(!w.isFrozen&&!w.passed&&hitCooldown<=0&&foot<top-0.15f&&head>bot+0.15f){
                    // pemain tertabrak platform dari SAMPING (badan menembus area platform, bukan mendarat di atas)
                    w.isFrozen=true; w.alive=false;                 // platform ini "hancur" (dihapus)
                    Wall rep=w;                                      // buat salinan platform untuk di-spawn ulang nanti
                    rep.passed=rep.playerOn=rep.isFrozen=false;
                    rep.alive=true; rep.waitingDelay=true; rep.delayTimer=1.f; rep.x=999.f; // parkir dulu 1 detik sebelum muncul lagi
                    pending.push_back(rep);
                    for(auto& fw:walls) if(fw.alive&&!fw.isFrozen&&!fw.waitingDelay) fw.isFrozen=true; // bekukan semua platform lain sesaat (efek "jeda" saat kena hit)
                    takeDamage();                                    // pemain kena damage
                }
            }
            for(auto& r:pending) walls.push_back(r);              // masukkan platform pengganti yang tadi disiapkan
            player.onGround=onGnd;
            if(onGnd){ player.standingY=floor; player.pos.y=floor; player.velY=0; } // kalau menapak, kunci posisi Y ke permukaan pijakan
            if(nextSpawn&&gameState==PLAYING) spawnWall(false,0.15f); // platform baru diinjak -> munculkan platform berikutnya (delay singkat)
            wallSpeed=std::min(5.5f+(float)wallsPassed*0.45f,22.f);   // makin tinggi skor, platform makin cepat (maksimal 22)

            // -----------------------------------------------------------------
            // ★★★ BAGIAN ROKET: SPAWN, UPDATE, & TABRAKAN ★★★
            // -----------------------------------------------------------------
            int ms=wallsPassed/10;                                  // hitung "level milestone": tiap kelipatan 10 skor
            if(ms>0&&ms>lastMilestone){                             // kalau baru saja mencapai milestone baru (misal skor 10, 20, 30...)
                lastMilestone=ms;                                    // catat milestone ini supaya tidak trigger berulang
                std::mt19937 rng(std::random_device{}());            // generator angka acak
                std::uniform_real_distribution<float> xd(-5.5f,5.5f); // distribusi posisi X acak, dalam rentang arena
                std::uniform_int_distribution<int> countd(1,2);       // distribusi JUMLAH roket acak: hasilnya 1 atau 2
                int rocketCount=countd(rng);                          // undi sekali: dapat 1 atau 2 roket untuk milestone ini
                for(int k=0;k<rocketCount;k++)                        // setiap milestone, spawn SEACAK rocketCount roket (bisa 1, bisa 2)
                    rockets.push_back({
                        xd(rng),                  // x: posisi horizontal acak
                        player.pos.y+8.f,         // y: mulai jauh di ATAS posisi pemain saat ini (di luar layar atas)
                        -(7.f+ms*.3f),             // velY: NEGATIF -> roket bergerak TURUN, makin tinggi milestone makin cepat turunnya
                        0,                          // trail: timer animasi api, mulai dari 0
                        true                        // alive: aktif
                    });
            }
            for(auto& r:rockets){                                    // update posisi & cek tabrakan tiap roket aktif
                if(!r.alive) continue;                                 // lewati roket yang sudah tidak aktif
                r.y+=r.velY*dt; r.trail+=dt;                           // gerakkan roket sesuai velY (turun), update timer animasi api
                if(r.y<player.standingY-2){ r.alive=false; continue; } // kalau roket sudah jauh di bawah pijakan pemain -> matikan (hemat resource, tidak perlu digambar/cek lagi)
                if(hitCooldown<=0                                      // hanya kena damage kalau pemain tidak sedang kebal
                   &&fabs(r.x-player.pos.x)<PHW+0.25f                  // dan posisi X roket dekat dengan pemain (overlap horizontal)
                   &&r.y>=player.pos.y-.1f&&r.y<=player.pos.y+PH+.2f){ // dan posisi Y roket setinggi badan pemain (overlap vertikal)
                    r.alive=false; takeDamage();                       // roket "meledak" (hilang) dan pemain kena damage
                }
            }
            // buang semua roket yang statusnya alive=false dari vector (bersihkan memori)
            rockets.erase(std::remove_if(rockets.begin(),rockets.end(),[](const Rocket& r){return !r.alive;}),rockets.end());
            // -----------------------------------------------------------------
            // ★★★ AKHIR BAGIAN ROKET UTAMA (nanti roket masih digambar & ditampilkan
            //     peringatannya di bagian render & HUD di bawah) ★★★
            // -----------------------------------------------------------------

            // --- UPDATE POSISI PLATFORM (WALL) ---
            for(size_t i=0;i<walls.size();i++){
                if(!walls[i].alive) continue;
                if(walls[i].waitingDelay){                            // platform masih dalam masa tunggu (delay)
                    walls[i].delayTimer-=dt;
                    if(walls[i].delayTimer<=0){                        // waktu tunggu habis -> aktifkan platform, taruh di tepi layar
                        walls[i].waitingDelay=false;
                        walls[i].x=walls[i].speedSign<0?16.f:-16.f;
                    }
                    continue;
                }
                if(!walls[i].isFrozen) walls[i].x+=walls[i].speedSign*wallSpeed*dt; // gerakkan platform kalau tidak sedang beku
                bool out=(walls[i].speedSign<0&&walls[i].x<-16)||(walls[i].speedSign>0&&walls[i].x>16); // cek apakah sudah keluar layar
                if(out&&!walls[i].isFrozen){ walls[i].alive=false; spawnWall(true,0.5f); } // kalau keluar layar -> hapus & spawn platform baru di sisi yang sama
            }

            // --- UPDATE KAMERA MENGIKUTI PEMAIN ---
            camTargetY=player.pos.y+2.5f;                           // target tinggi kamera = tinggi pemain + offset
            camPos.y+=(camTargetY-camPos.y)*8.f*dt;                 // gerakkan kamera perlahan (smooth follow / lerp) menuju target
        }

        // --- UPDATE JUDUL WINDOW SESUAI STATUS GAME ---
        std::string title;
        if(gameState==PLAYING) title="Sylvan Odyssey | Score:"+std::to_string(wallsPassed)+" | Lives:"+std::to_string(playerLives)+" | ESC=Pause";
        else if(gameState==PAUSED) title="PAUSED | ESC=Resume | R=Restart";
        else title="GAME OVER | Score:"+std::to_string(wallsPassed)+" | R=Restart";
        glfwSetWindowTitle(win,title.c_str());

        // ================= MULAI RENDER FRAME =================
        float camOff=camPos.y-2.5f;                                // offset kamera dari posisi awal, dipakai untuk gerakkan background ikut naik
        glClearColor(0.38f,0.62f,0.88f,1);                         // warna bersih layar (biru langit dasar)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);          // bersihkan buffer warna & depth sebelum gambar frame baru

        glm::mat4 proj=glm::perspective(glm::radians(62.f),(float)SCR_W/SCR_H,0.1f,100.f); // matriks proyeksi perspektif (field of view 62°)
        glm::mat4 view=glm::lookAt(camPos,camPos+camFront,camUp);  // matriks kamera (lihat dari camPos ke arah camFront)
        setUniforms(shaderProg,proj,view);
        glUniform3fv(glGetUniformLocation(shaderProg,"lightPos"),1,glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProg,"lightColor"),1,glm::value_ptr(lightColor));

        // --- GAMBAR LATAR: LANGIT, MATAHARI, TANAH ---
        drawCube(shaderProg,{0,3.5f+camOff,-9},{22,20,0.3f},COL_SKY);        // panel langit besar di kejauhan
        drawCube(shaderProg,{0,6.5f+camOff,-7},{0.6f,0.6f,0.2f},COL_SUN);    // matahari
        drawCube(shaderProg,{0,-0.3f,0},{15,0.3f,2.5f},COL_GROUND);          // tanah
        drawCube(shaderProg,{0,0.02f,0},{15,0.05f,2.5f},COL_GROUND_TOP);     // lapisan rumput tipis di atas tanah
        setUniforms(shaderProg,proj,view);

        // --- GAMBAR SEMUA PLATFORM (WALL) ---
        for(auto& w:walls){
            if(!w.alive||w.waitingDelay) continue;
            glm::vec3 wc=w.isFrozen?COL_WALL*0.82f:COL_WALL;               // platform beku sedikit lebih gelap
            glm::vec3 edge=w.isFrozen?COL_WALL_EDGE*0.82f:COL_WALL_EDGE;
            drawCube(shaderProg,{w.x,w.y,0},{w.width,w.height,0.55f},wc);           // badan platform
            drawCube(shaderProg,{w.x,w.y+w.height,0},{w.width+0.02f,0.02f,0.58f},edge); // garis tepi atas platform
        }

        // --- GAMBAR BAYANGAN LONJONG DI BAWAH PEMAIN ---
        {
            setUniforms(shadowProg,proj,view);
            float sf=GROUND_Y;                                              // sf = permukaan tertinggi tepat di bawah pemain (default tanah)
            for(auto& w:walls)
                if(w.alive&&!w.waitingDelay&&w.y+w.height<=player.pos.y+0.02f&&fabs(player.pos.x-w.x)<PHW+w.width)
                    sf=std::max(sf,w.y+w.height);                            // cari platform tertinggi di bawah pemain untuk taruh bayangan
            float h=std::max(player.pos.y-sf,0.f);                          // ketinggian pemain relatif ke permukaan tersebut
            float rX=std::max(0.55f-h*.045f,.18f), rZ=std::max(0.32f-h*.025f,.10f), al=std::max(0.85f-h*.07f,.15f), sy=sf+0.08f; // bayangan mengecil & memudar makin tinggi pemain melompat
            glUniform3f(glGetUniformLocation(shadowProg,"shadowCenter"),player.pos.x,sy,player.pos.z);
            glUniform2f(glGetUniformLocation(shadowProg,"shadowRadius"),rX,rZ);
            glUniform1f(glGetUniformLocation(shadowProg,"alpha"),al);
            glDepthMask(GL_FALSE);                                          // matikan penulisan depth buffer supaya bayangan tidak "menutupi" objek lain secara aneh
            glm::mat4 sm=glm::scale(glm::translate(glm::mat4(1),{player.pos.x,sy,player.pos.z}),{rX*2.2f,0.005f,rZ*2.2f}); // kubus super pipih = jadi elips bayangan
            glUniformMatrix4fv(glGetUniformLocation(shadowProg,"model"),1,GL_FALSE,glm::value_ptr(sm));
            glDrawArrays(GL_TRIANGLES,0,36);
            glDepthMask(GL_TRUE);                                           // nyalakan lagi depth mask untuk objek berikutnya
            setUniforms(shaderProg,proj,view);                              // kembali ke shader utama untuk lanjut gambar objek biasa
        }

        // -----------------------------------------------------------------
        // ★★★ GAMBAR SEMUA ROKET AKTIF ★★★
        // Tiap roket digambar dari 4 kubus terpisah yang ditumpuk vertikal:
        //   1) badan roket (silinder panjang, disederhanakan jadi kubus)
        //   2) hidung/ujung roket (di bawah badan, mengarah ke bawah karena roket "jatuh")
        //   3) sirip roket (agak di atas badan)
        //   4) nyala api knalpot yang berkedip-kedip (paling atas, karena roket
        //      turun dengan "belakang"/apinya menghadap ke atas)
        // -----------------------------------------------------------------
        for(auto& r:rockets){
            if(!r.alive) continue;                                          // lewati roket tidak aktif
            float t=(float)glfwGetTime();                                   // waktu global, dipakai untuk animasi kedip api
            float fl=0.15f+0.1f*sinf(t*20+r.x*3);                           // ukuran api berosilasi (efek berkedip), fase digeser oleh posisi x supaya tiap roket tidak sinkron
            drawCube(shaderProg,{r.x,r.y,0},      {0.18f,0.75f,0.18f},COL_ROCKET_BODY); // badan roket: silinder ramping (tinggi jauh lebih besar dari lebar)
            drawCube(shaderProg,{r.x,r.y-0.6f,0}, {0.12f,0.25f,0.12f},COL_ROCKET_NOSE); // hidung roket, ditaruh di bawah badan (karena roket menghadap turun)
            drawCube(shaderProg,{r.x,r.y+0.4f,0}, {0.34f,0.14f,0.14f},COL_ROCKET_FIN);  // sirip roket, lebih lebar & pipih, di atas badan
            drawCube(shaderProg,{r.x,r.y+0.85f,0},{fl,0.3f,fl},COL_FLAME,0.85f);        // api knalpot di ujung paling atas, ukurannya berkedip (variabel fl), sedikit transparan (alpha 0.85)
        }

        // --- GAMBAR PEMAIN (dengan animasi jalan sederhana) ---
        float leg=player.onGround?sinf(player.animTimer*3)*8.f:-22.f;   // kalau menapak, kaki berayun sinus; kalau di udara, kaki menekuk tetap (-22°)
        float arm=player.onGround?sinf(player.animTimer*3)*8.f:-30.f;   // sama untuk tangan
        drawPlayer(shaderProg,player.pos,0.58f,leg,arm);

        // ================= GAMBAR HUD (Head-Up Display) 2D =================
        {
            glDisable(GL_DEPTH_TEST);                                       // matikan depth test supaya HUD selalu tampil di atas dunia 3D
            glm::mat4 hudP=glm::ortho(0.f,(float)SCR_W,(float)SCR_H,0.f,-1.f,1.f); // proyeksi ortografis 2D (koordinat piksel layar)
            glUseProgram(fontProg);
            glUniformMatrix4fv(glGetUniformLocation(fontProg,"proj"),1,GL_FALSE,glm::value_ptr(hudP));
            float sc=2, cw=16, ch=16;                                       // skala teks, lebar/tinggi 1 karakter dalam piksel

            renderRect(0,0,(float)SCR_W,ch+14,0.03f,0.08f,0.04f,0.88f);     // bar gelap transparan di bagian atas layar (latar HUD)
            renderRect(0,ch+12,(float)SCR_W,2,0.22f,0.68f,0.3f,0.6f);       // garis hijau tipis pemisah HUD

            for(int i=0;i<5;i++) renderHeart(10+i*32,5,sc*.9f,i<playerLives); // gambar 5 ikon hati, terisi sesuai sisa nyawa

            std::string sc_str="SCORE: "+std::to_string(wallsPassed);
            renderText(sc_str,SCR_W/2.f-sc_str.size()*cw/2,5,sc,0.7f,0.98f,0.72f); // skor di tengah atas

            renderText("A/D=MOVE  ESC=PAUSE",SCR_W-10-20*cw,5,sc,0.4f,0.6f,0.42f,0.8f); // petunjuk kontrol di kanan atas

            // ★ Peringatan ROKET di HUD: muncul selama masih ada roket aktif di layar
            if(!rockets.empty()){
                std::string w2="!! ROCKET INCOMING !!";
                renderText(w2,SCR_W/2.f-w2.size()*cw*.8f/2,ch+20,.8f*sc,1,.3f,.05f); // teks merah peringatan di bawah bar skor
            }

            // Efek layar berkedip merah saat pemain baru saja kena damage (hitCooldown aktif)
            if(hitCooldown>0){
                float fl2=fmodf(hitCooldown*6,1)>.5f?.35f:0;                 // berkedip on/off dengan frekuensi berdasar sisa cooldown
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.9f,0.1f,0.1f,fl2);
            }

            // Layar PAUSE
            if(gameState==PAUSED){
                float ox=SCR_W/2.f, oy=SCR_H/2.f;                            // titik tengah layar
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.02f,0.06f,0.03f,0.72f); // overlay gelap seluruh layar
                renderRect(ox-155,oy-95,310,190,0.04f,0.11f,0.06f,0.97f);    // kotak panel pause
                renderText("PAUSED",ox-6*8*3.f/2,oy-60,3,0.32f,0.98f,0.48f);
                renderText("ESC/P - RESUME",ox-14*8*1.5f/2,oy,1.5f,0.72f,0.92f,0.74f);
                renderText("R - RESTART",ox-11*8*1.5f/2,oy+30,1.5f,0.72f,0.92f,0.74f);
                renderText("SCORE "+std::to_string(wallsPassed),ox-8*8*1.5f/2,oy+60,1.5f,0.5f,0.72f,0.52f);
            }

            // Layar GAME OVER
            if(gameState==GAME_OVER){
                float ox=SCR_W/2.f, oy=SCR_H/2.f;
                renderRect(0,0,(float)SCR_W,(float)SCR_H,0.05f,0.01f,0.01f,0.75f); // overlay merah gelap
                renderRect(ox-180,oy-115,360,230,0.1f,0.02f,0.02f,0.97f);    // kotak panel game over
                renderText("GAME  OVER",ox-10*8*3.f/2,oy-90,3,0.98f,0.18f,0.18f);
                renderText("THE CANOPY CLAIMED YOU",ox-22*8*1.6f/2,oy-45,1.6f,0.75f,0.4f,0.4f);
                renderText("SCORE: "+std::to_string(wallsPassed),ox-7*8*1.8f/2,oy-10,1.8f,.88f,.62f,.62f);
                renderText("PRESS R TO RESTART",ox-18*8*1.5f/2,oy+44,1.5f,1,.92f,.92f);
                renderText("PRESS M FOR MENU",ox-16*8*1.5f/2,oy+64,1.5f,.8f,.72f,.72f);
            }
            glEnable(GL_DEPTH_TEST);                                        // nyalakan lagi depth test untuk frame 3D berikutnya
        }

        glfwSwapBuffers(win);   // tukar buffer belakang<->depan (tampilkan frame yang baru selesai digambar)
        glfwPollEvents();       // proses semua event yang masuk (keyboard, close window, dll)
    }
    // ============================= AKHIR GAME LOOP =============================

    glDeleteVertexArrays(1,&VAO); glDeleteBuffers(1,&VBO); // bersihkan resource GPU
    glfwTerminate();                                        // matikan GLFW
    return g_backToMenu;                                    // true kalau keluar karena tombol M (balik ke menu), false kalau window ditutup paksa
}

// ----------------------------------------------------------------------------
// Bagian bawah ini mengurus jendela MENU (webview HTML), bukan game 3D-nya.
// ----------------------------------------------------------------------------
static std::atomic<bool> g_startGame{false}, g_exit{false}; // flag hasil klik tombol menu, aman diakses lintas callback

// onMenuInvoke(): dipanggil saat tombol di HTML menu diklik (lewat window.external.invoke)
void onMenuInvoke(struct webview* w,const char* arg){
    std::string cmd(arg);                                    // isi argumen sesuai nama tombol yang diklik
    if(cmd=="startGame"){ g_startGame=true; webview_terminate(w); }  // tombol Start -> set flag, tutup webview
    else if(cmd=="options") webview_eval(w,"alert('HOW TO PLAY\\n\\nSPACE/W/UP = Jump\\nA/D = Move\\n\\nLompat ke platform bergerak untuk naik.\\nHindari platform yang menabrak dari samping!\\n\\nESC/P = Pause  |  R = Restart  |  M = Menu');"); // tombol How To Play -> munculkan alert petunjuk lewat JS
    else if(cmd=="exit"){ g_exit=true; webview_terminate(w); }       // tombol Exit -> set flag keluar, tutup webview
}

// makeDataUrl(): mengubah string HTML menjadi "data: URL" (URL-encode) supaya
// bisa langsung dimuat oleh webview tanpa perlu file HTML terpisah.
static std::string makeDataUrl(const char* html){
    std::string out="data:text/html,";
    for(const char* p=html;*p;++p){                          // loop tiap karakter dalam HTML
        unsigned char c=(unsigned char)*p;
        if(isalnum(c)||strchr("-_.!~*'()/:@,;?=&#+ ",c)) out+=(char)c; // karakter "aman" langsung disalin apa adanya
        else{ char b[4]; snprintf(b,4,"%%%02X",c); out+=b; }   // karakter lain di-encode jadi %XX (percent-encoding)
    }
    return out;
}

#if defined(_WIN32)||defined(_WIN64)
// centerWebview() versi Windows: memposisikan window webview di tengah layar
static void centerWebview(struct webview* wv){
    HWND h=(HWND)wv->priv.hwnd; if(!h) return;               // ambil handle native window Windows
    int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN); // ukuran layar
    RECT r; GetWindowRect(h,&r);                              // ukuran window saat ini
    SetWindowPos(h,NULL,(sw-(r.right-r.left))/2,(sh-(r.bottom-r.top))/2,0,0,SWP_NOSIZE|SWP_NOZORDER); // pindahkan ke tengah
}
#else
static void centerWebview(struct webview*){}                 // di OS lain, tidak melakukan apa-apa (no-op)
#endif

// ----------------------------------------------------------------------------
// main(): alur program keseluruhan -> tampilkan menu -> kalau Start ditekan,
// jalankan game 3D -> kalau kembali ke menu (M), ulangi lagi -> kalau Exit,
// program berhenti.
// ----------------------------------------------------------------------------
int main(){
    std::string url=makeDataUrl(MENU_HTML);                   // siapkan URL data dari HTML menu (sekali saja, dipakai berulang)
    while(true){                                                // loop utama: menu <-> game
        g_startGame=false; g_exit=false;                        // reset flag tiap kali kembali ke menu
        struct webview menu={};                                 // siapkan struct konfigurasi webview
        menu.title="Sylvan Odyssey"; menu.url=url.c_str();
        menu.width=SCR_W; menu.height=SCR_H; menu.resizable=0; menu.external_invoke_cb=onMenuInvoke; // hubungkan callback klik tombol
        if(webview_init(&menu)!=0) return 1;                    // gagal membuat webview -> keluar dengan kode error
        centerWebview(&menu);                                    // posisikan window menu di tengah layar
        while(webview_loop(&menu,1)==0){}                        // jalankan loop event webview sampai ditutup
        webview_exit(&menu);                                     // bersihkan resource webview

        if(g_exit) return 0;                                     // tombol Exit ditekan -> keluar program sepenuhnya
        if(!g_startGame) break;                                  // window ditutup tanpa klik Start -> keluar dari loop (selesai)

#if defined(_WIN32)||defined(_WIN64)
        Sleep(120);                                               // beri jeda sedikit supaya transisi window mulus
        { HWND h=FindWindowA(NULL,"Sylvan Odyssey"); if(h) ShowWindow(h,SW_HIDE); Sleep(150); } // sembunyikan sisa window menu lama
#else
        usleep(270000);                                           // versi non-Windows: jeda dengan usleep
#endif
        if(!runGame()) break;                                     // jalankan game; kalau keluar BUKAN karena tombol M (kembali ke menu) -> selesai program
        // kalau runGame() return true (pemain tekan M), loop while(true) berlanjut -> tampilkan menu lagi
    }
    return 0;                                                     // program selesai dengan normal
}