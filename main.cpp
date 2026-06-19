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
      <button class="btn btn-primary" id="btn-start" onclick="this.textContent='Loading...';this.disabled=true;window.external.invoke('openLevelSelect');">Start Journey</button>
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

// LEVEL_HTML sekarang di-generate secara dinamis lewat fungsi di bawah
// agar status locked/unlocked bisa diupdate berdasarkan progress pemain.

static std::string buildLevelHTML(bool lvl1cleared, bool lvl2cleared, bool lvl3cleared) {
    // Helper lambda untuk generate satu kartu level
    auto card = [](int num, const char* name, const char* desc,
                   const char* invoke, bool unlocked, bool cleared) -> std::string {
        std::string cls   = unlocked ? "unlocked" : "locked";
        std::string click = unlocked
            ? std::string("onclick=\"this.style.opacity='0.6';window.external.invoke('") + invoke + "');\"" 
            : "";
        std::string badge = cleared  ? "<span class=\"badge current\">Selesai &#10003;</span>"
                          : unlocked ? "<span class=\"badge current\">Buka sekarang</span>"
                          : "<span class=\"badge locked-b\">&#128274; Terkunci</span>";
        std::string lockDesc = unlocked ? desc
                             : (std::string("Selesaikan Level ") + std::to_string(num-1) + " untuk membuka");
        return
            "<div class=\"card " + cls + "\" " + click + ">"
            "<div class=\"lvl-num\">" + std::to_string(num) + "</div>"
            "<div class=\"info\">"
            "<div class=\"lvl-name\">" + name + "</div>"
            "<div class=\"lvl-desc\">" + lockDesc + "</div>"
            "</div>"
            "<div class=\"right\">" + badge + "</div>"
            "</div>\n";
    };

    std::string html = R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{background:#0a1a0f;font-family:serif;min-height:100vh;overflow:hidden;position:relative;}
.bg{position:fixed;inset:0;background:linear-gradient(to bottom,#050d08 0%,#0e2616 40%,#1a3d24 70%,#0d2010 100%);z-index:0;}
.stars{position:fixed;inset:0;z-index:1;pointer-events:none;}
.vignette{position:fixed;inset:0;background:radial-gradient(circle,transparent 30%,rgba(3,8,5,0.82) 100%);z-index:3;pointer-events:none;}
.wrap{position:relative;z-index:10;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;padding:24px 16px;}
.back-btn{position:fixed;top:20px;left:24px;z-index:20;font-family:serif;font-size:0.75rem;letter-spacing:3px;text-transform:uppercase;color:#86b390;border:1px solid #336e43;background:rgba(13,34,19,0.7);padding:8px 18px;border-radius:3px;cursor:pointer;transition:all 0.3s;}
.back-btn:hover{color:#fff;border-color:#5dd668;}
.header{text-align:center;margin-bottom:36px;}
.title{font-family:serif;font-size:1.6rem;font-weight:900;color:#e4f5e8;letter-spacing:8px;text-transform:uppercase;text-shadow:0 0 20px rgba(93,214,104,0.3);}
.sub{font-size:0.78rem;color:#86b390;letter-spacing:5px;text-transform:uppercase;margin-top:6px;}
.grid{display:flex;flex-direction:column;gap:14px;width:100%;max-width:560px;}
.card{position:relative;background:linear-gradient(135deg,rgba(22,54,31,0.55),rgba(13,34,19,0.75));border:1px solid #2a5c36;border-radius:5px;padding:18px 22px;display:flex;align-items:center;gap:18px;cursor:pointer;transition:all 0.3s;overflow:hidden;}
.card.unlocked{border-color:#4a9e5e;background:linear-gradient(135deg,rgba(37,92,51,0.65),rgba(16,43,23,0.85));}
.card.unlocked:hover{border-color:#5dd668;transform:translateX(4px);box-shadow:0 0 22px rgba(93,214,104,0.18);}
.card.locked{opacity:0.4;cursor:not-allowed;pointer-events:none;}
.lvl-num{font-family:serif;font-size:2rem;font-weight:900;min-width:44px;text-align:center;}
.unlocked .lvl-num{color:#5dd668;text-shadow:0 0 12px rgba(93,214,104,0.4);}
.locked .lvl-num{color:#2d5c35;}
.info{flex:1;}
.lvl-name{font-family:serif;font-size:1rem;font-weight:700;letter-spacing:2px;text-transform:uppercase;}
.unlocked .lvl-name{color:#cce3d2;}
.locked .lvl-name{color:#2d5c35;}
.lvl-desc{font-size:0.78rem;margin-top:4px;}
.unlocked .lvl-desc{color:#86b390;}
.locked .lvl-desc{color:#1e3d26;}
.right{display:flex;flex-direction:column;align-items:flex-end;gap:6px;}
.badge{font-family:serif;font-size:0.65rem;letter-spacing:2px;text-transform:uppercase;padding:3px 10px;border-radius:2px;}
.badge.current{background:rgba(74,158,94,0.2);border:1px solid #4a9e5e;color:#9effa9;}
.badge.locked-b{background:rgba(22,42,28,0.3);border:1px solid #1e3d26;color:#2d5c35;}
.footer-text{margin-top:28px;font-size:0.72rem;letter-spacing:2px;text-transform:uppercase;color:#2d5c35;}
@keyframes twinkle{from{opacity:var(--min);}to{opacity:var(--max);}}
</style>
</head>
<body>
<div class="bg"></div>
<div class="stars" id="stars"></div>
<div class="vignette"></div>
<div class="wrap">
  <button class="back-btn" onclick="window.external.invoke('backToMenu');">&#8592; Menu</button>
  <div class="header">
    <div class="title">Select Level</div>
    <div class="sub">The Rising Canopy</div>
  </div>
  <div class="grid">
)";

    html += card(1, "Verdant Hollow",
                 "Lembah hijau - pelajari dasar melompat &amp; menghindari rintangan",
                 "startLevel1", true, lvl1cleared);
    html += card(2, "Mossy Ascent",
                 "Arena tembakan - dodge serangan musuh dari atas!",
                 "startLevel2", lvl1cleared, lvl2cleared);
    html += card(3, "Canopy Rush",
                 "30 tangga menjulang + musuh penembak &mdash; jangan lengah!",
                 "startLevel3", lvl2cleared, lvl3cleared);
    html += card(4, "Thornwood Surge",
                 "Duri dan rintangan ganda",
                 "startLevel4", false, false);
    html += card(5, "The Great Ascent",
                 "Tantangan tertinggi",
                 "startLevel5", false, false);

    html += R"(  </div>
  <div class="footer-text">STMIK AMIK Bandung &copy; 2026</div>
</div>
<script>
var sts=document.getElementById('stars');
for(var i=0;i<55;i++){var s=document.createElement('div');var sz=1+Math.random()*2;s.style.cssText='position:absolute;top:'+Math.random()*100+'%;left:'+Math.random()*100+'%;width:'+sz+'px;height:'+sz+'px;background:#fff;border-radius:50%;--dur:'+(2+Math.random()*3)+'s;--min:'+(0.1+Math.random()*0.2)+';--max:'+(0.5+Math.random()*0.4)+';animation:twinkle var(--dur) ease-in-out infinite alternate;';sts.appendChild(s);}
</script>
</body>
</html>)";

    return html;
}

// =============================================
// SETTINGS
// =============================================
const unsigned int SCR_WIDTH  = 900;
const unsigned int SCR_HEIGHT = 600;

// =============================================
// CAMERA (side view)
// =============================================
glm::vec3 cameraPos   = glm::vec3(0.0f, 2.5f, 11.0f);
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
const int MAX_STAIRS = 10;
int   playerLives  = 3;    // nyawa pemain
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
uniform float alpha;
void main(){
    FragColor = vec4(0.0, 0.0, 0.0, alpha);
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

void drawHumanoid(unsigned int prog, glm::vec3 base, float s, glm::vec3 bodyC, glm::vec3 headC, glm::vec3 limbC, float legSwing, float armSwing, bool isShadow=false) {
    if(isShadow){
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(base.x, base.y + 0.01f, base.z));
        m = glm::scale(m, glm::vec3(s*0.55f, 0.02f, s*0.32f));
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform1f(glGetUniformLocation(prog,"alpha"),0.28f);
        glDrawArrays(GL_TRIANGLES,0,36);
        return;
    }
    // LEFT LEG
    {
        glm::vec3 pivot = base + glm::vec3(-s*0.20f, s*0.30f, 0);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pivot);
        m = glm::rotate(m, glm::radians(legSwing), glm::vec3(1,0,0));
        m = glm::translate(m, glm::vec3(0,-s*0.30f,0));
        m = glm::scale(m, glm::vec3(s*0.16f, s*0.30f, s*0.16f));
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(limbC));
        glUniform1f(glGetUniformLocation(prog, "alpha"), 1.0f);
        glDrawArrays(GL_TRIANGLES,0,36);
    }
    // RIGHT LEG
    {
        glm::vec3 pivot = base + glm::vec3(s*0.20f, s*0.30f, 0);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pivot);
        m = glm::rotate(m, glm::radians(-legSwing), glm::vec3(1,0,0));
        m = glm::translate(m, glm::vec3(0,-s*0.30f,0));
        m = glm::scale(m, glm::vec3(s*0.16f, s*0.30f, s*0.16f));
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(limbC));
        glUniform1f(glGetUniformLocation(prog, "alpha"), 1.0f);
        glDrawArrays(GL_TRIANGLES,0,36);
    }
    // BODY
    drawCube(prog, base+glm::vec3(0,s*0.85f,0), glm::vec3(s*0.36f,s*0.38f,s*0.20f), bodyC);
    // LEFT ARM
    {
        glm::vec3 pivot = base + glm::vec3(-s*0.52f, s*0.95f, 0);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pivot);
        m = glm::rotate(m, glm::radians(-armSwing), glm::vec3(1,0,0));
        m = glm::translate(m, glm::vec3(0,-s*0.24f,0));
        m = glm::scale(m, glm::vec3(s*0.14f, s*0.26f, s*0.14f));
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(limbC));
        glUniform1f(glGetUniformLocation(prog, "alpha"), 1.0f);
        glDrawArrays(GL_TRIANGLES,0,36);
    }
    // RIGHT ARM
    {
        glm::vec3 pivot = base + glm::vec3(s*0.52f, s*0.95f, 0);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pivot);
        m = glm::rotate(m, glm::radians(armSwing), glm::vec3(1,0,0));
        m = glm::translate(m, glm::vec3(0,-s*0.24f,0));
        m = glm::scale(m, glm::vec3(s*0.14f, s*0.26f, s*0.14f));
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(prog,"objectColor"),1,glm::value_ptr(limbC));
        glUniform1f(glGetUniformLocation(prog, "alpha"), 1.0f);
        glDrawArrays(GL_TRIANGLES,0,36);
    }
    // HEAD & EYES
    drawCube(prog, base+glm::vec3(0,s*1.52f,0), glm::vec3(s*0.33f,s*0.30f,s*0.26f), headC);
    drawCube(prog, base+glm::vec3(-s*0.14f,s*1.58f,s*0.27f), glm::vec3(s*0.06f,s*0.06f,0.02f), glm::vec3(0.1f,0.1f,0.1f));
    drawCube(prog, base+glm::vec3( s*0.14f,s*1.58f,s*0.27f), glm::vec3(s*0.06f,s*0.06f,0.02f), glm::vec3(0.1f,0.1f,0.1f));
}

// =============================================
// SPAWN WALL
// =============================================
void spawnWall(bool forceSameDirection = false, float customDelay = 0.0f){
    if(wallsPassed >= MAX_STAIRS) return;

    Wall w;
    w.id       = wallsPassed + 1;
    w.height   = 0.16f;
    w.passed   = false;
    w.alive    = true;
    w.playerOn = false;
    w.isFrozen = false;

    w.waitingDelay = (customDelay > 0.0f);
    w.delayTimer   = customDelay;

    w.width = 11.0f - (float)w.id;
    if(w.width < 1.0f) w.width = 1.0f;

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
    playerLives       = 3;
    overlayTimer      = 0.0f;
    hitCooldown       = 0.0f;
    spawnFromRight    = false;
    cameraPos.y       = 2.5f;
    cameraTargetY     = 2.5f;

    spawnWall(false, 1.2f);
}

// =============================================
// CALLBACKS (GLFW)
// =============================================
static bool g_backToMenuFromGame = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
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
        // Restart
        if(key==GLFW_KEY_R && (gameState==GAME_OVER || gameState==WIN)) resetGame();
        // Resume from pause with R too
        if(key==GLFW_KEY_R && gameState==PAUSED) gameState=PLAYING;
        // Kembali ke menu (M) saat game over, win, atau pause
        if(key==GLFW_KEY_M && (gameState==GAME_OVER || gameState==WIN || gameState==PAUSED)){
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
            player.pos.x  = PLAYER_X;

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

            for(auto& w : walls){
                if(!w.alive || w.waitingDelay) continue;

                bool xOverlap = fabs(PLAYER_X - w.x) < PLAYER_HW + w.width;
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
                        if(wallsPassed < MAX_STAIRS){
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
                            rep.delayTimer   = 1.0f; // muncul 1 detik setelah kena hit
                            rep.x            = 999.0f;
                            walls.push_back(rep);
                        }
                        // Respawn player ke ground
                        player.pos.y     = GROUND_Y;
                        player.velY      = 0.0f;
                        player.onGround  = true;
                        player.standingY = GROUND_Y;
                        cameraPos.y      = 2.5f;
                        cameraTargetY    = 2.5f;
                    }
                }
            }

            player.onGround = localOnGround;
            if(player.onGround) {
                player.standingY = currentFloor;
                player.pos.y     = player.standingY;
                player.velY      = 0.0f;
            }

            // FIX: WIN saat semua stairs cleared & player mendarat (tidak perlu cek standingY)
            if(wallsPassed >= MAX_STAIRS && player.onGround) {
                gameState    = WIN;
                overlayTimer = 0.0f;
            }

            if(triggersNextSpawn && gameState == PLAYING){
                spawnWall(false, 0.15f);
            }

            wallSpeed = 5.5f + (float)wallsPassed * 0.4f;

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

            cameraTargetY = 2.5f + player.standingY * 0.85f;
            cameraPos.y  += (cameraTargetY - cameraPos.y) * 4.0f * deltaTime;
         } // end if PLAYING (physics)
        } // end if PLAYING || PAUSED

        for(auto& c : clouds){
            c.x -= c.speed * deltaTime;
            if(c.x < -13.0f) c.x = 14.0f;
        }

        std::string title;
        if(gameState == PLAYING)
            title = "Sylvan Odyssey  |  Stairs: " + std::to_string(wallsPassed) + "/" + std::to_string(MAX_STAIRS) + "  |  Lives: " + std::to_string(playerLives) + "  |  ESC = Pause";
        else if(gameState == PAUSED)
            title = "PAUSED  |  Press ESC or P to Resume  |  R to Restart";
        else if(gameState == WIN)
            title = "VICTORY!  |  Press R to Play Again";
        else
            title = "GAME OVER!  |  Stairs: " + std::to_string(wallsPassed) + "/" + std::to_string(MAX_STAIRS) + "  |  Press R to Restart";
        glfwSetWindowTitle(window, title.c_str());

        // Sky gradient - dawn/dusk atmosphere
        float skyR = 0.38f + player.standingY * 0.018f;
        float skyG = 0.62f + player.standingY * 0.010f;
        float skyB = 0.88f - player.standingY * 0.008f;
        glClearColor(
            std::min(skyR, 0.62f),
            std::min(skyG, 0.80f),
            std::min(skyB, 0.98f),
            1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)SCR_WIDTH/SCR_HEIGHT;
        glm::mat4 proj = glm::perspective(glm::radians(48.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        setUniforms(shaderProg, proj, view);
        glUniform3fv(glGetUniformLocation(shaderProg,"lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProg,"lightColor"), 1, glm::value_ptr(lightColor));

        float camOffsetY = cameraPos.y - 2.5f;
        drawCube(shaderProg, glm::vec3(0.0f, 3.5f + camOffsetY, -9.0f), glm::vec3(22.0f, 16.0f, 0.3f), glm::vec3(0.55f, 0.78f, 0.98f));
        drawCube(shaderProg, glm::vec3(6.5f, 6.8f + camOffsetY, -7.0f), glm::vec3(0.60f, 0.60f, 0.2f), glm::vec3(1.0f, 0.95f, 0.35f));

        for(auto& c : clouds){
            drawCube(shaderProg, glm::vec3(c.x, c.y + camOffsetY, c.z), glm::vec3(1.3f, 0.45f, 0.4f), glm::vec3(1.0f, 1.0f, 1.0f));
        }

        drawCube(shaderProg, glm::vec3(0, -0.3f, 0), glm::vec3(15.0f, 0.3f, 2.5f), glm::vec3(0.32f, 0.58f, 0.24f));
        drawCube(shaderProg, glm::vec3(0, 0.02f, 0), glm::vec3(15.0f, 0.05f, 2.5f), glm::vec3(0.42f, 0.72f, 0.32f));

        setUniforms(shadowProg, proj, view);
        float shadowFloor = GROUND_Y;
        for(auto& w : walls){
            if(w.alive && !w.waitingDelay && (w.y + w.height) <= player.pos.y + 0.02f && fabs(PLAYER_X - w.x) < PLAYER_HW + w.width) {
                shadowFloor = std::max(shadowFloor, w.y + w.height);
            }
        }
        drawHumanoid(shadowProg, glm::vec3(player.pos.x, shadowFloor, player.pos.z), 0.58f, glm::vec3(0), glm::vec3(0), glm::vec3(0), 0, 0, true);

        setUniforms(shaderProg, proj, view);
        for(auto& w : walls){
            if(!w.alive || w.waitingDelay) continue;

            float factor = (float)w.id / (float)MAX_STAIRS;
            glm::vec3 wc = glm::mix(glm::vec3(0.92f, 0.25f, 0.20f), glm::vec3(0.15f, 0.85f, 0.40f), factor);
            if(w.isFrozen) wc = wc * 0.82f;

            glm::vec3 wcTop  = glm::clamp(wc * 1.25f, glm::vec3(0), glm::vec3(1));

            drawCube(shaderProg, glm::vec3(w.x, w.y, 0.0f), glm::vec3(w.width, w.height, 0.55f), wc);
            drawCube(shaderProg, glm::vec3(w.x, w.y + w.height, 0.0f), glm::vec3(w.width + 0.02f, 0.02f, 0.58f), wcTop);
        }

        float legSwing = player.onGround ? sinf(player.animTimer * 3.0f) * 8.0f : -22.0f;
        float armSwing = player.onGround ? sinf(player.animTimer * 3.0f) * 8.0f : -30.0f;
        drawHumanoid(shaderProg, player.pos, 0.58f,
                     glm::vec3(0.20f, 0.55f, 0.95f),
                     glm::vec3(0.96f, 0.80f, 0.62f),
                     glm::vec3(0.12f, 0.22f, 0.60f),
                     legSwing, armSwing, false);

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
            for(int h=0;h<3;h++){
                renderHeart(hx + h*32, hy, sc*0.9f, h < playerLives);
            }

            // ── Score text ──
            std::string scoreStr = "STAIRS: " + std::to_string(wallsPassed) + "/" + std::to_string(MAX_STAIRS);
            float sw = scoreStr.size()*cw;
            renderText(scoreStr, SCR_WIDTH/2.0f - sw/2.0f, 5, sc,
                       0.70f, 0.98f, 0.72f, 1.0f);

            // ── Progress bar ──
            float barW=160, barH=6, barX=SCR_WIDTH-barW-10, barY=ch+2;
            renderRect(barX,barY,barW,barH, 0.08f,0.18f,0.09f,0.9f);
            float fill=(float)wallsPassed/(float)MAX_STAIRS;
            if(fill>0){
                float fr=0.95f*(1-fill)+0.20f*fill;
                float fg=0.25f*(1-fill)+0.90f*fill;
                float fb=0.20f*(1-fill)+0.35f*fill;
                renderRect(barX,barY,barW*fill,barH,fr,fg,fb,1.0f);
            }

            // ── ESC hint ──
            renderText("ESC=PAUSE", SCR_WIDTH-10-9*cw, 5, sc, 0.40f,0.60f,0.42f,0.8f);

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
                // Stairs & Lives info
                std::string si="STAIRS  "+std::to_string(wallsPassed)+"/"+std::to_string(MAX_STAIRS);
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
                    std::string s1="STAIRS: "+std::to_string(wallsPassed)+" / "+std::to_string(MAX_STAIRS);
                    renderText(s1, ox-s1.size()*8*sc2/2, oy-10, sc2, 0.88f,0.62f,0.62f,ft);

                    // Star dots progress
                    float dotX=ox-(float)MAX_STAIRS*12/2;
                    for(int s=0;s<MAX_STAIRS;s++){
                        float dc = s<wallsPassed?1.0f:0.22f;
                        float pulse = s<wallsPassed ? 0.85f+0.15f*sinf(overlayTimer*4.0f+s*0.4f) : 1.0f;
                        renderRect(dotX+s*12, oy+15, 9, 9,
                                   dc*0.98f*pulse, dc*0.82f*pulse, dc*0.10f, ft);
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

            // ──────────────────────────────────
            // VICTORY OVERLAY
            // ──────────────────────────────────
            if(gameState==WIN){
                float t   = std::min(overlayTimer*1.8f,1.0f);
                float pul = 0.7f+0.3f*sinf(overlayTimer*3.0f);
                float ox=SCR_WIDTH/2.0f, oy=SCR_HEIGHT/2.0f;
                float bcy = sinf(overlayTimer*4.0f)*5.0f*(1.0f-std::min(overlayTimer,1.0f));

                // Dim
                renderRect(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT, 0.01f,0.06f,0.02f,0.70f*t);

                // Panel
                float pw=380*t, ph=240*t;
                renderRect(ox-pw/2, oy-ph/2+bcy, pw,ph, 0.03f,0.12f,0.05f,0.97f);

                // Border hijau bersinar
                float bc=0.30f*pul;
                renderRect(ox-pw/2,        oy-ph/2+bcy,    pw, 3,  0.25f,0.88f,0.35f,bc+0.4f);
                renderRect(ox-pw/2,        oy+ph/2-3+bcy,  pw, 3,  0.25f,0.88f,0.35f,bc+0.4f);
                renderRect(ox-pw/2,        oy-ph/2+bcy,    3,  ph, 0.25f,0.88f,0.35f,bc+0.4f);
                renderRect(ox+pw/2-3,      oy-ph/2+bcy,    3,  ph, 0.25f,0.88f,0.35f,bc+0.4f);

                if(t>0.4f){
                    float ft=(t-0.4f)/0.6f;
                    // Judul VICTORY!
                    float sc3=3.0f;
                    std::string vt="VICTORY!";
                    renderText(vt, ox-vt.size()*8*sc3/2, oy-95+bcy, sc3, 0.28f,1.00f,0.42f,ft);

                    // Sub judul
                    float sc2=1.6f;
                    std::string vs="YOU  ESCAPED  THE  CANOPY!";
                    renderText(vs, ox-vs.size()*8*sc2/2, oy-48+bcy, sc2, 0.62f,0.95f,0.65f,ft);

                    // Garis dekorasi
                    renderRect(ox-140,oy-20+bcy,280,2, 0.25f,0.80f,0.32f,0.7f*ft);

                    // Stats
                    float sc18=1.8f;
                    std::string st="ALL  "+std::to_string(MAX_STAIRS)+" / "+std::to_string(MAX_STAIRS)+"  STAIRS  CLEARED!";
                    renderText(st, ox-st.size()*8*sc18/2, oy-5+bcy, sc18, 0.70f,1.00f,0.72f,ft);

                    std::string sl="LIVES  REMAINING:  "+std::to_string(playerLives);
                    renderText(sl, ox-sl.size()*8*sc18/2, oy+22+bcy, sc18, 0.55f,0.85f,0.58f,ft);

                    // Semua stars berkilau
                    float dotX=ox-(float)MAX_STAIRS*12/2;
                    for(int s=0;s<MAX_STAIRS;s++){
                        float glow=0.82f+0.18f*sinf(overlayTimer*5.0f+s*0.5f);
                        renderRect(dotX+s*12, oy+48+bcy, 9,9, glow*0.98f,glow*0.85f,0.10f, ft);
                    }

                    // Box hint
                    renderRect(ox-145, oy+68+bcy, 290, 48, 0.04f,0.18f,0.06f,0.85f*ft);
                    renderRect(ox-145, oy+68+bcy, 290, 2,  0.25f,0.88f,0.35f,0.7f*ft);
                    // Hint
                    float sc15=1.5f;
                    std::string hr="PRESS  R  TO  PLAY  AGAIN";
                    float pa=0.55f+0.45f*sinf(overlayTimer*2.5f);
                    renderText(hr, ox-hr.size()*8*sc15/2, oy+74+bcy, sc15, 0.88f,1.0f,0.90f,pa*ft);
                    std::string hm="PRESS  M  FOR  MENU";
                    renderText(hm, ox-hm.size()*8*sc15/2, oy+94+bcy, sc15, 0.70f,0.88f,0.72f,pa*ft);
                }
            }

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


// =============================================

// =============================================
// LEVEL 2 - MOSSY ASCENT
// Identik dengan lvl1: platform bergerak, lompat
// TAMBAHAN: A/D gerak, 1 musuh nembak, 20 tangga
// =============================================
// =============================================
// LEVEL 2 - MOSSY ASCENT
// State sepenuhnya TERPISAH dari Level 1.
// Tidak ada shared globals (player, walls, cameraPos, dll)
// Gameplay: naik 20 tangga + 1 musuh nembak. Hanya SPACE/lompat.
// =============================================

struct Bullet { float x,y,velX,velY; bool alive; };

// ── State L2 (semua lokal ke file ini) ──
struct L2Player {
    float y=0,velY=0,standingY=0,animTimer=0;
    bool onGround=true;
} l2p;

struct L2Wall {
    float x,y,width,height,speedSign,delayTimer;
    int id;
    bool passed,alive,isFrozen,waitingDelay;
};

static std::vector<L2Wall>  l2_walls;
static std::vector<Bullet>  l2_bullets;
static float  l2_camY=2.5f, l2_camTargetY=2.5f;
static float  l2_playerX=0.0f;  // posisi X player (A/D)
static bool   l2_keysHeld[512]={};
static float  l2_wallSpeed=5.5f;
static bool   l2_spawnRight=false;
static int    l2_wallId=200;
static int    l2_wallsPassed=0;
static int    l2_lives=3;
static float  l2_hitCD=0,l2_overlayT=0,l2_gameTime=0;
static GameState l2_state=PLAYING;
static bool   l2_backToMenu=false;

// Enemy
static float l2_exX=0,l2_exVelX=2.2f,l2_exPhase=0,l2_exShoot=0;

const int   L2_MAX=20;
const float L2_GY =0.0f;  // ground Y

static void l2_spawnWall(float delayOverride=-1.0f){
    int activeCount=0;
    for(auto& w0:l2_walls) if(w0.alive&&!w0.isFrozen) activeCount++;
    if(activeCount>4) return; // max 5 wall AKTIF (belum diinjak) di layar
    L2Wall w;
    w.id=l2_wallId++;
    w.height=0.16f;
    int stairNum=w.id-200+1;
    w.width=9.0f-stairNum*0.35f; if(w.width<1.0f)w.width=1.0f;
    w.y=l2p.standingY+0.75f;
    w.speedSign=l2_spawnRight?-1.0f:1.0f;
    l2_spawnRight=!l2_spawnRight;
    float delay=delayOverride>=0?delayOverride:0.0f;
    w.waitingDelay=(delay>0);
    w.delayTimer=delay;
    w.x=w.waitingDelay?9999.0f:(w.speedSign<0?16.0f:-16.0f);
    w.passed=false; w.alive=true; w.isFrozen=false;
    l2_walls.push_back(w);
}

static void l2_fullReset(){
    l2p={};
    l2_walls.clear(); l2_bullets.clear();
    l2_camY=2.5f; l2_camTargetY=2.5f;
    l2_wallSpeed=5.5f; l2_spawnRight=false; l2_wallId=200;
    l2_wallsPassed=0; l2_lives=3;
    l2_hitCD=0; l2_overlayT=0; l2_gameTime=0;
    l2_state=PLAYING; l2_backToMenu=false;
    l2_playerX=0.0f; memset(l2_keysHeld,0,sizeof(l2_keysHeld));
    l2_exX=0; l2_exVelX=2.2f; l2_exPhase=0; l2_exShoot=0;
    l2_spawnWall(1.2f); // tangga pertama muncul setelah 1.2 detik
}

void l2_key_callback(GLFWwindow* w,int key,int,int action,int){
    if(key>=0&&key<512){ if(action==GLFW_PRESS) l2_keysHeld[key]=true; if(action==GLFW_RELEASE) l2_keysHeld[key]=false; }
    if(action==GLFW_PRESS){
        if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W)
            &&l2p.onGround&&l2_state==PLAYING){
            l2p.velY=JUMP_FORCE; l2p.onGround=false;
        }
        if((key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P)&&l2_state==PLAYING)  l2_state=PAUSED;
        else if((key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P)&&l2_state==PAUSED) l2_state=PLAYING;
        if(key==GLFW_KEY_R&&(l2_state==GAME_OVER||l2_state==WIN||l2_state==PAUSED)) l2_fullReset();
        if(key==GLFW_KEY_M){ l2_backToMenu=true; glfwSetWindowShouldClose(w,GLFW_TRUE); }
    }
}

bool runGame2(){
    l2_fullReset();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,
        "Sylvan Odyssey - Level 2: Mossy Ascent",NULL,NULL);
#if defined(_WIN32)||defined(_WIN64)
    { int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
      glfwSetWindowPos(win,(sw-(int)SCR_WIDTH)/2,(sh-(int)SCR_HEIGHT)/2); }
#else
    { GLFWmonitor* m=glfwGetPrimaryMonitor(); const GLFWvidmode* v=glfwGetVideoMode(m);
      if(v) glfwSetWindowPos(win,(v->width-(int)SCR_WIDTH)/2,(v->height-(int)SCR_HEIGHT)/2); }
#endif
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback);
    glfwSetKeyCallback(win,l2_key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    unsigned int sp2=makeShader(vertSrc,fragSrc);
    unsigned int shp2=makeShader(vertSrc,shadowFragSrc);
    unsigned int fp2=makeShader(fontVertSrc,fontFragSrc);

    // Font textures — OpenGL context baru, harus init ulang
    unsigned int fTex2=0,fVAO2=0,fVBO2=0,wTex2=0;
    {
        unsigned char atlas[48][128]={};
        for(int c=0;c<96;c++){ int col=c%16,row=c/16;
            for(int y2=0;y2<8;y2++){ unsigned char bits=FONT8[c][y2];
                for(int x2=0;x2<8;x2++) if(bits&(0x80>>x2)) atlas[row*8+y2][col*8+x2]=255; }}
        glGenTextures(1,&fTex2); glBindTexture(GL_TEXTURE_2D,fTex2);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RED,128,48,0,GL_RED,GL_UNSIGNED_BYTE,atlas);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        unsigned char w1=255;
        glGenTextures(1,&wTex2); glBindTexture(GL_TEXTURE_2D,wTex2);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&w1);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glGenVertexArrays(1,&fVAO2); glGenBuffers(1,&fVBO2);
        glBindVertexArray(fVAO2); glBindBuffer(GL_ARRAY_BUFFER,fVBO2);
        glBufferData(GL_ARRAY_BUFFER,sizeof(float)*6*4,NULL,GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float))); glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    unsigned int VAO2,VBO2b;
    glGenVertexArrays(1,&VAO2); glGenBuffers(1,&VBO2b);
    glBindVertexArray(VAO2); glBindBuffer(GL_ARRAY_BUFFER,VBO2b);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    // ── Helpers 2D ──
    auto qd=[&](float x,float y,float w,float h,float u0,float v0,float u1,float v1){
        float vt[6][4]={{x,y+h,u0,v1},{x,y,u0,v0},{x+w,y,u1,v0},{x,y+h,u0,v1},{x+w,y,u1,v0},{x+w,y+h,u1,v1}};
        glBindVertexArray(fVAO2); glBindBuffer(GL_ARRAY_BUFFER,fVBO2);
        glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(vt),vt); glDrawArrays(GL_TRIANGLES,0,6); };
    auto rR=[&](float x,float y,float w,float h,float r,float g,float b,float a){
        glUseProgram(fp2); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,wTex2);
        glUniform1i(glGetUniformLocation(fp2,"tex"),0);
        glUniform4f(glGetUniformLocation(fp2,"color"),r,g,b,a); qd(x,y,w,h,0,0,1,1); };
    auto rT=[&](const std::string& s,float x,float y,float sc,float r=1,float g=1,float b=1,float a=1){
        glUseProgram(fp2); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,fTex2);
        glUniform1i(glGetUniformLocation(fp2,"tex"),0);
        glUniform4f(glGetUniformLocation(fp2,"color"),r,g,b,a);
        float cx=x;
        for(char c:s){ if(c<32||c>127)c=32; int idx=c-32,col=idx%16,row=idx/16;
            float u0=col/16.0f,u1=(col+1)/16.0f,v0=row*8/48.0f,v1=(row+1)*8/48.0f;
            float vt[6][4]={{cx,y+8*sc,u0,v1},{cx,y,u0,v0},{cx+8*sc,y,u1,v0},
                            {cx,y+8*sc,u0,v1},{cx+8*sc,y,u1,v0},{cx+8*sc,y+8*sc,u1,v1}};
            glBindVertexArray(fVAO2); glBindBuffer(GL_ARRAY_BUFFER,fVBO2);
            glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(vt),vt); glDrawArrays(GL_TRIANGLES,0,6);
            cx+=8*sc; } };
    auto rH=[&](float x,float y,float s,bool f){
        float r=f?0.95f:0.28f,g=f?0.15f:0.22f,b=f?0.18f:0.26f;
        rR(x+s,y,s,s,r,g,b,1); rR(x+4*s,y,s,s,r,g,b,1);
        rR(x,y+s,6*s,s,r,g,b,1); rR(x,y+2*s,6*s,s,r,g,b,1);
        rR(x+s,y+3*s,4*s,s,r,g,b,1); rR(x+2*s,y+4*s,2*s,s,r,g,b,1);
        rR(x+3*s,y+5*s,s,s,r,g,b,1); };
    auto sU=[&](unsigned int p,const glm::mat4& pj,const glm::mat4& vw){
        glUseProgram(p);
        glUniformMatrix4fv(glGetUniformLocation(p,"projection"),1,GL_FALSE,glm::value_ptr(pj));
        glUniformMatrix4fv(glGetUniformLocation(p,"view"),1,GL_FALSE,glm::value_ptr(vw)); };
    auto dC=[&](unsigned int p,glm::vec3 pos,glm::vec3 sc,glm::vec3 col,float a=1.0f){
        glUseProgram(p);
        glm::mat4 m=glm::translate(glm::mat4(1),pos)*glm::scale(glm::mat4(1),sc);
        glUniformMatrix4fv(glGetUniformLocation(p,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(p,"objectColor"),1,glm::value_ptr(col));
        glUniform1f(glGetUniformLocation(p,"alpha"),a);
        glBindVertexArray(VAO2); glDrawArrays(GL_TRIANGLES,0,36); };

    // Awan
    struct Cld{float x,y,z,spd;};
    std::vector<Cld> clouds;
    for(int i=0;i<5;i++) clouds.push_back({-12.0f+i*5.5f,5.5f+i*0.6f,-6.5f,0.4f+i*0.12f});

    float lf=(float)glfwGetTime();

    while(!glfwWindowShouldClose(win)){
        float now=(float)glfwGetTime();
        float dt=now-lf; if(dt>0.05f)dt=0.05f; lf=now;

        // ── UPDATE ──
        if(l2_state==PLAYING){
            l2_gameTime+=dt; l2p.animTimer+=dt;
            if(l2_hitCD>0) l2_hitCD-=dt;

            // A/D movement
            const float L2_MOVE=7.0f, L2_BOUND=7.5f;
            if(l2_keysHeld[GLFW_KEY_A]||l2_keysHeld[GLFW_KEY_LEFT])  l2_playerX-=L2_MOVE*dt;
            if(l2_keysHeld[GLFW_KEY_D]||l2_keysHeld[GLFW_KEY_RIGHT]) l2_playerX+=L2_MOVE*dt;
            if(l2_playerX< -L2_BOUND) l2_playerX=-L2_BOUND;
            if(l2_playerX>  L2_BOUND) l2_playerX= L2_BOUND;

            // Fisika player
            l2p.velY+=GRAVITY*dt;
            l2p.y+=l2p.velY*dt;
            bool localOn=false; float curFloor=L2_GY;

            // Ground
            if(l2p.y<=L2_GY){ l2p.y=L2_GY; l2p.velY=0; localOn=true; curFloor=L2_GY; }

            // Collision tangga
            const float LTOL=0.35f,STOL=0.15f;
            bool doSpawn=false;
            for(auto& w:l2_walls){
                if(!w.alive) continue;
                if(w.waitingDelay){
                    w.delayTimer-=dt;
                    if(w.delayTimer<=0){ w.waitingDelay=false; w.x=w.speedSign<0?16.0f:-16.0f; }
                    continue;
                }
                if(!w.isFrozen) w.x+=w.speedSign*l2_wallSpeed*dt;
                // Keluar layar
                if((w.speedSign<0&&w.x<-16)||(w.speedSign>0&&w.x>16)){
                    if(!w.isFrozen){ w.alive=false; } continue;
                }
                // X overlap (player di l2_playerX)
                if(fabs(l2_playerX-w.x)>=PLAYER_HW+w.width) continue;
                float pF=l2p.y, pH=l2p.y+PLAYER_H;
                float wT=w.y+w.height, wB=w.y-w.height;
                // Landing
                if(l2p.velY<=0&&pF>=wT-LTOL&&pF<=wT+0.15f){
                    if(!w.isFrozen&&!w.passed){
                        w.isFrozen=true; w.passed=true;
                        l2_wallsPassed++; doSpawn=true;
                    }
                    w.isFrozen=true;
                    l2p.y=wT; l2p.velY=0;
                    localOn=true; curFloor=wT;
                }
                // Side hit
                else if(!w.isFrozen&&!w.passed&&l2_hitCD<=0&&pF<wT-STOL&&pH>wB+STOL){
                    l2_lives--; w.isFrozen=true; w.alive=false; l2_hitCD=1.8f;
                    if(l2_lives<=0){ l2_state=GAME_OVER; l2_overlayT=0; }
                    // Spawn pengganti — player TIDAK reset posisi
                    else{ L2Wall rep=w; rep.passed=false; rep.alive=true; rep.isFrozen=false;
                          rep.waitingDelay=true; rep.delayTimer=1.0f; rep.x=9999;
                          l2_walls.push_back(rep); }
                }
            }
            l2_walls.erase(std::remove_if(l2_walls.begin(),l2_walls.end(),
                [](const L2Wall& w){return !w.alive&&!w.waitingDelay;}),l2_walls.end());

            l2p.onGround=localOn;
            if(localOn){ l2p.standingY=curFloor; l2p.y=curFloor; l2p.velY=0; }

            // WIN
            if(l2_wallsPassed>=L2_MAX&&l2p.onGround&&l2_state==PLAYING){
                l2_state=WIN; l2_overlayT=0;
            }

            // Spawn tangga berikutnya
            if(doSpawn&&l2_state==PLAYING) l2_spawnWall(0.15f);

            // Wall speed naik
            l2_wallSpeed=5.5f+l2_wallsPassed*0.35f;

            // Camera
            l2_camTargetY=2.5f+l2p.standingY*0.85f;
            l2_camY+=(l2_camTargetY-l2_camY)*4.0f*dt;

            // Clouds
            for(auto& c:clouds){ c.x-=c.spd*dt; if(c.x<-14)c.x=14; }

            // Enemy melayang di atas player
            l2_exX+=l2_exVelX*dt;
            if(l2_exX>5.0f){ l2_exX=5.0f; l2_exVelX=-fabs(l2_exVelX); }
            if(l2_exX<-5.0f){ l2_exX=-5.0f; l2_exVelX=fabs(l2_exVelX); }
            l2_exPhase+=dt*1.5f;
            float eY=l2p.standingY+5.5f+sinf(l2_exPhase)*0.4f;

            float intv=std::max(0.85f,2.3f-l2_wallsPassed*0.045f);
            l2_exShoot+=dt;
            if(l2_exShoot>=intv){
                l2_exShoot=0;
                float dx=l2_playerX-l2_exX, dy=(l2p.y+0.95f)-eY;
                float dist=sqrtf(dx*dx+dy*dy); if(dist<0.1f)dist=0.1f;
                float bspd=7.0f+l2_wallsPassed*0.18f;
                l2_bullets.push_back({l2_exX,eY,dx/dist*bspd,dy/dist*bspd,true});
            }

            // Peluru
            for(auto& b:l2_bullets){
                if(!b.alive) continue;
                b.x+=b.velX*dt; b.y+=b.velY*dt;
                if(fabs(b.x)>13||b.y<-2||b.y>50){ b.alive=false; continue; }
                if(l2_hitCD<=0){
                    float dx=b.x-l2_playerX, dy=b.y-(l2p.y+0.95f);
                    if(fabs(dx)<0.45f&&fabs(dy)<1.0f){
                        b.alive=false; l2_lives--; l2_hitCD=1.8f;
                        if(l2_lives<=0){ l2_state=GAME_OVER; l2_overlayT=0; }
                    }
                }
            }
            l2_bullets.erase(std::remove_if(l2_bullets.begin(),l2_bullets.end(),
                [](const Bullet& b){return !b.alive;}),l2_bullets.end());

        } else { l2_overlayT+=dt; }

        // ── RENDER ──
        float skyR=0.38f+l2p.standingY*0.018f;
        float skyG=0.62f+l2p.standingY*0.010f;
        float skyB=0.88f-l2p.standingY*0.008f;
        glClearColor(std::min(skyR,0.62f),std::min(skyG,0.80f),std::min(skyB,0.98f),1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        float asp=(float)SCR_WIDTH/SCR_HEIGHT;
        glm::vec3 camP(0,l2_camY,11.0f);
        glm::vec3 camF(0,-0.15f,-1.0f);
        glm::mat4 pj=glm::perspective(glm::radians(48.0f),asp,0.1f,100.0f);
        glm::mat4 vw=glm::lookAt(camP,camP+camF,glm::vec3(0,1,0));
        sU(sp2,pj,vw);
        glUniform3fv(glGetUniformLocation(sp2,"lightPos"),1,glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(sp2,"lightColor"),1,glm::value_ptr(lightColor));

        float cOY=l2_camY-2.5f;
        // Langit + matahari
        dC(sp2,glm::vec3(0,3.5f+cOY,-9),glm::vec3(22,16,0.3f),glm::vec3(0.55f,0.78f,0.98f));
        dC(sp2,glm::vec3(6.5f,6.8f+cOY,-7),glm::vec3(0.6f,0.6f,0.2f),glm::vec3(1,0.95f,0.35f));
        // Awan
        for(auto& c:clouds) dC(sp2,glm::vec3(c.x,c.y+cOY,c.z),glm::vec3(1.3f,0.45f,0.4f),glm::vec3(1,1,1));
        // Ground — warna hijau tua (mossy, beda dari lvl1)
        dC(sp2,glm::vec3(0,-0.3f,0),glm::vec3(15,0.3f,2.5f),glm::vec3(0.22f,0.40f,0.14f));
        dC(sp2,glm::vec3(0,0.02f,0),glm::vec3(15,0.05f,2.5f),glm::vec3(0.30f,0.58f,0.20f));

        // Bayangan player
        sU(shp2,pj,vw);
        float shadowF=L2_GY;
        for(auto& w:l2_walls)
            if(w.alive&&!w.waitingDelay&&(w.y+w.height)<=l2p.y+0.02f&&fabs(0.0f-w.x)<PLAYER_HW+w.width)
                shadowF=std::max(shadowF,w.y+w.height);
        drawHumanoid(shp2,glm::vec3(0,shadowF,0),0.58f,glm::vec3(0),glm::vec3(0),glm::vec3(0),0,0,true);

        // Tangga — gradient oranye→biru (beda dari lvl1)
        sU(sp2,pj,vw);
        for(auto& w:l2_walls){
            if(!w.alive||w.waitingDelay) continue;
            float f=(float)(w.id-200)/(float)L2_MAX;
            glm::vec3 wc=glm::mix(glm::vec3(0.92f,0.55f,0.10f),glm::vec3(0.10f,0.65f,0.90f),f);
            if(w.isFrozen) wc*=0.82f;
            dC(sp2,glm::vec3(w.x,w.y,0),glm::vec3(w.width,w.height,0.55f),wc);
            dC(sp2,glm::vec3(w.x,w.y+w.height,0),glm::vec3(w.width+0.02f,0.02f,0.58f),glm::clamp(wc*1.25f,glm::vec3(0),glm::vec3(1)));
        }

        // Player — selalu di X=0 (tengah)
        float eYr=l2p.standingY+5.5f+sinf(l2_exPhase)*0.4f;
        float leg=l2p.onGround?sinf(l2p.animTimer*3.0f)*8.0f:-22.0f;
        float arm=l2p.onGround?sinf(l2p.animTimer*3.0f)*8.0f:-30.0f;
        glm::vec3 pcol=(l2_hitCD>0&&fmodf(l2_hitCD*6,1)>0.5f)?glm::vec3(1,0.3f,0.3f):glm::vec3(0.20f,0.55f,0.95f);
        // Leg animation: jalan kalau A/D ditekan
        bool l2Moving=l2_keysHeld[GLFW_KEY_A]||l2_keysHeld[GLFW_KEY_D]||l2_keysHeld[GLFW_KEY_LEFT]||l2_keysHeld[GLFW_KEY_RIGHT];
        leg=l2p.onGround?(l2Moving?sinf(l2p.animTimer*3.0f)*8.0f:0.0f):-22.0f;
        arm=l2p.onGround?(l2Moving?sinf(l2p.animTimer*3.0f)*8.0f:0.0f):-30.0f;
        drawHumanoid(sp2,glm::vec3(l2_playerX,l2p.y,0),0.58f,pcol,glm::vec3(0.96f,0.80f,0.62f),glm::vec3(0.12f,0.22f,0.60f),leg,arm,false);

        // Enemy merah
        float pulse=0.82f+0.18f*sinf(now*3.5f);
        glm::vec3 ec(0.88f,0.12f,0.12f);
        dC(sp2,glm::vec3(l2_exX,eYr,0),glm::vec3(0.75f,0.45f,0.38f),ec*pulse);
        dC(sp2,glm::vec3(l2_exX-1.1f,eYr+0.1f,0),glm::vec3(0.55f,0.22f,0.2f),ec*0.65f);
        dC(sp2,glm::vec3(l2_exX+1.1f,eYr+0.1f,0),glm::vec3(0.55f,0.22f,0.2f),ec*0.65f);
        dC(sp2,glm::vec3(l2_exX-0.25f,eYr+0.22f,0.43f),glm::vec3(0.10f,0.10f,0.1f),glm::vec3(1,1,1));
        dC(sp2,glm::vec3(l2_exX+0.25f,eYr+0.22f,0.43f),glm::vec3(0.10f,0.10f,0.1f),glm::vec3(1,1,1));
        dC(sp2,glm::vec3(l2_exX-0.25f,eYr+0.22f,0.45f),glm::vec3(0.06f,0.06f,0.1f),glm::vec3(0.05f,0.05f,0.05f));
        dC(sp2,glm::vec3(l2_exX+0.25f,eYr+0.22f,0.45f),glm::vec3(0.06f,0.06f,0.1f),glm::vec3(0.05f,0.05f,0.05f));
        // Charge indicator
        float si2=std::min(l2_exShoot/std::max(0.85f,2.3f-l2_wallsPassed*0.045f),1.0f);
        dC(sp2,glm::vec3(l2_exX,eYr-0.62f,0.5f),glm::vec3(0.12f*si2+0.02f,0.12f*si2+0.02f,0.05f),
           glm::mix(glm::vec3(0.95f,0.8f,0.1f),glm::vec3(0.95f,0.1f,0.1f),si2));

        // Peluru
        for(auto& b:l2_bullets){
            if(!b.alive) continue;
            dC(sp2,glm::vec3(b.x,b.y,0),glm::vec3(0.15f,0.15f,0.15f),glm::vec3(0.98f,0.72f,0.08f));
            dC(sp2,glm::vec3(b.x-b.velX*0.04f,b.y-b.velY*0.04f,0),glm::vec3(0.10f,0.10f,0.10f),glm::vec3(0.98f,0.45f,0.08f),0.45f);
        }

        // ── HUD 2D ──
        {
            glDisable(GL_DEPTH_TEST);
            glm::mat4 hp=glm::ortho(0.0f,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.0f,-1.0f,1.0f);
            glUseProgram(fp2);
            glUniformMatrix4fv(glGetUniformLocation(fp2,"proj"),1,GL_FALSE,glm::value_ptr(hp));
            float sc=2.0f,cw=8*sc,ch=8*sc;
            rR(0,0,(float)SCR_WIDTH,ch+14,0.03f,0.08f,0.04f,0.88f);
            rR(0,ch+12,(float)SCR_WIDTH,2,0.22f,0.68f,0.30f,0.60f);
            for(int h=0;h<3;h++) rH(10+h*32,5,sc*0.9f,h<l2_lives);
            std::string ss="STAIRS: "+std::to_string(l2_wallsPassed)+"/"+std::to_string(L2_MAX);
            rT(ss,SCR_WIDTH/2.0f-ss.size()*cw/2,5,sc,0.70f,0.98f,0.72f,1);
            float bW=160,bH=6,bX=SCR_WIDTH-bW-10,bY=ch+2;
            rR(bX,bY,bW,bH,0.08f,0.18f,0.09f,0.9f);
            float fill=(float)l2_wallsPassed/(float)L2_MAX;
            if(fill>0){ float fr=0.95f*(1-fill)+0.20f*fill,fg=0.25f*(1-fill)+0.90f*fill,fb=0.20f*(1-fill)+0.35f*fill;
                rR(bX,bY,bW*fill,bH,fr,fg,fb,1.0f); }
            rT("A/D=MOVE ESC=PAUSE",SCR_WIDTH-19*cw,5,sc,0.40f,0.60f,0.42f,0.8f);
            if(l2_hitCD>0){ float fl=fmodf(l2_hitCD*6,1)>0.5f?0.35f:0.0f;
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.9f,0.1f,0.1f,fl); }

            if(l2_state==PAUSED){
                float ox=SCR_WIDTH/2.0f,oy=SCR_HEIGHT/2.0f;
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.02f,0.06f,0.03f,0.72f);
                rR(ox-155,oy-105,310,220,0.04f,0.11f,0.06f,0.97f);
                rR(ox-155,oy-105,310,3,0.22f,0.72f,0.32f,0.85f); rR(ox-155,oy+112,310,3,0.22f,0.72f,0.32f,0.85f);
                rR(ox-155,oy-105,3,218,0.22f,0.72f,0.32f,0.85f); rR(ox+152,oy-105,3,218,0.22f,0.72f,0.32f,0.85f);
                float sc3=3.0f; std::string pt="PAUSED";
                rT(pt,ox-pt.size()*8*sc3/2,oy-88,sc3,0.32f,0.98f,0.48f,1);
                rR(ox-90,oy-42,180,2,0.22f,0.65f,0.30f,0.65f);
                float sc2=1.5f;
                rT("ESC / P  -  RESUME", ox-18*8*sc2/2,oy-22,sc2,0.72f,0.92f,0.74f,0.95f);
                rT("R        -  RESTART",ox-19*8*sc2/2,oy+ 5,sc2,0.72f,0.92f,0.74f,0.95f);
                rT("M        -  MENU",   ox-17*8*sc2/2,oy+32,sc2,0.72f,0.92f,0.74f,0.95f);
                rR(ox-90,oy+57,180,1,0.18f,0.45f,0.22f,0.5f);
                std::string di="STAIRS  "+std::to_string(l2_wallsPassed)+"/"+std::to_string(L2_MAX);
                std::string li="LIVES   "+std::to_string(l2_lives);
                rT(di,ox-di.size()*8*sc2/2,oy+68,sc2,0.50f,0.72f,0.52f,0.85f);
                rT(li,ox-li.size()*8*sc2/2,oy+90,sc2,0.50f,0.72f,0.52f,0.85f);
            }
            if(l2_state==GAME_OVER){
                float t=std::min(l2_overlayT*2.0f,1.0f),pul=0.6f+0.4f*sinf(l2_overlayT*4);
                float ox=SCR_WIDTH/2.0f,oy=SCR_HEIGHT/2.0f;
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.05f,0.01f,0.01f,0.75f*t);
                float pw=360*t,ph=230*t;
                rR(ox-pw/2,oy-ph/2,pw,ph,0.10f,0.02f,0.02f,0.97f);
                float bc=0.80f*pul;
                rR(ox-pw/2,oy-ph/2,pw,3,bc,0.08f,0.08f,0.9f); rR(ox-pw/2,oy+ph/2-3,pw,3,bc,0.08f,0.08f,0.9f);
                rR(ox-pw/2,oy-ph/2,3,ph,bc,0.08f,0.08f,0.9f); rR(ox+pw/2-3,oy-ph/2,3,ph,bc,0.08f,0.08f,0.9f);
                if(t>0.5f){ float ft=(t-0.5f)*2;
                    float sc3=3.0f; std::string got="GAME  OVER";
                    rT(got,ox-got.size()*8*sc3/2,oy-90,sc3,0.98f,0.18f,0.18f,ft);
                    float sc16=1.6f; std::string sub="THE  CANOPY  CLAIMED  YOU";
                    rT(sub,ox-sub.size()*8*sc16/2,oy-45,sc16,0.75f,0.40f,0.40f,ft*0.85f);
                    rR(ox-130,oy-28,260,2,0.70f,0.10f,0.10f,0.7f*ft);
                    float sc2=1.8f; std::string s1="STAIRS: "+std::to_string(l2_wallsPassed)+" / "+std::to_string(L2_MAX);
                    rT(s1,ox-s1.size()*8*sc2/2,oy-10,sc2,0.88f,0.62f,0.62f,ft);
                    float dotX=ox-L2_MAX*6.0f;
                    for(int s2=0;s2<L2_MAX;s2++){ float dc=s2<l2_wallsPassed?1.0f:0.22f;
                        rR(dotX+s2*12,oy+15,9,9,dc*0.98f,dc*0.82f,0.10f,ft); }
                    float sc15=1.5f,pa=0.5f+0.5f*sinf(l2_overlayT*3);
                    rT("PRESS  R  TO  RESTART",ox-21*8*sc15/2,oy+44,sc15,0.95f,0.95f,0.95f,pa*ft);
                    rT("M  -  BACK  TO  MENU", ox-20*8*sc15/2,oy+68,sc15,0.72f,0.72f,0.72f,ft); }
            }
            if(l2_state==WIN){
                float t=std::min(l2_overlayT*1.8f,1.0f),pul=0.7f+0.3f*sinf(l2_overlayT*3);
                float ox=SCR_WIDTH/2.0f,oy=SCR_HEIGHT/2.0f;
                float bcy=sinf(l2_overlayT*4)*5*(1-std::min(l2_overlayT,1.0f));
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.01f,0.06f,0.02f,0.70f*t);
                float pw=380*t,ph=260*t;
                rR(ox-pw/2,oy-ph/2+bcy,pw,ph,0.03f,0.12f,0.05f,0.97f);
                float bc=0.30f*pul;
                rR(ox-pw/2,oy-ph/2+bcy,pw,3,0.25f,0.88f,0.35f,bc+0.4f);
                rR(ox-pw/2,oy+ph/2-3+bcy,pw,3,0.25f,0.88f,0.35f,bc+0.4f);
                rR(ox-pw/2,oy-ph/2+bcy,3,ph,0.25f,0.88f,0.35f,bc+0.4f);
                rR(ox+pw/2-3,oy-ph/2+bcy,3,ph,0.25f,0.88f,0.35f,bc+0.4f);
                if(t>0.4f){ float ft=(t-0.4f)/0.6f;
                    float sc3=3.0f; std::string vt2="VICTORY!";
                    rT(vt2,ox-vt2.size()*8*sc3/2,oy-100+bcy,sc3,0.30f,0.98f,0.45f,ft);
                    float sc2=1.6f; std::string vs="MOSSY  ASCENT  CLEARED!";
                    rT(vs,ox-vs.size()*8*sc2/2,oy-52+bcy,sc2,0.65f,0.95f,0.68f,ft);
                    rR(ox-140,oy-25+bcy,280,2,0.25f,0.80f,0.32f,0.7f*ft);
                    float sc18=1.8f;
                    std::string sl="LIVES  REMAINING:  "+std::to_string(l2_lives);
                    rT(sl,ox-sl.size()*8*sc18/2,oy+5+bcy,sc18,0.55f,0.85f,0.58f,ft);
                    float dotX=ox-L2_MAX*6.0f;
                    for(int s2=0;s2<L2_MAX;s2++){ float g2=0.85f+0.15f*sinf(l2_overlayT*5+s2*0.5f);
                        rR(dotX+s2*12,oy+32+bcy,9,9,g2*0.98f,g2*0.85f,0.10f,ft); }
                    float sc15=1.5f,pa=0.5f+0.5f*sinf(l2_overlayT*2.5f);
                    rT("PRESS  R  TO  PLAY  AGAIN",ox-25*8*sc15/2,oy+58+bcy,sc15,0.80f,1.0f,0.82f,pa*ft);
                    rT("M  -  BACK  TO  MENU",ox-20*8*sc15/2,oy+82+bcy,sc15,0.70f,0.90f,0.72f,ft); }
            }
            glEnable(GL_DEPTH_TEST);
        }
        glfwSwapBuffers(win); glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO2); glDeleteBuffers(1,&VBO2b);
    glDeleteVertexArrays(1,&fVAO2); glDeleteBuffers(1,&fVBO2);
    glDeleteTextures(1,&fTex2); glDeleteTextures(1,&wTex2);
    glfwTerminate();
    return l2_backToMenu;
}

