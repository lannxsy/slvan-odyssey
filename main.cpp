// =============================================
// Sylvan Odyssey - main.cpp
// Menu (webview HTML) → Game (OpenGL/GLFW)
// Gabungan index.html + main.cpp via webview.h
// =============================================

// ---- Pilih platform webview sesuai OS Anda ----
// Windows : tambahkan -DWEBVIEW_WINAPI saat compile
// Linux   : tambahkan -DWEBVIEW_GTK
// macOS   : tambahkan -DWEBVIEW_COCOA
// ------------------------------------------------

#define WEBVIEW_IMPLEMENTATION
#include "webview.h"

#if defined(_WIN32) || defined(_WIN64)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <atomic>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// =============================================
// EMBEDDED HTML (inline - no file:// needed)
// =============================================

static const char* MENU_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Sylvan Odyssey</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
html,body{width:100%;height:100%;overflow:hidden;background:#0a1a0f;font-family:serif;}
#game-menu{width:100%;height:100vh;background:#0a1a0f;position:relative;overflow:hidden;}
.sky{position:absolute;inset:0;background:linear-gradient(to bottom,#050d08 0%,#0e2616 40%,#1a3d24 70%,#0d2010 100%);}
.stars{position:absolute;inset:0;}
.star{position:absolute;background:#fff;border-radius:50%;animation:twinkle var(--dur) ease-in-out infinite alternate;}
@keyframes twinkle{from{opacity:var(--min);}to{opacity:var(--max);}}
.vignette{position:absolute;inset:0;background:radial-gradient(circle,transparent 30%,rgba(3,8,5,0.8) 100%);pointer-events:none;}
.container{position:relative;z-index:10;width:100%;height:100%;display:flex;flex-direction:column;justify-content:center;align-items:center;padding:20px;}
.title-area{text-align:center;margin-bottom:60px;}
.main-title{font-family:serif;font-size:4rem;font-weight:900;color:#e4f5e8;text-transform:uppercase;letter-spacing:10px;text-shadow:0 0 20px rgba(93,214,104,0.4);margin-bottom:10px;}
.subtitle{font-family:serif;font-size:1.1rem;color:#86b390;letter-spacing:5px;text-transform:uppercase;}
.menu-actions{display:flex;flex-direction:column;gap:18px;min-width:280px;}
.btn{background:linear-gradient(135deg,rgba(22,54,31,0.6),rgba(13,34,19,0.8));border:1px solid #336e43;border-radius:4px;color:#cce3d2;padding:15px 30px;font-family:serif;font-size:1rem;font-weight:700;letter-spacing:3px;text-transform:uppercase;cursor:pointer;transition:all 0.3s;}
.btn:hover{color:#fff;border-color:#5dd668;box-shadow:0 0 25px rgba(93,214,104,0.3);transform:translateY(-2px);}
.btn-primary{background:linear-gradient(135deg,rgba(37,92,51,0.8),rgba(16,43,23,0.9));border-color:#4a9e5e;color:#fff;}
.footer-text{position:absolute;bottom:25px;left:50%;transform:translateX(-50%);color:#45694e;font-size:0.8rem;letter-spacing:2px;text-transform:uppercase;white-space:nowrap;}
.firefly{position:absolute;width:4px;height:4px;background:#9effa9;border-radius:50%;filter:blur(1px);box-shadow:0 0 8px #5dd668;animation:fly var(--dur) linear infinite;}
@keyframes fly{0%{transform:translate(0,0);opacity:0;}15%{opacity:0.6;}85%{opacity:0.6;}100%{transform:translate(var(--x1),var(--y1)) translate(var(--x2),var(--y2));opacity:0;}}
</style>
</head>
<body>
<div id="game-menu">
  <div class="sky"></div>
  <div class="stars" id="stars"></div>
  <div id="fireflies"></div>
  <div class="vignette"></div>
  <div class="container">
    <div class="title-area">
      <h1 class="main-title">Sylvan Odyssey</h1>
      <div class="subtitle">The Rising Canopy</div>
    </div>
    <div class="menu-actions">
      <button class="btn btn-primary" id="btn-start" onclick="this.textContent='Loading...';this.disabled=true;window.external.invoke('startGame');">Start Journey</button>
      <button class="btn" onclick="window.external.invoke('options');">HOW TO PLAY</button>
      <button class="btn" onclick="window.external.invoke('exit');">Exit</button>
    </div>
    <div class="footer-text">STMIK AMIK Bandung &copy; 2026</div>
  </div>
</div>
<script>
var sts=document.getElementById('stars');
for(var i=0;i<60;i++){var s=document.createElement('div');s.className='star';var sz=1+Math.random()*2;s.style.cssText='top:'+Math.random()*100+'%;left:'+Math.random()*100+'%;width:'+sz+'px;height:'+sz+'px;--dur:'+(2+Math.random()*3)+'s;--min:'+(0.1+Math.random()*0.3)+';--max:'+(0.6+Math.random()*0.4)+';';sts.appendChild(s);}
var ff=document.getElementById('fireflies');
for(var j=0;j<15;j++){var f=document.createElement('div');f.className='firefly';f.style.cssText='top:'+(20+Math.random()*60)+'%;left:'+(10+Math.random()*80)+'%;--dur:'+(6+Math.random()*6)+'s;--x1:'+(Math.random()*100-50)+'px;--y1:'+(Math.random()*70-35)+'px;--x2:'+(Math.random()*100-50)+'px;--y2:'+(Math.random()*70-35)+'px;animation-delay:'+Math.random()*8+'s;';ff.appendChild(f);}
</script>
</body>
</html>)HTML";


// =============================================
// SETTINGS
// =============================================
const unsigned int SCR_WIDTH  = 900;
const unsigned int SCR_HEIGHT = 600;

// =============================================
// CAMERA (side view)
// =============================================
glm::vec3 cameraPos   = glm::vec3(0.0f, 2.5f, 13.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.15f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f,  1.0f,  0.0f);

// =============================================
// LIGHTING
// =============================================
glm::vec3 lightPos(5.0f, 10.0f, 6.0f);
glm::vec3 lightColor(1.0f, 0.95f, 0.85f);

// =============================================
// GAME STATE
// =============================================
enum GameState { PLAYING, PAUSED, GAME_OVER, WIN };
GameState gameState = PLAYING;

float deltaTime    = 0.0f;
float lastFrame    = 0.0f;
float gameTime     = 0.0f;
int   score        = 0;
int   wallsPassed  = 0;
int   playerLives  = 5;    // nyawa pemain
float overlayTimer = 0.0f; // animasi overlay game over/win
float hitCooldown  = 0.0f; // invincibility frames setelah kena hit

// =============================================
// PLAYER
// =============================================
const float GROUND_Y   = 0.0f;
const float GRAVITY    = -28.0f;
const float JUMP_FORCE =  10.5f;

const float PLAYER_X   = 0.0f;
const float PLAYER_HW  = 0.36f;
const float PLAYER_H   = 1.90f;
const float PLAYER_MOVE_SPEED = 6.0f;
const float PLAYER_BOUND      = 6.8f; // batas gerak kiri-kanan dari tengah (lantai lebar 15 -> setengah 7.5, sisakan margin)

struct Player {
    glm::vec3 pos       = glm::vec3(PLAYER_X, GROUND_Y, 0.0f);
    float     velY      = 0.0f;
    bool      onGround  = true;
    float     animTimer = 0.0f;
    float     standingY = GROUND_Y;
} player;

// =============================================
// WALLS / PLATFORMS
// =============================================
struct Wall {
    float x;
    float y;
    float width;
    float height;
    float speedSign;
    bool  passed;
    bool  alive;
    bool  playerOn;
    bool  isFrozen;
    int   id;
    bool  waitingDelay;
    float delayTimer;
};
std::vector<Wall> walls;

float wallSpeed     = 5.5f;
bool spawnFromRight = false;
float cameraTargetY = 2.5f;

// =============================================
// OBSTACLE SYSTEM — Roket jatuh dari atas tiap 10 tangga
// Milestone 1 (tangga 10) = max 1 roket, milestone 2 = max 2, dst
// =============================================
struct Rocket {
    float x, y;    // posisi
    float velY;    // kecepatan jatuh (negatif = turun)
    float trail;   // timer animasi api ekor
    bool  alive;
};
std::vector<Rocket> rockets;
float rocketTimer           = 0.0f;
int   lastObstacleMilestone = 0;

// =============================================
// SHADERS
// =============================================
const char* vertSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
out vec3 FragPos;
out vec3 Normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main(){
    FragPos     = vec3(model * vec4(aPos,1.0));
    Normal      = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos,1.0);
}
)";

const char* fragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
uniform vec3  lightPos;
uniform vec3  lightColor;
uniform vec3  objectColor;
uniform float alpha;
void main(){
    float ambient = 0.38;
    vec3  norm     = normalize(Normal);
    vec3  lightDir = normalize(lightPos - FragPos);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  result   = (ambient + diff*0.62) * lightColor * objectColor;
    FragColor      = vec4(result, alpha);
}
)";

const char* shadowFragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
uniform float alpha;
uniform vec3  shadowCenter;
uniform vec2  shadowRadius;
void main(){
    float dx   = (FragPos.x - shadowCenter.x) / shadowRadius.x;
    float dz   = (FragPos.z - shadowCenter.z) / shadowRadius.y;
    float dist = dx*dx + dz*dz;
    if(dist > 1.0) discard;
    FragColor = vec4(0.0, 0.0, 0.0, alpha * (1.0 - dist));
}
)";


// =============================================
// BITMAP FONT SYSTEM (8x8 pixel font, ASCII 32-127)
// =============================================
// Font data: setiap karakter = 8 byte, setiap byte = 1 baris piksel (MSB=kiri)
static const unsigned char FONT8[96][8] = {
// SP(blank) !       "       #       $       %       &       '
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
{0x6C,0x6C,0x28,0x00,0x00,0x00,0x00,0x00},{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
{0x18,0x7C,0xD0,0x7C,0x16,0xFC,0x18,0x00},{0xC2,0xC6,0x0C,0x18,0x30,0x66,0xC6,0x00},
{0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
// (   )   *   +   ,   -   .   /
{0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
// 0-7
{0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x7C,0xC6,0x06,0x1C,0x70,0xC6,0xFE,0x00},{0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},
{0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00},{0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},
{0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},{0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00},
// 8-9 : ; < = > ?
{0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},{0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},
{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
{0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
{0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},{0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
// @ A-G
{0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00},{0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0x00},
{0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},{0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},
{0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},{0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},
{0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00},{0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3A,0x00},
// H-O
{0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},{0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
{0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00},{0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00},
{0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},{0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},{0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00},
// P-W
{0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00},{0x38,0x6C,0xC6,0xC6,0xCE,0x6C,0x3A,0x00},
{0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00},{0x78,0xCC,0xC0,0x70,0x18,0xCC,0x78,0x00},
{0xFC,0xB4,0x30,0x30,0x30,0x30,0x78,0x00},{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00},
{0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
// X-Z [ \ ] ^ _
{0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00},{0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00},
{0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
{0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
{0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00},
// ` a-g
{0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00},
{0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00},{0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00},
{0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00},{0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00},
{0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8},
// h-o
{0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00},{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
{0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C},{0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00},
{0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x00,0x00,0xCC,0xFE,0xD6,0xC6,0xC6,0x00},
{0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00},{0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00},
// p-w
{0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0},{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E},
{0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00},{0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00},
{0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00},
{0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00},{0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},
// x-z { | } ~ DEL
{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},{0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8},
{0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00},{0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00},
{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},{0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00},
{0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00},
};


unsigned int fontTex=0, fontVAO=0, fontVBO2=0, fontProg=0, whiteTex=0;

const char* fontVertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 vUV;
uniform mat4 proj;
void main(){ vUV=aUV; gl_Position=proj*vec4(aPos,0.0,1.0); }
)";
const char* fontFragSrc = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D tex;
uniform vec4 color;
void main(){ float a=texture(tex,vUV).r; FragColor=vec4(color.rgb,color.a*a); }
)";

void initFont(){
    // Font atlas 128x48
    static unsigned char atlas[48][128];
    memset(atlas,0,sizeof(atlas));
    for(int c=0;c<96;c++){
        int col=c%16,row=c/16;
        for(int y=0;y<8;y++){
            unsigned char bits=FONT8[c][y];
            for(int x=0;x<8;x++)
                if(bits&(0x80>>x)) atlas[row*8+y][col*8+x]=255;
        }
    }
    glGenTextures(1,&fontTex);
    glBindTexture(GL_TEXTURE_2D,fontTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,128,48,0,GL_RED,GL_UNSIGNED_BYTE,atlas);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    // White 1x1
    unsigned char w=255;
    glGenTextures(1,&whiteTex);
    glBindTexture(GL_TEXTURE_2D,whiteTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&w);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    // Quad VAO
    glGenVertexArrays(1,&fontVAO);
    glGenBuffers(1,&fontVBO2);
    glBindVertexArray(fontVAO);
    glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*6*4,NULL,GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

static void quadDraw(float x,float y,float w,float h,float u0,float v0,float u1,float v1){
    float v[6][4]={{x,y+h,u0,v1},{x,y,u0,v0},{x+w,y,u1,v0},
                   {x,y+h,u0,v1},{x+w,y,u1,v0},{x+w,y+h,u1,v1}};
    glBindVertexArray(fontVAO);
    glBindBuffer(GL_ARRAY_BUFFER,fontVBO2);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);
    glDrawArrays(GL_TRIANGLES,0,6);
}

void renderRect(float x,float y,float w,float h,float r,float g,float b,float a){
    glUseProgram(fontProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,whiteTex);
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);
    quadDraw(x,y,w,h,0,0,1,1);
}

void renderText(const std::string& s,float x,float y,float sc,
                float r=1,float g=1,float b=1,float a=1){
    glUseProgram(fontProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,fontTex);
    glUniform1i(glGetUniformLocation(fontProg,"tex"),0);
    glUniform4f(glGetUniformLocation(fontProg,"color"),r,g,b,a);
    float cx=x;
    for(char c:s){
        if(c<32||c>127)c=32;
        int idx=c-32,col=idx%16,row=idx/16;
        float u0=col*8/128.0f,u1=(col+1)*8/128.0f;
        float v0=row*8/48.0f, v1=(row+1)*8/48.0f;
        quadDraw(cx,y,8*sc,8*sc,u0,v0,u1,v1);
        cx+=8*sc;
    }
}

void renderHeart(float x,float y,float s,bool filled){
    float r=filled?0.95f:0.28f,g=filled?0.15f:0.22f,b=filled?0.18f:0.26f;
    renderRect(x+s,  y,    s,  s,  r,g,b,1);
    renderRect(x+4*s,y,    s,  s,  r,g,b,1);
    renderRect(x,    y+s,  6*s,s,  r,g,b,1);
    renderRect(x,    y+2*s,6*s,s,  r,g,b,1);
    renderRect(x+s,  y+3*s,4*s,s,  r,g,b,1);
    renderRect(x+2*s,y+4*s,2*s,s,  r,g,b,1);
    renderRect(x+3*s,y+5*s,s,  s,  r,g,b,1);
}

// =============================================
// CUBE GEOMETRY
// =============================================
float cubeVerts[] = {
    -1,-1,-1, 0,0,-1,  1,-1,-1, 0,0,-1,  1,1,-1, 0,0,-1,
     1, 1,-1, 0,0,-1, -1, 1,-1, 0,0,-1, -1,-1,-1, 0,0,-1,
    -1,-1, 1, 0,0,1,   1,-1,1, 0,0,1,   1,1,1, 0,0,1,
     1, 1, 1, 0,0,1,  -1, 1,1, 0,0,1,  -1,-1,1, 0,0,1,
    -1, 1, 1,-1,0,0,  -1, 1,-1,-1,0,0, -1,-1,-1,-1,0,0,
    -1,-1,-1,-1,0,0,  -1,-1, 1,-1,0,0, -1, 1, 1,-1,0,0,
     1, 1, 1,1,0,0,   1, 1,-1,1,0,0,   1,-1,-1,1,0,0,
     1,-1,-1,1,0,0,   1,-1, 1,1,0,0,   1, 1, 1,1,0,0,
    -1,-1,-1,0,-1,0,  1,-1,-1,0,-1,0,  1,-1,1,0,-1,0,
     1,-1, 1,0,-1,0, -1,-1, 1,0,-1,0, -1,-1,-1,0,-1,0,
    -1, 1,-1,0,1,0,   1, 1,-1,0,1,0,   1, 1,1,0,1,0,
     1, 1, 1,0,1,0,  -1, 1, 1,0,1,0,  -1, 1,-1,0,1,0,
};

unsigned int VAO, VBO;
unsigned int shaderProg, shadowProg;

// =============================================
// HELPERS
// =============================================
unsigned int makeShader(const char* vs, const char* fs){
    auto compile = [](unsigned int type, const char* src){
        unsigned int s = glCreateShader(type);
        glShaderSource(s,1,&src,NULL); glCompileShader(s);
        int ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
        if(!ok){ char log[512]; glGetShaderInfoLog(s,512,NULL,log); std::cerr<<log; }
        return s;
    };
    unsigned int v=compile(GL_VERTEX_SHADER,vs), f=compile(GL_FRAGMENT_SHADER,fs);
    unsigned int p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void setUniforms(unsigned int prog, glm::mat4& proj, glm::mat4& view){
    glUseProgram(prog);
    glBindVertexArray(VAO);
    glUniformMatrix4fv(glGetUniformLocation(prog,"projection"),1,GL_FALSE,glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(prog,"view"),1,GL_FALSE,glm::value_ptr(view));
}

void drawCube(unsigned int prog, glm::vec3 pos, glm::vec3 scale, glm::vec3 color, float alpha=1.0f){
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), scale);
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(prog,"alpha"),alpha);
    glDrawArrays(GL_TRIANGLES,0,36);
}

// =============================================
// HUMANOID RENDERING
// Teknik: setiap bagian tubuh = drawCube dengan
// transform berbeda. Shadow = tiap bagian
// diproyeksikan ke lantai (Y dikecilkan jadi tipis).
// =============================================

// Helper: gambar satu bagian tubuh dengan rotasi pivot (untuk anggota gerak)
void drawLimb(unsigned int prog, glm::vec3 pivot, float angle, glm::vec3 axis,
              glm::vec3 offset, glm::vec3 scale, glm::vec3 color, float alpha=1.0f){
    glm::mat4 m = glm::translate(glm::mat4(1.0f), pivot);
    m = glm::rotate(m, glm::radians(angle), axis);
    m = glm::translate(m, offset);
    m = glm::scale(m, scale);
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
    glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(prog,"alpha"),alpha);
    glDrawArrays(GL_TRIANGLES,0,36);
}

// Gambar karakter humanoid — bisa mode normal ATAU shadow (hitam pipih di lantai)
// Parameter isShadow=true  → render tiap bagian tubuh sebagai siluet hitam di floorY
// Parameter isShadow=false → render karakter berwarna normal
void drawHumanoid(unsigned int prog, glm::vec3 base, float s,
                  glm::vec3 bodyC, glm::vec3 headC, glm::vec3 limbC,
                  float legSwing, float armSwing, bool isShadow=false, float floorY=0.0f)
{
    glm::vec3 black(0,0,0);
    float shadowAlpha = 0.85f; // gelap & jelas

    if(isShadow){
        // Shadow = setiap bagian tubuh diproyeksikan ke lantai (floorY)
        // Caranya: posisi Y diganti floorY+0.01, skala Y dikecilkan jadi sangat tipis
        // sehingga membentuk "cetakan" hitam sesuai bentuk karakter

        auto shadowCube = [&](glm::vec3 pos3D, glm::vec3 sc3D){
            // Proyeksi: X dan Z tetap, Y diratakan ke lantai
            glm::vec3 spos(pos3D.x, floorY + 0.01f, pos3D.z);
            // Skala X & Z diperbesar sedikit (efek perspektif cahaya dari atas)
            float spread = 1.0f + (pos3D.y - floorY) * 0.08f;
            glm::vec3 ssc(sc3D.x * spread, 0.018f, sc3D.z * spread);
            glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), spos), ssc);
            glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
            glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(black));
            glUniform1f(glGetUniformLocation(prog,"alpha"), shadowAlpha);
            glDrawArrays(GL_TRIANGLES,0,36);
        };

        // Shadow tiap bagian tubuh (posisi sama dengan karakter normal)
        shadowCube(base+glm::vec3(-s*0.20f, s*0.15f, 0), glm::vec3(s*0.16f,s*0.30f,s*0.16f)); // kaki kiri
        shadowCube(base+glm::vec3( s*0.20f, s*0.15f, 0), glm::vec3(s*0.16f,s*0.30f,s*0.16f)); // kaki kanan
        shadowCube(base+glm::vec3(0,s*0.85f,0),          glm::vec3(s*0.36f,s*0.38f,s*0.20f)); // badan
        shadowCube(base+glm::vec3(-s*0.52f, s*0.70f, 0), glm::vec3(s*0.14f,s*0.26f,s*0.14f)); // lengan kiri
        shadowCube(base+glm::vec3( s*0.52f, s*0.70f, 0), glm::vec3(s*0.14f,s*0.26f,s*0.14f)); // lengan kanan
        shadowCube(base+glm::vec3(0,s*1.52f,0),          glm::vec3(s*0.33f,s*0.30f,s*0.26f)); // kepala
        return;
    }

    // ── Karakter Normal (berwarna) ──
    glm::vec3 rotAxis(1,0,0);

    // Kaki kiri & kanan (berayun saat berjalan)
    drawLimb(prog,
        base+glm::vec3(-s*0.20f, s*0.30f, 0),  // pivot di pangkal paha
        legSwing, rotAxis,
        glm::vec3(0,-s*0.30f,0),                // offset ke bawah dari pivot
        glm::vec3(s*0.16f,s*0.30f,s*0.16f), limbC);
    drawLimb(prog,
        base+glm::vec3(s*0.20f, s*0.30f, 0),
        -legSwing, rotAxis,
        glm::vec3(0,-s*0.30f,0),
        glm::vec3(s*0.16f,s*0.30f,s*0.16f), limbC);

    // Badan
    drawCube(prog, base+glm::vec3(0,s*0.85f,0), glm::vec3(s*0.36f,s*0.38f,s*0.20f), bodyC);

    // Lengan kiri & kanan (berayun berlawanan dengan kaki)
    drawLimb(prog,
        base+glm::vec3(-s*0.52f, s*0.95f, 0),
        -armSwing, rotAxis,
        glm::vec3(0,-s*0.24f,0),
        glm::vec3(s*0.14f,s*0.26f,s*0.14f), limbC);
    drawLimb(prog,
        base+glm::vec3(s*0.52f, s*0.95f, 0),
        armSwing, rotAxis,
        glm::vec3(0,-s*0.24f,0),
        glm::vec3(s*0.14f,s*0.26f,s*0.14f), limbC);

    // Kepala
    drawCube(prog, base+glm::vec3(0,s*1.52f,0), glm::vec3(s*0.33f,s*0.30f,s*0.26f), headC);
    // Mata putih & pupil
    drawCube(prog, base+glm::vec3(-s*0.13f,s*1.59f,s*0.30f), glm::vec3(s*0.07f,s*0.07f,s*0.04f), glm::vec3(0.95f,0.95f,0.95f));
    drawCube(prog, base+glm::vec3( s*0.13f,s*1.59f,s*0.30f), glm::vec3(s*0.07f,s*0.07f,s*0.04f), glm::vec3(0.95f,0.95f,0.95f));
    drawCube(prog, base+glm::vec3(-s*0.13f,s*1.59f,s*0.35f), glm::vec3(s*0.04f,s*0.04f,s*0.03f), glm::vec3(0.05f,0.05f,0.05f));
    drawCube(prog, base+glm::vec3( s*0.13f,s*1.59f,s*0.35f), glm::vec3(s*0.04f,s*0.04f,s*0.03f), glm::vec3(0.05f,0.05f,0.05f));
}

// =============================================
// SPAWN WALL
// =============================================
void spawnWall(bool forceSameDirection = false, float customDelay = 0.0f){
    Wall w;
    w.id       = wallsPassed + 1;
    w.height   = 0.16f;
    w.passed   = false;
    w.alive    = true;
    w.playerOn = false;
    w.isFrozen = false;

    w.waitingDelay = (customDelay > 0.0f);
    w.delayTimer   = customDelay;

    // Tiap 10 tangga, lebar platform mengecil (minimum 1.5)
    int   milestones = wallsPassed / 10;
    w.width = std::max(6.0f - milestones * 0.55f, 1.5f);

    bool rightSide = spawnFromRight;
    if(forceSameDirection){
        rightSide = !spawnFromRight;
    }

    if(rightSide){
        w.speedSign = -1.0f;
    } else {
        w.speedSign = +1.0f;
    }

    if(w.waitingDelay) {
        w.x = 999.0f;
    } else {
        w.x = (rightSide) ? 16.0f : -16.0f;
    }

    if(!forceSameDirection){
        spawnFromRight = !spawnFromRight;
    }

    float stepUp = 0.75f;
    w.y = player.standingY + stepUp;

    walls.push_back(w);
}

// =============================================
// RESET
// =============================================
void resetGame(){
    player.pos        = glm::vec3(PLAYER_X, GROUND_Y, 0.0f);
    player.velY       = 0.0f;
    player.onGround   = true;
    player.animTimer  = 0.0f;
    player.standingY  = GROUND_Y;
    walls.clear();
    score             = 0;
    wallsPassed       = 0;
    wallSpeed         = 5.5f;
    gameTime          = 0.0f;
    gameState         = PLAYING;
    playerLives       = 5;
    overlayTimer      = 0.0f;
    hitCooldown       = 0.0f;
    spawnFromRight    = false;
    cameraPos.y       = 2.5f;
    cameraTargetY     = 2.5f;

    rockets.clear();
    rocketTimer            = 0.0f;
    lastObstacleMilestone  = 0;

    spawnWall(false, 1.2f);
}

// =============================================
// CALLBACKS (GLFW)
// =============================================
static bool g_backToMenuFromGame = false;

static bool keysHeld[512] = {};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key>=0 && key<512){
        if(action==GLFW_PRESS)   keysHeld[key]=true;
        if(action==GLFW_RELEASE) keysHeld[key]=false;
    }
    if(action == GLFW_PRESS){
        if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W) && player.onGround && gameState==PLAYING){
            player.velY     = JUMP_FORCE;
            player.onGround = false;
        }
        // Pause toggle
        if(key==GLFW_KEY_ESCAPE || key==GLFW_KEY_P){
            if(gameState==PLAYING) gameState=PAUSED;
            else if(gameState==PAUSED) gameState=PLAYING;
        }
        // Restart kapan saja dengan R
        if(key==GLFW_KEY_R) resetGame();
        // Kembali ke menu (M) kapan saja
        if(key==GLFW_KEY_M){
            g_backToMenuFromGame = true;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
}

void framebuffer_size_callback(GLFWwindow* w, int width, int height){
    glViewport(0,0,width,height);
}

// =============================================
// RUN GAME (dipanggil setelah menu ditutup)
// return true = kembali ke menu, false = exit
// =============================================
bool runGame(){
    g_backToMenuFromGame = false;
    srand((unsigned)time(0));
    // FIX: reset lastFrame agar deltaTime tidak spike besar di frame pertama
    lastFrame = 0.0f;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sylvan Odyssey", NULL, NULL);
#if defined(_WIN32)||defined(_WIN64)
    { int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
      glfwSetWindowPos(window,(sw-(int)SCR_WIDTH)/2,(sh-(int)SCR_HEIGHT)/2); }
#endif
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaderProg = makeShader(vertSrc, fragSrc);
    shadowProg = makeShader(vertSrc, shadowFragSrc);
    fontProg   = makeShader(fontVertSrc, fontFragSrc);
    initFont();

    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    struct Cloud{ float x, y, z, speed; };
    std::vector<Cloud> clouds = {
        {-3.0f, 5.8f, -5.0f, 0.40f}, {2.5f, 6.5f, -6.0f, 0.25f},
        {7.0f, 5.2f, -5.5f, 0.35f},  {-8.0f, 6.9f, -6.5f, 0.20f},
        {11.0f, 6.0f, -5.0f, 0.30f}
    };

    resetGame();

    while(!glfwWindowShouldClose(window)){
        float currentFrame = (float)glfwGetTime();
        deltaTime = std::min(currentFrame-lastFrame, 0.05f);
        lastFrame = currentFrame;

        overlayTimer += deltaTime;
        if(hitCooldown > 0.0f) hitCooldown -= deltaTime;

        if(gameState==PLAYING || gameState==PAUSED){
         if(gameState==PLAYING){
            gameTime         += deltaTime;
            player.animTimer += deltaTime;

            player.velY  += GRAVITY * deltaTime;
            player.pos.y += player.velY * deltaTime;
            if(keysHeld[GLFW_KEY_A] || keysHeld[GLFW_KEY_LEFT])  player.pos.x -= PLAYER_MOVE_SPEED * deltaTime;
            if(keysHeld[GLFW_KEY_D] || keysHeld[GLFW_KEY_RIGHT]) player.pos.x += PLAYER_MOVE_SPEED * deltaTime;
            if(player.pos.x < -PLAYER_BOUND) player.pos.x = -PLAYER_BOUND;
            if(player.pos.x >  PLAYER_BOUND) player.pos.x =  PLAYER_BOUND;

            bool localOnGround = false;
            float currentFloor = GROUND_Y;

            if(player.pos.y <= GROUND_Y){
                player.pos.y   = GROUND_Y;
                player.velY    = 0.0f;
                localOnGround  = true;
                currentFloor   = GROUND_Y;
            }

            const float LAND_TOL = 0.35f;
            const float SIDE_TOL = 0.15f;

            bool triggersNextSpawn = false;
            std::vector<Wall> pendingReplacements; // hindari push_back ke walls saat masih di-iterate

            for(auto& w : walls){
                if(!w.alive || w.waitingDelay) continue;

                bool xOverlap = fabs(player.pos.x - w.x) < PLAYER_HW + w.width;
                if(!xOverlap) continue;

                float playerFoot  = player.pos.y;
                float playerHead  = player.pos.y + PLAYER_H;
                float platformTop = w.y + w.height;
                float platformBot = w.y - w.height;

                if(player.velY <= 0.0f && playerFoot >= platformTop - LAND_TOL && playerFoot <= platformTop + 0.15f) {
                    if(!w.isFrozen && !w.passed){
                        w.isFrozen        = true;
                        w.passed          = true;
                        wallsPassed++;
                        triggersNextSpawn = true;
                    }

                    w.isFrozen       = true;
                    player.pos.y     = platformTop;
                    player.velY      = 0.0f;
                    localOnGround    = true;
                    currentFloor     = platformTop;
                    w.playerOn       = true;
                }
                else if(!w.isFrozen && !w.passed && hitCooldown <= 0.0f &&
                        playerFoot < platformTop - SIDE_TOL && playerHead > platformBot + SIDE_TOL) {
                    playerLives--;
                    // Matikan wall yang nabrak & spawn penggantinya langsung
                    w.isFrozen = true;
                    w.alive    = false;
                    hitCooldown = 1.8f;
                    if(playerLives <= 0){
                        gameState    = GAME_OVER;
                        overlayTimer = 0.0f;
                    } else {
                        // Spawn wall pengganti dari arah yang sama (delay singkat)
                        {
                            Wall rep;
                            rep.id           = w.id;
                            rep.height       = w.height;
                            rep.width        = w.width;
                            rep.passed       = false;
                            rep.alive        = true;
                            rep.playerOn     = false;
                            rep.isFrozen     = false;
                            rep.speedSign    = w.speedSign;
                            rep.y            = w.y;
                            rep.waitingDelay = true;
                            rep.delayTimer   = 1.0f;
                            rep.x            = 999.0f;
                            pendingReplacements.push_back(rep);
                        }
                        // Respawn player ke tangga terakhir yang dipijak (bukan ke lantai dasar)
                        player.pos.x     = PLAYER_X;
                        player.pos.y     = player.standingY;
                        player.velY      = 0.0f;
                        player.onGround  = true;
                        // standingY tetap (sudah benar — tidak direset ke GROUND_Y)
                        // Freeze wall yang sedang aktif berjalan di lantai saat ini
                        // (KECUALI wall yang masih waitingDelay — termasuk wall pengganti
                        // yang baru dibuat — supaya dia bisa benar-benar muncul & bergerak
                        // normal nanti, bukan lahir dalam keadaan frozen permanen).
                        for(auto& fw : walls) if(fw.alive && !fw.isFrozen && !fw.waitingDelay) fw.isFrozen = true;
                        cameraTargetY = player.standingY + 2.5f;
                    }
                }
            }
            for(auto& rep : pendingReplacements) walls.push_back(rep);

            player.onGround = localOnGround;
            if(player.onGround) {
                player.standingY = currentFloor;
                player.pos.y     = player.standingY;
                player.velY      = 0.0f;
            }

            // Endless mode: tidak ada kondisi menang, terus lanjut sampai game over

            if(triggersNextSpawn && gameState == PLAYING){
                spawnWall(false, 0.15f);
            }

            // Makin lama makin cepat, tanpa batas atas yang ketat (dicap halus agar tetap bisa dimainkan)
            wallSpeed = std::min(5.5f + (float)wallsPassed * 0.45f, 22.0f);

            // =============================================
            // OBSTACLE SYSTEM — Roket jatuh dari atas
            // Tiap 10 tangga: spawn wave baru, random 1 atau 2 roket
            // =============================================
            {
                int currentMilestone = wallsPassed / 10;
                // Deteksi milestone baru (setiap kelipatan 10 tangga)
                if(currentMilestone > 0 && currentMilestone > lastObstacleMilestone){
                    lastObstacleMilestone = currentMilestone;
                    // Langsung spawn wave roket saat milestone tercapai
                    std::mt19937 rng(std::random_device{}());
                    // Random 1 atau 2 roket per wave
                    int count = 1 + (rng() % 2);
                    std::uniform_real_distribution<float> xDist(-5.5f, 5.5f);
                    for(int k = 0; k < count; k++){
                        Rocket r;
                        r.x     = xDist(rng);
                        r.y     = player.pos.y + 8.0f; // spawn dari atas layar
                        r.velY  = -(7.0f + lastObstacleMilestone * 0.3f); // makin cepat tiap milestone
                        r.trail = 0.0f;
                        r.alive = true;
                        rockets.push_back(r);
                    }
                }

                // Update & collision roket
                for(auto& r : rockets){
                    if(!r.alive) continue;
                    r.y     += r.velY * deltaTime;
                    r.trail += deltaTime;
                    // Matikan kalau sudah lewat lantai
                    if(r.y < player.standingY - 2.0f){ r.alive = false; continue; }
                    // Collision dengan player
                    if(hitCooldown <= 0.0f &&
                       fabs(r.x - player.pos.x) < PLAYER_HW + 0.25f &&
                       r.y >= player.pos.y - 0.1f &&
                       r.y <= player.pos.y + PLAYER_H + 0.2f){
                        r.alive = false;
                        playerLives--;
                        hitCooldown = 1.8f;
                        if(playerLives <= 0){ gameState = GAME_OVER; overlayTimer = 0.0f; }
                    }
                }
                rockets.erase(std::remove_if(rockets.begin(), rockets.end(),
                    [](const Rocket& r){ return !r.alive; }), rockets.end());
            }
            // =============================================

            for(size_t i = 0; i < walls.size(); ++i){
                if(!walls[i].alive) continue;

                if(walls[i].waitingDelay){
                    walls[i].delayTimer -= deltaTime;
                    if(walls[i].delayTimer <= 0.0f){
                        walls[i].waitingDelay = false;
                        bool rightSide = (walls[i].speedSign < 0);
                        walls[i].x = (rightSide) ? 16.0f : -16.0f;
                    }
                    continue;
                }

                if(!walls[i].isFrozen) {
                    walls[i].x += walls[i].speedSign * wallSpeed * deltaTime;
                }

                if((walls[i].speedSign < 0 && walls[i].x < -16.0f) || (walls[i].speedSign > 0 && walls[i].x > 16.0f)){
                    if(!walls[i].isFrozen && walls[i].alive){
                        walls[i].alive = false;
                        spawnWall(true, 0.5f);
                    }
                }
            }

            // Kamera selalu ikuti player agar player tetap di tengah layar
            cameraTargetY = player.pos.y + 2.5f;
            cameraPos.y  += (cameraTargetY - cameraPos.y) * 8.0f * deltaTime;
         } // end if PLAYING (physics)
        } // end if PLAYING || PAUSED

        for(auto& c : clouds){
            c.x -= c.speed * deltaTime;
            if(c.x < -13.0f) c.x = 14.0f;
        }

        std::string title;
        if(gameState == PLAYING)
            title = "Sylvan Odyssey  |  Score: " + std::to_string(wallsPassed) + "  |  Lives: " + std::to_string(playerLives) + "  |  ESC = Pause";
        else if(gameState == PAUSED)
            title = "PAUSED  |  Press ESC or P to Resume  |  R to Restart";
        else
            title = "GAME OVER!  |  Score: " + std::to_string(wallsPassed) + "  |  Press R to Restart";
        glfwSetWindowTitle(window, title.c_str());

        // Sky color ikut kamera (satu warna solid, bukan dua)
        float camOffsetY = cameraPos.y - 2.5f;
        glClearColor(0.38f, 0.62f, 0.88f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)SCR_WIDTH/SCR_HEIGHT;
        glm::mat4 proj = glm::perspective(glm::radians(62.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        setUniforms(shaderProg, proj, view);
        glUniform3fv(glGetUniformLocation(shaderProg,"lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProg,"lightColor"), 1, glm::value_ptr(lightColor));

        // Background langit, matahari, awan — semua ikut camOffsetY
        drawCube(shaderProg, glm::vec3(0.0f, 3.5f + camOffsetY, -9.0f), glm::vec3(22.0f, 20.0f, 0.3f), glm::vec3(0.55f, 0.78f, 0.98f));
        drawCube(shaderProg, glm::vec3(6.5f, 6.8f + camOffsetY, -7.0f), glm::vec3(0.60f, 0.60f, 0.2f), glm::vec3(1.0f, 0.95f, 0.35f));
        for(auto& c : clouds)
            drawCube(shaderProg, glm::vec3(c.x, c.y + camOffsetY, c.z), glm::vec3(1.3f, 0.45f, 0.4f), glm::vec3(1.0f, 1.0f, 1.0f));

        drawCube(shaderProg, glm::vec3(0, -0.3f, 0), glm::vec3(15.0f, 0.3f, 2.5f), glm::vec3(0.32f, 0.58f, 0.24f));
        drawCube(shaderProg, glm::vec3(0, 0.02f, 0), glm::vec3(15.0f, 0.05f, 2.5f), glm::vec3(0.42f, 0.72f, 0.32f));

        // Render platform dulu
        setUniforms(shaderProg, proj, view);
        for(auto& w : walls){
            if(!w.alive || w.waitingDelay) continue;
            float factor = (float)((w.id - 1) % 10 + 1) / 10.0f;
            glm::vec3 wc = glm::mix(glm::vec3(0.92f,0.25f,0.20f), glm::vec3(0.15f,0.85f,0.40f), factor);
            if(w.isFrozen) wc *= 0.82f;
            drawCube(shaderProg, glm::vec3(w.x,w.y,0.0f), glm::vec3(w.width,w.height,0.55f), wc);
            drawCube(shaderProg, glm::vec3(w.x,w.y+w.height,0.0f), glm::vec3(w.width+0.02f,0.02f,0.58f), glm::clamp(wc*1.25f,glm::vec3(0),glm::vec3(1)));
        }

        // Shadow SETELAH platform — depthMask=false agar tidak ketutup platform
        {
            setUniforms(shadowProg, proj, view);
            float shadowFloor = GROUND_Y;
            for(auto& w : walls)
                if(w.alive && !w.waitingDelay && w.y+w.height <= player.pos.y+0.02f && fabs(player.pos.x-w.x) < PLAYER_HW+w.width)
                    shadowFloor = std::max(shadowFloor, w.y+w.height);
            float h     = std::max(player.pos.y - shadowFloor, 0.0f);
            float rX    = std::max(0.55f - h*0.045f, 0.18f);
            float rZ    = std::max(0.32f - h*0.025f, 0.10f);
            float alpha = std::max(0.85f - h*0.07f,  0.15f);
            glUniform3f(glGetUniformLocation(shadowProg,"shadowCenter"), player.pos.x, shadowFloor+0.025f, player.pos.z);
            glUniform2f(glGetUniformLocation(shadowProg,"shadowRadius"), rX, rZ);
            glUniform1f(glGetUniformLocation(shadowProg,"alpha"), alpha);
            glDepthMask(GL_FALSE);
            glm::mat4 sm = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(player.pos.x,shadowFloor+0.025f,player.pos.z)), glm::vec3(rX*2.2f,0.005f,rZ*2.2f));
            glUniformMatrix4fv(glGetUniformLocation(shadowProg,"model"),1,GL_FALSE,glm::value_ptr(sm));
            glDrawArrays(GL_TRIANGLES,0,36);
            glDepthMask(GL_TRUE);
            setUniforms(shaderProg, proj, view);
        }

        // ── Render Roket (jatuh dari atas) ──
        for(auto& r : rockets){
            if(!r.alive) continue;
            float t = (float)glfwGetTime();

            // Badan utama roket (silinder panjang, jatuh vertikal)
            drawCube(shaderProg, glm::vec3(r.x, r.y, 0.0f),
                     glm::vec3(0.18f, 0.70f, 0.18f), glm::vec3(0.85f, 0.85f, 0.90f));

            // Hidung merah di BAWAH (ujung yang mengarah ke bawah)
            drawCube(shaderProg, glm::vec3(r.x, r.y - 0.55f, 0.0f),
                     glm::vec3(0.14f, 0.28f, 0.14f), glm::vec3(0.90f, 0.12f, 0.08f));
            // Ujung runcing hidung
            drawCube(shaderProg, glm::vec3(r.x, r.y - 0.75f, 0.0f),
                     glm::vec3(0.08f, 0.14f, 0.08f), glm::vec3(0.75f, 0.08f, 0.05f));

            // Jendela/detail tengah badan (garis gelap)
            drawCube(shaderProg, glm::vec3(r.x, r.y + 0.05f, 0.0f),
                     glm::vec3(0.20f, 0.06f, 0.20f), glm::vec3(0.30f, 0.40f, 0.55f));

            // Sirip kiri (di bagian atas badan)
            drawCube(shaderProg, glm::vec3(r.x - 0.28f, r.y + 0.38f, 0.0f),
                     glm::vec3(0.18f, 0.20f, 0.12f), glm::vec3(0.55f, 0.55f, 0.65f));
            drawCube(shaderProg, glm::vec3(r.x + 0.28f, r.y + 0.38f, 0.0f),
                     glm::vec3(0.18f, 0.20f, 0.12f), glm::vec3(0.55f, 0.55f, 0.65f));
            // Sirip kecil bawah (stabilizer)
            drawCube(shaderProg, glm::vec3(r.x - 0.20f, r.y - 0.30f, 0.0f),
                     glm::vec3(0.10f, 0.12f, 0.10f), glm::vec3(0.50f, 0.50f, 0.60f));
            drawCube(shaderProg, glm::vec3(r.x + 0.20f, r.y - 0.30f, 0.0f),
                     glm::vec3(0.10f, 0.12f, 0.10f), glm::vec3(0.50f, 0.50f, 0.60f));

            // Api ekor di ATAS (roket jatuh ke bawah, api keluar ke atas)
            float fl = 0.12f + 0.10f * sinf(t * 22.0f + r.x * 3.0f);
            // Lapisan api dalam (putih-kuning, kecil)
            drawCube(shaderProg, glm::vec3(r.x, r.y + 0.62f, 0.0f),
                     glm::vec3(fl * 0.7f, 0.22f, fl * 0.7f), glm::vec3(1.0f, 1.0f, 0.6f), 1.0f);
            // Lapisan api tengah (oranye)
            drawCube(shaderProg, glm::vec3(r.x, r.y + 0.80f, 0.0f),
                     glm::vec3(fl + 0.06f, 0.30f, fl + 0.06f), glm::vec3(1.0f, 0.50f, 0.05f), 0.85f);
            // Lapisan api luar (merah, besar, bergoyang)
            drawCube(shaderProg, glm::vec3(r.x, r.y + 1.02f, 0.0f),
                     glm::vec3(fl + 0.10f, 0.28f, fl + 0.10f), glm::vec3(0.95f, 0.18f, 0.02f), 0.65f);
            // Asap di ujung api (abu gelap, transparan)
            float sm = 0.08f + 0.06f * sinf(t * 10.0f - r.x);
            drawCube(shaderProg, glm::vec3(r.x + sm * 0.5f, r.y + 1.25f, 0.0f),
                     glm::vec3(0.22f, 0.18f, 0.22f), glm::vec3(0.50f, 0.48f, 0.50f), 0.35f);
        }

        float legSwing = player.onGround ? sinf(player.animTimer * 3.0f) * 8.0f : -22.0f;
        float armSwing = player.onGround ? sinf(player.animTimer * 3.0f) * 8.0f : -30.0f;
        drawHumanoid(shaderProg, player.pos, 0.58f,
                     glm::vec3(0.20f, 0.55f, 0.95f),   // badan: biru
                     glm::vec3(0.96f, 0.80f, 0.62f),   // kepala: skin tone
                     glm::vec3(0.12f, 0.22f, 0.60f),   // anggota gerak: biru tua
                     legSwing, armSwing, false, 0.0f);

        // =============================================
        // HUD + OVERLAYS (font-based)
        // =============================================
        {
            glDisable(GL_DEPTH_TEST);
            glm::mat4 hudProj = glm::ortho(0.0f,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.0f,-1.0f,1.0f);
            glUseProgram(fontProg);
            glUniformMatrix4fv(glGetUniformLocation(fontProg,"proj"),1,GL_FALSE,glm::value_ptr(hudProj));

            float sc = 2.0f; // scale teks (2 = 16px per karakter)
            float cw = 8.0f*sc;
            float ch = 8.0f*sc;

            // ── Panel atas (gradient gelap + border bawah) ──
            renderRect(0,0,(float)SCR_WIDTH,ch+14, 0.03f,0.08f,0.04f,0.88f);
            renderRect(0,ch+12,(float)SCR_WIDTH,2,  0.22f,0.68f,0.30f,0.60f); // garis hijau tipis

            // ── Hearts / Nyawa ──
            float hx=10, hy=5;
            for(int h=0;h<5;h++){
                renderHeart(hx + h*32, hy, sc*0.9f, h < playerLives);
            }

            // ── Score text ──
            std::string scoreStr = "SCORE: " + std::to_string(wallsPassed);
            float sw = scoreStr.size()*cw;
            renderText(scoreStr, SCR_WIDTH/2.0f - sw/2.0f, 5, sc,
                       0.70f, 0.98f, 0.72f, 1.0f);

            // ── Speed indicator (gantinya progress bar, karena endless tidak ada target akhir) ──
            float barW=160, barH=6, barX=SCR_WIDTH-barW-10, barY=ch+2;
            renderRect(barX,barY,barW,barH, 0.08f,0.18f,0.09f,0.9f);
            float speedFrac = std::min(wallSpeed/22.0f, 1.0f);
            if(speedFrac>0){
                float fr=0.20f*(1-speedFrac)+0.95f*speedFrac;
                float fg=0.90f*(1-speedFrac)+0.25f*speedFrac;
                float fb=0.35f;
                renderRect(barX,barY,barW*speedFrac,barH,fr,fg,fb,1.0f);
            }

            // ── ESC hint ──
            renderText("A/D=MOVE  ESC=PAUSE", SCR_WIDTH-10-20*cw, 5, sc, 0.40f,0.60f,0.42f,0.8f);

            // ── Platform Size Indicator ──
            {
                int milestones = wallsPassed / 10;
                std::string sizeStr = "PLATFORM: ";
                float sizeRatio = std::max(6.0f - milestones * 0.55f, 1.5f) / 6.0f;
                if(sizeRatio > 0.75f)       sizeStr += "WIDE";
                else if(sizeRatio > 0.50f)  sizeStr += "MEDIUM";
                else if(sizeRatio > 0.30f)  sizeStr += "NARROW";
                else                         sizeStr += "TINY!";
                float pr = 0.30f + (1.0f - sizeRatio) * 0.65f;
                float pg = 0.85f * sizeRatio;
                renderText(sizeStr, SCR_WIDTH/2.0f - sizeStr.size()*cw/2.0f, ch+18, 1.5f, pr, pg, 0.20f, 0.85f);
            }

            // ── Obstacle Warning (aktif selama ada rintangan) ──
            if(!rockets.empty()){
                float pulse = 0.65f + 0.35f * sinf((float)glfwGetTime() * 6.0f);
                std::string warn = "!! ROCKET INCOMING !!";
                renderText(warn, SCR_WIDTH/2.0f - warn.size()*cw*0.8f/2.0f, ch+36, 0.8f*sc, 1.0f, 0.30f*pulse, 0.05f, pulse);
            }

            // ── Hit flash (invincibility) ──
            if(hitCooldown > 0.0f){
                float flash = fmodf(hitCooldown*6.0f,1.0f) > 0.5f ? 0.35f : 0.0f;
                renderRect(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT, 0.9f,0.1f,0.1f,flash);
            }

            // ──────────────────────────────────
            // PAUSE OVERLAY
            // ──────────────────────────────────
            if(gameState==PAUSED){
                float ox=SCR_WIDTH/2.0f, oy=SCR_HEIGHT/2.0f;
                // Dim
                renderRect(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT, 0.02f,0.06f,0.03f,0.72f);
                // Panel
                float pw=310,ph=190;
                renderRect(ox-pw/2,oy-ph/2,pw,ph, 0.04f,0.11f,0.06f,0.97f);
                // Border (semua sisi)
                renderRect(ox-pw/2,   oy-ph/2,   pw, 3,  0.22f,0.72f,0.32f,0.85f);
                renderRect(ox-pw/2,   oy+ph/2-3, pw, 3,  0.22f,0.72f,0.32f,0.85f);
                renderRect(ox-pw/2,   oy-ph/2,   3,  ph, 0.22f,0.72f,0.32f,0.85f);
                renderRect(ox+pw/2-3, oy-ph/2,   3,  ph, 0.22f,0.72f,0.32f,0.85f);
                // Judul PAUSED
                float sc3=3.0f;
                std::string pt="PAUSED";
                renderText(pt, ox-pt.size()*8*sc3/2, oy-78, sc3, 0.32f,0.98f,0.48f,1.0f);
                // Garis dekorasi
                renderRect(ox-90,oy-32,180,2, 0.22f,0.65f,0.30f,0.65f);
                // Hints dengan box kecil
                float sc2=1.5f;
                std::string h1="ESC  /  P  ─  RESUME";
                std::string h2="R        ─  RESTART";
                renderText(h1, ox-h1.size()*8*sc2/2, oy-12, sc2, 0.72f,0.92f,0.74f,0.95f);
                renderText(h2, ox-h2.size()*8*sc2/2, oy+18, sc2, 0.72f,0.92f,0.74f,0.95f);
                // Garis pemisah
                renderRect(ox-90,oy+42,180,1, 0.18f,0.45f,0.22f,0.5f);
                // Score & Lives info
                std::string si="SCORE   "+std::to_string(wallsPassed);
                std::string li="LIVES   "+std::to_string(playerLives);
                renderText(si, ox-si.size()*8*sc2/2, oy+52, sc2, 0.50f,0.72f,0.52f,0.85f);
                renderText(li, ox-li.size()*8*sc2/2, oy+72, sc2, 0.50f,0.72f,0.52f,0.85f);
            }

            // ──────────────────────────────────
            // GAME OVER OVERLAY
            // ──────────────────────────────────
            if(gameState==GAME_OVER){
                float t   = std::min(overlayTimer*2.0f,1.0f);
                float pul = 0.6f+0.4f*sinf(overlayTimer*4.0f);
                float ox=SCR_WIDTH/2.0f, oy=SCR_HEIGHT/2.0f;

                // Dim fade-in
                renderRect(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT, 0.05f,0.01f,0.01f,0.75f*t);

                // Panel
                float pw=360*t,ph=230*t;
                renderRect(ox-pw/2,oy-ph/2,pw,ph, 0.10f,0.02f,0.02f,0.97f);

                // Border berkedip
                float bc=0.80f*pul;
                renderRect(ox-pw/2,   oy-ph/2,   pw, 3,  bc,0.08f,0.08f,0.9f);
                renderRect(ox-pw/2,   oy+ph/2-3, pw, 3,  bc,0.08f,0.08f,0.9f);
                renderRect(ox-pw/2,   oy-ph/2,   3,  ph, bc,0.08f,0.08f,0.9f);
                renderRect(ox+pw/2-3, oy-ph/2,   3,  ph, bc,0.08f,0.08f,0.9f);

                if(t>0.5f){
                    float ft=(t-0.5f)*2.0f;
                    // Judul GAME OVER
                    float sc3=3.0f;
                    std::string got="GAME  OVER";
                    renderText(got, ox-got.size()*8*sc3/2, oy-90, sc3, 0.98f,0.18f,0.18f,ft);

                    // Sub judul
                    float sc16=1.6f;
                    std::string sub="THE  CANOPY  CLAIMED  YOU";
                    renderText(sub, ox-sub.size()*8*sc16/2, oy-45, sc16, 0.75f,0.40f,0.40f,ft*0.85f);

                    // Garis dekorasi
                    renderRect(ox-130,oy-28,260,2, 0.70f,0.10f,0.10f,0.7f*ft);

                    // Stats
                    float sc2=1.8f;
                    std::string s1="SCORE: "+std::to_string(wallsPassed);
                    renderText(s1, ox-s1.size()*8*sc2/2, oy-10, sc2, 0.88f,0.62f,0.62f,ft);

                    // Hiasan bintang berkedip (dekorasi, tidak lagi merepresentasikan progres bertarget)
                    int starCount = std::min(wallsPassed, 10);
                    float dotX=ox-(float)starCount*12/2;
                    for(int s=0;s<starCount;s++){
                        float pulse = 0.85f+0.15f*sinf(overlayTimer*4.0f+s*0.4f);
                        renderRect(dotX+s*12, oy+15, 9, 9,
                                   0.98f*pulse, 0.82f*pulse, 0.10f, ft);
                    }

                    // Box hint
                    renderRect(ox-145, oy+38, 290, 48, 0.20f,0.04f,0.04f,0.85f*ft);
                    renderRect(ox-145, oy+38, 290, 2,  0.80f,0.15f,0.15f,0.7f*ft);
                    // Hint restart
                    float sc15=1.5f;
                    std::string hr="PRESS  R  TO  RESTART";
                    float pa=0.55f+0.45f*sinf(overlayTimer*3.0f);
                    renderText(hr, ox-hr.size()*8*sc15/2, oy+44, sc15, 1.0f,0.92f,0.92f,pa*ft);
                    std::string hm="PRESS  M  FOR  MENU";
                    renderText(hm, ox-hm.size()*8*sc15/2, oy+64, sc15, 0.80f,0.72f,0.72f,pa*ft);
                }
            }

            // (Victory overlay dihapus — mode endless tidak punya kondisi menang)

            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glfwTerminate();
    return g_backToMenuFromGame;
}
// WEBVIEW CALLBACKS
// =============================================
// Flags global
static std::atomic<bool> g_startGame{false};
static std::atomic<bool> g_exit{false};

// webview.h lama Windows pakai external_invoke_cb dengan notify()
void onMenuInvoke(struct webview* w, const char* arg){
    std::string cmd(arg);
    if(cmd=="startGame"){ g_startGame=true; webview_terminate(w); }
    else if(cmd=="options"){
        webview_eval(w,"alert('HOW TO PLAY\\n\\nSPACE / W / UP  =  Jump\\nA / D / LEFT / RIGHT  =  Move\\n\\nLompat ke platform yang bergerak untuk naik tangga.\\nGeser kiri-kanan untuk menghindari platform yang menabrak dari samping!\\nMakin lama, makin cepat — tahan selama mungkin!\\n\\nESC / P  =  Pause\\nR        =  Restart\\nM        =  Back to Menu');");
    }
    else if(cmd=="exit"){ g_exit=true; webview_terminate(w); }
}

// =============================================
// HELPER: Build data:text/html, URI
// Encode hanya karakter yang wajib di-encode agar
// webview lama (IE/MSHTML & WebKitGTK) bisa terima.
// =============================================
static std::string makeDataUrl(const char* html){
    std::string out = "data:text/html,";
    for(const char* p = html; *p; ++p){
        unsigned char c = (unsigned char)*p;
        // Karakter aman: huruf, angka, dan beberapa simbol
        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c=='-' || c=='_' || c=='.' || c=='!' || c=='~' ||
           c=='*' || c=='\'' || c=='(' || c==')' ||
           c=='/' || c==':' || c=='@' || c==',' || c==';' ||
           c=='?' || c=='=' || c=='&' || c=='#' || c=='+'){
            out += (char)c;
        } else {
            // Percent-encode
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

// =============================================
// HELPER: Center webview window on screen (Windows only)
// =============================================
#if defined(_WIN32) || defined(_WIN64)
static void centerWebview(struct webview* wv){
    HWND hwnd = (HWND)wv->priv.hwnd;
    if(!hwnd) return;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    RECT r; GetWindowRect(hwnd, &r);
    int w = r.right - r.left;
    int h = r.bottom - r.top;
    SetWindowPos(hwnd, NULL, (sw-w)/2, (sh-h)/2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
}
#else
static void centerWebview(struct webview* /*wv*/){}
#endif

// =============================================
// MAIN
// =============================================
int main(){
    // Pre-build data URL untuk menu (static)
    std::string menuDataUrl = makeDataUrl(MENU_HTML);

    bool running = true;
    while(running){
        // ─── MAIN MENU ───────────────────────────────
        g_startGame = false;
        g_exit      = false;

        struct webview menu = {};
        menu.title              = "Sylvan Odyssey";
        menu.url                = menuDataUrl.c_str();
        menu.width              = SCR_WIDTH;
        menu.height             = SCR_HEIGHT;
        menu.resizable          = 0;
        menu.debug              = 0;
        menu.external_invoke_cb = onMenuInvoke;
        if(webview_init(&menu) != 0) return 1;
        centerWebview(&menu);
        while(webview_loop(&menu, 1) == 0){}
        webview_exit(&menu);

        if(g_exit)         return 0;
        if(!g_startGame){ running = false; break; }

#if defined(_WIN32) || defined(_WIN64)
        Sleep(120);
#else
        usleep(120000);
#endif

        // Sembunyikan webview sebelum game muncul
#if defined(_WIN32)||defined(_WIN64)
        { HWND h=FindWindowA(NULL,"Sylvan Odyssey"); if(h) ShowWindow(h,SW_HIDE); Sleep(150); }
#else
        usleep(150000);
#endif

        // ─── GAMEPLAY (endless, single mode) ─────────
        bool goMenu = runGame();
        if(!goMenu) running = false;
    } // end while(running)

    return 0;
}