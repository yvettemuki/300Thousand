#version 430            

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

layout (location = 0) in vec3 pos_attrib;                                             

out VertexData
{
    vec2 tex_coord;
    vec3 pw;       //world-space vertex position
    vec3 nw;   //world-space normal vector
} outData;

void main(void)
{
	gl_Position = PV * vec4(pos_attrib, 1.0);
}