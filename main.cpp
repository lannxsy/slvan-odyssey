// ============================================================
//  Sylvan Odyssey — ZOI Style Complete
//  Kamera ortho front-view, background texture, UI screen proper
// ============================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image inline
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

// ============================================================
const int   SCR_W    = 1280, SCR_H = 720;
const float AREA_W   = 10.f;   // play area half-width  (-5 .. +5)
const float AREA_H   = 7.f;    // play area half-height (-3.5 .. +3.5)
const float FLOOR_Y  = -3.0f;
const float CEIL_Y   =  3.2f;
const float SCORE_L2 = 500.f;
const float SCORE_L3 = 1000.f;
const float SCORE_WIN= 1500.f;

// ============================================================
//  Input
// ============================================================
bool keys[1024]={}, keyHit[1024]={};
static void cbKey(GLFWwindow*,int k,int,int a,int){
    if(k<0||k>=1024)return;
    if(a==GLFW_PRESS){keys[k]=true;keyHit[k]=true;}
    if(a==GLFW_RELEASE)keys[k]=false;
}

// ============================================================
//  Shader / Program
// ============================================================
static GLuint compSh(GLenum t,const char* s){
    GLuint sh=glCreateShader(t); glShaderSource(sh,1,&s,nullptr); glCompileShader(sh);
    int ok; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){char b[512];glGetShaderInfoLog(sh,512,nullptr,b);std::cerr<<"SH:"<<b<<"\n";}
    return sh;
}
static GLuint mkProg(const char* vs,const char* fs){
    GLuint v=compSh(GL_VERTEX_SHADER,vs),f=compSh(GL_FRAGMENT_SHADER,fs);
    GLuint p=glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f); return p;
}
static GLuint loadProg(const char* vf,const char* ff){
    auto rd=[](const char* p){std::ifstream f(p);std::stringstream s;s<<f.rdbuf();return s.str();};
    auto vs=rd(vf),fs=rd(ff); return mkProg(vs.c_str(),fs.c_str());
}

// ============================================================
//  OBJ / MTL loader
// ============================================================
struct Vtx{glm::vec3 pos,nrm;};
struct SubMesh{std::string mat;std::vector<Vtx> verts;GLuint VAO=0,VBO=0;};
struct Mat{glm::vec3 Kd{0.7f};};
static std::map<std::string,Mat> g_mats;

static std::map<std::string,Mat> loadMTL(const std::string& p){
    std::map<std::string,Mat> m; std::ifstream f(p); if(!f)return m;
    std::string ln,cur;
    while(std::getline(f,ln)){
        if(!ln.empty()&&ln.back()=='\r')ln.pop_back();
        std::istringstream ss(ln); std::string t; ss>>t;
        if(t=="newmtl"){ss>>cur;m[cur]={};}
        else if(t=="Kd"&&!cur.empty())ss>>m[cur].Kd.r>>m[cur].Kd.g>>m[cur].Kd.b;
    }
    return m;
}
static std::vector<SubMesh> loadOBJ(const std::string& path,const std::string& base){
    std::ifstream f(path); if(!f){std::cerr<<"No OBJ:"<<path<<"\n";return{};}
    std::vector<glm::vec3> pos,nrm; std::vector<SubMesh> ms;
    std::string curMat="default";
    {std::ifstream tmp(path);std::string ln;
     while(std::getline(tmp,ln)){if(ln.size()>6&&ln.substr(0,6)=="mtllib"){
         std::string n=ln.substr(7);
         while(!n.empty()&&(n.back()=='\r'||n.back()=='\n'||n.back()==' '))n.pop_back();
         g_mats=loadMTL(base+n);break;}}}
    ms.push_back({}); ms.back().mat=curMat;
    std::string ln;
    while(std::getline(f,ln)){
        if(!ln.empty()&&ln.back()=='\r')ln.pop_back();
        std::istringstream ss(ln); std::string t; ss>>t;
        if(t=="v"){glm::vec3 p;ss>>p.x>>p.y>>p.z;pos.push_back(p);}
        else if(t=="vn"){glm::vec3 n;ss>>n.x>>n.y>>n.z;nrm.push_back(n);}
        else if(t=="usemtl"){ss>>curMat;
            if(!ms.back().verts.empty()){ms.push_back({});ms.back().mat=curMat;}
            else ms.back().mat=curMat;}
        else if(t=="f"){
            std::vector<std::pair<int,int>> fi; std::string tok;
            while(ss>>tok){int vi=0,ni=0;
                auto s1=tok.find('/'); vi=std::stoi(tok.substr(0,s1));
                if(s1!=std::string::npos){auto s2=tok.find('/',s1+1);
                    if(s2!=std::string::npos&&s2+1<tok.size())ni=std::stoi(tok.substr(s2+1));}
                fi.push_back({vi,ni});}
            for(int i=1;i+1<(int)fi.size();i++)
                for(int k:{0,i,i+1}){Vtx vx;
                    int vi=fi[k].first-1,ni=fi[k].second-1;
                    vx.pos=(vi>=0&&vi<(int)pos.size())?pos[vi]:glm::vec3(0);
                    vx.nrm=(ni>=0&&ni<(int)nrm.size())?nrm[ni]:glm::vec3(0,1,0);
                    ms.back().verts.push_back(vx);}
        }
    }
    for(auto& m:ms){if(m.verts.empty())continue;
        glGenVertexArrays(1,&m.VAO);glGenBuffers(1,&m.VBO);
        glBindVertexArray(m.VAO);glBindBuffer(GL_ARRAY_BUFFER,m.VBO);
        glBufferData(GL_ARRAY_BUFFER,m.verts.size()*sizeof(Vtx),m.verts.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)0);glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)offsetof(Vtx,nrm));glEnableVertexAttribArray(1);
        glBindVertexArray(0);}
    return ms;
}

