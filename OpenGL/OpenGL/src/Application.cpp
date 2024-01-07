#include <GL/glew.h> //glfw���� ���� include�ؾ� ��
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

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


glm::mat4 GetTranslationMatrix(float tx, float ty, float tz)
{
	glm::mat4 TranslationMatrix{  1.0f, 0.0f, 0.0f, tx,
								  0.0f, 1.0f, 0.0f, ty,
								  0.0f, 0.0f, 1.0f, tz,
								  0.0f, 0.0f, 0.0f, 1.0f };
	return TranslationMatrix;
}

int main(void)
{
	glm::mat4 T = GetTranslationMatrix(1.0f, 0.0f, 0.0f);
	glm::vec4 position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	glm::vec4 translatedPosition = T * position;


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

		Shader shader{ "res/shaders/Basic.shader" };
		shader.Bind();

		va.Unbind();
		vb.UnBind();
		ib.UnBind();
		shader.Unbind();

		Renderer renderer;

		while (!glfwWindowShouldClose(window))
		{
			renderer.Clear();

			va.Bind();
			shader.Bind();
			renderer.Draw(va, ib, shader);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	glfwTerminate();
	return 0;
}

