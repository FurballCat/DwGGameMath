#include "dwgSimpleGraphics.h"
#include <stdlib.h>
#include <stdio.h>
#include "glad/glad.h"
#include "glfw3.h"
#include "vectormath.hpp"
#include <cassert>
#include <string.h>

// vertex shader code
static const char* vertex_shader_text =
"#version 110\n"
"uniform mat4 MVP;\n"
"uniform vec3 tint;\n"
"attribute vec3 vCol;\n"
"attribute vec3 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 1.0);\n"
"    color = vCol * tint;\n"
"}\n";

// pixel/fragment shader code
static const char* fragment_shader_text =
"#version 110\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

#define DWG_MAX_DEBUG_VERTICES 4096
#define DWG_MAX_DEBUG_SPHERES 1024

#define DWG_MAX(_x, _y) _x > _y ? _x : _y;

struct DebugVertex
{
	float x, y, z;
	float r, g, b;
};

struct DebugSphere
{
	Vector3 pos;
	Vector3 color;
	Vector3 scale;
};

struct DwGSimpleGraphics
{
	GLFWwindow* window = nullptr;
	
	// graphics pipeline
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;
	GLuint shaderProgram = 0;
	GLuint vertexShaderMVPLoc = 0;
	GLuint vertexShaderPositionLoc = 0;
	GLuint vertexShaderColorLoc = 0;
	GLuint vertexShaderTintLoc = 0;

	// debug lines
	DebugVertex* dataLines = nullptr;
	int32_t numDataLines = 0;
	GLuint vertexBufferLines = 0;

	// debug spheres
	DebugVertex* dataSphereMesh = nullptr;
	int32_t numDataSphereVertices = 0;
	
	uint16_t* sphereIndices = nullptr;
	int32_t numSphereIndices = 0;

	GLuint vertexBufferSphereMesh = 0;
	GLuint indexBufferSphereMesh = 0;

	DebugSphere* spheres = nullptr;
	int32_t numSpheres = 0;

	// time
	double globalTime = 0.0f;
	float deltaTime = 0.016f;
};

DwGSimpleGraphics g_dwg;

