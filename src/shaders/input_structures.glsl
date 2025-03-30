layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 model;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 cameraPosition;
	vec4 lightPosition;
} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData{   

	vec4 color_factors;
	float metal_factors;
	float rough_factors;
	float ao;
	uint has_metal_rough_texture;

	
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalMap;