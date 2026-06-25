#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_time;
uniform float u_speed;
uniform float u_intensity;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);

    // Pulsating brightness effect using time
    float pulse = sin(u_time * u_speed) * 0.5 + 0.5;
    float brightness = 1.0 + (pulse * u_intensity);

    vec3 result = inputColor.rgb * brightness;

    fragColor = vec4(result, inputColor.a);
}