// =============================================

// =============================================
// LEVEL 3 - CANOPY RUSH
// Identik dengan lvl2: platform bergerak, lompat
// TAMBAHAN: A/D gerak, 1 musuh nembak, 30 tangga (lebih cepat & padat)
// =============================================
// =============================================
// LEVEL 3 - CANOPY RUSH
// State sepenuhnya TERPISAH dari Level 1 & 2.
// Tidak ada shared globals (player, walls, cameraPos, dll)
// Gameplay: naik 30 tangga + 1 musuh nembak. Hanya SPACE/lompat.
// =============================================

// (struct Bullet sudah dideklarasikan di Level 2 — dipakai bersama)

// ── State L3 (semua lokal ke file ini) ──
struct L3Player {
    float y=0,velY=0,standingY=0,animTimer=0;
    bool onGround=true;
} l3p;

struct L3Wall {
    float x,y,width,height,speedSign,delayTimer;
    int id;
    bool passed,alive,isFrozen,waitingDelay;
};

static std::vector<L3Wall>  l3_walls;
static std::vector<Bullet>  l3_bullets;
static float  l3_camY=2.5f, l3_camTargetY=2.5f;
static float  l3_playerX=0.0f;  // posisi X player (A/D)
static bool   l3_keysHeld[512]={};
static float  l3_wallSpeed=5.5f;
static bool   l3_spawnRight=false;
static int    l3_wallId=300;
static int    l3_wallsPassed=0;
static int    l3_lives=3;
static float  l3_hitCD=0,l3_overlayT=0,l3_gameTime=0;
static GameState l3_state=PLAYING;
static bool   l3_backToMenu=false;

