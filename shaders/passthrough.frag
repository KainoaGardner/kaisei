#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;

void main() {
    fragColor = texture(u_inputTexture, v_texCoord);
}
