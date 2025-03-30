// #version 450

// #extension GL_GOOGLE_include_directive : require
// #include "input_structures.glsl"

// layout (location = 0) in vec3 inNormal;
// layout (location = 1) in vec3 inColor;
// layout (location = 2) in vec2 inUV;
// layout (location = 3) in vec3 inPos;

// layout (location = 0) out vec4 outFragColor;

// // Fresnel-Schlick approximation for reflectance
// vec3 fresnelSchlick(float cosTheta, vec3 F0) {
//     return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
// }

// vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
//     return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
// }

// // Normal Distribution Function (GGX)
// float distributionGGX(vec3 N, vec3 H, float roughness) {
//     float a = roughness * roughness;
//     float a2 = a * a;
//     float NdotH = max(dot(N, H), 0.0);
//     float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
//     return a2 / (3.14159265359 * denom * denom);
// }

// // Geometric Shadowing Function (Schlick-GGX)
// float geometrySchlickGGX(float NdotV, float roughness) {
//     float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
//     return NdotV / (NdotV * (1.0 - k) + k);
// }

// // Smith function for both view & light
// float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
//     float NdotV = max(dot(N, V), 0.0);
//     float NdotL = max(dot(N, L), 0.0);
//     return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
// }

// void main() {
//     // Input vectors
//     vec3 N = normalize(inNormal); // Normal
//     vec3 V = normalize(sceneData.cameraPosition.xyz - inPos); // View direction
//     vec3 L = normalize(sceneData.lightPosition.xyz - inPos); // Light direction
//     vec3 H = normalize(V + L); // Halfway vector

//     vec3 mrSample = texture(metalRoughTex, inUV).rgb;
//     vec3 texColor = texture(colorTex, inUV).rgb;
//     // vec3 normalTS = texture(normalMap, colorTex).rgb * 2.0 - 1.0;
//     float metallic = (materialData.has_metal_rough_texture > 0) ? mrSample.b : materialData.metal_factors;
//     float roughness = (materialData.has_metal_rough_texture > 0) ? mrSample.g : materialData.rough_factors;
//     roughness = clamp(roughness, 0.04, 1.0);


//     // Compute reflectance
//     vec3 F0 = mix(vec3(0.04), materialData.color_factors.rgb, metallic);
//     F0 = mix(F0, vec3(0.85), roughness * roughness);
//     vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0, roughness);


//     float NDF = distributionGGX(N, H, roughness);
//     float G = geometrySmith(N, V, L, roughness);
//     vec3 numerator_brdf = NDF * G * F;
//     float denominator_brdf = max(dot(N, V) * dot(N, L), 0.1);
//     vec3 specular = (numerator_brdf / denominator_brdf) * sceneData.sunlightColor.rgb;

//     // Diffuse term (energy conservation)
//     vec3 kD = (1.0 - F) * (1.0 - metallic);
//     vec3 diffuse = kD * materialData.color_factors.rgb * sceneData.sunlightColor.rgb * (1.0 / 3.14159265359);

//     // Combine lighting contributions
//     float NdotL = max(dot(N, L), 0.0);
//     vec3 color = (diffuse + specular) * sceneData.lightPosition.w * NdotL;

//     // Apply ambient & AO
//     vec3 ambient = (1.0 - metallic) * (kD * materialData.color_factors.rgb * sceneData.ambientColor.rgb + 
//                 F * sceneData.ambientColor.rgb) * mrSample.r;

    
//     outFragColor = vec4((ambient + color) * texColor, 1.0);
// }


#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPos;
layout (location = 4) in vec3 inTangent; // New: Tangent vector from vertex shader

layout (location = 0) out vec4 outFragColor;



// Fresnel-Schlick approximation for reflectance
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Normal Distribution Function (GGX)
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (3.14159265359 * denom * denom);
}

// Geometric Shadowing Function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith function for both view & light
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

vec3 toneMapACES(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    // Input vectors
    vec3 V = normalize(sceneData.cameraPosition.xyz - inPos); // View direction
    vec3 L = normalize(sceneData.lightPosition.xyz - inPos); // Light direction
    vec3 H = normalize(V + L); // Halfway vector

    // Sample textures
    vec3 mrSample = texture(metalRoughTex, inUV).rgb;
    vec3 texColor = texture(colorTex, inUV).rgb * materialData.color_factors.rgb;
    texColor = pow(texColor, vec3(2.2));
    vec3 normalTS = texture(normalMap, inUV).rgb * 2.0 - 1.0; // Convert [0,1] to [-1,1]

    // Extract material properties
    float metallic = (materialData.has_metal_rough_texture > 0) ? mrSample.b : materialData.metal_factors;
    float roughness = (materialData.has_metal_rough_texture > 0) ? mrSample.g : materialData.rough_factors;
    roughness = clamp(roughness, 0.04, 1.0);

    // Compute Tangent-Bitangent-Normal (TBN) matrix
    vec3 T = (inTangent);
    vec3 B = normalize(cross(inNormal, inTangent)); // Compute bitangent
    mat3 TBN = mat3(T, B, (inNormal));

    // Transform normal map from tangent space to world space
    vec3 normalWS = normalize(TBN * normalTS);

    // Compute reflectance
    vec3 F0 = mix(vec3(0.04), texColor.rgb, metallic);
    F0 = mix(F0, vec3(0.85), roughness * roughness);
    vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0, roughness);

    // vec3 F0 = vec3(0.04); // Default dielectric reflectance
    // vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0, roughness);

    // Use normalWS in shading
    float NDF = distributionGGX(normalWS, H, roughness);
    float G = geometrySmith(normalWS, V, L, roughness);
    vec3 numerator_brdf = NDF * G * F;
    float denominator_brdf = max(dot(normalWS, V) * dot(normalWS, L), 0.1);
    vec3 specular = (numerator_brdf / denominator_brdf) * sceneData.sunlightColor.rgb;

    // Diffuse term (energy conservation)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * texColor.rgb * sceneData.sunlightColor.rgb * (1.0 / 3.14159265359);

    // Combine lighting contributions
    float NdotL = max(dot(normalWS, L), 0.0);
    vec3 color = (diffuse + specular) * sceneData.lightPosition.w * NdotL;

    // Apply ambient & AO
    vec3 ambient = (1.0 - metallic) * (kD * texColor.rgb * sceneData.ambientColor.rgb + 
                F * sceneData.ambientColor.rgb) * mrSample.r;

    outFragColor = vec4(toneMapACES((ambient + color) * texColor), 1.0);
}
