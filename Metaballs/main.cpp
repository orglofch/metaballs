/*
* Copyright (c) 2016 Owen Glofcheski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
*    1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
*
*    2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
*    3. This notice may not be removed or altered from any source
*    distribution.
*/

#include <ctime>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <vector>

#include "Utility\algebra.hpp"
#include "Utility\gl.hpp"

/** If you change this value, change it in the shaders as well. **/
const static int MAX_METABALLS = 100;

enum RenderMode
{
	RENDER_MODE_2D,
	RENDER_MODE_3D
};

struct Metaball
{
	Point3 position;
	Vector3 velocity;
	
	int radius = 1;
};

struct MetaballShader2D : public Shader
{
	Uniform metaball_uniform = -1;
	Uniform metaball_count_uniform = -1;
	Uniform threshold_uniform = -1;
};

struct MetaballShader3D : public Shader
{
	Uniform metaball_uniform = -1;
	Uniform metaball_count_uniform = -1;
	Uniform threshold_uniform = -1;
};

struct State
{
	int window = 0;

	RenderMode render_mode = RENDER_MODE_2D;

	MetaballShader2D shader_2d;
	MetaballShader3D shader_3d;

	Metaball metaballs[MAX_METABALLS];
	int active_metaballs = 0;

	float threshold = 1000;
};

State state;

void update() {
	// Move the metaballs.
	for (int i = 0; i < state.active_metaballs; ++i) {
		Metaball &metaball = state.metaballs[i];
		metaball.position += metaball.velocity;
		if (metaball.position.x < 0) {
			metaball.position.x = 0;
			metaball.velocity.x *= -0.99;
		}
		if (metaball.position.x > 1080) {
			metaball.position.x = 1080;
			metaball.velocity.x *= -0.99;
		}
		if (metaball.position.y < 0) {
			metaball.position.y = 0;
			metaball.velocity.y *= -0.99;
		}
		if (metaball.position.y > 720) {
			metaball.position.y = 720;
			metaball.velocity.y *= -0.99;
		}
	}
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Serialize the metaballs.
	GLfloat *data = new GLfloat[MAX_METABALLS * 4];
	for (int i = 0; i < state.active_metaballs; ++i) {
		const Metaball &metaball = state.metaballs[i];
		int index = i * 4;
		data[index + 0] = metaball.position.x;
		data[index + 1] = metaball.position.y;
		data[index + 2] = metaball.position.z;
		data[index + 3] = metaball.radius;
	}

	if (state.render_mode == RENDER_MODE_2D) {
		glUseProgram(state.shader_2d.program);
		glUniform4fv(state.shader_2d.metaball_uniform, MAX_METABALLS, data);
		glUniform1i(state.shader_2d.metaball_count_uniform, state.active_metaballs);
		glUniform1f(state.shader_2d.threshold_uniform, state.threshold);
	} else if (state.render_mode == RENDER_MODE_3D) {
		glUseProgram(state.shader_3d.program);
		glUniform4fv(state.shader_3d.metaball_uniform, MAX_METABALLS, data);
		glUniform1i(state.shader_3d.metaball_count_uniform, state.active_metaballs);
		glUniform1f(state.shader_3d.threshold_uniform, state.threshold);
	}

	delete[] data;

	glSetOrthographicProjection(0, 1080, 0, 720, 0, 1);
	glDrawRect(0, 1080, 0, 720, 0);

	glutSwapBuffers();
	glutPostRedisplay();
}

void tick() {
	update();
	render();
}

void handleMouseButton(int button, int button_state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			break;
		case GLUT_RIGHT_BUTTON:
			break;
	}
}

void handlePressNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'a':
		case 'A':
			break;
		case 'd':
		case 'D':
			break;
		case 'w':
		case 'W':
			break;
		case 's':
		case 'S':
			break;
		case 'q':
		case 'Q':
		case 27:
			exit(EXIT_SUCCESS);
	}
}

void handleReleaseNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'a':
		case 'A':
			break;
		case 'd':
		case 'D':
			break;
		case 'w':
		case 'W':
			break;
		case 's':
		case 'S':
			break;
	}
}

void handlePressSpecialKey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			break;
	}
}

void handleReleaseSpecialKey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			break;
	}
}


void init() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glutDisplayFunc(tick);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(handleMouseButton);
	glutKeyboardFunc(handlePressNormalKeys);
	glutKeyboardUpFunc(handleReleaseNormalKeys);
	glutSpecialFunc(handlePressSpecialKey);
	glutSpecialUpFunc(handleReleaseSpecialKey);

	state.shader_2d.program = glLoadShader("pass_through.vert", "metaball_shader_2d.frag");
	state.shader_2d.metaball_uniform = glGetUniform(state.shader_2d, "metaballs");
	state.shader_2d.metaball_count_uniform = glGetUniform(state.shader_2d, "metaball_count");
	state.shader_2d.threshold_uniform = glGetUniform(state.shader_2d, "threshold");

	state.shader_3d.program = glLoadShader("pass_through.vert", "metaball_shader_3d.frag");
	state.shader_3d.metaball_uniform = glGetUniform(state.shader_3d, "metaballs");
	state.shader_3d.metaball_count_uniform = glGetUniform(state.shader_3d, "metaball_count");
	state.shader_3d.threshold_uniform = glGetUniform(state.shader_3d, "threshold");
}

void cleanup() {
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	for (int i = 0; i < MAX_METABALLS; ++i) {
		state.metaballs[i].position.x = Rand(0, 1080);
		state.metaballs[i].position.y = Rand(0, 720);
		state.metaballs[i].velocity.x = Rand(-5, 5);
		state.metaballs[i].velocity.y = Rand(-5, 5);
		state.metaballs[i].radius = Rand(100, 600);
	}
	state.active_metaballs = MAX_METABALLS;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1080, 720);
	state.window = glutCreateWindow("Metaballs");
	//glutFullScreen();
	glewInit();

	init();

	glutMainLoop();

	cleanup();
}