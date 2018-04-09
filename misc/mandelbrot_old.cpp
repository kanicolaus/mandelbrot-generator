#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <sys/stat.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



// Standard Error callback for errors with GLFW
static void error_callback(int error, const char *description) {
	std::cerr << "GLFW Error: " << description << std::endl;
}
// Initializing variables
double cx = 0.0, cy = 0.0, zoom = 0.5;
int itr = 100;
int fps = 0;
// Create GLFW window -> nothing there yet
GLFWwindow *window = nullptr;

int w = 640;
int h = 480;
// GLEW commands for starting the shader program
GLuint program;
GLuint shader;
// GLuint shader2ID;

double last_time = 0, current_time = 0;
unsigned int ticks = 0;

bool keys[1024] = { 0 };

static void cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	double xr = 2.0 * (xpos / (double)w - 0.5);
	double yr = 2.0 * (ypos / (double)h - 0.5);

	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		cx += (xr - cx) / zoom / 2.0;
		cy -= (yr - cy) / zoom / 2.0;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	zoom += yoffset * 0.1 * zoom;
	if(zoom < 0.1) {
		zoom = 0.1;
	}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	const double d = 0.1 / zoom;

	if(action == GLFW_PRESS) {
		keys[key] = true;
	} else if(action == GLFW_RELEASE) {
		keys[key] = false;
	}

	if(keys[GLFW_KEY_ESCAPE]) {
		glfwSetWindowShouldClose(window, 1);
	} else if(keys[GLFW_KEY_A]) {
		cx -= d;
	} else if(keys[GLFW_KEY_D]) {
		cx += d;
	} else if(keys[GLFW_KEY_W]) {
		cy += d;
	} else if(keys[GLFW_KEY_S]) {
		cy -= d;
	} else if(keys[GLFW_KEY_MINUS]) {
		itr += 10;
	} else if(keys[GLFW_KEY_EQUAL]) {
		itr -= 10;
		if(itr <= 0) {
			itr = 0;
		}
	}

}

const char* vertex_shader =
"#version 410\n"
"in vec3 vp;"
"void main () {"
"  gl_Position = vec4 (vp, 1.0);"
"}";

const GLchar* vertexSource = R"glsl(
	#version 330 core
	layout (location = 0) in vec3 aPos;
	layout (location = 1) in vec3 aColor;
	layout (location = 2) in vec2 aTexCoord;

	out vec3 ourColor;
	out vec2 TexCoord;

	void main()
	{
		gl_Position = vec4(aPos, 1.0);
		ourColor = aColor;
		TexCoord = vec2(aTexCoord.x, aTexCoord.y);
	}
)glsl";
const GLchar* fragmentSource = R"glsl(
	#version 330 core
	out vec4 FragColor;

	in vec3 ourColor;
	in vec2 TexCoord;

	// texture sampler
	uniform sampler2D texture1;

	void main()
	{
		FragColor = texture(texture1, TexCoord);
	}
)glsl";

static void update_window_title()
{
	std::ostringstream ss;
	ss << "Karl Nicolaus";
	ss << ", FPS: " << fps;
	ss << ", Iterations: " << itr;
	ss << ", Zoom: " << zoom;
	ss << ", At: (" << std::setprecision(8) << cx << " + " << cy << "i)";
	glfwSetWindowTitle(window, ss.str().c_str());
}

