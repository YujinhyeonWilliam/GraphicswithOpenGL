#include <GL/glew.h> //glfw���� ���� include�ؾ� ��
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragSource;
};


#pragma region State machine vs OOP

// OpenGL�� State Machine�� �����.�Ϲ����� ��ü ���� ���α׷��� ����ʹ� �ٸ���,
// ���� ó���� �����͸� ������ ��, ���� �����ؾ� �� �۾��� �Լ� ȣ���� ���� ó���ϴ� �����
// �����ϰ� �� �ﰢ���� ȭ�鿡 �׸��� �ǻ��ڵ�� �����ϸ�,
// ---��ü���� ���
// Triangle t1, t2; //�ﰢ�� �� ���� ����
// Draw(t1); //t1 ��ü�� ���������ν� �ﰢ�� 1�� �׸�
// Draw(t2); //t2 ��ü�� ���������ν� �ﰢ�� 2�� �׸�

// ---State Machine ���
// Triangle t1, t2; //�ﰢ�� �� ���� ����
// Activate(t1); //�ﰢ�� 1�� ó������ ���·� ����
// Draw(); //���� ó������ ������(=�ﰢ�� 1)�� ȭ�鿡 �׸�
// Activate(t2); //�ﰢ�� 2�� ó������ ���·� ����
// Draw(); //���� ó������ ������(=�ﰢ�� 2)�� ȭ�鿡 �׸�
#pragma endregion

//���̴� ���� �Ľ� �Լ�
static ShaderProgramSource ParseShader(const std::string& filepath)
{
	std::ifstream stream(filepath);


	enum class ShaderType
	{
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::string line;
	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;
	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos) //vertex ���̴� ����
			{
				type = ShaderType::VERTEX;
			}
			else if (line.find("fragment") != std::string::npos) //fragment ���̴� ����
			{
				type = ShaderType::FRAGMENT;
			}
		}
		else
		{
			ss[(int)type] << line << '\n'; //�ڵ带 stringstream�� ����
		}
	}

	return { ss[0].str(), ss[1].str() };
}

#pragma region Shader-related Methods

//--------Shader ������ �Լ�----------//
static unsigned int CompileShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type); //���̴� ��ü ����(��������)
	const char* src = source.c_str();
	glShaderSource(id, // ���̴��� �ҽ� �ڵ� ���, �ҽ� �ڵ带 ����� ���̴� ��ü id
		1, // �� ���� �ҽ� �ڵ带 ����� ������
		&src, // ���� �ҽ� �ڵ尡 ����ִ� ���ڿ��� �ּҰ�
		nullptr); // �ش� ���ڿ� ��ü�� ����� ��� nullptr�Է�, �ƴ϶�� ���� ���
	glCompileShader(id); // id�� �ش��ϴ� ���̴� ������

#pragma region Error Handling

	// Error Handling(������ ���̴� ���α׷����Ҷ� ���Ӵ�...)
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result); //���̴� ���α׷����κ��� ������ ���(log)�� ����
	if (result == GL_FALSE) //�����Ͽ� ������ ���
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length); //log�� ���̸� ����
		char* message = (char*)alloca(length * sizeof(char)); //stack�� �����Ҵ�
		glGetShaderInfoLog(id, length, &length, message); //���̸�ŭ log�� ����
		std::cout << "���̴� ������ ����! " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id); //������ ������ ��� ���̴� ����
		return 0;
	}

#pragma endregion

	return id;
}

//--------Shader ���α׷� ����, ������, ��ũ----------//
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragShader)
{
	unsigned int program = glCreateProgram(); //���̴� ���α׷� ��ü ����(int�� ����Ǵ� ���� id)
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragShader);

	// ���̴� ���α׷����� ���� ũ�� (1) ���ؽ����α׷� (2) �����׸�Ʈ ���α׷��� �ִ�.
	// float�����͸� ���� vertex Program�� �Ѱ� ������ ��ġ�� �˷��ְ� �� ���� fragmentShader�� �Ѱ��ִ� ���ε� �� �ȿ��� ��� �׷������� ����ϰ� �ȴ�.
	// �� �� ���̴� ���α׷� (vs, fs)�� �� �ʿ��ϴ�.

	//�����ϵ� ���̴� �ڵ带 program�� �߰��ϰ� ��ũ
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	//���̴� ���α׷��� ���������Ƿ� vs, fs ���� ���α׷��� ���̻� �ʿ� ����
	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

#pragma endregion

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "OpenGL Study", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	#pragma region OPENGL 3.3 Apply
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#pragma endregion

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// glfwMakeContextCurrent�� ȣ��� �Ŀ� glewInit�� ����Ǿ�� ��
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Error\n";
	}

	std::cout << glGetString(GL_VERSION) << std::endl; //�� �÷����� GL_Version ����غ���

	// �� ���� ������ �δ� ������ Stack�� ��ü ���������� �Ҹ��� �˾Ƽ� �� ȣ��ǰԲ� �ҷ�����
	{
		// :: �齺���̽� �ø�
		glEnable(GL_CULL_FACE);

		// :: ������ ��ġ�� (vertex positions)
		float position[] = {
			-0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 0.0f,// 0 Vertex Pos, Vertex Color
			 0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 0.0f,// 1 " "
			 0.5f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f,// 2 " "
			-0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f,// 3 " "
		};

		// :: index buffer
		unsigned int indices[] =
		{
			0, 1, 2, // t1
			2, 3, 0  // t2
		};

		VertexArray va;
		VertexBuffer vb{ position, 4 * 6 * sizeof(float) };
		
		VertexBufferLayout layout;
		layout.Push<float>(3);
		layout.Push<float>(3);

		va.AddBuffer(vb, layout);

		// ������ �ؼ� ��� (Vertex Position)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);

		// Vertex Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(3 * sizeof(float)));

		IndexBuffer ib{ indices, 6 };


		ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
		unsigned int shader = CreateShader(source.VertexSource, source.FragSource); // shader���α׷��� Index
		GLCall(glUseProgram(shader)); //StateMachine�̱� ������ UseProgram�Լ��� ���� � �ε����� ���̴� ���α׷��� Ȱ��ȭ��ų��[active(bind)] �˷�����Ѵ�.

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glUseProgram(0);

		while (!glfwWindowShouldClose(window))
		{
			GLCall(glClear(GL_COLOR_BUFFER_BIT));

			GLCall(glUseProgram(shader));
			va.Bind();
			GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		GLCall(glDeleteProgram(shader));
	}

	glfwTerminate();
	return 0;
}

