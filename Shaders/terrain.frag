#version 450

layout(location = 0) out vec4 fragColor;

in vec3 vNormal;
in vec2 vUV;

uniform sampler2D uDiffuseMap;

vec3 lightDir = normalize(vec3(0.1, 0.5, 0.1));

void main() {
   vec3 terrainColor = texture(uDiffuseMap, vUV).rgb;

   float diffuse = max(dot(vNormal, lightDir), 0.0f);
   vec3 col = diffuse * vec3(1.28, 1.20, 0.99);
   col += (vNormal.y * 0.5 + 0.5) * vec3(0.16, 0.20, 0.28);
   col *= terrainColor;
   fragColor = vec4(col, 1.0f);
}