bool dwgInitApp(int32_t width, int32_t height, const char* title)
{
	// init window
	{
		glfwSetErrorCallback(error_callback);

		if (!glfwInit())
			return false;

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		g_dwg.window = glfwCreateWindow(1600, 900, "DwG - Game Math", NULL, NULL);
		if (!g_dwg.window)
		{
			glfwTerminate();
			return false;
		}

		glfwSetKeyCallback(g_dwg.window, key_callback);
	}
	
	// init OpenGL
	{
		glfwMakeContextCurrent(g_dwg.window);
		gladLoadGL();
		glfwSwapInterval(1);
	}
	
	// init rendering
	{
		g_dwg.vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(g_dwg.vertexShader, 1, &vertex_shader_text, NULL);
		glCompileShader(g_dwg.vertexShader);

		g_dwg.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(g_dwg.fragmentShader, 1, &fragment_shader_text, NULL);
		glCompileShader(g_dwg.fragmentShader);

		g_dwg.shaderProgram = glCreateProgram();
		glAttachShader(g_dwg.shaderProgram, g_dwg.vertexShader);
		glAttachShader(g_dwg.shaderProgram, g_dwg.fragmentShader);
		glLinkProgram(g_dwg.shaderProgram);

		g_dwg.vertexShaderMVPLoc = glGetUniformLocation(g_dwg.shaderProgram, "MVP");
		g_dwg.vertexShaderTintLoc = glGetUniformLocation(g_dwg.shaderProgram, "tint");
		g_dwg.vertexShaderPositionLoc = glGetAttribLocation(g_dwg.shaderProgram, "vPos");
		g_dwg.vertexShaderColorLoc = glGetAttribLocation(g_dwg.shaderProgram, "vCol");

		glEnable(GL_DEPTH_TEST);

		g_dwg.globalTime = glfwGetTime();
	}

	// create debug lines vertex buffer
	{
		g_dwg.dataLines = new DebugVertex[DWG_MAX_DEBUG_VERTICES];
		memset(g_dwg.dataLines, 0, sizeof(DebugVertex) * DWG_MAX_DEBUG_VERTICES);

		glGenBuffers(1, &g_dwg.vertexBufferLines);
		glBindBuffer(GL_ARRAY_BUFFER, g_dwg.vertexBufferLines);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * DWG_MAX_DEBUG_VERTICES, g_dwg.dataLines, GL_DYNAMIC_DRAW);
	}

	// init debug spheres mesh vertex buffer + array
	{
		g_dwg.spheres = new DebugSphere[DWG_MAX_DEBUG_SPHERES];
		g_dwg.numSpheres = 0;

		const int32_t stackCount = 20;
		const int32_t sectorCount = 30;
		const float radius = 1.0f;

		g_dwg.numDataSphereVertices = (stackCount + 1) * (sectorCount + 1);

		g_dwg.dataSphereMesh = new DebugVertex[g_dwg.numDataSphereVertices];
		memset(g_dwg.dataSphereMesh, 0, sizeof(DebugVertex) * g_dwg.numDataSphereVertices);

		float x, y, z, xy;								// vertex position

		float sectorStep = 2 * (float)DWG_PI / sectorCount;
		float stackStep = (float)DWG_PI / stackCount;
		float sectorAngle, stackAngle;

		DebugVertex* vertex = g_dwg.dataSphereMesh;

		const Vector3 fakeLightDir = { 1.0f, 1.0f, 1.0f };

		for (int32_t i = 0; i <= stackCount; ++i)
		{
			stackAngle = (float)DWG_PI / 2 - i * stackStep;	// starting from pi/2 to -pi/2
			xy = radius * cosf(stackAngle);				// r * cos(u)
			z = radius * sinf(stackAngle);				// r * sin(u)

			// add (sectorCount+1) vertices per stack
			// the first and last vertices have same position
			for (int32_t j = 0; j <= sectorCount; ++j)
			{
				sectorAngle = j * sectorStep;           // starting from 0 to 2pi

				// vertex position (x, y, z)
				x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
				y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
				
				vertex->x = x;
				vertex->y = y;
				vertex->z = z;

				Vector3 normal(x, y, z);
				normal = normalize(normal);	// just in case
				const float intensity = clamp(dot(normal, fakeLightDir), 0.0f, 1.0f);

				vertex->r = 0.2f + intensity * 0.8f;
				vertex->g = 0.2f + intensity * 0.8f;
				vertex->b = 0.2f + intensity * 0.8f;

				vertex++;
			}
		}

		glGenBuffers(1, &g_dwg.vertexBufferSphereMesh);
		glBindBuffer(GL_ARRAY_BUFFER, g_dwg.vertexBufferSphereMesh);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex)* g_dwg.numDataSphereVertices, g_dwg.dataSphereMesh, GL_STATIC_DRAW);

		// todo: rewrite that... I'm lazy
		g_dwg.numSphereIndices = 0;
		for (int i = 0; i < stackCount; ++i)
		{
			for (int j = 0; j < sectorCount; ++j)
			{
				if (i != 0)
				{
					g_dwg.numSphereIndices += 3;
				}

				if (i != (stackCount - 1))
				{
					g_dwg.numSphereIndices += 3;
				}
			}
		}

		g_dwg.sphereIndices = new uint16_t[g_dwg.numSphereIndices];
		uint16_t* index = g_dwg.sphereIndices;

		// generate CW index list of sphere triangles
		// k1--k1+1
		// |  / |
		// | /  |
		// k2--k2+1
		uint16_t k1, k2;
		for (uint16_t i = 0; i < stackCount; ++i)
		{
			k1 = i * (sectorCount + 1);	// beginning of current stack
			k2 = k1 + sectorCount + 1;	// beginning of next stack

			for (uint16_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
			{
				// 2 triangles per sector excluding first and last stacks
				// k1 => k2 => k1+1
				if (i != 0)
				{
					index[0] = k1;
					index[1] = k1 + 1;
					index[2] = k2;

					index += 3;
				}

				// k1+1 => k2 => k2+1
				if (i != (stackCount - 1))
				{
					index[0] = k1 + 1;
					index[1] = k2 + 1;
					index[2] = k2;

					index += 3;
				}
			}
		}

		glGenBuffers(1, &g_dwg.indexBufferSphereMesh);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_dwg.indexBufferSphereMesh);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * g_dwg.numSphereIndices, g_dwg.sphereIndices, GL_STATIC_DRAW);
	}

	return true;
}

bool dwgShouldClose()
{
	return glfwWindowShouldClose(g_dwg.window);
}

