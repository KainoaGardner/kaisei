#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_time;
uniform vec2 u_resolution;
uniform float u_amplitude;
uniform float u_frequency;
uniform float u_speed;

void main() {
    vec2 uv = v_texCoord;

    // Wave distortion that moves over time
    float wave = sin(uv.y * u_frequency + u_time * u_speed) * u_amplitude;
    uv.x += wave;

    // Sample with distorted coordinates
    vec4 inputColor = texture(u_inputTexture, uv);

    fragColor = inputColor;
}
