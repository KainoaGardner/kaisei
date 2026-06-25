#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_strength;
uniform float u_radius;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);

    // Calculate distance from center
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(v_texCoord, center);

    // Calculate vignette factor
    float vignette = smoothstep(u_radius, u_radius - 0.5, dist);
    vignette = mix(1.0, vignette, u_strength);

    // Apply vignette
    fragColor = vec4(inputColor.rgb * vignette, inputColor.a);
}
