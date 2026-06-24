#version 450 core

uniform int u_shadeSteps;
uniform float u_edgeThickness;
uniform sampler2D u_inputTexture;

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);
    vec3 quantized = floor(inputColor.rgb * float(u_shadeSteps)) / float(u_shadeSteps);

    vec2 texelSize = vec2(u_edgeThickness);
    vec3 n = texture(u_inputTexture, v_texCoord + vec2(0.0, texelSize.y)).rgb;
    vec3 s = texture(u_inputTexture, v_texCoord - vec2(0.0, texelSize.y)).rgb;
    vec3 e = texture(u_inputTexture, v_texCoord + vec2(texelSize.x, 0.0)).rgb;
    vec3 w = texture(u_inputTexture, v_texCoord - vec2(texelSize.x, 0.0)).rgb;

    float edge = length(n - s) + length(e - w);
    edge = step(0.5, edge);

    vec3 result = mix(quantized, vec3(0.0), edge);
    fragColor = vec4(result, inputColor.a);
}
