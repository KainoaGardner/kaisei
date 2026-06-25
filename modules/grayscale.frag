#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);
    float gray = 0.299 * inputColor.r + 0.587 * inputColor.g + 0.114 * inputColor.b;

    fragColor = vec4(vec3(gray), inputColor.a);
}