static void compile_shader(GLuint &prog)
{	
	// Vertex shader creation
	GLuint vs = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vs, 1, &vertex_shader, NULL);
	glCompileShader (vs);

	// MY vertex shader creation
	// GLuint vs2 = glCreateShader (GL_VERTEX_SHADER);
	// glShaderSource (vs2, 1, &name_vertex_shader, NULL);
	// glCompileShader (vs2);

	std::ifstream t("shader.glsl");
	if(!t.is_open()) {
		std::cerr << "Cannot open shader.glsl!" << std::endl;
		return;
	}
	std::string str((std::istreambuf_iterator<char>(t)),
					 std::istreambuf_iterator<char>());
	const char *src  = str.c_str();

	// Fragment shader creation
	GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fs, 1, &src, NULL);
	glCompileShader (fs);
	// My fragment shader creation
	// Gluint fs2 = glCreateShader (GL_FRAGMENT_SHADER);
	// glShaderSource (fs2, 1, &<data source>, NULL);
	// glCompileShader (fs2);



	int success;
	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if(!success) {
		int s;
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &s);

		char *buf = new char[s];
		glGetShaderInfoLog(fs, s, &s, buf);

		std::cerr << buf << std::endl;
		delete [] buf;
		return;
	}

	prog = glCreateProgram ();
	glAttachShader (prog, fs);
	glAttachShader (prog, vs);
	// glAttachShader (prog, fs2);
	// glAttachShader (prog, vs2);
	glLinkProgram (prog);

	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if(!success) {
		int s;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &s);

		char *buf = new char[s];
		glGetProgramInfoLog(prog, s, &s, buf);

		std::cerr << buf << std::endl;
		delete [] buf;
		return;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);
	// glDeleteShader(vs2);
	// glDeleteShader(fs2);
}

static time_t last_mtime;

static time_t get_mtime(const char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) == -1) {
		perror(path);
		exit(1);
	}
	return statbuf.st_mtime;
}

int main(int argc, char *argv[])
{
	if(!glfwInit()) {
		std::cerr << "Failed to init GLFW" << std::endl;
		return 1; // Simple check to see if the initialization worked
	}
	atexit(glfwTerminate);

	glfwSetErrorCallback(error_callback); // Error callback handling (for initialization)

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(w, h, "Mandelbrot", NULL, NULL);
	if(!window) {
		std::cerr << "Failed to create window" << std::endl;
		return 1;
	}

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	glewInit();

	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	GLuint prog;

	// COMPILE SHADER HERE
	compile_shader(prog);

	last_mtime = get_mtime("shader.glsl");

	float points[] = {
	   -1.0f,  1.0f,  0.0f,
	   -1.0f,  -1.0f,  0.0f,
	   1.0f,  -1.0f,  0.0f,

	   -1.0f,  1.0f,  0.0f,
	   1.0f,  -1.0f,  0.0f,
	   1.0f,  1.0f,  0.0f,
	};

	// ++++++++++++++++++++++++++++++
	float vertices[] = {
	    // positions          // colors           // texture coords
	    -1.0f,  1.0f, 0.0f,   0.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
	    -0.6f,  1.0f, 0.0f,   0.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // bottom right
	    -0.6f, -0.6f, 0.0f,   0.0f, 0.0f, 0.0f,   0.0f, 0.0f,   // bottom left
	    -1.0f,  0.6f, 0.0f,   0.0f, 0.0f, 0.0f,   0.0f, 1.0f    // top left 
	};
    unsigned int indices[] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // load and create a texture 
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
    unsigned char *data = stbi_load(("name_image_full.png").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    // +++++++++++++++++++++++++++++++

	GLuint vbo = 0;
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (GL_ARRAY_BUFFER, 2 * 9 * sizeof (float), points, GL_STATIC_DRAW);

	GLuint vao = 0;
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glUseProgram (prog);

	last_time = glfwGetTime();

	glBindVertexArray (vao);

	// Iterating through this loop
	while(!glfwWindowShouldClose(window)) {
		time_t new_time = get_mtime("shader.glsl");
		if(new_time != last_mtime) {
			glDeleteProgram(prog);
			compile_shader(prog);
			glUseProgram(prog);
			last_mtime = new_time;

			std::cout << "Reloaded shader: " << last_mtime << std::endl;
		}

		glfwGetWindowSize(window, &w, &h);
		glUniform2d(glGetUniformLocation(prog, "screen_size"), (double)w, (double)h);
		glUniform1d(glGetUniformLocation(prog, "screen_ratio"), (double)w / (double)h);
		glUniform2d(glGetUniformLocation(prog, "center"), cx, cy);
		glUniform1d(glGetUniformLocation(prog, "zoom"), zoom);
		glUniform1i(glGetUniformLocation(prog, "itr"), itr);

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays (GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();

		ticks++;
		current_time = glfwGetTime();
		if(current_time - last_time > 1.0) {
			fps = ticks;
			update_window_title();
			last_time = glfwGetTime();
			ticks = 0;
		}
	}

	glfwDestroyWindow(window);
}
