// #version 450

// #extension GL_GOOGLE_include_directive : require
// #extension GL_EXT_buffer_reference : require

// #include "input_structures.glsl"

// layout (location = 0) out vec3 outNormal;
// layout (location = 1) out vec3 outColor;
// layout (location = 2) out vec2 outUV;
// layout (location = 3) out vec3 outPos;

// struct Vertex {

// 	vec3 position;
// 	float uv_x;
// 	vec3 normal;
// 	float uv_y;
// 	vec4 color;
// }; 

// layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
// 	Vertex vertices[];
// };

// //push constants block
// layout( push_constant ) uniform constants
// {
// 	mat4 render_matrix;
// 	VertexBuffer vertexBuffer;
// } PushConstants;

// // void main() 
// // {
// // 	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
// // 	vec4 position = vec4(v.position, 1.0f);

// // 	gl_Position =  sceneData.viewproj * PushConstants.render_matrix *position;

// // 	outNormal = (PushConstants.render_matrix * vec4(v.normal, 0.f)).xyz;
// // 	outColor = v.color.xyz * materialData.color_factors.xyz;	
// // 	outUV.x = v.uv_x;
// // 	outUV.y = v.uv_y;
// // }


// void main() {

// 	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
// 	vec4 position = vec4(v.position, 1.0f);

//     mat4 modelView = sceneData.view * PushConstants.render_matrix;
//     outPos = (modelView * position).xyz; // Position in View Space
//     outNormal = (PushConstants.render_matrix * vec4(v.normal, 0.f)).xyz; // Transform normal to View Space
// 	outColor = v.color.xyz * materialData.color_factors.xyz;	
// 	outUV.x = v.uv_x;
// 	outUV.y = v.uv_y;

//     gl_Position = sceneData.viewproj * PushConstants.render_matrix * position;
// }

#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outPos;
layout (location = 4) out vec3 outTangent; // New tangent output

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec3 tangent; // New tangent field
    vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer { 
    Vertex vertices[];
};

// Push constants block
layout(push_constant) uniform constants {
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    
    vec4 position = vec4(v.position, 1.0f);

    // Transformations
    mat4 modelView = sceneData.view * PushConstants.render_matrix;
    outPos = (modelView * position).xyz; // Position in View Space

    // Transform Normal and Tangent to World Space
    outNormal = normalize((PushConstants.render_matrix * vec4(v.normal, 0.f)).xyz);
    outTangent = normalize((PushConstants.render_matrix * vec4(v.tangent, 0.0)).xyz); // Transform tangent

    outColor = v.color.xyz * materialData.color_factors.xyz;    
    outUV = vec2(v.uv_x, v.uv_y);

    gl_Position = sceneData.viewproj * PushConstants.render_matrix * position;
}
