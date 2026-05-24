#version 330 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D bgTex;
void main(){
    vec4 c = texture(bgTex, uv);
    // Tint sedikit biru laut agar blend dengan scene
    c.rgb = mix(c.rgb, vec3(0.01, 0.05, 0.2), 0.25);
    FragColor = c;
}