void dwgRender(const Matrix4& camera, const float fov)
{
	// update debug vertex buffers
	{
		// lines
		if (g_dwg.numDataLines > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, g_dwg.vertexBufferLines);
			void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			GLenum err = glGetError();
			memcpy(ptr, g_dwg.dataLines, sizeof(DebugVertex) * g_dwg.numDataLines);
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}
	}

	// render current frame
	{
		float ratio;
		int width, height;

		glfwGetFramebufferSize(g_dwg.window, &width, &height);
		ratio = width / (float)height;

		const float viewHalfLength = 10.0f;
		Matrix4 p = Matrix4::perspective(fov * (float)DWG_PI / 360.0f, ratio, 0.1f, 1000.0f);
		Matrix4 mvp = p * camera;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(g_dwg.shaderProgram);

		// draw lines
		if(g_dwg.numDataLines > 0)
		{
			const Vector3 colorWhite = { 1.0f, 1.0f, 1.0f };
			glUniform3fv(g_dwg.vertexShaderTintLoc, 1, toFloatPtr(colorWhite));
			glBindBuffer(GL_ARRAY_BUFFER, g_dwg.vertexBufferLines);
			glUniformMatrix4fv(g_dwg.vertexShaderMVPLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

			// todo: do I need to call it here? I don't remember...
			glEnableVertexAttribArray(g_dwg.vertexShaderPositionLoc);
			glVertexAttribPointer(g_dwg.vertexShaderPositionLoc, 3, GL_FLOAT, GL_FALSE,
				sizeof(DebugVertex), (void*)0);
			glEnableVertexAttribArray(g_dwg.vertexShaderColorLoc);
			glVertexAttribPointer(g_dwg.vertexShaderColorLoc, 3, GL_FLOAT, GL_FALSE,
				sizeof(DebugVertex), (void*)(sizeof(float) * 3));

			glDrawArrays(GL_LINES, 0, g_dwg.numDataLines);
		}

		if (g_dwg.numSpheres > 0)
		{
			Matrix4 mvpSphere = Matrix4::identity();

			glBindBuffer(GL_ARRAY_BUFFER, g_dwg.vertexBufferSphereMesh);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_dwg.indexBufferSphereMesh);

			// todo: do I need to call it here? I don't remember...
			glEnableVertexAttribArray(g_dwg.vertexShaderPositionLoc);
			glVertexAttribPointer(g_dwg.vertexShaderPositionLoc, 3, GL_FLOAT, GL_FALSE,
				sizeof(DebugVertex), (void*)0);
			glEnableVertexAttribArray(g_dwg.vertexShaderColorLoc);
			glVertexAttribPointer(g_dwg.vertexShaderColorLoc, 3, GL_FLOAT, GL_FALSE,
				sizeof(DebugVertex), (void*)(sizeof(float) * 3));

			// todo: rewrite to mesh instancing
			for (int32_t i = 0; i < g_dwg.numSpheres; ++i)
			{
				const DebugSphere& sphere = g_dwg.spheres[i];
				mvpSphere = mvp * Matrix4::translation(sphere.pos)* Matrix4::scale(sphere.scale);

				glUniform3fv(g_dwg.vertexShaderTintLoc, 1, toFloatPtr(sphere.color));
				glUniformMatrix4fv(g_dwg.vertexShaderMVPLoc, 1, GL_FALSE, (const GLfloat*)&mvpSphere);

				glDrawElements(GL_TRIANGLES, g_dwg.numSphereIndices, GL_UNSIGNED_SHORT, NULL);
			}
		}

		glfwSwapBuffers(g_dwg.window);
		glfwPollEvents();
	}

	// clear stuff for the next update + calculate delta time
	{
		g_dwg.numDataLines = 0;
		g_dwg.numSpheres = 0;

		const double nextTime = glfwGetTime();
		g_dwg.deltaTime = DWG_MAX((float)(nextTime - g_dwg.globalTime), 0.1f);	// clamp max time to 0.1, so we have something predictable once placing breakpoint in code
		g_dwg.globalTime = nextTime;
	}
}

void dwgReleaseApp()
{
	delete[] g_dwg.dataLines;
	delete[] g_dwg.sphereIndices;
	delete[] g_dwg.dataSphereMesh;
	delete[] g_dwg.spheres;

	glfwDestroyWindow(g_dwg.window);
	glfwTerminate();
}

float dwgDeltaTime()
{
	return g_dwg.deltaTime;
}

double dwgGlobalTime()
{
	return g_dwg.globalTime;
}

void dwgDebugLine(const Vector3& start, const Vector3& end, const Vector3& color)
{
	assert(g_dwg.numDataLines + 2 < DWG_MAX_DEBUG_VERTICES && g_dwg.dataLines != nullptr);

	DebugVertex* v = &g_dwg.dataLines[g_dwg.numDataLines];

	v->x = start.getX();
	v->y = start.getY();
	v->z = start.getZ();

	v->r = color.getX();
	v->g = color.getY();
	v->b = color.getZ();

	++v;

	v->x = end.getX();
	v->y = end.getY();
	v->z = end.getZ();

	v->r = color.getX();
	v->g = color.getY();
	v->b = color.getZ();

	g_dwg.numDataLines += 2;
}

void dwgDebugSphere(const Vector3& position, const Vector3& scale, const Vector3& color)
{
	assert(g_dwg.numSpheres + 1 < DWG_MAX_DEBUG_SPHERES && g_dwg.spheres != nullptr);

	DebugSphere& sphere = g_dwg.spheres[g_dwg.numSpheres];
	sphere.pos = position;
	sphere.color = color;
	sphere.scale = scale;

	g_dwg.numSpheres += 1;
}
