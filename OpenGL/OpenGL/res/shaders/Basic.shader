#shader vertex
#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 a_color;
out vec3 v_Color; // out Keyward는 다음 단계로 넘겨주는 것. (예) Vertex Shader -> Fragment Shader에서 쓸 수 있도록 넘겨줌

void main()
{
	gl_Position = position; // gl_Position에 넣어야 할 정보 : Clip Space Coordinates
	v_Color = a_color;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_Color;
in vec3 v_Color; // out keyward로 이전 단계에서 넘어온 것을 받을 때 in Keyward 사용

void main()
{
	color = vec4(v_Color, 1.0);
};