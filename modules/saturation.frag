#version 450 core

uniform float u_saturation;
uniform sampler2D u_inputTexture;

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);
    float gray = dot(inputColor.rgb, vec3(0.299, 0.587, 0.114));
    vec3 result = mix(vec3(gray), inputColor.rgb, u_saturation);
    fragColor = vec4(result, inputColor.a);
}
