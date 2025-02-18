#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPos;

layout (location = 0) out vec4 outFragColor;

struct SHCoefficients {
    vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

const SHCoefficients grace = SHCoefficients(
    vec3( 0.3623915,  0.2624130,  0.2326261 ),
    vec3( 0.1759131,  0.1436266,  0.1260569 ),
    vec3(-0.0247311, -0.0101254, -0.0010745 ),
    vec3( 0.0346500,  0.0223184,  0.0101350 ),
    vec3( 0.0198140,  0.0144073,  0.0043987 ),
    vec3(-0.0469596, -0.0254485, -0.0117786 ),
    vec3(-0.0898667, -0.0760911, -0.0740964 ),
    vec3( 0.0050194,  0.0038841,  0.0001374 ),
    vec3(-0.0818750, -0.0321501,  0.0033399 )
);

vec3 calcIrradiance(vec3 nor) {
    const SHCoefficients c = grace;
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    return (
        c1 * c.l22 * (nor.x * nor.x - nor.y * nor.y) +
        c3 * c.l20 * nor.z * nor.z +
        c4 * c.l00 -
        c5 * c.l20 +
        2.0 * c1 * c.l2m2 * nor.x * nor.y +
        2.0 * c1 * c.l21  * nor.x * nor.z +
        2.0 * c1 * c.l2m1 * nor.y * nor.z +
        2.0 * c2 * c.l11  * nor.x +
        2.0 * c2 * c.l1m1 * nor.y +
        2.0 * c2 * c.l10  * nor.z
    );
}

// Fresnel-Schlick approximation for reflectance
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Normal Distribution Function (GGX)
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (3.14159265359 * denom * denom);
}

// Geometric Shadowing Function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness) {
    float k = (roughness * roughness) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith function for both view & light
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// void main() 
// {
// 	float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1f);

// 	vec3 color = inColor * texture(colorTex,inUV).xyz;
// 	vec3 ambient = color *  sceneData.ambientColor.xyz;

// 	outFragColor = vec4(color * lightValue *  sceneData.sunlightColor.w + ambient ,1.0f);
// }


// void main() {
//     vec3 N = normalize(inNormal); // Normalized normal
//     vec3 L = normalize(sceneData.sunlightDirection.rgb); // Light direction
//     vec3 V = normalize(sceneData.cameraPosition.rgb - inPos); // View direction
//     vec3 R = reflect(-L, N); // Reflected light direction

//     // Ambient component
//     vec3 ambient = materialData.color_factors.rgb * sceneData.ambientColor.rgb *1.5;

//     // Diffuse component
//     float diff = max(dot(N, L), 0.0);
//     vec3 diffuse = diff * materialData.metal_rough_factors.rgb * sceneData.sunlightColor.rgb;

//     // Specular component
//     float spec = pow(max(dot(R, V), 0.0), materialData.specular_factors.a);
//     vec3 specular = spec * materialData.specular_factors.rgb * sceneData.sunlightColor.rgb;

//     // Combine lighting
//     vec3 phong = ambient + diffuse + specular;

//     // Sample texture
//     vec3 texColor = texture(colorTex, inUV).rgb;

//     outFragColor = vec4(phong * texColor, 1.0);
// }

void main() {
    // Input vectors
    vec3 N = normalize(inNormal); // Normal
    vec3 V = normalize(sceneData.cameraPosition.xyz - inPos); // View direction
    vec3 L = normalize(sceneData.lightPosition.xyz - inPos); // Light direction
    vec3 H = normalize(V + L); // Halfway vector

    vec3 mrSample = texture(metalRoughTex, inUV).rgb;
    float metallic = (materialData.has_metal_rough_texture > 0) ? mrSample.b : materialData.metal_factors;
    float roughness = (materialData.has_metal_rough_texture > 0) ? mrSample.g : materialData.rough_factors;

    // Compute reflectance
    vec3 F0 = mix(vec3(0.04), materialData.color_factors.rgb, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Compute BRDF terms
    // Compute BRDF Terms

  

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 numerator_brdf = NDF * G * F;
    float denominator_brdf = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = (numerator_brdf / denominator_brdf) * sceneData.sunlightColor.rgb;

    // Specular term
    // vec3 numerator = NDF * G * F;
    // float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    // vec3 specular = (numerator / denominator) * sceneData.sunlightColor.rgb;

    // Diffuse term (energy conservation)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * materialData.color_factors.rgb * sceneData.sunlightColor.rgb / 3.14159265359;

    // Combine lighting contributions
    float NdotL = max(dot(N, L), 0.0);
    vec3 color = (diffuse + specular) * sceneData.lightPosition.w * NdotL;

    // Apply ambient & AO
    vec3 ambient = (1.0 - metallic) * materialData.color_factors.rgb * sceneData.ambientColor.rgb * materialData.ao;

    vec3 texColor = texture(colorTex, inUV).rgb;
    outFragColor = vec4((ambient + color) * texColor, 1.0);
}