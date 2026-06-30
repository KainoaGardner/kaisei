#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;

void main() {
    // Just output solid blue - ignores input completely
    fragColor = vec4(0.0, 0.0, 1.0, 1.0);
}
