#version 430            
layout(location = 0) uniform mat4 M;

// quad for the arena ground plane
const vec4 quad[4] = vec4[] (
	vec4(-1.0, 1.0, 0.0, 1.0), 
	vec4(-1.0, -1.0, 0.0, 1.0), 
	vec4( 1.0, 1.0, 0.0, 1.0), 
	vec4( 1.0, -1.0, 0.0, 1.0)
);

//const int MAX_BONES = 100;
//layout(location = 20) uniform mat4 bone_xform[MAX_BONES];

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

out VertexData
{
    vec2 tex_coord;
    vec3 pw;       //world-space vertex position
    vec3 nw;   //world-space normal vector
} outData;

void main(void)
{
	gl_Position = PV * M * quad[gl_VertexID];
	outData.tex_coord = 0.5 * (quad[gl_VertexID].xy + vec2(1.0));
	outData.pw = vec3(M * quad[gl_VertexID]);
	outData.nw = vec3(M * vec4(0.0, 1.0, 0.0, 0.0));
}