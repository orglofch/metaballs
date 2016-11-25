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
#include "Utility\quaternion.hpp"

/** If you change this value, change it in the shaders as well. **/
const static int MAX_METABALLS = 80;

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
	Uniform origin_uniform = -1;

	Uniform camera_matrix_uniform = -1;

	Uniform time_uniform = -1;
};

struct State
{
	int window = 0;

	RenderMode render_mode = RENDER_MODE_3D;

	MetaballShader2D shader_2d;
	MetaballShader3D shader_3d;

	Metaball metaballs[MAX_METABALLS];
	int active_metaballs = 0;

	float threshold = 1000;

	Quaternion rotation;

	bool paused = false;

	Size3 bounding_box = Size3(450, 350, 450);

	int time = 0;
};

State state;

void update() {
	// TODO(orglofch): Sort the metaballs in some fashion so the shader
	// can shortcut some work.

	// Move the metaballs.
	for (int i = 0; i < state.active_metaballs; ++i) {
		Metaball &metaball = state.metaballs[i];
		metaball.position += metaball.velocity;
		if (metaball.position.x < -state.bounding_box.width / 2) {
			metaball.position.x = -state.bounding_box.width / 2;
			metaball.velocity.x *= -0.99;
		} else if (metaball.position.x > state.bounding_box.width / 2) {
			metaball.position.x = state.bounding_box.width / 2;
			metaball.velocity.x *= -0.99;
		}
		if (metaball.position.y < -state.bounding_box.height / 2) {
			metaball.position.y = -state.bounding_box.height / 2;
			metaball.velocity.y *= -0.99;
		} else if (metaball.position.y > state.bounding_box.height / 2) {
			metaball.position.y = state.bounding_box.height / 2;
			metaball.velocity.y *= -0.99;
		}
		if (metaball.position.z < -state.bounding_box.depth / 2) {
			metaball.position.z = -state.bounding_box.depth / 2;
			metaball.velocity.z *= -0.99;
		} else if (metaball.position.z > state.bounding_box.depth / 2) {
			metaball.position.z = state.bounding_box.depth / 2;
			metaball.velocity.z *= -0.99;
		}
	}
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	// Serialize the metaballs.
	GLfloat *metaball_data = new GLfloat[MAX_METABALLS * 4];
	for (int i = 0; i < state.active_metaballs; ++i) {
		const Metaball &metaball = state.metaballs[i];
		int index = i * 4;
		metaball_data[index + 0] = metaball.position.x;
		metaball_data[index + 1] = metaball.position.y;
		metaball_data[index + 2] = metaball.position.z;
		metaball_data[index + 3] = metaball.radius;
	}

	if (state.render_mode == RENDER_MODE_2D) {
		glUseProgram(state.shader_2d.program);
		glUniform4fv(state.shader_2d.metaball_uniform, MAX_METABALLS, metaball_data);
		glUniform1i(state.shader_2d.metaball_count_uniform, state.active_metaballs);
		glUniform1f(state.shader_2d.threshold_uniform, state.threshold);
	} else if (state.render_mode == RENDER_MODE_3D) {
		glUseProgram(state.shader_3d.program);
		glUniform4fv(state.shader_3d.metaball_uniform, MAX_METABALLS, metaball_data);
		glUniform1i(state.shader_3d.metaball_count_uniform, state.active_metaballs);
		glUniform1f(state.shader_3d.threshold_uniform, state.threshold);
		glUniform1i(state.shader_3d.time_uniform, state.time);

		Point3 eye = state.rotation.matrix() * Point3(0, 0, -600);

		GLfloat origin[3];
		for (int i = 0; i < 3; ++i) {
			origin[i] = eye.d[i];
		}
		glUniform3fv(state.shader_3d.origin_uniform, 1, origin);

		Vector3 view = Point3(0, 0, 0) - eye;
		view.normalize();
		Vector3 up(0, 1, 0);

		double d = view.length();
		double h = 2.0 * d * tan(toRad(60 /* fov */) / 2.0);
		Matrix4x4 t1 = Matrix4x4::translation(-540, -360, d);
		Matrix4x4 s2 = Matrix4x4::scaling(-h / 720, -h / 720, 1.0);
		Matrix4x4 r3 = Matrix4x4::rotation(eye, view, up);
		Matrix4x4 t4 = Matrix4x4::translation(eye - Point3(0, 0, 0));
		Matrix4x4 camera_matrix = t4 * r3 * s2 * t1;
		GLfloat camera_data[16];
		for (int i = 0; i < 16; ++i) {
			camera_data[i] = camera_matrix.d[i];
		}
		glUniformMatrix4fv(state.shader_3d.camera_matrix_uniform, 1, false, camera_data);
	}

	delete[] metaball_data;
	glDrawRect(-1, 1, -1, 1, 0);

	glutSwapBuffers();
	glutPostRedisplay();
}

void tick() {
	if (!state.paused) {
		update();
	}
	render();
	state.rotation *= Quaternion(0.0, 0.002, 0.0, 1.0);
	state.time++;
}

void handleMouseButton(int button, int button_state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			break;
		case GLUT_RIGHT_BUTTON:
			state.paused = !state.paused;
			break;
	}
}

void handlePressNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'q':
		case 'Q':
		case 27:
			exit(EXIT_SUCCESS);
	}
}

void handleReleaseNormalKeys(unsigned char key, int x, int y) {
}

void handlePressSpecialKey(int key, int x, int y) {
}

void handleReleaseSpecialKey(int key, int x, int y) {
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

	state.shader_3d.program = glLoadShader("metaball_shader_3d.vert", "metaball_shader_3d.frag");
	state.shader_3d.metaball_uniform = glGetUniform(state.shader_3d, "metaballs");
	state.shader_3d.metaball_count_uniform = glGetUniform(state.shader_3d, "metaball_count");
	state.shader_3d.threshold_uniform = glGetUniform(state.shader_3d, "threshold");
	state.shader_3d.origin_uniform = glGetUniform(state.shader_3d, "origin");
	state.shader_3d.camera_matrix_uniform = glGetUniform(state.shader_3d, "camera_matrix");
	state.shader_3d.time_uniform = glGetUniform(state.shader_3d, "time");
}

void cleanup() {
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	for (int i = 0; i < MAX_METABALLS; ++i) {
		state.metaballs[i].position.x = Randf(-state.bounding_box.width / 2, state.bounding_box.width / 2);
		state.metaballs[i].position.y = Randf(-state.bounding_box.height / 2, state.bounding_box.height / 2);
		state.metaballs[i].position.z = Randf(-state.bounding_box.depth / 2, state.bounding_box.depth / 2);
		state.metaballs[i].velocity.x = Randf(-1, 1);
		state.metaballs[i].velocity.y = Randf(-1, 1);
		state.metaballs[i].velocity.z = Randf(-1, 1);
		state.metaballs[i].radius = Randf(30, 100);
	}
	state.active_metaballs = MAX_METABALLS;
	/*state.metaballs[0].position = Point3(100, 100, 0);
	//state.metaballs[0].velocity.x = -1;
	state.metaballs[0].radius = 100;
	state.metaballs[1].position = Point3(200, 200, -50);
	state.metaballs[1].velocity.x = -1;
	state.metaballs[1].radius = 100;
	//state.metaballs[2].position = Point3(300, 300, -100);
	//state.metaballs[2].velocity.x = -1;
	//state.metaballs[2].radius = 100;
	state.active_metaballs = 2;*/

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