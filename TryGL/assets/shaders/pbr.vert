//
//  pbr.vert
//  PBR — 顶点着色器（无切线空间版本）
//

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

out vec2 uv;
out vec3 worldPos;
out vec3 worldNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() {
    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    worldPos = worldPosition.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    worldNormal = normalize(normalMatrix * aNormal);
    
    uv = aUV;
    gl_Position = projectionMatrix * viewMatrix * worldPosition;
}