// ============================================================
//  Primitives
// ============================================================
GLuint g_cubeVAO,g_cubeVBO,g_sphVAO,g_sphVBO,g_quadVAO,g_quadVBO;
int g_sphN=0;

static void buildCube(){
    float v[]={
        -.5f,-.5f,-.5f,0,0,-1,.5f,-.5f,-.5f,0,0,-1,.5f,.5f,-.5f,0,0,-1,.5f,.5f,-.5f,0,0,-1,-.5f,.5f,-.5f,0,0,-1,-.5f,-.5f,-.5f,0,0,-1,
        -.5f,-.5f,.5f,0,0,1,.5f,-.5f,.5f,0,0,1,.5f,.5f,.5f,0,0,1,.5f,.5f,.5f,0,0,1,-.5f,.5f,.5f,0,0,1,-.5f,-.5f,.5f,0,0,1,
        -.5f,.5f,.5f,-1,0,0,-.5f,.5f,-.5f,-1,0,0,-.5f,-.5f,-.5f,-1,0,0,-.5f,-.5f,-.5f,-1,0,0,-.5f,-.5f,.5f,-1,0,0,-.5f,.5f,.5f,-1,0,0,
        .5f,.5f,.5f,1,0,0,.5f,.5f,-.5f,1,0,0,.5f,-.5f,-.5f,1,0,0,.5f,-.5f,-.5f,1,0,0,.5f,-.5f,.5f,1,0,0,.5f,.5f,.5f,1,0,0,
        -.5f,-.5f,-.5f,0,-1,0,.5f,-.5f,-.5f,0,-1,0,.5f,-.5f,.5f,0,-1,0,.5f,-.5f,.5f,0,-1,0,-.5f,-.5f,.5f,0,-1,0,-.5f,-.5f,-.5f,0,-1,0,
        -.5f,.5f,-.5f,0,1,0,.5f,.5f,-.5f,0,1,0,.5f,.5f,.5f,0,1,0,.5f,.5f,.5f,0,1,0,-.5f,.5f,.5f,0,1,0,-.5f,.5f,-.5f,0,1,0,
    };
    glGenVertexArrays(1,&g_cubeVAO);glGenBuffers(1,&g_cubeVBO);
    glBindVertexArray(g_cubeVAO);glBindBuffer(GL_ARRAY_BUFFER,g_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);
}
static void buildSphere(int st=14,int sl=14){
    std::vector<float> flat;
    for(int i=0;i<st;i++)for(int j=0;j<sl;j++){
        auto pt=[&](int ii,int jj){float p=glm::pi<float>()*ii/st,t=2*glm::pi<float>()*jj/sl;
            glm::vec3 n(sinf(p)*cosf(t),cosf(p),sinf(p)*sinf(t));
            flat.insert(flat.end(),{n.x,n.y,n.z,n.x,n.y,n.z});};
        pt(i,j);pt(i+1,j);pt(i+1,j+1);pt(i,j);pt(i+1,j+1);pt(i,j+1);}
    g_sphN=(int)flat.size()/6;
    glGenVertexArrays(1,&g_sphVAO);glGenBuffers(1,&g_sphVBO);
    glBindVertexArray(g_sphVAO);glBindBuffer(GL_ARRAY_BUFFER,g_sphVBO);
    glBufferData(GL_ARRAY_BUFFER,flat.size()*sizeof(float),flat.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);
}
// Fullscreen quad (for background)
static void buildQuad(){
    float v[]={-1,-1,0,0,0,1, 1,-1,0,0,0,1, 1,1,0,0,0,1, -1,-1,0,0,0,1, 1,1,0,0,0,1, -1,1,0,0,0,1};
    glGenVertexArrays(1,&g_quadVAO);glGenBuffers(1,&g_quadVBO);
    glBindVertexArray(g_quadVAO);glBindBuffer(GL_ARRAY_BUFFER,g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);
}

// ============================================================
//  Draw helpers
// ============================================================
GLuint g_prog=0;
static void uM4(const char* n,glm::mat4 m){glUniformMatrix4fv(glGetUniformLocation(g_prog,n),1,GL_FALSE,glm::value_ptr(m));}
static void u3f(const char* n,glm::vec3 v){glUniform3fv(glGetUniformLocation(g_prog,n),1,glm::value_ptr(v));}
static void u1f(const char* n,float v){glUniform1f(glGetUniformLocation(g_prog,n),v);}

static void dCube(glm::mat4 m,glm::vec3 c,float fl=0){
    uM4("model",m);u3f("objectColor",c);u1f("flashIntensity",fl);u1f("fogStrength",0);
    glBindVertexArray(g_cubeVAO);glDrawArrays(GL_TRIANGLES,0,36);}
static void dSphere(glm::mat4 m,glm::vec3 c,float fl=0){
    uM4("model",m);u3f("objectColor",c);u1f("flashIntensity",fl);u1f("fogStrength",0);
    glBindVertexArray(g_sphVAO);glDrawArrays(GL_TRIANGLES,0,g_sphN);}
static void dModel(const std::vector<SubMesh>& ms,glm::mat4 m,glm::vec3 fog={0,0,0},float fogS=0,float fl=0){
    uM4("model",m);u1f("flashIntensity",fl);u3f("zoneFogColor",fog);u1f("fogStrength",fogS);
    for(const auto& sm:ms){if(!sm.VAO||sm.verts.empty())continue;
        glm::vec3 c(0.7f);auto it=g_mats.find(sm.mat);if(it!=g_mats.end())c=it->second.Kd;
        u3f("objectColor",c);glBindVertexArray(sm.VAO);glDrawArrays(GL_TRIANGLES,0,(GLsizei)sm.verts.size());}
}

