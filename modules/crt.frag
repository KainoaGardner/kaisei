#version 450 core

uniform float u_curvature;
uniform float u_scanlineIntensity;
uniform float u_scanlineCount;
uniform sampler2D u_inputTexture;

in vec2 v_texCoord;
out vec4 fragColor;

vec2 curveUV(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(u_curvature * 6.0);
    uv = uv + uv * offset * offset;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main() {
    vec2 curvedUV = curveUV(v_texCoord);

    if (curvedUV.x < 0.0 || curvedUV.x > 1.0 || curvedUV.y < 0.0 || curvedUV.y > 1.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 inputColor = texture(u_inputTexture, curvedUV);
    float scanline = sin(curvedUV.y * u_scanlineCount * 3.14159) * 0.5 + 0.5;
    scanline = mix(1.0, scanline, u_scanlineIntensity);

    vec3 result = inputColor.rgb * scanline;
    fragColor = vec4(result, inputColor.a);
}