// Enemy
static float l3_exX=0,l3_exVelX=2.2f,l3_exPhase=0,l3_exShoot=0;

const int   L3_MAX=30;
const float L3_GY =0.0f;  // ground Y

static void l3_spawnWall(float delayOverride=-1.0f){
    int activeCount=0;
    for(auto& w0:l3_walls) if(w0.alive&&!w0.isFrozen) activeCount++;
    if(activeCount>4) return; // max 5 wall AKTIF (belum diinjak) di layar
    L3Wall w;
    w.id=l3_wallId++;
    w.height=0.16f;
    int stairNum=w.id-300+1;
    w.width=9.0f-stairNum*0.35f; if(w.width<1.0f)w.width=1.0f;
    w.y=l3p.standingY+0.75f;
    w.speedSign=l3_spawnRight?-1.0f:1.0f;
    l3_spawnRight=!l3_spawnRight;
    float delay=delayOverride>=0?delayOverride:0.0f;
    w.waitingDelay=(delay>0);
    w.delayTimer=delay;
    w.x=w.waitingDelay?9999.0f:(w.speedSign<0?16.0f:-16.0f);
    w.passed=false; w.alive=true; w.isFrozen=false;
    l3_walls.push_back(w);
}

static void l3_fullReset(){
    l3p={};
    l3_walls.clear(); l3_bullets.clear();
    l3_camY=2.5f; l3_camTargetY=2.5f;
    l3_wallSpeed=5.5f; l3_spawnRight=false; l3_wallId=300;
    l3_wallsPassed=0; l3_lives=3;
    l3_hitCD=0; l3_overlayT=0; l3_gameTime=0;
    l3_state=PLAYING; l3_backToMenu=false;
    l3_playerX=0.0f; memset(l3_keysHeld,0,sizeof(l3_keysHeld));
    l3_exX=0; l3_exVelX=2.2f; l3_exPhase=0; l3_exShoot=0;
    l3_spawnWall(1.2f); // tangga pertama muncul setelah 1.2 detik (Level 3)
}