// ============================================================
//  Background shader + texture
// ============================================================
GLuint g_bgProg=0, g_bgTex=0, g_bgTex2=0, g_bgTex3=0;
const char* BG_VS=R"(#version 330 core
layout(location=0)in vec3 aPos;
out vec2 vUV;
void main(){vUV=(aPos.xy+1.0)*0.5;gl_Position=vec4(aPos,1.0);})";
const char* BG_FS=R"(#version 330 core
in vec2 vUV;out vec4 FragColor;
uniform sampler2D tex;
uniform float alpha;
uniform vec3 tint;
void main(){vec4 t=texture(tex,vUV);FragColor=vec4(mix(t.rgb,tint,0.25)*alpha+(1.0-alpha)*tint,1.0);})";

static GLuint loadTex(const char* path){
    int w,h,c; stbi_set_flip_vertically_on_load(true);
    unsigned char* d=stbi_load(path,&w,&h,&c,0);
    if(!d){std::cerr<<"Tex fail:"<<path<<"\n";return 0;}
    GLuint tex; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
    GLenum fmt=(c==4)?GL_RGBA:GL_RGB;
    glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,d);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(d); return tex;
}

static void drawBackground(int level, float flashT){
    glDisable(GL_DEPTH_TEST);
    glUseProgram(g_bgProg);
    GLuint tex = (level==1)?g_bgTex:(level==2)?g_bgTex2:g_bgTex3;
    glm::vec3 tint=(level==1)?glm::vec3(0,0.1f,0.3f):(level==2)?glm::vec3(0.1f,0.3f,0.05f):glm::vec3(0.02f,0,0.1f);
    if(flashT>0) tint=glm::mix(tint,glm::vec3(1),glm::clamp(flashT/0.12f,0.f,1.f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,tex?tex:0);
    glUniform1i(glGetUniformLocation(g_bgProg,"tex"),0);
    glUniform1f(glGetUniformLocation(g_bgProg,"alpha"),tex?1.0f:0.0f);
    glUniform3fv(glGetUniformLocation(g_bgProg,"tint"),1,glm::value_ptr(tint));
    glBindVertexArray(g_quadVAO); glDrawArrays(GL_TRIANGLES,0,6);
    glEnable(GL_DEPTH_TEST);
}

// ============================================================
//  Shadow
// ============================================================
const int SHW=2048; GLuint g_sFBO,g_sTex,g_dProg;
static void initShadow(){
    glGenFramebuffers(1,&g_sFBO);glGenTextures(1,&g_sTex);
    glBindTexture(GL_TEXTURE_2D,g_sTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,SHW,SHW,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    float bc[]={1,1,1,1};glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,bc);
    glBindFramebuffer(GL_FRAMEBUFFER,g_sFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,g_sTex,0);
    glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);glBindFramebuffer(GL_FRAMEBUFFER,0);
    const char* dv=R"(#version 330 core
layout(location=0)in vec3 aPos;
uniform mat4 lightSpaceMatrix,model;
void main(){gl_Position=lightSpaceMatrix*model*vec4(aPos,1.0);})";
    const char* df=R"(#version 330 core
void main(){})";
    g_dProg=mkProg(dv,df);
}

// ============================================================
//  Game data
// ============================================================
enum class GS{INTRO,PLAYING,PAUSED,GAMEOVER,WIN};
GS g_gs=GS::INTRO;
int g_level=1;
float g_score=0,g_speed=0,g_spawnT=0,g_flashT=0,g_gameTime=0,g_dispScore=0;
bool g_hasWand=false;

struct Player{float x=0,y=FLOOR_Y,velY=0,invT=0,shootCD=0;int hearts=3;}g_pl;

struct Wall{float y,gapCX,gapHW,thick;bool active=true;int lv;};
struct Orb{float x,y,vx,vy;bool active=true;bool enemy=false;};
struct Boss{float x=0,y=2.5f,vx=0.05f,shootCD=2.f,flashT=0;int hp=8;bool active=false;}g_boss;

std::vector<Wall> g_walls;
std::vector<Orb>  g_orbs;

static float rng(float a,float b){return a+(b-a)*((float)rand()/RAND_MAX);}

// ============================================================
//  Game logic
// ============================================================
static void hitPlayer(){
    if(g_pl.invT>0)return;
    g_pl.hearts--; g_pl.invT=1.8f;
    if(g_pl.hearts<=0){g_gs=GS::GAMEOVER;}
}

static void resetGame(){
    g_level=1;g_score=0;g_speed=2.5f;g_hasWand=false;
    g_pl={};g_pl.hearts=3;
    g_walls.clear();g_orbs.clear();
    g_boss={};
    g_spawnT=0;g_flashT=0;g_gameTime=0;g_dispScore=0;
    g_gs=GS::PLAYING;
}

static void spawnWall(){
    Wall w;
    w.y=CEIL_Y+0.4f;
    float diff=glm::clamp(g_score/SCORE_WIN,0.f,1.f);
    w.gapHW=glm::mix(2.5f,1.2f,diff);
    w.gapCX=rng(-AREA_W+w.gapHW+0.5f, AREA_W-w.gapHW-0.5f);
    w.thick=0.5f; w.active=true; w.lv=g_level;
    g_walls.push_back(w);
}

static bool onGround(){ return g_pl.y<=FLOOR_Y+0.05f; }

