//http://glew.sourceforge.net/
//사이트에서 GLEW 바이너리 버전다운로드. 필요 라이브러리 dependencies에 복사 후 설정
//http://glew.sourceforge.net/basic.html
//아래 예제 코드실행 확인 

//GLEW_STATIC Define 필요
#include <GL/glew.h> //glfw보다 먼저 include해야 함
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragSource;
};

//__debugbreak()는 MSVC에만 사용 가능
#define ASSERT(x) if ((!(x))) __debugbreak(); 
#define GLCall(x) GLClearError();\
				  x;\
				  ASSERT(GLLogCall(#x, __FILE__, __LINE__))

//glGetError는 에러를 하나씩만 반환하기 때문에, 한 번 확인에 모든 오류를 뽑아내는 것이 필요함
static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR); // GL_NO_ERROR == 0
}

static bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGL Error] (" << error << ") : " << function <<
			" " << file << " in line " << line << std::endl;
		return false;
	}
	return true;
}


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

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// glfwMakeContextCurrent가 호출된 후에 glewInit이 수행되어야 함
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Error\n";
	}

	std::cout << glGetString(GL_VERSION) << std::endl; //내 플랫폼의 GL_Version 출력해보기

	// :: 백스페이스 컬링
	glEnable(GL_CULL_FACE);

	// :: 정점의 위치들 (vertex positions)
	float position[] = {
		-0.5f, -0.5f,  0.0f, // 0 v
		 0.5f, -0.5f,  0.0f, // 1 v
		 0.5f,  0.5f,  0.0f, // 2 v
		-0.5f,  0.5f,  0.0f // 3 v
	};

	// :: index buffer
	unsigned int indices[] =
	{
		0, 1, 2, // t1
		2, 3, 0  // t2
	};

	// :: CPU에서 GPU로 정보를 전달하기 위해서 버퍼라는 개체를 만들어 사용한다.
	unsigned int bufferID;
	glGenBuffers(1, &bufferID); 
	glBindBuffer(GL_ARRAY_BUFFER, bufferID); 									
	glBufferData(GL_ARRAY_BUFFER, 
				 12 * sizeof(float), 
				 position, 
				 GL_STATIC_DRAW); 

	unsigned int ibo; // Index Buffer Object 
	glGenBuffers(1, &ibo); // gpu 사이드에다가 버퍼를 생성해주고 그 버퍼에 ibo 주소값을 넘겨 저장한다.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); // 위에서 gen해줬던 버퍼를 activate해준다. 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		6 * sizeof(unsigned int),
		indices,
		GL_STATIC_DRAW); // 버퍼 데이터를 통해서 데이터를 어떻게 읽어야하는지 알려준다.

	// 데이터 해석 방법
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 
						  3, // 하나의 vertex에 몇 개의 데이터를 넘기는지 
						  GL_FLOAT, // 데이터 타입
						  GL_FALSE, // 정규화가 필요한가 
						  sizeof(float) * 3, // 사이즈
						  0); // offset

	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.VertexSource, source.FragSource); // shader프로그램의 Index
	GLCall(glUseProgram(shader)); //StateMachine이기 때문에 UseProgram함수를 통해 어떤 인덱스의 셰이더 프로그램을 활성화시킬지[active(bind)] 알려줘야한다.
	

	// :: 셰이더 내 Uniform 변수에 값을 넣어줌
	int location = glGetUniformLocation(shader, "u_Color");
	ASSERT(location != -1);
	glUniform4f(location, 0.2f, 0.3f, 0.8f, 1.0f);


	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		// glDrawArrays(GL_TRIANGLES, 0, 3);  !이제 인덱스 버퍼를 이용하기 때문에 drawcall 함수를 아래거로 써야한다.!

		GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glDeleteProgram(shader);
	glfwTerminate();
	return 0;
}

