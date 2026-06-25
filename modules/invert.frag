#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);

    // Invert RGB, keep alpha
    fragColor = vec4(1.0 - inputColor.rgb, inputColor.a);
}
