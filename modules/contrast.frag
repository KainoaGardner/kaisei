#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_contrast;

void main() {
    vec4 inputColor = texture(u_inputTexture, v_texCoord);

    // Apply contrast around 0.5 midpoint
    vec3 result = ((inputColor.rgb - 0.5) * u_contrast) + 0.5;

    fragColor = vec4(result, inputColor.a);
}
