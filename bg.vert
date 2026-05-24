#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
out vec2 uv;
uniform float scrollY; // 0=dasar laut, 1=permukaan
void main(){
    uv = aUV;
    // scroll UV vertikal: makin tinggi player, lihat bagian atas gambar
    uv.y = aUV.y * 0.6 + scrollY * 0.4;
    gl_Position = vec4(aPos.xy, 0.999, 1.0); // z=0.999 = paling belakang
}
