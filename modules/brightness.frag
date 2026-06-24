#version 450 core

uniform float u_brightness;
uniform sampler2D u_inputTexture;

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);
    vec3 result = inputColor.rgb + vec3(u_brightness);
    fragColor = vec4(result, inputColor.a);
}
