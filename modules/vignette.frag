#version 450 core

uniform float u_strength;
uniform float u_radius;
uniform sampler2D u_inputTexture;

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(v_texCoord, center);
    float vignette = smoothstep(u_radius, u_radius - 0.3, dist);
    vec3 result = mix(inputColor.rgb * (1.0 - u_strength), inputColor.rgb, vignette);
    fragColor = vec4(result, inputColor.a);
}
