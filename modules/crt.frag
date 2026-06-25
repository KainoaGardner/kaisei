#version 450 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;
uniform float u_curvature;
uniform float u_scanlineIntensity;

// Apply barrel distortion (CRT curvature)
vec2 curveScreen(vec2 uv, float amount) {
    if (amount == 0.0) return uv;

    uv = uv * 2.0 - 1.0;  // Map to -1 to 1

    // Simple barrel distortion
    float r2 = uv.x * uv.x + uv.y * uv.y;
    uv = uv * (1.0 + amount * r2);

    uv = uv * 0.5 + 0.5;  // Map back to 0 to 1
    return uv;
}

void main() {
    vec2 uv = v_texCoord;

    // Apply curvature if enabled
    if (u_curvature > 0.0) {
        uv = curveScreen(v_texCoord, u_curvature);

        // Clamp to screen bounds (black outside)
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    // Sample texture
    vec4 inputColor = texture(u_inputTexture, uv);

    // Add scanlines
    float scanline = 1.0;
    if (u_scanlineIntensity > 0.0) {
        scanline = sin(uv.y * 600.0) * 0.5 + 0.5;
        scanline = mix(1.0, scanline, u_scanlineIntensity);
    }

    vec3 result = inputColor.rgb * scanline;

    fragColor = vec4(result, inputColor.a);
}
