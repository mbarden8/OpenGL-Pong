#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const float PADDLE_MOVE_SPEED = 2.5f;
float leftPaddleY = 0.0f;
float rightPaddleY = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() {

	// Initialize glfw and onfigure opengl version
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create a window
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Pong", NULL, NULL);
	if (window == NULL) {
		std::cout << "FAILURE: Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Load OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "FAILURE: Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

    // SHADER STUFF

    // Read in our shader source code
    std::string vertexCode;
    std::string fragmentCode;

    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.open("../../GPU_Final/shader_vs.txt");
    fShaderFile.open("../../GPU_Final/shader_fs.txt");

    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    vShaderFile.close();
    fShaderFile.close();

    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();

    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShaderLeft = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderLeft, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShaderLeft);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShaderLeft, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderLeft, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int vertexShaderRight = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderRight, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShaderRight);
    // check for shader compile errors
    glGetShaderiv(vertexShaderRight, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderRight, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgramLeft = glCreateProgram();
    glAttachShader(shaderProgramLeft, vertexShaderLeft);
    glAttachShader(shaderProgramLeft, fragmentShader);
    glLinkProgram(shaderProgramLeft);
    // check for linking errors
    glGetProgramiv(shaderProgramLeft, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgramLeft, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgramRight = glCreateProgram();
    glAttachShader(shaderProgramRight, vertexShaderRight);
    glAttachShader(shaderProgramRight, fragmentShader);
    glLinkProgram(shaderProgramRight);
    // check for linking errors
    glGetProgramiv(shaderProgramRight, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgramRight, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShaderRight);
    glDeleteShader(vertexShaderLeft);
    glDeleteShader(fragmentShader);

    // END OF SHADER STUFF

    // PADDLES
    float leftPaddleStartingPoint[] = {
        -0.955f,  0.125f, 0.0f,  // top right
        -0.955f, -0.125f, 0.0f,  // bottom right
        -0.985f, -0.125f, 0.0f,  // bottom left
        -0.985f,  0.125f, 0.0f   // top left 
    };

    float rightPaddleStartingPoint[] = {
        0.985f,  0.125f, 0.0f,  // top right
        0.985f, -0.125f, 0.0f,  // bottom right
        0.955f, -0.125f, 0.0f,  // bottom left
        0.955f,  0.125f, 0.0f   // top left 
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    unsigned int VBOs[2], VAOs[2], EBOs[2];
    glGenVertexArrays(2, VAOs);
    glGenBuffers(2, VBOs);
    glGenBuffers(2, EBOs);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAOs[0]);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(leftPaddleStartingPoint), leftPaddleStartingPoint, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAOs[1]);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rightPaddleStartingPoint), rightPaddleStartingPoint, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

	while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process any input for the game
		processInput(window);

        // Pong has a black background
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

        // Update positioning of left paddle
        glUseProgram(shaderProgramLeft);
        unsigned int transformLoc = glGetUniformLocation(shaderProgramLeft, "offsetY");
        glUniform1f(transformLoc, leftPaddleY);

        // For some reason if we update positioning of our right paddle before
        // drawing left paddle then our right paddle also controls our left paddle
        // and we can not control our left paddle at all
        // Uncomment to see this in action

        /*
        glUseProgram(shaderProgramRight);
        unsigned int transformLoc1 = glGetUniformLocation(shaderProgramRight, "offsetY");
        glUniform1f(transformLoc1, rightPaddleY);
        */

        // Draw our left paddle
        glBindVertexArray(VAOs[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Update positioning of right paddle
        glUseProgram(shaderProgramRight);
        unsigned int transformLoc1 = glGetUniformLocation(shaderProgramRight, "offsetY");
        glUniform1f(transformLoc1, rightPaddleY);

        // Draw our right paddle
        glBindVertexArray(VAOs[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();

	}

    glDeleteVertexArrays(2, VAOs);
    glDeleteBuffers(2, VBOs);
    glDeleteBuffers(2, EBOs);
    glDeleteProgram(shaderProgramLeft);

	glfwTerminate();

	return 0;

}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        leftPaddleY += PADDLE_MOVE_SPEED * deltaTime;
        if (leftPaddleY >= 0.83f) {
            leftPaddleY = 0.83f;
        }
    }
        
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        leftPaddleY -= PADDLE_MOVE_SPEED * deltaTime;
        if (leftPaddleY <= -0.83f) {
            leftPaddleY = -0.83f;
        }
    }
        
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rightPaddleY += PADDLE_MOVE_SPEED * deltaTime;
        if (rightPaddleY >= 0.83f) {
            rightPaddleY = 0.83f;
        }
    }
        
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rightPaddleY -= PADDLE_MOVE_SPEED * deltaTime;
        if (rightPaddleY <= -0.83f) {
            rightPaddleY = -0.83f;
        }
    }
        
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
