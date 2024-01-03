//http://glew.sourceforge.net/
//����Ʈ���� GLEW ���̳ʸ� �����ٿ�ε�. �ʿ� ���̺귯�� dependencies�� ���� �� ����
//http://glew.sourceforge.net/basic.html
//�Ʒ� ���� �ڵ���� Ȯ�� 

//GLEW_STATIC Define �ʿ�
#include <GL/glew.h> //glfw���� ���� include�ؾ� ��
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

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// glfwMakeContextCurrent�� ȣ��� �Ŀ� glewInit�� ����Ǿ�� ��
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Error\n";
	}

	std::cout << glGetString(GL_VERSION) << std::endl; //�� �÷����� GL_Version ����غ���

	// :: �齺���̽� �ø�
	glEnable(GL_CULL_FACE);

	// :: ������ ��ġ�� (vertex positions)
	float position[] = {
		-0.5f, -0.5f, 0.0f, // 0 v
		 0.5f, -0.5f, 0.0f, // 1 v
		 0.5f,  0.5f, 0.0f, // 2 v
		-0.5f,  0.5f, 0.0f // 3 v
	};

	// :: index buffer
	unsigned int indices[] =
	{
		0, 1, 2, // t1
		2, 3, 0  // t2
	};

	// :: CPU���� GPU�� ������ �����ϱ� ���ؼ� ���۶�� ��ü�� ����� ����Ѵ�.
	unsigned int bufferID;
	glGenBuffers(1, &bufferID); 
	glBindBuffer(GL_ARRAY_BUFFER, bufferID); 									
	glBufferData(GL_ARRAY_BUFFER, 
				 12 * sizeof(float), 
				 position, 
				 GL_STATIC_DRAW); 

	unsigned int ibo; // Index Buffer Object 
	glGenBuffers(1, &ibo); // gpu ���̵忡�ٰ� ���۸� �������ְ� �� ���ۿ� ibo �ּҰ��� �Ѱ� �����Ѵ�.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); // ������ gen����� ���۸� activate���ش�. 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		6 * sizeof(unsigned int),
		indices,
		GL_STATIC_DRAW); // ���� �����͸� ���ؼ� �����͸� ��� �о���ϴ��� �˷��ش�.

	// ������ �ؼ� ���
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 
						  3, // �ϳ��� vertex�� �� ���� �����͸� �ѱ���� 
						  GL_FLOAT, // ������ Ÿ��
						  GL_FALSE, // ����ȭ�� �ʿ��Ѱ� 
						  sizeof(float) * 3, // ������
						  0); // offset

	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.VertexSource, source.FragSource); // shader���α׷��� Index
	glUseProgram(shader); //StateMachine�̱� ������ UseProgram�Լ��� ���� � �ε����� ���̴� ���α׷��� Ȱ��ȭ��ų��[active(bind)] �˷�����Ѵ�.
	
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		// glDrawArrays(GL_TRIANGLES, 0, 3);  !���� �ε��� ���۸� �̿��ϱ� ������ drawcall �Լ��� �Ʒ��ŷ� ����Ѵ�.!

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glDeleteProgram(shader);
	glfwTerminate();
	return 0;
}