static void update(float dt){
    if(g_gs!=GS::PLAYING)return;
    g_gameTime+=dt;
    g_flashT=glm::max(0.f,g_flashT-dt);
    g_pl.invT=glm::max(0.f,g_pl.invT-dt);
    g_pl.shootCD=glm::max(0.f,g_pl.shootCD-dt);

    // Score & speed
    g_score+=45.f*dt;
    g_dispScore+=(g_score-g_dispScore)*8.f*dt;
    g_speed=2.5f+g_score/SCORE_WIN*4.f;

    // Level up
    if(g_score>=SCORE_L2&&g_level==1){
        g_level=2; g_hasWand=true;
        std::cout<<"[LEVEL 2] Hutan! SPACE=loncat, dapat tongkat!\n";
    }
    if(g_score>=SCORE_L3&&g_level==2){
        g_level=3; g_boss.active=true;
        std::cout<<"[LEVEL 3] Langit! Boss muncul!\n";
    }
    if(g_score>=SCORE_WIN&&!g_boss.active&&g_level==3){g_gs=GS::WIN;}

    // Player move
    float ms=6.f;
    if(keys[GLFW_KEY_A]||keys[GLFW_KEY_LEFT]) g_pl.x-=ms*dt;
    if(keys[GLFW_KEY_D]||keys[GLFW_KEY_RIGHT])g_pl.x+=ms*dt;
    g_pl.x=glm::clamp(g_pl.x,-AREA_W+0.5f,AREA_W-0.5f);

    // Jump (lvl 2+)
    if(g_level>=2){
        if(keys[GLFW_KEY_SPACE]&&onGround()){g_pl.velY=8.f;}
        g_pl.velY+=(-16.f)*dt;
        g_pl.y+=g_pl.velY*dt;
        if(g_pl.y<=FLOOR_Y){g_pl.y=FLOOR_Y;g_pl.velY=0;}
        if(g_pl.y>CEIL_Y-0.6f){g_pl.y=CEIL_Y-0.6f;g_pl.velY=-2.f;}
    }

    // Shoot (lvl 3)
    if(g_level>=3&&(keys[GLFW_KEY_Z]||keys[GLFW_KEY_F])&&g_pl.shootCD<=0&&g_hasWand){
        Orb o;o.x=g_pl.x;o.y=g_pl.y+0.5f;o.vx=0;o.vy=9.f;o.enemy=false;
        g_orbs.push_back(o);g_pl.shootCD=0.22f;
    }

    // Walls
    g_spawnT-=dt;
    float si=glm::mix(1.2f,0.55f,glm::clamp(g_score/SCORE_WIN,0.f,1.f));
    if(g_spawnT<=0){spawnWall();g_spawnT=si;}

    float pw=0.45f,ph=0.75f;
    for(auto& w:g_walls){
        if(!w.active)continue;
        w.y-=g_speed*dt;
        // Collision
        if(fabs(g_pl.y-w.y)<(ph+w.thick)*0.5f){
            bool inGap=(g_pl.x>w.gapCX-w.gapHW&&g_pl.x<w.gapCX+w.gapHW);
            if(!inGap) hitPlayer();
        }
        if(w.y<FLOOR_Y-1.f)w.active=false;
    }
    g_walls.erase(std::remove_if(g_walls.begin(),g_walls.end(),[](auto&w){return !w.active;}),g_walls.end());

    // Orbs
    for(auto& o:g_orbs){
        if(!o.active)continue;
        o.x+=o.vx*dt;o.y+=o.vy*dt;
        if(o.y>CEIL_Y+1||o.y<FLOOR_Y-1||fabs(o.x)>AREA_W+1){o.active=false;continue;}
        if(!o.enemy&&g_boss.active){
            if(fabs(o.x-g_boss.x)<1.0f&&fabs(o.y-g_boss.y)<1.0f){
                o.active=false;g_boss.hp--;g_boss.flashT=0.15f;
                if(g_boss.hp<=0){g_boss.active=false;g_gs=GS::WIN;}
            }
        }
        if(o.enemy&&fabs(o.x-g_pl.x)<0.5f&&fabs(o.y-g_pl.y)<0.7f){hitPlayer();o.active=false;}
    }
    g_orbs.erase(std::remove_if(g_orbs.begin(),g_orbs.end(),[](auto&o){return !o.active;}),g_orbs.end());

    // Boss
    if(g_boss.active){
        g_boss.flashT=glm::max(0.f,g_boss.flashT-dt);
        g_boss.x+=g_boss.vx*60.f*dt;
        if(g_boss.x>AREA_W-1.5f){g_boss.x=AREA_W-1.5f;g_boss.vx=-fabs(g_boss.vx);}
        if(g_boss.x<-AREA_W+1.5f){g_boss.x=-AREA_W+1.5f;g_boss.vx=fabs(g_boss.vx);}
        g_boss.shootCD-=dt;
        if(g_boss.shootCD<=0){
            g_boss.shootCD=1.8f;g_flashT=0.12f;
            for(int i=-1;i<=1;i++){
                Orb o;o.x=g_boss.x;o.y=g_boss.y-1.f;o.enemy=true;
                glm::vec2 d=glm::normalize(glm::vec2(g_pl.x-g_boss.x+i*1.5f,g_pl.y-g_boss.y));
                o.vx=d.x*4.f;o.vy=d.y*4.f;g_orbs.push_back(o);
            }
        }
    }
}

// ============================================================
//  Render helpers — overlay panels (colored cube panels in world)
// ============================================================
// Draw a colored panel quad at world pos (for UI overlays)
static void drawPanel(glm::vec3 pos,glm::vec2 size,glm::vec3 col,float alpha=0.85f){
    // drawn as flat cube with near-zero depth
    glm::mat4 m=glm::translate(glm::mat4(1),pos)*glm::scale(glm::mat4(1),glm::vec3(size.x,size.y,0.05f));
    u1f("fogStrength",0);u1f("flashIntensity",0);
    uM4("model",m);u3f("objectColor",col);
    glBindVertexArray(g_cubeVAO);glDrawArrays(GL_TRIANGLES,0,36);
}

