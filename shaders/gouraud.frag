#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Color;
in vec3 VertexLightingColor;

uniform vec3 objectColor;
uniform bool useVertexColor;
uniform sampler2D imageTexture;
uniform int textureMode;
uniform int blendMode;
uniform float uvScale;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    if (textureMode == 4) {
        FragColor = vec4(useVertexColor ? Color : objectColor, 1.0);
        return;
    }

    vec2 scaledUV = TexCoords * uvScale;
    vec3 baseCol = vec3(1.0);

    if (textureMode == 0) {
        baseCol = useVertexColor ? Color : objectColor;
    } else if (textureMode == 1) {
        baseCol = texture(imageTexture, scaledUV).rgb;
    } else if (textureMode == 2) {
        float cx = floor(scaledUV.x);
        float cy = floor(scaledUV.y);
        float res = mod(cx + cy, 2.0);
        baseCol = mix(vec3(0.9), vec3(0.2), res);
    } else if (textureMode == 3) {
        float n = hash(scaledUV);
        baseCol = mix(vec3(0.8), vec3(0.6), n);
    }

    if (blendMode == 1) {
        vec3 vCol = useVertexColor ? Color : objectColor;
        baseCol *= vCol;
    } else if (blendMode == 2) {
        float wave = (sin(scaledUV.x * 2.0) + cos(scaledUV.y * 2.0) + 2.0) / 4.0;
        vec3 waveCol = mix(vec3(0.2, 0.4, 0.8), vec3(0.8, 0.3, 0.2), wave);
        baseCol *= waveCol;
    }

    FragColor = vec4(baseCol * VertexLightingColor, 1.0);
}