void l3_key_callback(GLFWwindow* w,int key,int,int action,int){
    if(key>=0&&key<512){ if(action==GLFW_PRESS) l3_keysHeld[key]=true; if(action==GLFW_RELEASE) l3_keysHeld[key]=false; }
    if(action==GLFW_PRESS){
        if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W)
            &&l3p.onGround&&l3_state==PLAYING){
            l3p.velY=JUMP_FORCE; l3p.onGround=false;
        }
        if((key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P)&&l3_state==PLAYING)  l3_state=PAUSED;
        else if((key==GLFW_KEY_ESCAPE||key==GLFW_KEY_P)&&l3_state==PAUSED) l3_state=PLAYING;
        if(key==GLFW_KEY_R&&(l3_state==GAME_OVER||l3_state==WIN||l3_state==PAUSED)) l3_fullReset();
        if(key==GLFW_KEY_M){ l3_backToMenu=true; glfwSetWindowShouldClose(w,GLFW_TRUE); }
    }
}

bool runGame3(){
    l3_fullReset();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,
        "Sylvan Odyssey - Level 3: Canopy Rush",NULL,NULL);
#if defined(_WIN32)||defined(_WIN64)
    { int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
      glfwSetWindowPos(win,(sw-(int)SCR_WIDTH)/2,(sh-(int)SCR_HEIGHT)/2); }