// Draw a row of dots as a simple "text-like" indicator
static void drawDots(float x,float y,int n,glm::vec3 col,float r=0.12f){
    for(int i=0;i<n;i++){
        dSphere(glm::translate(glm::mat4(1),glm::vec3(x+i*r*2.5f,y,0.8f))*glm::scale(glm::mat4(1),glm::vec3(r)),col);
    }
}

// Draw heart
static void drawHeart(float x,float y,float s=0.22f){
    glm::vec3 col(0.95f,0.1f,0.15f);
    glm::vec3 p(x,y,0.8f);
    dSphere(glm::translate(glm::mat4(1),p)*glm::scale(glm::mat4(1),glm::vec3(s)),col);
    dSphere(glm::translate(glm::mat4(1),p+glm::vec3(-s*0.55f,s*0.45f,0))*glm::scale(glm::mat4(1),glm::vec3(s*0.65f)),col);
    dSphere(glm::translate(glm::mat4(1),p+glm::vec3( s*0.55f,s*0.45f,0))*glm::scale(glm::mat4(1),glm::vec3(s*0.65f)),col);
}

// ============================================================
//  Main scene render
// ============================================================
static void renderScene(glm::mat4 view,glm::mat4 proj,glm::mat4 lsm,
                        const std::vector<SubMesh>& plMesh){
    uM4("view",view); uM4("projection",proj); uM4("lightSpaceMatrix",lsm);

    glm::vec3 fog(0);float fogS=0;
    if(g_level==1){fog=glm::vec3(0,0.2f,0.5f);fogS=0.15f;}
    else if(g_level==2){fog=glm::vec3(0.05f,0.28f,0.05f);fogS=0.08f;}
    else{fog=glm::vec3(0.05f,0,0.2f);fogS=0.2f;}

    float flash=(g_flashT>0)?glm::clamp(g_flashT/0.12f,0.f,1.f):0.f;

    // ---- Play area side borders ----
    glm::vec3 borderC(0.08f,0.08f,0.15f);
    dCube(glm::translate(glm::mat4(1),glm::vec3(-AREA_W-0.6f,0,0))*glm::scale(glm::mat4(1),glm::vec3(1.2f,AREA_H*2+2,2)),borderC);
    dCube(glm::translate(glm::mat4(1),glm::vec3( AREA_W+0.6f,0,0))*glm::scale(glm::mat4(1),glm::vec3(1.2f,AREA_H*2+2,2)),borderC);

    // ---- Floor ----
    glm::vec3 flC=(g_level==1)?glm::vec3(0.08f,0.25f,0.45f):(g_level==2)?glm::vec3(0.18f,0.4f,0.12f):glm::vec3(0.06f,0.02f,0.18f);
    dCube(glm::translate(glm::mat4(1),glm::vec3(0,FLOOR_Y-0.28f,0))*glm::scale(glm::mat4(1),glm::vec3(AREA_W*2,0.45f,2.5f)),flC);

    // ---- Decorative BG objects ----
    if(g_level==1){
        // Coral/rocks
        for(int i=-4;i<=4;i++){
            float bx=i*2.4f,by=FLOOR_Y+0.3f;
            dSphere(glm::translate(glm::mat4(1),glm::vec3(bx,by,-1.f))*glm::scale(glm::mat4(1),glm::vec3(0.35f+0.1f*(i%2))),glm::vec3(0.1f,0.5f,0.5f));
        }
    } else if(g_level==2){
        // Trees
        for(int i=-4;i<=4;i++){
            float tx=i*2.5f;
            dCube(glm::translate(glm::mat4(1),glm::vec3(tx,FLOOR_Y+1.f,-1.f))*glm::scale(glm::mat4(1),glm::vec3(0.22f,2.f,0.22f)),glm::vec3(0.3f,0.18f,0.07f));
            dSphere(glm::translate(glm::mat4(1),glm::vec3(tx,FLOOR_Y+2.4f,-1.f))*glm::scale(glm::mat4(1),glm::vec3(0.85f)),glm::vec3(0.1f,0.5f,0.1f));
        }
    } else {
        // Stars
        for(int i=0;i<12;i++){
            float sx=rng(-AREA_W+0.5f,AREA_W-0.5f),sy=rng(FLOOR_Y+0.5f,CEIL_Y-0.5f);
            float ss=0.06f+0.04f*sinf(g_gameTime*2.5f+i*1.3f);
            dSphere(glm::translate(glm::mat4(1),glm::vec3(sx,sy,-0.9f))*glm::scale(glm::mat4(1),glm::vec3(ss)),glm::vec3(1,0.95f,0.7f));
        }
    }

    // ---- Walls (ZOI style) ----
    for(const auto& w:g_walls){
        if(!w.active)continue;
        float lW=(AREA_W+w.gapCX-w.gapHW);
        float rW=(AREA_W-w.gapCX-w.gapHW);
        float lX=-AREA_W+lW*0.5f;
        float rX= AREA_W-rW*0.5f;
        glm::vec3 wC=(w.lv==1)?glm::vec3(0.12f,0.4f,0.72f):(w.lv==2)?glm::vec3(0.22f,0.55f,0.18f):glm::vec3(0.48f,0.08f,0.68f);
        glm::vec3 wCDark=wC*0.7f;
        if(lW>0.05f){
            dCube(glm::translate(glm::mat4(1),glm::vec3(lX,w.y,0))*glm::scale(glm::mat4(1),glm::vec3(lW,w.thick,2.f)),wC);
            // Edge highlight
            dCube(glm::translate(glm::mat4(1),glm::vec3(w.gapCX-w.gapHW,w.y,0.6f))*glm::scale(glm::mat4(1),glm::vec3(0.12f,w.thick*1.1f,0.12f)),glm::vec3(1,1,0.6f));
        }
        if(rW>0.05f){
            dCube(glm::translate(glm::mat4(1),glm::vec3(rX,w.y,0))*glm::scale(glm::mat4(1),glm::vec3(rW,w.thick,2.f)),wC);
            dCube(glm::translate(glm::mat4(1),glm::vec3(w.gapCX+w.gapHW,w.y,0.6f))*glm::scale(glm::mat4(1),glm::vec3(0.12f,w.thick*1.1f,0.12f)),glm::vec3(1,1,0.6f));
        }
    }

    // ---- Player ----
    bool blink=g_pl.invT<=0||(fmod(glfwGetTime(),0.15f)>0.075f);
    if(blink){
        glm::mat4 pm=glm::translate(glm::mat4(1),glm::vec3(g_pl.x,g_pl.y,0))*glm::scale(glm::mat4(1),glm::vec3(2.5f));
        dModel(plMesh,pm,fog,fogS,flash*0.35f);
    }

    // ---- Wand orbiting player ----
    if(g_hasWand){
        float ang=(float)glfwGetTime()*3.f;
        glm::vec3 wp(g_pl.x+cosf(ang)*0.75f,g_pl.y+sinf(ang)*0.35f+0.45f,0.3f);
        dCube(glm::translate(glm::mat4(1),wp)*glm::scale(glm::mat4(1),glm::vec3(0.07f,0.65f,0.07f)),glm::vec3(0.8f,0.6f,0.1f));
        dSphere(glm::translate(glm::mat4(1),wp+glm::vec3(0,0.38f,0))*glm::scale(glm::mat4(1),glm::vec3(0.18f)),glm::vec3(0.3f,0.9f,1));
    }

    // ---- Orbs ----
    for(const auto& o:g_orbs){
        if(!o.active)continue;
        glm::vec3 oc=o.enemy?glm::vec3(1,0.3f,0):glm::vec3(0.2f,0.9f,1);
        dSphere(glm::translate(glm::mat4(1),glm::vec3(o.x,o.y,0))*glm::scale(glm::mat4(1),glm::vec3(0.25f)),oc);
    }

    // ---- Boss ----
    if(g_boss.active){
        float bf=g_boss.flashT>0?0.9f:0.f;
        glm::mat4 bm=glm::translate(glm::mat4(1),glm::vec3(g_boss.x,g_boss.y,0));
        dSphere(bm*glm::scale(glm::mat4(1),glm::vec3(1.0f)),glm::vec3(0.6f,0,0.8f),bf);
        dCube(bm*glm::translate(glm::mat4(1),glm::vec3(0,-0.85f,0))*glm::scale(glm::mat4(1),glm::vec3(0.7f,0.55f,0.7f)),glm::vec3(0.4f,0,0.6f),bf);
        dCube(bm*glm::translate(glm::mat4(1),glm::vec3(0,-1.55f,0))*glm::scale(glm::mat4(1),glm::vec3(0.35f,0.9f,0.35f)),glm::vec3(0.3f,0,0.5f),bf);
        for(int i=0;i<g_boss.hp;i++){
            float a=i*(glm::pi<float>()*2/8.f)+(float)glfwGetTime()*1.5f;
            glm::vec3 hp(g_boss.x+cosf(a)*1.5f,g_boss.y+sinf(a)*0.5f,0);
            dSphere(glm::translate(glm::mat4(1),hp)*glm::scale(glm::mat4(1),glm::vec3(0.2f)),glm::vec3(1,0.85f,0));
        }
    }

    // ============================================================
    //  HUD
    // ============================================================
    // Hearts
    for(int i=0;i<g_pl.hearts;i++) drawHeart(-AREA_W+0.6f+i*0.7f, CEIL_Y-0.4f);

    // Level dots (top right)
    for(int i=0;i<g_level;i++){
        glm::vec3 lc=(i==0)?glm::vec3(0.2f,0.5f,1):(i==1)?glm::vec3(0.2f,0.8f,0.2f):glm::vec3(0.7f,0.2f,1);
        dSphere(glm::translate(glm::mat4(1),glm::vec3(AREA_W-0.5f-i*0.6f,CEIL_Y-0.4f,0.8f))*glm::scale(glm::mat4(1),glm::vec3(0.2f)),lc);
    }

    // Score bar (top center)
    float barFull=AREA_W*1.6f;
    float barFill=barFull*glm::clamp(g_dispScore/SCORE_WIN,0.f,1.f);
    // Background bar
    dCube(glm::translate(glm::mat4(1),glm::vec3(0,CEIL_Y+0.18f,0.5f))*glm::scale(glm::mat4(1),glm::vec3(barFull,0.18f,0.3f)),glm::vec3(0.15f,0.15f,0.15f));
    // Fill
    if(barFill>0.01f)
        dCube(glm::translate(glm::mat4(1),glm::vec3(-barFull*0.5f+barFill*0.5f,CEIL_Y+0.18f,0.6f))*glm::scale(glm::mat4(1),glm::vec3(barFill,0.18f,0.35f)),glm::vec3(0.9f,0.75f,0.1f));
    // Level markers
    float m1=-barFull*0.5f+barFull*(SCORE_L2/SCORE_WIN);
    float m2=-barFull*0.5f+barFull*(SCORE_L3/SCORE_WIN);
    dCube(glm::translate(glm::mat4(1),glm::vec3(m1,CEIL_Y+0.18f,0.7f))*glm::scale(glm::mat4(1),glm::vec3(0.07f,0.4f,0.4f)),glm::vec3(0.2f,1,0.2f));
    dCube(glm::translate(glm::mat4(1),glm::vec3(m2,CEIL_Y+0.18f,0.7f))*glm::scale(glm::mat4(1),glm::vec3(0.07f,0.4f,0.4f)),glm::vec3(0.8f,0.2f,1));

    // ============================================================
    //  OVERLAY SCREENS (in-world panels)
    // ============================================================
    if(g_gs==GS::PAUSED){
        // Dark overlay
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0,0.9f))*glm::scale(glm::mat4(1),glm::vec3(AREA_W*2,AREA_H*2,0.05f)),glm::vec3(0,0,0.1f));
        // Panel
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0.5f,1.f))*glm::scale(glm::mat4(1),glm::vec3(4.f,2.5f,0.1f)),glm::vec3(0.1f,0.1f,0.3f));
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0.5f,1.05f))*glm::scale(glm::mat4(1),glm::vec3(3.8f,2.3f,0.05f)),glm::vec3(0.15f,0.15f,0.4f));
        // PAUSE text as dots
        drawDots(-1.0f,1.3f,5,glm::vec3(1,1,0.5f),0.2f); // "PAUSED" indicator dots
        // Resume button
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0.5f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(2.5f,0.55f,0.05f)),glm::vec3(0.2f,0.6f,0.2f));
        drawDots(-0.4f,0.5f,3,glm::vec3(1),0.18f); // resume
        // Quit dots
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,-0.3f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(2.5f,0.55f,0.05f)),glm::vec3(0.5f,0.1f,0.1f));
        drawDots(-0.3f,-0.3f,2,glm::vec3(1),0.18f); // quit
        std::cout.flush();
    }

    if(g_gs==GS::GAMEOVER){
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0,0.9f))*glm::scale(glm::mat4(1),glm::vec3(AREA_W*2,AREA_H*2,0.05f)),glm::vec3(0.3f,0,0));
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0.5f,1.f))*glm::scale(glm::mat4(1),glm::vec3(5.f,3.f,0.1f)),glm::vec3(0.2f,0.05f,0.05f));
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0.5f,1.05f))*glm::scale(glm::mat4(1),glm::vec3(4.8f,2.8f,0.05f)),glm::vec3(0.3f,0.08f,0.08f));
        // GAME OVER dots
        drawDots(-1.2f,1.5f,8,glm::vec3(1,0.3f,0.3f),0.22f);
        // Retry button
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0.4f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(3.f,0.6f,0.05f)),glm::vec3(0.6f,0.15f,0.15f));
        drawDots(-0.5f,0.4f,4,glm::vec3(1,0.8f,0.8f),0.18f);
        // Score indicator
        drawDots(-1.f,-0.4f,(int)(g_score/100.f)%10+1,glm::vec3(1,0.9f,0.3f),0.15f);
    }

    if(g_gs==GS::WIN){
        // Green overlay
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,0,0.9f))*glm::scale(glm::mat4(1),glm::vec3(AREA_W*2,AREA_H*2,0.05f)),glm::vec3(0,0.15f,0));
        // Main panel (like screenshot: banner style)
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,1.5f,1.f))*glm::scale(glm::mat4(1),glm::vec3(7.f,1.2f,0.1f)),glm::vec3(0.1f,0.3f,0.1f));
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,1.5f,1.08f))*glm::scale(glm::mat4(1),glm::vec3(6.8f,1.0f,0.05f)),glm::vec3(0.15f,0.55f,0.15f));
        // YOU WIN dots (big, yellow)
        drawDots(-1.5f,1.5f,7,glm::vec3(1,0.95f,0.2f),0.28f);

        // Level badge (top-left style)
        dCube(glm::translate(glm::mat4(1),glm::vec3(-AREA_W+1.5f,2.2f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(2.f,1.2f,0.08f)),glm::vec3(0.3f,0.3f,0.35f));
        dModel(plMesh,glm::translate(glm::mat4(1),glm::vec3(-AREA_W+1.5f,2.2f,1.2f))*glm::scale(glm::mat4(1),glm::vec3(1.5f)),{},0,0);

        // GIFT box (top-right style)
        dCube(glm::translate(glm::mat4(1),glm::vec3(AREA_W-1.5f,2.2f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(2.f,1.2f,0.08f)),glm::vec3(0.3f,0.3f,0.35f));
        dCube(glm::translate(glm::mat4(1),glm::vec3(AREA_W-1.5f,2.2f,1.2f))*glm::scale(glm::mat4(1),glm::vec3(0.5f,0.5f,0.1f)),glm::vec3(0.8f,0.2f,0.2f));
        drawDots(AREA_W-2.f,2.2f,2,glm::vec3(1,0.8f,0.2f),0.18f);

        // LEVEL 3 indicator dots
        drawDots(-0.4f,0.6f,3,glm::vec3(0.5f,1,0.5f),0.22f);

        // BACK button (bottom-left)
        dCube(glm::translate(glm::mat4(1),glm::vec3(-AREA_W+2.f,-2.2f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(3.f,0.75f,0.08f)),glm::vec3(0.25f,0.25f,0.35f));
        drawDots(-AREA_W+0.8f,-2.2f,3,glm::vec3(0.85f),0.18f);

        // NEXT button (bottom-right)
        dCube(glm::translate(glm::mat4(1),glm::vec3(AREA_W-2.f,-2.2f,1.1f))*glm::scale(glm::mat4(1),glm::vec3(3.f,0.75f,0.08f)),glm::vec3(0.25f,0.35f,0.25f));
        drawDots(AREA_W-3.f,-2.2f,3,glm::vec3(0.7f,1,0.7f),0.18f);

        // Score bar at bottom
        dCube(glm::translate(glm::mat4(1),glm::vec3(0,-3.f,1.f))*glm::scale(glm::mat4(1),glm::vec3(AREA_W*1.8f,0.5f,0.1f)),glm::vec3(0.1f,0.1f,0.1f));
        drawDots(-1.5f,-3.f,8,glm::vec3(0.9f,0.7f,0.1f),0.15f);
    }
}

// ============================================================
//  main
// ============================================================
int main(){
    srand((unsigned)time(nullptr));
    if(!glfwInit())return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(SCR_W,SCR_H,"Sylvan Odyssey",nullptr,nullptr);
    if(!win){glfwTerminate();return -1;}
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win,cbKey);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    g_prog  =loadProg("shader.vert","shader.frag");
    g_bgProg=mkProg(BG_VS,BG_FS);
    initShadow();
    buildCube();buildSphere();buildQuad();

    // Load textures (ocean for lv1, reuse for lv2/3 with tint)
    g_bgTex =loadTex("textures/ocean.jpg");
    g_bgTex2=loadTex("textures/ocean.jpg"); // reuse, tinted green in shader
    g_bgTex3=loadTex("textures/ocean.jpg"); // reuse, tinted dark in shader

    auto plMesh=loadOBJ("models/model.obj","models/");
    glUseProgram(g_prog);
    glUniform1i(glGetUniformLocation(g_prog,"shadowMap"),1);

    std::cout<<"\n====== SYLVAN ODYSSEY ======\n"
             <<"A/D = gerak | SPACE = loncat (lv2+)\n"
             <<"Z/F = tembak (lv3) | P = pause\n"
             <<"Score 500->Hutan | 1000->Langit+Boss\n"
             <<"ENTER = mulai!\n============================\n";

    float lastT=(float)glfwGetTime();
    bool introPrinted=false;

    while(!glfwWindowShouldClose(win)){
        float now=(float)glfwGetTime();
        float dt=glm::clamp(now-lastT,0.f,0.05f);lastT=now;
        glfwPollEvents();
        if(keys[GLFW_KEY_ESCAPE])break;

        // ---- State inputs ----
        if(g_gs==GS::INTRO){
            if(keyHit[GLFW_KEY_ENTER]||keyHit[GLFW_KEY_SPACE]){memset(keyHit,0,sizeof(keyHit));resetGame();}
            glClearColor(0.02f,0,0.08f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffers(win);continue;
        }
        if(g_gs==GS::PLAYING&&keyHit[GLFW_KEY_P]){g_gs=GS::PAUSED;memset(keyHit,0,sizeof(keyHit));}
        if(g_gs==GS::PAUSED){
            if(keyHit[GLFW_KEY_P]||keyHit[GLFW_KEY_ENTER]){g_gs=GS::PLAYING;memset(keyHit,0,sizeof(keyHit));}
            if(keyHit[GLFW_KEY_Q]){g_gs=GS::INTRO;memset(keyHit,0,sizeof(keyHit));}
        }
        if(g_gs==GS::GAMEOVER&&keyHit[GLFW_KEY_R]){memset(keyHit,0,sizeof(keyHit));resetGame();}
        if(g_gs==GS::WIN&&keyHit[GLFW_KEY_R]){memset(keyHit,0,sizeof(keyHit));g_gs=GS::INTRO;}

        update(dt);
        memset(keyHit,0,sizeof(keyHit));

        // ---- Camera: true front-view ortho-like ----
        glm::vec3 camPos(0,0,16.f);
        glm::vec3 camTgt(0,0,0);
        glm::mat4 view=glm::lookAt(camPos,camTgt,glm::vec3(0,1,0));
        // Narrow FOV = near-ortho feel
        glm::mat4 proj=glm::perspective(glm::radians(38.f),(float)SCR_W/SCR_H,0.1f,100.f);

        // ---- Light ----
        glm::vec3 lp(2,8,12);
        glm::mat4 lProj=glm::ortho(-14.f,14.f,-8.f,8.f,0.1f,50.f);
        glm::mat4 lView=glm::lookAt(lp,glm::vec3(0),glm::vec3(0,1,0));
        glm::mat4 lsm=lProj*lView;

        // ---- Shadow pass ----
        glViewport(0,0,SHW,SHW);
        glBindFramebuffer(GL_FRAMEBUFFER,g_sFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_dProg);
        glUniformMatrix4fv(glGetUniformLocation(g_dProg,"lightSpaceMatrix"),1,GL_FALSE,glm::value_ptr(lsm));
        {glm::mat4 pm=glm::translate(glm::mat4(1),glm::vec3(g_pl.x,g_pl.y,0))*glm::scale(glm::mat4(1),glm::vec3(2.5f));
         glUniformMatrix4fv(glGetUniformLocation(g_dProg,"model"),1,GL_FALSE,glm::value_ptr(pm));
         glBindVertexArray(g_cubeVAO);glDrawArrays(GL_TRIANGLES,0,36);}
        glBindFramebuffer(GL_FRAMEBUFFER,0);

        // ---- Background ----
        glViewport(0,0,SCR_W,SCR_H);
        glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        drawBackground(g_level,g_flashT);

        // ---- Main render ----
        glUseProgram(g_prog);
        glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,g_sTex);
        glUniform3fv(glGetUniformLocation(g_prog,"lightPos"),1,glm::value_ptr(lp));
        glUniform3fv(glGetUniformLocation(g_prog,"viewPos"),1,glm::value_ptr(camPos));
        renderScene(view,proj,lsm,plMesh);

        glfwSwapBuffers(win);
    }
    glfwTerminate();
    return 0;
}