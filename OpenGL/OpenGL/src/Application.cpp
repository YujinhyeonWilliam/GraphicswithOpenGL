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

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragSource;
};


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
			if (line.find("vertex") != std::string::npos) //vertex 셰이더 섹션
			{
				type = ShaderType::VERTEX;
			}
			else if (line.find("fragment") != std::string::npos) //fragment 셰이더 섹션
			{
				type = ShaderType::FRAGMENT;
			}
		}
		else
		{
			ss[(int)type] << line << '\n'; //코드를 stringstream에 삽입
		}
	}

	return { ss[0].str(), ss[1].str() };
}

#pragma region Shader-related Methods

//--------Shader 컴파일 함수----------//
static unsigned int CompileShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type); //셰이더 객체 생성(마찬가지)
	const char* src = source.c_str();
	glShaderSource(id, // 셰이더의 소스 코드 명시, 소스 코드를 명시할 셰이더 객체 id
		1, // 몇 개의 소스 코드를 명시할 것인지
		&src, // 실제 소스 코드가 들어있는 문자열의 주소값
		nullptr); // 해당 문자열 전체를 사용할 경우 nullptr입력, 아니라면 길이 명시
	glCompileShader(id); // id에 해당하는 셰이더 컴파일

#pragma region Error Handling

	// Error Handling(없으면 셰이더 프로그래밍할때 괴롭다...)
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result); //셰이더 프로그램으로부터 컴파일 결과(log)를 얻어옴
	if (result == GL_FALSE) //컴파일에 실패한 경우
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length); //log의 길이를 얻어옴
		char* message = (char*)alloca(length * sizeof(char)); //stack에 동적할당
		glGetShaderInfoLog(id, length, &length, message); //길이만큼 log를 얻어옴
		std::cout << "셰이더 컴파일 실패! " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id); //컴파일 실패한 경우 셰이더 삭제
		return 0;
	}

#pragma endregion

	return id;
}

//--------Shader 프로그램 생성, 컴파일, 링크----------//
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragShader)
{
	unsigned int program = glCreateProgram(); //셰이더 프로그램 객체 생성(int에 저장되는 것은 id)
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragShader);

	// 셰이더 프로그램에는 보통 크게 (1) 버텍스프로그램 (2) 프래그먼트 프로그램이 있다.
	// float데이터를 갖고 vertex Program에 넘겨 정점의 위치를 알려주고 그 이후 fragmentShader에 넘겨주는 것인데 그 안에서 어떻게 그려줄지를 계산하게 된다.
	// 위 두 셰이더 프로그램 (vs, fs)은 꼭 필요하다.

	//컴파일된 셰이더 코드를 program에 추가하고 링크
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	//셰이더 프로그램을 생성했으므로 vs, fs 개별 프로그램은 더이상 필요 없음
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


		ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
		unsigned int shader = CreateShader(source.VertexSource, source.FragSource); // shader프로그램의 Index
		GLCall(glUseProgram(shader)); //StateMachine이기 때문에 UseProgram함수를 통해 어떤 인덱스의 셰이더 프로그램을 활성화시킬지[active(bind)] 알려줘야한다.

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