#else
    { GLFWmonitor* m=glfwGetPrimaryMonitor(); const GLFWvidmode* v=glfwGetVideoMode(m);
      if(v) glfwSetWindowPos(win,(v->width-(int)SCR_WIDTH)/2,(v->height-(int)SCR_HEIGHT)/2); }
#endif
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,framebuffer_size_callback);
    glfwSetKeyCallback(win,l3_key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    unsigned int sp3=makeShader(vertSrc,fragSrc);
    unsigned int shp3=makeShader(vertSrc,shadowFragSrc);
    unsigned int fp3=makeShader(fontVertSrc,fontFragSrc);

    // Font textures — OpenGL context baru, harus init ulang
    unsigned int fTex3=0,fVAO3=0,fVBO3=0,wTex3=0;
    {
        unsigned char atlas3[48][128]={};
        for(int c=0;c<96;c++){ int col=c%16,row=c/16;
            for(int y2=0;y2<8;y2++){ unsigned char bits=FONT8[c][y2];
                for(int x2=0;x2<8;x2++) if(bits&(0x80>>x2)) atlas3[row*8+y2][col*8+x2]=255; }}
        glGenTextures(1,&fTex3); glBindTexture(GL_TEXTURE_2D,fTex3);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RED,128,48,0,GL_RED,GL_UNSIGNED_BYTE,atlas3);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        unsigned char w1=255;
        glGenTextures(1,&wTex3); glBindTexture(GL_TEXTURE_2D,wTex3);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&w1);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glGenVertexArrays(1,&fVAO3); glGenBuffers(1,&fVBO3);
        glBindVertexArray(fVAO3); glBindBuffer(GL_ARRAY_BUFFER,fVBO3);
        glBufferData(GL_ARRAY_BUFFER,sizeof(float)*6*4,NULL,GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float))); glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    unsigned int VAO3,VBO3b;
    glGenVertexArrays(1,&VAO3); glGenBuffers(1,&VBO3b);
    glBindVertexArray(VAO3); glBindBuffer(GL_ARRAY_BUFFER,VBO3b);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    // ── Helpers 2D ──
    auto qd=[&](float x,float y,float w,float h,float u0,float v0,float u1,float v1){
        float vt[6][4]={{x,y+h,u0,v1},{x,y,u0,v0},{x+w,y,u1,v0},{x,y+h,u0,v1},{x+w,y,u1,v0},{x+w,y+h,u1,v1}};
        glBindVertexArray(fVAO3); glBindBuffer(GL_ARRAY_BUFFER,fVBO3);
        glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(vt),vt); glDrawArrays(GL_TRIANGLES,0,6); };
    auto rR=[&](float x,float y,float w,float h,float r,float g,float b,float a){
        glUseProgram(fp3); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,wTex3);
        glUniform1i(glGetUniformLocation(fp3,"tex"),0);
        glUniform4f(glGetUniformLocation(fp3,"color"),r,g,b,a); qd(x,y,w,h,0,0,1,1); };
    auto rT=[&](const std::string& s,float x,float y,float sc,float r=1,float g=1,float b=1,float a=1){
        glUseProgram(fp3); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,fTex3);
        glUniform1i(glGetUniformLocation(fp3,"tex"),0);
        glUniform4f(glGetUniformLocation(fp3,"color"),r,g,b,a);
        float cx=x;
        for(char c:s){ if(c<32||c>127)c=32; int idx=c-32,col=idx%16,row=idx/16;
            float u0=col/16.0f,u1=(col+1)/16.0f,v0=row*8/48.0f,v1=(row+1)*8/48.0f;
            float vt[6][4]={{cx,y+8*sc,u0,v1},{cx,y,u0,v0},{cx+8*sc,y,u1,v0},
                            {cx,y+8*sc,u0,v1},{cx+8*sc,y,u1,v0},{cx+8*sc,y+8*sc,u1,v1}};
            glBindVertexArray(fVAO3); glBindBuffer(GL_ARRAY_BUFFER,fVBO3);
            glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(vt),vt); glDrawArrays(GL_TRIANGLES,0,6);
            cx+=8*sc; } };
    auto rH=[&](float x,float y,float s,bool f){
        float r=f?0.95f:0.28f,g=f?0.15f:0.22f,b=f?0.18f:0.26f;
        rR(x+s,y,s,s,r,g,b,1); rR(x+4*s,y,s,s,r,g,b,1);
        rR(x,y+s,6*s,s,r,g,b,1); rR(x,y+2*s,6*s,s,r,g,b,1);
        rR(x+s,y+3*s,4*s,s,r,g,b,1); rR(x+2*s,y+4*s,2*s,s,r,g,b,1);
        rR(x+3*s,y+5*s,s,s,r,g,b,1); };
    auto sU=[&](unsigned int p,const glm::mat4& pj,const glm::mat4& vw){
        glUseProgram(p);
        glUniformMatrix4fv(glGetUniformLocation(p,"projection"),1,GL_FALSE,glm::value_ptr(pj));
        glUniformMatrix4fv(glGetUniformLocation(p,"view"),1,GL_FALSE,glm::value_ptr(vw)); };
    auto dC=[&](unsigned int p,glm::vec3 pos,glm::vec3 sc,glm::vec3 col,float a=1.0f){
        glUseProgram(p);
        glm::mat4 m=glm::translate(glm::mat4(1),pos)*glm::scale(glm::mat4(1),sc);
        glUniformMatrix4fv(glGetUniformLocation(p,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(p,"objectColor"),1,glm::value_ptr(col));
        glUniform1f(glGetUniformLocation(p,"alpha"),a);
        glBindVertexArray(VAO3); glDrawArrays(GL_TRIANGLES,0,36); };

    // Awan
    struct Cld{float x,y,z,spd;};
    std::vector<Cld> clouds;
    for(int i=0;i<5;i++) clouds.push_back({-12.0f+i*5.5f,5.5f+i*0.6f,-6.5f,0.4f+i*0.12f});

    float lf=(float)glfwGetTime();

    while(!glfwWindowShouldClose(win)){
        float now=(float)glfwGetTime();
        float dt=now-lf; if(dt>0.05f)dt=0.05f; lf=now;

        // ── UPDATE ──
        if(l3_state==PLAYING){
            l3_gameTime+=dt; l3p.animTimer+=dt;
            if(l3_hitCD>0) l3_hitCD-=dt;

            // A/D movement
            const float L3_MOVE=7.0f, L3_BOUND=7.5f;
            if(l3_keysHeld[GLFW_KEY_A]||l3_keysHeld[GLFW_KEY_LEFT])  l3_playerX-=L3_MOVE*dt;
            if(l3_keysHeld[GLFW_KEY_D]||l3_keysHeld[GLFW_KEY_RIGHT]) l3_playerX+=L3_MOVE*dt;
            if(l3_playerX< -L3_BOUND) l3_playerX=-L3_BOUND;
            if(l3_playerX>  L3_BOUND) l3_playerX= L3_BOUND;

            // Fisika player
            l3p.velY+=GRAVITY*dt;
            l3p.y+=l3p.velY*dt;
            bool localOn=false; float curFloor=L3_GY;

            // Ground
            if(l3p.y<=L3_GY){ l3p.y=L3_GY; l3p.velY=0; localOn=true; curFloor=L3_GY; }

            // Collision tangga
            const float LTOL=0.35f,STOL=0.15f;
            bool doSpawn=false;
            for(auto& w:l3_walls){
                if(!w.alive) continue;
                if(w.waitingDelay){
                    w.delayTimer-=dt;
                    if(w.delayTimer<=0){ w.waitingDelay=false; w.x=w.speedSign<0?16.0f:-16.0f; }
                    continue;
                }
                if(!w.isFrozen) w.x+=w.speedSign*l3_wallSpeed*dt;
                // Keluar layar
                if((w.speedSign<0&&w.x<-16)||(w.speedSign>0&&w.x>16)){
                    if(!w.isFrozen){ w.alive=false; } continue;
                }
                // X overlap (player di l3_playerX)
                if(fabs(l3_playerX-w.x)>=PLAYER_HW+w.width) continue;
                float pF=l3p.y, pH=l3p.y+PLAYER_H;
                float wT=w.y+w.height, wB=w.y-w.height;
                // Landing
                if(l3p.velY<=0&&pF>=wT-LTOL&&pF<=wT+0.15f){
                    if(!w.isFrozen&&!w.passed){
                        w.isFrozen=true; w.passed=true;
                        l3_wallsPassed++; doSpawn=true;
                    }
                    w.isFrozen=true;
                    l3p.y=wT; l3p.velY=0;
                    localOn=true; curFloor=wT;
                }
                // Side hit
                else if(!w.isFrozen&&!w.passed&&l3_hitCD<=0&&pF<wT-STOL&&pH>wB+STOL){
                    l3_lives--; w.isFrozen=true; w.alive=false; l3_hitCD=1.8f;
                    if(l3_lives<=0){ l3_state=GAME_OVER; l3_overlayT=0; }
                    // Spawn pengganti — player TIDAK reset posisi
                    else{ L3Wall rep=w; rep.passed=false; rep.alive=true; rep.isFrozen=false;
                          rep.waitingDelay=true; rep.delayTimer=1.0f; rep.x=9999;
                          l3_walls.push_back(rep); }
                }
            }
            l3_walls.erase(std::remove_if(l3_walls.begin(),l3_walls.end(),
                [](const L3Wall& w){return !w.alive&&!w.waitingDelay;}),l3_walls.end());

            l3p.onGround=localOn;
            if(localOn){ l3p.standingY=curFloor; l3p.y=curFloor; l3p.velY=0; }

            // WIN
            if(l3_wallsPassed>=L3_MAX&&l3p.onGround&&l3_state==PLAYING){
                l3_state=WIN; l3_overlayT=0;
            }

            // Spawn tangga berikutnya
            if(doSpawn&&l3_state==PLAYING) l3_spawnWall(0.15f);

            // Wall speed naik
            l3_wallSpeed=5.5f+l3_wallsPassed*0.35f;

            // Camera
            l3_camTargetY=2.5f+l3p.standingY*0.85f;
            l3_camY+=(l3_camTargetY-l3_camY)*4.0f*dt;

            // Clouds
            for(auto& c:clouds){ c.x-=c.spd*dt; if(c.x<-14)c.x=14; }

            // Enemy melayang di atas player
            l3_exX+=l3_exVelX*dt;
            if(l3_exX>5.0f){ l3_exX=5.0f; l3_exVelX=-fabs(l3_exVelX); }
            if(l3_exX<-5.0f){ l3_exX=-5.0f; l3_exVelX=fabs(l3_exVelX); }
            l3_exPhase+=dt*1.5f;
            float eY=l3p.standingY+5.5f+sinf(l3_exPhase)*0.4f;

            float intv=std::max(0.85f,2.3f-l3_wallsPassed*0.045f);
            l3_exShoot+=dt;
            if(l3_exShoot>=intv){
                l3_exShoot=0;
                float dx=l3_playerX-l3_exX, dy=(l3p.y+0.95f)-eY;
                float dist=sqrtf(dx*dx+dy*dy); if(dist<0.1f)dist=0.1f;
                float bspd=7.0f+l3_wallsPassed*0.18f;
                l3_bullets.push_back({l3_exX,eY,dx/dist*bspd,dy/dist*bspd,true});
            }

            // Peluru
            for(auto& b:l3_bullets){
                if(!b.alive) continue;
                b.x+=b.velX*dt; b.y+=b.velY*dt;
                if(fabs(b.x)>13||b.y<-2||b.y>50){ b.alive=false; continue; }
                if(l3_hitCD<=0){
                    float dx=b.x-l3_playerX, dy=b.y-(l3p.y+0.95f);
                    if(fabs(dx)<0.45f&&fabs(dy)<1.0f){
                        b.alive=false; l3_lives--; l3_hitCD=1.8f;
                        if(l3_lives<=0){ l3_state=GAME_OVER; l3_overlayT=0; }
                    }
                }
            }
            l3_bullets.erase(std::remove_if(l3_bullets.begin(),l3_bullets.end(),
                [](const Bullet& b){return !b.alive;}),l3_bullets.end());

        } else { l3_overlayT+=dt; }

        // ── RENDER ──
        float skyR=0.38f+l3p.standingY*0.018f;
        float skyG=0.62f+l3p.standingY*0.010f;
        float skyB=0.88f-l3p.standingY*0.008f;
        glClearColor(std::min(skyR,0.62f),std::min(skyG,0.80f),std::min(skyB,0.98f),1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        float asp=(float)SCR_WIDTH/SCR_HEIGHT;
        glm::vec3 camP(0,l3_camY,11.0f);
        glm::vec3 camF(0,-0.15f,-1.0f);
        glm::mat4 pj=glm::perspective(glm::radians(48.0f),asp,0.1f,100.0f);
        glm::mat4 vw=glm::lookAt(camP,camP+camF,glm::vec3(0,1,0));
        sU(sp3,pj,vw);
        glUniform3fv(glGetUniformLocation(sp3,"lightPos"),1,glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(sp3,"lightColor"),1,glm::value_ptr(lightColor));

        float cOY=l3_camY-2.5f;
        // Langit + matahari
        dC(sp3,glm::vec3(0,3.5f+cOY,-9),glm::vec3(22,16,0.3f),glm::vec3(0.55f,0.78f,0.98f));
        dC(sp3,glm::vec3(6.5f,6.8f+cOY,-7),glm::vec3(0.6f,0.6f,0.2f),glm::vec3(1,0.95f,0.35f));
        // Awan
        for(auto& c:clouds) dC(sp3,glm::vec3(c.x,c.y+cOY,c.z),glm::vec3(1.3f,0.45f,0.4f),glm::vec3(1,1,1));
        // Ground — warna ungu-hijau gelap (canopy, beda dari lvl1/lvl2)
        dC(sp3,glm::vec3(0,-0.3f,0),glm::vec3(15,0.3f,2.5f),glm::vec3(0.16f,0.30f,0.22f));
        dC(sp3,glm::vec3(0,0.02f,0),glm::vec3(15,0.05f,2.5f),glm::vec3(0.22f,0.46f,0.32f));

        // Bayangan player
        sU(shp3,pj,vw);
        float shadowF=L3_GY;
        for(auto& w:l3_walls)
            if(w.alive&&!w.waitingDelay&&(w.y+w.height)<=l3p.y+0.02f&&fabs(0.0f-w.x)<PLAYER_HW+w.width)
                shadowF=std::max(shadowF,w.y+w.height);
        drawHumanoid(shp3,glm::vec3(0,shadowF,0),0.58f,glm::vec3(0),glm::vec3(0),glm::vec3(0),0,0,true);

        // Tangga — gradient oranye→biru (beda dari lvl1)
        sU(sp3,pj,vw);
        for(auto& w:l3_walls){
            if(!w.alive||w.waitingDelay) continue;
            float f=(float)(w.id-300)/(float)L3_MAX;
            glm::vec3 wc=glm::mix(glm::vec3(0.92f,0.55f,0.10f),glm::vec3(0.10f,0.65f,0.90f),f);
            if(w.isFrozen) wc*=0.82f;
            dC(sp3,glm::vec3(w.x,w.y,0),glm::vec3(w.width,w.height,0.55f),wc);
            dC(sp3,glm::vec3(w.x,w.y+w.height,0),glm::vec3(w.width+0.02f,0.02f,0.58f),glm::clamp(wc*1.25f,glm::vec3(0),glm::vec3(1)));
        }

        // Player — selalu di X=0 (tengah)
        float eYr3=l3p.standingY+5.5f+sinf(l3_exPhase)*0.4f;
        float leg=l3p.onGround?sinf(l3p.animTimer*3.0f)*8.0f:-22.0f;
        float arm=l3p.onGround?sinf(l3p.animTimer*3.0f)*8.0f:-30.0f;
        glm::vec3 pcol=(l3_hitCD>0&&fmodf(l3_hitCD*6,1)>0.5f)?glm::vec3(1,0.3f,0.3f):glm::vec3(0.20f,0.55f,0.95f);
        // Leg animation: jalan kalau A/D ditekan
        bool l3Moving=l3_keysHeld[GLFW_KEY_A]||l3_keysHeld[GLFW_KEY_D]||l3_keysHeld[GLFW_KEY_LEFT]||l3_keysHeld[GLFW_KEY_RIGHT];
        leg=l3p.onGround?(l3Moving?sinf(l3p.animTimer*3.0f)*8.0f:0.0f):-22.0f;
        arm=l3p.onGround?(l3Moving?sinf(l3p.animTimer*3.0f)*8.0f:0.0f):-30.0f;
        drawHumanoid(sp3,glm::vec3(l3_playerX,l3p.y,0),0.58f,pcol,glm::vec3(0.96f,0.80f,0.62f),glm::vec3(0.12f,0.22f,0.60f),leg,arm,false);

        // Enemy merah
        float pulse=0.82f+0.18f*sinf(now*3.5f);
        glm::vec3 ec(0.88f,0.12f,0.12f);
        dC(sp3,glm::vec3(l3_exX,eYr3,0),glm::vec3(0.75f,0.45f,0.38f),ec*pulse);
        dC(sp3,glm::vec3(l3_exX-1.1f,eYr3+0.1f,0),glm::vec3(0.55f,0.22f,0.2f),ec*0.65f);
        dC(sp3,glm::vec3(l3_exX+1.1f,eYr3+0.1f,0),glm::vec3(0.55f,0.22f,0.2f),ec*0.65f);
        dC(sp3,glm::vec3(l3_exX-0.25f,eYr3+0.22f,0.43f),glm::vec3(0.10f,0.10f,0.1f),glm::vec3(1,1,1));
        dC(sp3,glm::vec3(l3_exX+0.25f,eYr3+0.22f,0.43f),glm::vec3(0.10f,0.10f,0.1f),glm::vec3(1,1,1));
        dC(sp3,glm::vec3(l3_exX-0.25f,eYr3+0.22f,0.45f),glm::vec3(0.06f,0.06f,0.1f),glm::vec3(0.05f,0.05f,0.05f));
        dC(sp3,glm::vec3(l3_exX+0.25f,eYr3+0.22f,0.45f),glm::vec3(0.06f,0.06f,0.1f),glm::vec3(0.05f,0.05f,0.05f));
        // Charge indicator
        float si3=std::min(l3_exShoot/std::max(0.85f,2.3f-l3_wallsPassed*0.045f),1.0f);
        dC(sp3,glm::vec3(l3_exX,eYr3-0.62f,0.5f),glm::vec3(0.12f*si3+0.02f,0.12f*si3+0.02f,0.05f),
           glm::mix(glm::vec3(0.95f,0.8f,0.1f),glm::vec3(0.95f,0.1f,0.1f),si3));

        // Peluru
        for(auto& b:l3_bullets){
            if(!b.alive) continue;
            dC(sp3,glm::vec3(b.x,b.y,0),glm::vec3(0.15f,0.15f,0.15f),glm::vec3(0.98f,0.72f,0.08f));
            dC(sp3,glm::vec3(b.x-b.velX*0.04f,b.y-b.velY*0.04f,0),glm::vec3(0.10f,0.10f,0.10f),glm::vec3(0.98f,0.45f,0.08f),0.45f);
        }

        // ── HUD 2D ──
        {
            glDisable(GL_DEPTH_TEST);
            glm::mat4 hp=glm::ortho(0.0f,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.0f,-1.0f,1.0f);
            glUseProgram(fp3);
            glUniformMatrix4fv(glGetUniformLocation(fp3,"proj"),1,GL_FALSE,glm::value_ptr(hp));
            float sc=2.0f,cw=8*sc,ch=8*sc;
            rR(0,0,(float)SCR_WIDTH,ch+14,0.03f,0.08f,0.04f,0.88f);
            rR(0,ch+12,(float)SCR_WIDTH,2,0.22f,0.68f,0.30f,0.60f);
            for(int h=0;h<3;h++) rH(10+h*32,5,sc*0.9f,h<l3_lives);
            std::string ss="STAIRS: "+std::to_string(l3_wallsPassed)+"/"+std::to_string(L3_MAX);
            rT(ss,SCR_WIDTH/2.0f-ss.size()*cw/2,5,sc,0.70f,0.98f,0.72f,1);
            float bW=160,bH=6,bX=SCR_WIDTH-bW-10,bY=ch+2;
            rR(bX,bY,bW,bH,0.08f,0.18f,0.09f,0.9f);
            float fill=(float)l3_wallsPassed/(float)L3_MAX;
            if(fill>0){ float fr=0.95f*(1-fill)+0.20f*fill,fg=0.25f*(1-fill)+0.90f*fill,fb=0.20f*(1-fill)+0.35f*fill;
                rR(bX,bY,bW*fill,bH,fr,fg,fb,1.0f); }
            rT("A/D=MOVE ESC=PAUSE",SCR_WIDTH-19*cw,5,sc,0.40f,0.60f,0.42f,0.8f);
            if(l3_hitCD>0){ float fl=fmodf(l3_hitCD*6,1)>0.5f?0.35f:0.0f;
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.9f,0.1f,0.1f,fl); }

            if(l3_state==PAUSED){
                float ox=SCR_WIDTH/2.0f,oy=SCR_HEIGHT/2.0f;
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.02f,0.06f,0.03f,0.72f);
                rR(ox-155,oy-105,310,220,0.04f,0.11f,0.06f,0.97f);
                rR(ox-155,oy-105,310,3,0.22f,0.72f,0.32f,0.85f); rR(ox-155,oy+112,310,3,0.22f,0.72f,0.32f,0.85f);
                rR(ox-155,oy-105,3,218,0.22f,0.72f,0.32f,0.85f); rR(ox+152,oy-105,3,218,0.22f,0.72f,0.32f,0.85f);
                float sc3=3.0f; std::string pt="PAUSED";
                rT(pt,ox-pt.size()*8*sc3/2,oy-88,sc3,0.32f,0.98f,0.48f,1);
                rR(ox-90,oy-42,180,2,0.22f,0.65f,0.30f,0.65f);
                float sc2=1.5f;
                rT("ESC / P  -  RESUME", ox-18*8*sc2/2,oy-22,sc2,0.72f,0.92f,0.74f,0.95f);
                rT("R        -  RESTART",ox-19*8*sc2/2,oy+ 5,sc2,0.72f,0.92f,0.74f,0.95f);
                rT("M        -  MENU",   ox-17*8*sc2/2,oy+32,sc2,0.72f,0.92f,0.74f,0.95f);
                rR(ox-90,oy+57,180,1,0.18f,0.45f,0.22f,0.5f);
                std::string di="STAIRS  "+std::to_string(l3_wallsPassed)+"/"+std::to_string(L3_MAX);
                std::string li="LIVES   "+std::to_string(l3_lives);
                rT(di,ox-di.size()*8*sc2/2,oy+68,sc2,0.50f,0.72f,0.52f,0.85f);
                rT(li,ox-li.size()*8*sc2/2,oy+90,sc2,0.50f,0.72f,0.52f,0.85f);
            }
            if(l3_state==GAME_OVER){
                float t=std::min(l3_overlayT*2.0f,1.0f),pul=0.6f+0.4f*sinf(l3_overlayT*4);
                float ox=SCR_WIDTH/2.0f,oy=SCR_HEIGHT/2.0f;
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.05f,0.01f,0.01f,0.75f*t);
                float pw=360*t,ph=230*t;
                rR(ox-pw/2,oy-ph/2,pw,ph,0.10f,0.02f,0.02f,0.97f);
                float bc=0.80f*pul;
                rR(ox-pw/2,oy-ph/2,pw,3,bc,0.08f,0.08f,0.9f); rR(ox-pw/2,oy+ph/2-3,pw,3,bc,0.08f,0.08f,0.9f);
                rR(ox-pw/2,oy-ph/2,3,ph,bc,0.08f,0.08f,0.9f); rR(ox+pw/2-3,oy-ph/2,3,ph,bc,0.08f,0.08f,0.9f);
                if(t>0.5f){ float ft=(t-0.5f)*2;
                    float sc3=3.0f; std::string got="GAME  OVER";
                    rT(got,ox-got.size()*8*sc3/2,oy-90,sc3,0.98f,0.18f,0.18f,ft);
                    float sc16=1.6f; std::string sub="THE  CANOPY  CLAIMED  YOU";
                    rT(sub,ox-sub.size()*8*sc16/2,oy-45,sc16,0.75f,0.40f,0.40f,ft*0.85f);
                    rR(ox-130,oy-28,260,2,0.70f,0.10f,0.10f,0.7f*ft);
                    float sc2=1.8f; std::string s1="STAIRS: "+std::to_string(l3_wallsPassed)+" / "+std::to_string(L3_MAX);
                    rT(s1,ox-s1.size()*8*sc2/2,oy-10,sc2,0.88f,0.62f,0.62f,ft);
                    float dotX=ox-L3_MAX*6.0f;
                    for(int s2=0;s2<L3_MAX;s2++){ float dc=s2<l3_wallsPassed?1.0f:0.22f;
                        rR(dotX+s2*12,oy+15,9,9,dc*0.98f,dc*0.82f,0.10f,ft); }
                    float sc15=1.5f,pa=0.5f+0.5f*sinf(l3_overlayT*3);
                    rT("PRESS  R  TO  RESTART",ox-21*8*sc15/2,oy+44,sc15,0.95f,0.95f,0.95f,pa*ft);
                    rT("M  -  BACK  TO  MENU", ox-20*8*sc15/2,oy+68,sc15,0.72f,0.72f,0.72f,ft); }
            }
            if(l3_state==WIN){
                float t=std::min(l3_overlayT*1.8f,1.0f),pul=0.7f+0.3f*sinf(l3_overlayT*3);
                float ox=SCR_WIDTH/2.0f,oy=SCR_HEIGHT/2.0f;
                float bcy=sinf(l3_overlayT*4)*5*(1-std::min(l3_overlayT,1.0f));
                rR(0,0,(float)SCR_WIDTH,(float)SCR_HEIGHT,0.01f,0.06f,0.02f,0.70f*t);
                float pw=380*t,ph=260*t;
                rR(ox-pw/2,oy-ph/2+bcy,pw,ph,0.03f,0.12f,0.05f,0.97f);
                float bc=0.30f*pul;
                rR(ox-pw/2,oy-ph/2+bcy,pw,3,0.25f,0.88f,0.35f,bc+0.4f);
                rR(ox-pw/2,oy+ph/2-3+bcy,pw,3,0.25f,0.88f,0.35f,bc+0.4f);
                rR(ox-pw/2,oy-ph/2+bcy,3,ph,0.25f,0.88f,0.35f,bc+0.4f);
                rR(ox+pw/2-3,oy-ph/2+bcy,3,ph,0.25f,0.88f,0.35f,bc+0.4f);
                if(t>0.4f){ float ft=(t-0.4f)/0.6f;
                    float sc3=3.0f; std::string vt2="VICTORY!";
                    rT(vt2,ox-vt2.size()*8*sc3/2,oy-100+bcy,sc3,0.30f,0.98f,0.45f,ft);
                    float sc2=1.6f; std::string vs="CANOPY  RUSH  CLEARED!";
                    rT(vs,ox-vs.size()*8*sc2/2,oy-52+bcy,sc2,0.65f,0.95f,0.68f,ft);
                    rR(ox-140,oy-25+bcy,280,2,0.25f,0.80f,0.32f,0.7f*ft);
                    float sc18=1.8f;
                    std::string sl="LIVES  REMAINING:  "+std::to_string(l3_lives);
                    rT(sl,ox-sl.size()*8*sc18/2,oy+5+bcy,sc18,0.55f,0.85f,0.58f,ft);
                    float dotX=ox-L3_MAX*6.0f;
                    for(int s2=0;s2<L3_MAX;s2++){ float g2=0.85f+0.15f*sinf(l3_overlayT*5+s2*0.5f);
                        rR(dotX+s2*12,oy+32+bcy,9,9,g2*0.98f,g2*0.85f,0.10f,ft); }
                    float sc15=1.5f,pa=0.5f+0.5f*sinf(l3_overlayT*2.5f);
                    rT("PRESS  R  TO  PLAY  AGAIN",ox-25*8*sc15/2,oy+58+bcy,sc15,0.80f,1.0f,0.82f,pa*ft);
                    rT("M  -  BACK  TO  MENU",ox-20*8*sc15/2,oy+82+bcy,sc15,0.70f,0.90f,0.72f,ft); }
            }
            glEnable(GL_DEPTH_TEST);
        }
        glfwSwapBuffers(win); glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO3); glDeleteBuffers(1,&VBO3b);
    glDeleteVertexArrays(1,&fVAO3); glDeleteBuffers(1,&fVBO3);
    glDeleteTextures(1,&fTex3); glDeleteTextures(1,&wTex3);
    glfwTerminate();
    return l3_backToMenu;
}
// WEBVIEW CALLBACKS
// =============================================
// Flags global
static std::atomic<bool> g_openLevelSelect{false};
static std::atomic<bool> g_startGame{false};
static std::atomic<bool> g_startLevel2{false};
static std::atomic<bool> g_startLevel3{false};
static std::atomic<bool> g_backToMenu{false};
static std::atomic<bool> g_exit{false};

