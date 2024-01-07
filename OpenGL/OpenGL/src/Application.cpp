#include <GL/glew.h> //glfw보다 먼저 include해야 함
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


glm::mat4 GetTranslationMatrix(float tx, float ty, float tz)
{
	glm::mat4 TranslationMatrix{  1.0f, 0.0f, 0.0f, tx,
								  0.0f, 1.0f, 0.0f, ty,
								  0.0f, 0.0f, 1.0f, tz,
								  0.0f, 0.0f, 0.0f, 1.0f };

	// :: 열벡터로 들어간 것들을 행벡터로 바꿔줘야함.
	return glm::transpose(TranslationMatrix);
}

glm::mat4 GetProjectionMatrix(float fovy, float aspect, float n, float f)
{
	float inverseAspectRatio = 1 / aspect;
	float halfCot = 1 / (glm::tan(fovy / 2));

	glm::mat4 P{ halfCot * inverseAspectRatio, 0.0f, 0.0f, 0.0f,
				 0.0f, halfCot, 0.0f, 0.0f,
				 0.0f, 0.0f, -(f + n) / (f - n), -2 * n * f / (f - n),
				 0.0f, 0.0f, -1.0f, 0.0f };

	return glm::transpose(P);
}

glm::mat4 GetScaleMatrix(float sx, float sy, float sz)
{
	glm::mat4 scaleMatrix{ sx,   0.0f, 0.0f, 0.0f,
						   0.0f, sy,   0.0f, 0.0f,
						   0.0f, 0.0f, sz,   0.0f,
						   0.0f, 0.0f, 0.0f, 1.0f };

	return scaleMatrix;
}


glm::mat4 GetRotationMatrix(float rx, float ry, float rz)
{ 
	const float DegreeToRad = 3.14f / 180.f;
	float radX = rx * DegreeToRad;
	float radY = ry * DegreeToRad;
	float radZ = rz * DegreeToRad;

	// α -> y축 회전, β -> x축, γ -> z축
	float cosAlpha = cos(radY);
	float cosBeta = cos(radX);
	float cosGamma = cos(radZ);
	
	float sinAlpha = sin(radY);
	float sinBeta = sin(radX);
	float sinGamma = sin(radZ);

	glm::vec4 Xlocal{ cosAlpha * cosGamma + sinAlpha * sinBeta * sinGamma,
					  cosBeta * sinGamma,
					  -sinAlpha * cosGamma + cosAlpha * sinBeta * sinGamma,
					  0.0f};

	glm::vec4 Ylocal{ -cosAlpha * sinGamma + sinAlpha * sinBeta * cosGamma,
					  cosBeta * cosGamma,
					  sinAlpha * sinGamma + cosAlpha * sinBeta * cosGamma,
					  0.0f};

	glm::vec4 Zlocal{ sinAlpha * cosBeta, 
					  -sinBeta, 
					  cosAlpha * cosBeta,
					  0.0f};

	glm::vec4 Wlocal{ 0.0f, 0.0f, 0.0f, 1.0f };

	// glm의 mat4는 열벡터로 꽂아넣어 생성하기 때문에 의도한 행렬의 형태가 바로 나오므로 전치할 필요가 없다.
	glm::mat4 RotationMatrix = glm::mat4{ Xlocal, Ylocal, Zlocal, Wlocal };

	return RotationMatrix;
}

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
			-0.5f, -0.5f, -5.0f,
			 0.5f, -0.5f, -5.0f,
			 0.5f,  0.5f, -5.0f,
			-0.5f,  0.5f, -5.0f,
		};

		// :: index buffer
		unsigned int indices[] =
		{
			0, 1, 2, // t1
			2, 3, 0  // t2
		};

		VertexArray va;
		VertexBuffer vb{ position, 4 * 3 * sizeof(float) };
		
		VertexBufferLayout layout;
		layout.Push<float>(3);

		va.AddBuffer(vb, layout);

		// 데이터 해석 방법 (Vertex Position)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

		// Vertex Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(3 * sizeof(float)));

		IndexBuffer ib{ indices, 6 };

#pragma region Model & View & Projection Matrix 
		
		glm::mat4 RotationMatrix = GetRotationMatrix(0.0f, 0.0f, 45.0f);
		glm::mat4 TransitionMatrix = GetTranslationMatrix(2.0f, -1.0f, 0.0f);
		glm::mat4 ScaleMatrix = GetScaleMatrix(2.0f, 1.0f, 1.0f);
		glm::mat4 modelMatrix = TransitionMatrix * RotationMatrix * ScaleMatrix;

		float FovRadian = 3.14f / 3.0f;
		float AspectRatio = 640.0f / 480.0f;
		float NearClipDist = 1.0f;
	    float FarClipDist = 100.0f;
		glm::mat4 projectionMatrix = GetProjectionMatrix(FovRadian, AspectRatio, NearClipDist, FarClipDist);

		// :: View Matrix는 카메라가 원점에 위치해 있기 때문에 Identity Matrix로 취급. 
		//	  그러므로 따로 행렬을 안만들어주고 위 두 개로만 행렬곱을함

#pragma endregion


#pragma region Shader-related 

		Shader shader{ "res/shaders/Basic.shader" };
		shader.Bind();

#pragma endregion

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
			shader.SetUniformMat4f("u_Model", modelMatrix);
			shader.SetUniformMat4f("u_Proj", projectionMatrix);

			renderer.Draw(va, ib, shader);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	glfwTerminate();
	return 0;
}

