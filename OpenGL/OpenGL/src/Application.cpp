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
#include "Window.h"

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

	// :: �����ͷ� �� �͵��� �຤�ͷ� �ٲ������.
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

	// �� -> y�� ȸ��, �� -> x��, �� -> z��
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

	// glm�� mat4�� �����ͷ� �ȾƳ־� �����ϱ� ������ �ǵ��� ����� ���°� �ٷ� �����Ƿ� ��ġ�� �ʿ䰡 ����.
	glm::mat4 RotationMatrix = glm::mat4{ Xlocal, Ylocal, Zlocal, Wlocal };

	return RotationMatrix;
}

int main(void)
{

	Window mainWindow{ 800,600 };
	mainWindow.Initialize();

	// �� ���� ������ �δ� ������ Stack�� ��ü ���������� �Ҹ��� �˾Ƽ� �� ȣ��ǰԲ� �ҷ�����
	{

		// :: ������ ��ġ�� (vertex positions)
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

		// ������ �ؼ� ��� (Vertex Position)
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

		// :: View Matrix�� ī�޶� ������ ��ġ�� �ֱ� ������ Identity Matrix�� ���. 
		//	  �׷��Ƿ� ���� ����� �ȸ�����ְ� �� �� ���θ� ��İ�����

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

		while (!mainWindow.GetShouldClose())
		{
			renderer.Clear();

			va.Bind();

			shader.Bind();
			shader.SetUniformMat4f("u_Model", modelMatrix);
			shader.SetUniformMat4f("u_Proj", projectionMatrix);

			renderer.Draw(va, ib, shader);

			mainWindow.SwapBuffers();
			glfwPollEvents();
		}
	}

	return 0;
}