// Progress flags — persistent selama sesi berjalan
static bool g_level1Cleared = false;
static bool g_level2Cleared = false;
static bool g_level3Cleared = false;

// webview.h lama Windows pakai external_invoke_cb dengan notify()
void onMenuInvoke(struct webview* w, const char* arg){
    std::string cmd(arg);
    if(cmd=="openLevelSelect"){ g_openLevelSelect=true; webview_terminate(w); }
    else if(cmd=="options"){
        webview_eval(w,"alert('HOW TO PLAY\\n\\nSPACE / W / UP  =  Jump\\n\\nLompat ke platform yang bergerak untuk naik tangga.\\nHindari platform yang menabrak dari samping!\\n\\nESC / P  =  Pause\\nR        =  Restart\\nM        =  Back to Menu');");
    }
    else if(cmd=="exit"){ g_exit=true; webview_terminate(w); }
}

void onLevelSelectInvoke(struct webview* w, const char* arg){
    std::string cmd(arg);
    if(cmd == "startLevel1"){
        g_startGame = true;
        webview_terminate(w);
    } else if(cmd == "startLevel2"){
        g_startLevel2 = true;
        webview_terminate(w);
    } else if(cmd == "startLevel3"){
        g_startLevel3 = true;
        webview_terminate(w);
    } else if(cmd == "backToMenu"){
        g_backToMenu = true;
        webview_terminate(w);
    }
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
        g_openLevelSelect = false;
        g_exit            = false;

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

        if(g_exit)            return 0;
        if(!g_openLevelSelect){ running = false; break; }

#if defined(_WIN32) || defined(_WIN64)
        Sleep(120);
#else
        usleep(120000);
#endif

        // ─── LEVEL SELECT LOOP ───────────────────────
        bool goLevelSelect = true;
        while(goLevelSelect){
            g_startGame  = false;
            g_startLevel2 = false;
            g_startLevel3 = false;
            g_backToMenu = false;

            // Bangun HTML level select fresh setiap loop agar status unlock terupdate
            std::string levelHtml    = buildLevelHTML(g_level1Cleared, g_level2Cleared, g_level3Cleared);
            std::string levelDataUrl = makeDataUrl(levelHtml.c_str());

            struct webview lvlwv = {};
            lvlwv.title              = "Sylvan Odyssey - Levels";
            lvlwv.url                = levelDataUrl.c_str();
            lvlwv.width              = SCR_WIDTH;
            lvlwv.height             = SCR_HEIGHT;
            lvlwv.resizable          = 0;
            lvlwv.debug              = 0;
            lvlwv.external_invoke_cb = onLevelSelectInvoke;
            if(webview_init(&lvlwv) != 0) return 1;
            centerWebview(&lvlwv);
            while(webview_loop(&lvlwv, 1) == 0){}
            webview_exit(&lvlwv);

#if defined(_WIN32) || defined(_WIN64)
            Sleep(120);
#else
            usleep(120000);
#endif

            if(g_backToMenu){
                goLevelSelect = false;
                break;
            }

            // Sembunyikan webview sebelum game muncul
#if defined(_WIN32)||defined(_WIN64)
            { HWND h=FindWindowA(NULL,"Sylvan Odyssey"); if(h) ShowWindow(h,SW_HIDE); Sleep(150); }
#else
            usleep(150000);
#endif
            if(g_startGame){
                bool goMenu = runGame();
                // Cek apakah level 1 berhasil diselesaikan (WIN state)
                if(gameState == WIN){
                    g_level1Cleared = true;
                }
                goLevelSelect = goMenu;
                if(!goMenu) running = false;
            } else if(g_startLevel2){
                bool goMenu = runGame2();
                // Cek apakah level 2 berhasil diselesaikan
                if(l2_state == WIN){
                    g_level2Cleared = true;
                }
                goLevelSelect = goMenu;
                if(!goMenu) running = false;
            } else if(g_startLevel3){
                bool goMenu = runGame3();
                // Cek apakah level 3 berhasil diselesaikan
                if(l3_state == WIN){
                    g_level3Cleared = true;
                }
                goLevelSelect = goMenu;
                if(!goMenu) running = false;
            } else {
                goLevelSelect = false;
                running       = false;
            }
        } // end while(goLevelSelect)
    } // end while(running)

    return 0;
}