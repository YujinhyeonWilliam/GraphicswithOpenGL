#shader vertex
#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 a_color;
out vec3 v_Color; // out Keyward�� ���� �ܰ�� �Ѱ��ִ� ��. (��) Vertex Shader -> Fragment Shader���� �� �� �ֵ��� �Ѱ���

void main()
{
	gl_Position = position; // gl_Position�� �־�� �� ���� : Clip Space Coordinates
	v_Color = a_color;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_Color;
in vec3 v_Color; // out keyward�� ���� �ܰ迡�� �Ѿ�� ���� ���� �� in Keyward ���

void main()
{
	color = vec4(v_Color, 1.0);
};