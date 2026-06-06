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
      <button class="btn" onclick="window.external.invoke('options');">Options</button>
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

static const char* LEVEL_HTML = R"HTML(<!DOCTYPE html>
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
.card.locked{opacity:0.4;cursor:not-allowed;}
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
    <div class="card unlocked" onclick="this.style.opacity='0.6';window.external.invoke('startLevel1');">
      <div class="lvl-num">1</div>
      <div class="info">
        <div class="lvl-name">Verdant Hollow</div>
        <div class="lvl-desc">Lembah hijau - pelajari dasar melompat &amp; menghindari rintangan</div>
      </div>
      <div class="right"><span class="badge current">Mulai di sini</span></div>
    </div>
    <div class="card locked">
      <div class="lvl-num">2</div>
      <div class="info"><div class="lvl-name">Mossy Ascent</div><div class="lvl-desc">Selesaikan Level 1 untuk membuka</div></div>
      <div class="right"><span class="badge locked-b">Terkunci</span></div>
    </div>
    <div class="card locked">
      <div class="lvl-num">3</div>
      <div class="info"><div class="lvl-name">Canopy Rush</div><div class="lvl-desc">Selesaikan Level 2 untuk membuka</div></div>
      <div class="right"><span class="badge locked-b">Terkunci</span></div>
    </div>
    <div class="card locked">
      <div class="lvl-num">4</div>
      <div class="info"><div class="lvl-name">Thornwood Surge</div><div class="lvl-desc">Selesaikan Level 3 untuk membuka</div></div>
      <div class="right"><span class="badge locked-b">Terkunci</span></div>
    </div>
    <div class="card locked">
      <div class="lvl-num">5</div>
      <div class="info"><div class="lvl-name">The Great Ascent</div><div class="lvl-desc">Selesaikan Level 4 untuk membuka</div></div>
      <div class="right"><span class="badge locked-b">Terkunci</span></div>
    </div>
  </div>
  <div class="footer-text">STMIK AMIK Bandung &copy; 2026</div>
</div>
<script>
var sts=document.getElementById('stars');
for(var i=0;i<55;i++){var s=document.createElement('div');var sz=1+Math.random()*2;s.style.cssText='position:absolute;top:'+Math.random()*100+'%;left:'+Math.random()*100+'%;width:'+sz+'px;height:'+sz+'px;background:#fff;border-radius:50%;--dur:'+(2+Math.random()*3)+'s;--min:'+(0.1+Math.random()*0.2)+';--max:'+(0.5+Math.random()*0.4)+';animation:twinkle var(--dur) ease-in-out infinite alternate;';sts.appendChild(s);}
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
enum GameState { PLAYING, GAME_OVER, WIN };
GameState gameState = PLAYING;

float deltaTime   = 0.0f;
float lastFrame   = 0.0f;
float gameTime    = 0.0f;
int   score       = 0;
int   wallsPassed = 0;
const int MAX_STAIRS = 10;

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
    spawnFromRight    = false;
    cameraPos.y       = 2.5f;
    cameraTargetY     = 2.5f;

    spawnWall(false, 1.2f);
}

// =============================================
// CALLBACKS (GLFW)
// =============================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(action == GLFW_PRESS){
        if((key==GLFW_KEY_SPACE||key==GLFW_KEY_UP||key==GLFW_KEY_W) && player.onGround && gameState==PLAYING){
            player.velY     = JUMP_FORCE;
            player.onGround = false;
        }
        if(key==GLFW_KEY_R && (gameState==GAME_OVER || gameState==WIN)) resetGame();
    }
}

void framebuffer_size_callback(GLFWwindow* w, int width, int height){
    glViewport(0,0,width,height);
}

