#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_edgeThickness;
uniform int u_shadeSteps;

// Sobel edge detection
vec3 detectEdgeSobel(sampler2D tex, vec2 coord, vec2 texelSize) {
    vec3 gx = vec3(0.0);
    vec3 gy = vec3(0.0);

    // Sobel kernels
    gx += texture(tex, coord + vec2(-texelSize.x, -texelSize.y)).rgb * -1.0;
    gx += texture(tex, coord + vec2(-texelSize.x,  0.0)).rgb * -2.0;
    gx += texture(tex, coord + vec2(-texelSize.x,  texelSize.y)).rgb * -1.0;
    gx += texture(tex, coord + vec2( texelSize.x, -texelSize.y)).rgb *  1.0;
    gx += texture(tex, coord + vec2( texelSize.x,  0.0)).rgb *  2.0;
    gx += texture(tex, coord + vec2( texelSize.x,  texelSize.y)).rgb *  1.0;

    gy += texture(tex, coord + vec2(-texelSize.x, -texelSize.y)).rgb * -1.0;
    gy += texture(tex, coord + vec2( 0.0, -texelSize.y)).rgb * -2.0;
    gy += texture(tex, coord + vec2( texelSize.x, -texelSize.y)).rgb * -1.0;
    gy += texture(tex, coord + vec2(-texelSize.x,  texelSize.y)).rgb *  1.0;
    gy += texture(tex, coord + vec2( 0.0,  texelSize.y)).rgb *  2.0;
    gy += texture(tex, coord + vec2( texelSize.x,  texelSize.y)).rgb *  1.0;

    return sqrt(gx * gx + gy * gy);
}

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);

    // Quantize to discrete shading levels (cel shading)
    vec3 quantized = floor(inputColor.rgb * float(u_shadeSteps)) / float(u_shadeSteps);

    // Detect edges
    vec2 texelSize = vec2(u_edgeThickness);
    vec3 edgeVec = detectEdgeSobel(u_inputTexture, v_texCoord, texelSize);
    float edge = length(edgeVec);

    // Clamp and scale edge value to reasonable range
    edge = clamp(edge * 5.0, 0.0, 1.0);

    // Mix quantized color with black edges
    vec3 result = mix(quantized, vec3(0.0), edge);

    fragColor = vec4(result, inputColor.a);
}
