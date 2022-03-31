#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>

#include "linmath.h"

#define VBO_MAX_SIZE 65536
#define IBO_MAX_SIZE 98304 // VBO_MAX_SIZE*6/4

// Cube
float cubeVertices[] = {
	// Pos
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, +0.5f,
	-0.5f, +0.5f, -0.5f,
	-0.5f, +0.5f, +0.5f,
	+0.5f, -0.5f, -0.5f,
	+0.5f, -0.5f, +0.5f,
	+0.5f, +0.5f, -0.5f,
	+0.5f, +0.5f, +0.5f
};

unsigned int cubeIndices[] = {
	0, 2, 1,
	1, 2, 3,
	4, 5, 6,
	5, 7, 6,
	0, 1, 5,
	0, 5, 4,
	2, 6, 7,
	2, 7, 3,
	0, 4, 6,
	0, 6, 2,
	1, 3, 7,
	1, 7, 5
};

const char* vertexShaderSource = R"(
#version 410 core
uniform mat4 wvp;
layout (location = 0) in vec3 aPos;
void main()
{
	gl_Position = wvp*vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 410 core
out vec4 FragColor;
void main()
{
	FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
)";

int g_numVertices = 0;
int g_numIndices = 0;
float* g_pVertices = nullptr;
unsigned int* g_pIndices = nullptr;

double g_time = 0.0;

void scanNodes(Alembic::Abc::IObject obj, int depth);
void seek(Alembic::Abc::IObject obj, double time, mat4x4 localMatrix);

auto main() -> int
{
	std::cout << "Begin\n";

	// Load file alembic
	std::shared_ptr<std::fstream> stream{};

	const char* path = "data/example.abc";

	stream.reset(new std::fstream());
	stream->open(path, std::ios::in | std::ios::binary);
	if (stream->is_open() == false)
	{
		std::cout << "Failed to open\n";
		return -1;
	}

	std::vector<std::istream*> streams{ stream.get() };
	Alembic::AbcCoreOgawa::ReadArchive readArchive(streams);

	Alembic::Abc::IArchive archive = Alembic::Abc::IArchive(readArchive(path), Alembic::Abc::kWrapExisting, Alembic::Abc::ErrorHandler::kThrowPolicy);
	if (!archive)
	{
		return -1;
	}

	if (glfwInit() == GLFW_FALSE) return EXIT_FAILURE;

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwSetErrorCallback([](int error, const char* description)
	{
		std::cout << "GLFW Error: " << error << " - " << description << "\n";
	});

	auto pWindow = glfwCreateWindow(800, 600, "Alembic", nullptr, nullptr);
	if (pWindow == nullptr) return EXIT_FAILURE;

	glfwMakeContextCurrent(pWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return EXIT_FAILURE;

	auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	GLint wvpLocation = glGetUniformLocation(program, "wvp");

	GLuint VAO1;
	glGenVertexArrays(1, &VAO1);
	glBindVertexArray(VAO1);
	{
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

			GLuint EBO;
			glGenBuffers(1, &EBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
	}
	glBindVertexArray(0);

	GLuint VAO2;
	GLuint VBO2;
	GLuint EBO2;
	glGenVertexArrays(1, &VAO2);
	glBindVertexArray(VAO2);
	{
		glGenBuffers(1, &VBO2);
		glBindBuffer(GL_ARRAY_BUFFER, VBO2);
			//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

			glGenBuffers(1, &EBO2);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
				//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
	}
	glBindVertexArray(0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	g_numVertices = 0;
	g_numIndices = 0;
	g_pVertices = new float[VBO_MAX_SIZE];
	g_pIndices = new unsigned int[IBO_MAX_SIZE];

	//scanNodes(archive.getTop(), 0);
	//mat4x4 localMatrix;
	//mat4x4_identity(localMatrix);
	//seek(archive.getTop(), 0.0f, localMatrix);

	GLfloat deltaTime = 0.0f;
	GLfloat lastFrame = 0.0f;
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(pWindow))
	{
		// Update
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

		g_numVertices = 0;
		g_numIndices = 0;
		mat4x4 localMatrix;
		mat4x4_identity(localMatrix);

		seek(archive.getTop(), g_time, localMatrix);
		g_time += 1.1*deltaTime;
		if (g_time > 3.0)
		{
			g_time = 0.0;
		}

		// Draw
		int width, height;
		glfwGetFramebufferSize(pWindow, &width, &height);

		mat4x4 world, view, proj, wvp;

		mat4x4_identity(world);
		//mat4x4_scale(world, world, 200.0f);
		//mat4x4_scale_aniso(world, world, 0.6f, 1.0f, 1.0f);
		//mat4x4_translate(world, 0.5f, 0.0f, 0.0f);
		mat4x4_rotate_Y(world, world, 0.6*(float)glfwGetTime());

		vec3 eye    = { 0.0f, 6.0f, 8.0f };
		vec3 center = { 0.0f, 0.0f, 0.0f };
		vec3 up     = { 0.0f, 1.0f, 0.0f };
		mat4x4_look_at(view, eye, center, up);

		mat4x4_perspective(proj, 45.0f*M_PI/180.0f, (float)width/height, 0.01, 100.0f);

		mat4x4_identity(wvp);
		mat4x4_mul(wvp, world, wvp);
		mat4x4_mul(wvp, view, wvp);
		mat4x4_mul(wvp, proj, wvp);

		glClearColor(0.0f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, width, height);

		glUseProgram(program);
		glUniformMatrix4fv(wvpLocation, 1, GL_FALSE, (const GLfloat*)wvp);

		glBindVertexArray(VAO1);
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glBindVertexArray(VAO2);
		glBindBuffer(GL_ARRAY_BUFFER, VBO2);
			glBufferData(GL_ARRAY_BUFFER, g_numVertices*3*sizeof(float), NULL, GL_DYNAMIC_DRAW);
			glBufferData(GL_ARRAY_BUFFER, g_numVertices*3*sizeof(float), g_pVertices, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_numIndices*sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_numIndices*sizeof(unsigned int), g_pIndices, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawElements(GL_TRIANGLES, g_numIndices, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(pWindow);
		glfwPollEvents();
	}

	delete[] g_pVertices;
	delete[] g_pIndices;

	glfwDestroyWindow(pWindow);

    glfwTerminate();

	std::cout << "End\n";

	return 0;
}

void scanNodes(Alembic::Abc::IObject obj, int depth)
{
	for (size_t i = 0; i < depth; ++i)
	{
		std::cout << '\t';
	}

	std::cout << obj.getName() << " ";

	const auto& metadata = obj.getMetaData();
	if (Alembic::AbcGeom::IXformSchema::matches(metadata))
	{
		std::cout << "(IXformSchema)";
	}
	else if (Alembic::AbcGeom::ICameraSchema::matches(metadata))
	{
		std::cout << "(ICameraSchema)";
	}
	else if (Alembic::AbcGeom::IPolyMeshSchema::matches(metadata))
	{
		std::cout << "(IPolyMeshSchema)";
	}

	std::cout << std::endl;

	size_t numChildren = obj.getNumChildren();
	for (size_t i = 0; i < numChildren; ++i)
	{
		scanNodes(obj.getChild(i), depth + 1);
	}
}

void seek(Alembic::Abc::IObject obj, double time, mat4x4 localMatrix)
{
	mat4x4 matrix;
	mat4x4_identity(matrix);
	mat4x4_mul(matrix, localMatrix, matrix);

	Alembic::Abc::ISampleSelector sampleSelector = Alembic::Abc::ISampleSelector(time);

	const auto& metadata = obj.getMetaData();
	if      (Alembic::AbcGeom::IXformSchema::matches(metadata))
	{
		Alembic::AbcGeom::IXformSchema schema = Alembic::AbcGeom::IXform(obj).getSchema();
		Alembic::AbcGeom::XformSample sample;

		schema.get(sample, sampleSelector);

		double* matrixValue = sample.getMatrix().getValue();
		mat4x4 m;
		for (size_t i = 0; i < 4; ++i)
		{
			for (size_t j = 0; j < 4; ++j)
			{
				m[i][j] = (float)matrixValue[i*4 + j];
			}
		}
		mat4x4_mul(matrix, m, localMatrix);
	}
	else if (Alembic::AbcGeom::IPolyMeshSchema::matches(metadata))
	{
		Alembic::AbcGeom::IPolyMeshSchema schema = Alembic::AbcGeom::IPolyMesh(obj).getSchema();
		Alembic::AbcGeom::IPolyMeshSchema::Sample sample;

		schema.get(sample, sampleSelector);

		int numFaces = sample.getFaceCounts().get()->size();
		//std::cout << obj.getName() << " " << numFaces << std::endl;

		int indexOffset = g_numVertices;
		// if (numFaces == 6)
		{
			int numPositions = sample.getPositions().get()->size();
			float* positions = (float*)sample.getPositions().get()->getData();
			for (size_t i = 0; i < numPositions; ++i)
			{
				int verticeId = g_numVertices++;

				vec4 pos = { positions[i*3 + 0], positions[i*3 + 1], positions[i*3 + 2], 1.0f };
				vec4 result;
				mat4x4_mul_vec4(result, matrix, pos);
				g_pVertices[verticeId*3 + 0] = result[0];
				g_pVertices[verticeId*3 + 1] = result[1];
				g_pVertices[verticeId*3 + 2] = result[2];
			}

			int* pIndices = (int*)sample.getFaceIndices().get()->getData();
			int startIndiceId = 0;
			for (size_t i = 0; i < numFaces; ++i)
			{
				int faceCount = sample.getFaceCounts().get()->get()[i];
				if      (faceCount == 3)
				{
					int indiceId = g_numIndices;

					g_pIndices[indiceId + 0] = (unsigned int)(pIndices[startIndiceId + 0] + indexOffset);
					g_pIndices[indiceId + 1] = (unsigned int)(pIndices[startIndiceId + 1] + indexOffset);
					g_pIndices[indiceId + 2] = (unsigned int)(pIndices[startIndiceId + 2] + indexOffset);

					g_numIndices += 3;
					startIndiceId += 3;
				}
				else if (faceCount == 4)
				{
					int indiceId = g_numIndices;

					g_pIndices[indiceId + 0] = (unsigned int)(pIndices[startIndiceId + 0] + indexOffset);
					g_pIndices[indiceId + 1] = (unsigned int)(pIndices[startIndiceId + 1] + indexOffset);
					g_pIndices[indiceId + 2] = (unsigned int)(pIndices[startIndiceId + 2] + indexOffset);

					g_pIndices[indiceId + 3] = (unsigned int)(pIndices[startIndiceId + 0] + indexOffset);
					g_pIndices[indiceId + 4] = (unsigned int)(pIndices[startIndiceId + 2] + indexOffset);
					g_pIndices[indiceId + 5] = (unsigned int)(pIndices[startIndiceId + 3] + indexOffset);

					g_numIndices += 6;
					startIndiceId += 4;
				}
				else
				{
					std::cout << "Wrong faceCount: "<< faceCount << std::endl;
				}
			}
		}
	}

	size_t numChildren = obj.getNumChildren();
	for (size_t i = 0; i < numChildren; ++i)
	{
		seek(obj.getChild(i), time, matrix);
	}
}
