#include <GL/glew.h> //glfw보다 먼저 include해야 함
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "VertexBufferLayout.h"

#pragma region State machine vs OOP

// OpenGL은 State Machine과 비슷함.일반적인 객체 지향 프로그램의 설계와는 다르게,
// 현재 처리할 데이터를 선택한 후, 현재 실행해야 할 작업을 함수 호출을 통해 처리하는 방식임
// 간단하게 두 삼각형을 화면에 그리는 의사코드로 설명하면,
// ---객체지향 방식
// Triangle t1, t2; //삼각형 두 개를 정의
// Draw(t1); //t1 객체를 전달함으로써 삼각형 1을 그림
// Draw(t2); //t2 객체를 전달함으로써 삼각형 2를 그림

// ---State Machine 방식
// Triangle t1, t2; //삼각형 두 개를 정의
// Activate(t1); //삼각형 1을 처리중인 상태로 설정
// Draw(); //현재 처리중인 데이터(=삼각형 1)를 화면에 그림
// Activate(t2); //삼각형 2를 처리중인 상태로 설정
// Draw(); //현재 처리중인 데이터(=삼각형 2)를 화면에 그림
#pragma endregion

//셰이더 파일 파싱 함수

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

	// glfwMakeContextCurrent가 호출된 후에 glewInit이 수행되어야 함
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Error\n";
	}

	std::cout << glGetString(GL_VERSION) << std::endl; //내 플랫폼의 GL_Version 출력해보기

	// ▼ 지역 스코프 두는 이유가 Stack에 객체 만들어놨으니 소멸자 알아서 잘 호출되게끔 할려고함
	{
		// :: 백스페이스 컬링
		glEnable(GL_CULL_FACE);

		// :: 정점의 위치들 (vertex positions)
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

		// 데이터 해석 방법 (Vertex Position)
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

