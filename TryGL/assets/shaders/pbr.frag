//
//  pbr.frag
//  PBR — Cook-Torrance BRDF（无切线版本，适合 bag 模型）
//

#version 330 core
out vec4 FragColor;

in vec2 uv;
in vec3 worldPos;
in vec3 worldNormal;

// ========== 贴图输入 ==========
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D envMap;

// ========== 光照参数 ==========
uniform vec3 camPos;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform int lightCount;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};
uniform DirectionalLight directionalLight;

// ========== PBR 核心函数 ==========

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = 3.14159265 * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 SampleSphericalMap(vec3 direction) {
    float u = atan(direction.z, direction.x) / (2.0 * 3.14159265) + 0.5;
    float v = asin(clamp(direction.y, -1.0, 1.0)) / 3.14159265 + 0.5;
    return texture(envMap, vec2(u, v)).rgb;
}

void main() {
    // 1. PBR 参数
    vec3 albedo = pow(texture(albedoMap, uv).rgb, vec3(2.2));
    float metallic = texture(metallicMap, uv).r;
    float roughness = texture(roughnessMap, uv).r;
    float ao = texture(aoMap, uv).r;
    vec3 N = normalize(worldNormal);
    
    // 2. 法线贴图（有则使用，无则 fallback）
    vec3 normalTS = texture(normalMap, uv).rgb;
    if (length(normalTS) > 0.1) {
        normalTS = normalize(normalTS * 2.0 - 1.0);
        // 简单近似：用世界法线作为基础
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, N));
        vec3 bitangent = cross(N, tangent);
        mat3 TBN = mat3(tangent, bitangent, N);
        N = normalize(TBN * normalTS);
    }
    
    vec3 V = normalize(camPos - worldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // 3. Cook-Torrance BRDF — 方向光
    vec3 Lo = vec3(0.0);
    {
        vec3 L = normalize(-directionalLight.direction);
        vec3 H = normalize(V + L);
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159265 + specular) * directionalLight.color * directionalLight.intensity * NdotL;
    }
    
    // 4. 点光源
    for (int i = 0; i < min(lightCount, 4); i++) {
        vec3 L = normalize(lightPositions[i] - worldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - worldPos);
        float attenuation = 1.0 / (distance * distance + 0.001);
        vec3 radiance = lightColors[i] * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    }
    
    // 5. IBL 环境光
    vec3 ambient = vec3(0.03) * albedo * ao;  // 固定环境光
    // 如果有环境贴图则使用
    // vec3 envColor = SampleSphericalMap(N);
    // vec3 ambient = envColor * albedo * ao * 0.3;
    
    vec3 color = ambient + Lo;
    
    // 6. Tone mapping + Gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
