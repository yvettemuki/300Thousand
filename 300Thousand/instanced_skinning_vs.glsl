#version 430            
layout(location = 1) uniform float time;
layout(location = 2) uniform int num_bones = 0;
layout(location = 3) uniform int Mode;
layout(location = 4) uniform int debug_id = 0;
layout(location = 5) uniform int frame_number = 0;
layout(location = 6) uniform int animTexHeight = 256;
layout(location = 7) uniform int animTexWidth = 256;
layout(location = 8) uniform int animationIndex = 0;
layout(location = 9) uniform int type;


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

layout(binding = 1) uniform sampler2D anim_tex0;
layout(binding = 2) uniform sampler2D anim_tex1;
layout(binding = 3) uniform sampler2D anim_tex2;
layout(binding = 4) uniform sampler2D anim_tex3;

layout (location = 0) in vec3 pos_attrib;                                             
layout (location = 1) in vec2 tex_coord_attrib;                                             
layout (location = 2) in vec3 normal_attrib;                                               
layout (location = 3) in ivec4 bone_id_attrib;
layout (location = 4) in vec4 weight_attrib;
//layout(location = 8) in vec3 modelMatPos;
layout (location = 8) in mat4 model_matrix;

out VertexData
{
    vec2 tex_coord;
    vec3 pw;       //world-space vertex position
    vec3 nw;   //world-space normal vector
	float w_debug;
} outData;

int getCellIndex(int bone_id, int row) {
	int frameFinal = int(mod(frame_number + (gl_InstanceID * 70), 150));
	return (frameFinal * num_bones * 4) + (bone_id * 4) + row;
}

vec2 getTexCoord(int bone_id, int row) {
	int cellIndex = getCellIndex(bone_id, row);

	float y = int(cellIndex / animTexWidth)/ float(animTexHeight);
	float x = float(mod(cellIndex, animTexWidth)) / float(animTexWidth);

	//float initNum = (float(cellIndex) / 256.0f);
	//float fractional = fract(initNum);
	//float y = floor(initNum)/ 256.0f;
	//float x = fractional;

	return vec2(x, y);
}

mat4 getBoneMatrixFromTexture(int bone_id) {
	vec4 animTexRow1 = vec4(0.0);
	vec4 animTexRow2 = vec4(0.0);
	vec4 animTexRow3 = vec4(0.0);
	vec4 animTexRow4 = vec4(0.0);
	//if (animationIndex == 0) {
		animTexRow1 = texture(anim_tex0, getTexCoord(bone_id, 0));
		animTexRow2 = texture(anim_tex0, getTexCoord(bone_id, 1));
		animTexRow3 = texture(anim_tex0, getTexCoord(bone_id, 2));
		animTexRow4 = texture(anim_tex0, getTexCoord(bone_id, 3));
	/* } else if (animationIndex == 1) {
		animTexRow1 = texture(anim_tex1, getTexCoord(bone_id, 0));
		animTexRow2 = texture(anim_tex1, getTexCoord(bone_id, 1));
		animTexRow3 = texture(anim_tex1, getTexCoord(bone_id, 2));
		animTexRow4 = texture(anim_tex1, getTexCoord(bone_id, 3));
	}
	else if (animationIndex == 2) {
		animTexRow1 = texture(anim_tex2, getTexCoord(bone_id, 0));
		animTexRow2 = texture(anim_tex2, getTexCoord(bone_id, 1));
		animTexRow3 = texture(anim_tex2, getTexCoord(bone_id, 2));
		animTexRow4 = texture(anim_tex2, getTexCoord(bone_id, 3));
	}
	else if (animationIndex == 3) {
		animTexRow1 = texture(anim_tex3, getTexCoord(bone_id, 0));
		animTexRow2 = texture(anim_tex3, getTexCoord(bone_id, 1));
		animTexRow3 = texture(anim_tex3, getTexCoord(bone_id, 2));
		animTexRow4 = texture(anim_tex3, getTexCoord(bone_id, 3));
	}*/

	return transpose(mat4(animTexRow1, animTexRow2, animTexRow3, animTexRow4));
	//return transpose(mat4(1.0f));
}

void main(void)
{
	mat4 M = model_matrix;
		//mat4 M = mat4(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0, 1.0, 0.0, 0.0), vec4(0.0, 0.0, 1.0, 0.0), vec4(modelMatPos, 1.0));
	if(Mode > 0)
	{
		mat4 Skinning = mat4(1.0);
		/*if (Mode > 2) {
			if (num_bones > 0)
			{
				//Linear blend skinning
				Skinning = bone_xform[bone_id_attrib[0]] * weight_attrib[0];
				Skinning += bone_xform[bone_id_attrib[1]] * weight_attrib[1];
				Skinning += bone_xform[bone_id_attrib[2]] * weight_attrib[2];
				Skinning += bone_xform[bone_id_attrib[3]] * weight_attrib[3];
			}

			//for debug visualization of bone weights
			/*outData.w_debug = 0.0;
			for (int i = 0; i < 4; i++)
			{
				if (bone_id_attrib[i] == debug_id)
				{
					outData.w_debug = weight_attrib[i];
				}
			}
		}
		else {*/
			if (num_bones > 0)
			{
				//Linear blend skinning
				Skinning = getBoneMatrixFromTexture(bone_id_attrib[0]) * weight_attrib[0];
				Skinning += getBoneMatrixFromTexture(bone_id_attrib[1]) * weight_attrib[1];
				Skinning += getBoneMatrixFromTexture(bone_id_attrib[2]) * weight_attrib[2];
				Skinning += getBoneMatrixFromTexture(bone_id_attrib[3]) * weight_attrib[3];
			}

			//for debug visualization of bone weights
			/*outData.w_debug = 0.0;
			for (int i = 0; i < 4; i++)
			{
				if (bone_id_attrib[i] == debug_id)
				{
					outData.w_debug = weight_attrib[i];
				}
			}*/
		//}
		

		vec4 anim_pos = Skinning * vec4(pos_attrib, 1.0);

		gl_Position  = PV*M * anim_pos;
		outData.pw = vec3(M*anim_pos);

		vec4 anim_normal = Skinning * vec4(normal_attrib, 0.0);
		outData.nw      = vec3(M * anim_normal);
	}
	else //show mesh in rest pose
	{
		gl_Position  = PV*M * vec4(pos_attrib, 1.0);
		outData.pw = vec3(M*vec4(pos_attrib, 1.0));
		outData.nw   = vec3(M * vec4(normal_attrib, 0.0));
	}
	
	outData.tex_coord = tex_coord_attrib;

}