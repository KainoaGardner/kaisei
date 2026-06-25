#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_intensity;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);

    // Sepia tone matrix
    vec3 sepia;
    sepia.r = dot(inputColor.rgb, vec3(0.393, 0.769, 0.189));
    sepia.g = dot(inputColor.rgb, vec3(0.349, 0.686, 0.168));
    sepia.b = dot(inputColor.rgb, vec3(0.272, 0.534, 0.131));

    // Mix between original and sepia
    vec3 result = mix(inputColor.rgb, sepia, u_intensity);

    fragColor = vec4(result, inputColor.a);
}
