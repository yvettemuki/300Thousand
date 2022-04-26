#version 430

layout(location = 0) uniform int collisionType;

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	if (collisionType == 0)
		fragcolor = vec4(0, 0, 0, 0);

	if (collisionType == 1)
		fragcolor = vec4(1, 0, 0, 1);
}