#version 330

varying vec4 vColor;
uniform mat4 gWVP;

void main()
{
    vColor = gl_Color;
	gl_Position = gWVP * gl_Vertex;
}