// =============================================
// RUN GAME (dipanggil setelah menu ditutup)
// =============================================
void runGame(){
    srand((unsigned)time(0));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sylvan Odyssey", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaderProg = makeShader(vertSrc, fragSrc);
    shadowProg = makeShader(vertSrc, shadowFragSrc);

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
                else if(!w.isFrozen && playerFoot < platformTop - SIDE_TOL && playerHead > platformBot + SIDE_TOL) {
                    gameState = GAME_OVER;
                }
            }

            player.onGround = localOnGround;
            if(player.onGround) {
                player.standingY = currentFloor;
                player.pos.y     = player.standingY;
                player.velY      = 0.0f;
            }

            if(wallsPassed >= MAX_STAIRS && player.onGround && player.standingY > GROUND_Y) {
                gameState = WIN;
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
        }

        for(auto& c : clouds){
            c.x -= c.speed * deltaTime;
            if(c.x < -13.0f) c.x = 14.0f;
        }

        std::string title;
        if(gameState == PLAYING)
            title = "Sylvan Odyssey  |  Stairs: " + std::to_string(wallsPassed) + "/" + std::to_string(MAX_STAIRS);
        else if(gameState == WIN)
            title = "VICTORY! You Escaped!  |  Press 'R' to Play Again";
        else
            title = "GAME OVER! Failed at Stairs: " + std::to_string(wallsPassed) + "  |  Press 'R' to Restart";
        glfwSetWindowTitle(window, title.c_str());

        glClearColor(0.48f, 0.76f, 0.95f, 1.0f);
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

        if(gameState == WIN){
            drawCube(shaderProg, glm::vec3(0.0f, cameraPos.y + 0.5f, 2.0f), glm::vec3(0.5f, 0.5f, 0.1f), glm::vec3(0.2f, 0.9f, 0.3f), 0.8f);
        } else if(gameState == GAME_OVER){
            for(int i = -2; i <= 2; i++){
                float fi = (float)i * 0.4f;
                drawCube(shaderProg, glm::vec3(fi, fi + cameraPos.y, 2.0f), glm::vec3(0.15f, 0.15f, 0.1f), glm::vec3(0.95f, 0.1f, 0.1f), 0.90f);
                drawCube(shaderProg, glm::vec3(fi, -fi + cameraPos.y, 2.0f), glm::vec3(0.15f, 0.15f, 0.1f), glm::vec3(0.95f, 0.1f, 0.1f), 0.90f);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glfwTerminate();
}

// =============================================
// WEBVIEW CALLBACKS
// =============================================
// Flags global
static std::atomic<bool> g_openLevelSelect{false};
static std::atomic<bool> g_startGame{false};
static std::atomic<bool> g_backToMenu{false};
static std::atomic<bool> g_exit{false};

// webview.h lama Windows pakai external_invoke_cb dengan notify()
void onMenuInvoke(struct webview* w, const char* arg){
    std::string cmd(arg);
    if(cmd == "openLevelSelect"){
        g_openLevelSelect = true;
        webview_terminate(w);
    } else if(cmd == "exit"){
        g_exit = true;
        webview_terminate(w);
    }
    // "options" diabaikan untuk sekarang
}

void onLevelSelectInvoke(struct webview* w, const char* arg){
    std::string cmd(arg);
    if(cmd == "startLevel1"){
        g_startGame = true;
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
// MAIN
// =============================================
int main(){
    // Pre-build data URLs (dilakukan sekali di awal)
    std::string menuDataUrl  = makeDataUrl(MENU_HTML);
    std::string levelDataUrl = makeDataUrl(LEVEL_HTML);

    bool running = true;
    while(running){
        // Reset flags
        g_openLevelSelect = false;
        g_backToMenu      = false;

        // --- Tampilkan Main Menu (HTML inline via data: URI) ---
        struct webview menu = {};
        menu.title              = "Sylvan Odyssey";
        menu.url                = menuDataUrl.c_str();
        menu.width              = SCR_WIDTH;
        menu.height             = SCR_HEIGHT;
        menu.resizable          = 0;
        menu.debug              = 0;
        menu.external_invoke_cb = onMenuInvoke;
        if(webview_init(&menu) != 0) return 1;
        while(webview_loop(&menu, 1) == 0){}
        webview_exit(&menu); // tutup bersih sebelum buka window berikutnya

        if(g_exit) return 0;
        if(!g_openLevelSelect) return 0;

#if defined(_WIN32) || defined(_WIN64)
        Sleep(150);
#else
        usleep(150000);
#endif

        // --- Tampilkan Level Select ---
        g_startGame  = false;
        g_backToMenu = false;
        struct webview lvlwv = {};
        lvlwv.title              = "Sylvan Odyssey";
        lvlwv.url                = levelDataUrl.c_str();
        lvlwv.width              = SCR_WIDTH;
        lvlwv.height             = SCR_HEIGHT;
        lvlwv.resizable          = 0;
        lvlwv.debug              = 0;
        lvlwv.external_invoke_cb = onLevelSelectInvoke;
        if(webview_init(&lvlwv) != 0) return 1;
        while(webview_loop(&lvlwv, 1) == 0){}
        webview_exit(&lvlwv); // tutup bersih

#if defined(_WIN32) || defined(_WIN64)
        Sleep(150);
#else
        usleep(150000);
#endif

        if(g_backToMenu){
            continue; // kembali ke loop atas -> tampilkan menu lagi
        }

        if(g_startGame){
            runGame();
        }

        running = false;
    }

    return 0;
}