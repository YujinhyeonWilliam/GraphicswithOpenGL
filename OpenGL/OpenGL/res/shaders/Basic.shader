#shader vertex
#version 330 core

layout(location = 0) in vec4 position;
//layout(location = 1) in vec3 a_color;

uniform mat4 u_Model;
uniform mat4 u_Proj;
void main()
{
	gl_Position = u_Proj * u_Model * position; // Clip Space Coordinates
	//v_Color = a_color;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

//uniform vec4 u_Color;
//in vec3 v_Color; // out keyward�� ���� �ܰ迡�� �Ѿ�� ���� ���� �� in Keyward ���

void main()
{
	color = vec4(1.0, 0.0, 0.0, 1.0);
	//color = vec4(v_Color, 1.0);
};