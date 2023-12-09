#version 450

layout(location = 0) in vec2 position;

uniform mat4 uVP;
uniform sampler2D uHeightMap;
uniform vec2 uInvTerrainSize;

out vec3 vNormal;
out vec2 vUV;

vec2 GetUV(vec2 p) {
   return p * uInvTerrainSize;
}

float SampleHeight(vec2 uv) {
   return texture(uHeightMap, uv).r * 256.0f;
}

void main() {
    float hRight = SampleHeight(GetUV(position + vec2(1.0f, 0.0f)));
    float hTop = SampleHeight(GetUV(position + vec2(0.0f, 1.0f)));

    float hLeft = SampleHeight(GetUV(position - vec2(1.0f, 0.0f)));
    float hBottom = SampleHeight(GetUV(position - vec2(0.0f, 1.0f)));

    vec2 uv = GetUV(position);
    float height = SampleHeight(uv);

    vNormal = normalize(vec3(hRight - hLeft, 1.0f, hTop - hBottom));
    vUV = uv;

    gl_Position = uVP * vec4(position.x, height, position.y, 1.0f);
}