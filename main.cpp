#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#include <chrono>
#include <thread>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window);
void handleBall();
void newRound();

// Screen stuff
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Paddle stuff
const float PADDLE_SCREEN_BOUND = 0.85f;
const float PADDLE_MOVE_SPEED = 2.5f;

// Since we are doing fake collision we need these gross 'magic' numbers
const float LEFT_PADDLE_BALL_COLLISION_X = -0.955f;
const float RIGHT_PADDLE_BALL_COLLISION_X = 0.955f;
const float PADDLE_WIDTH = 0.03f;
const float HALF_PADDLE_HEIGHT = 0.125f;
float leftPaddleY = 0.0f;
float rightPaddleY = 0.0f;

// Ball stuff
float ballX = 0.0f;
float ballY = 0.0f;
float ballDirectionX = -1.0f;
float ballDirectionY = -1.0f;
const float STARTING_BALL_X_SPEED = 0.5f;
const float STARTING_BALL_Y_SPEED = 0.1f;
const float BALL_SCREEN_BOUND = 0.95f;

// Utility stuff
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool roundStarted = false;
const float ROUND_RESTART_DELAY = 1.0f;
float roundRestartTimer = 0.0f;

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

    Shader leftPaddleShader  = Shader("../../GPU_Final/shader_vs.txt", "../../GPU_Final/shader_fs.txt");
    Shader rightPaddleShader = Shader("../../GPU_Final/shader_vs.txt", "../../GPU_Final/shader_fs.txt");
    Shader ballShader        = Shader("../../GPU_Final/shader_vs_ball.txt", "../../GPU_Final/shader_fs.txt");

    // END OF SHADER STUFF

    // SET UP STUFF FOR DRAWING
    // PADDLES
    float leftPaddleStartingPoint[] = {
        LEFT_PADDLE_BALL_COLLISION_X,                 HALF_PADDLE_HEIGHT, 0.0f,  // top right
        LEFT_PADDLE_BALL_COLLISION_X,                -HALF_PADDLE_HEIGHT, 0.0f,  // bottom right
        LEFT_PADDLE_BALL_COLLISION_X - PADDLE_WIDTH, -HALF_PADDLE_HEIGHT, 0.0f,  // bottom left
        LEFT_PADDLE_BALL_COLLISION_X - PADDLE_WIDTH,  HALF_PADDLE_HEIGHT, 0.0f   // top left 
    };

    float rightPaddleStartingPoint[] = {
        RIGHT_PADDLE_BALL_COLLISION_X + PADDLE_WIDTH,  HALF_PADDLE_HEIGHT, 0.0f,  // top right
        RIGHT_PADDLE_BALL_COLLISION_X + PADDLE_WIDTH, -HALF_PADDLE_HEIGHT, 0.0f,  // bottom right
        RIGHT_PADDLE_BALL_COLLISION_X,                -HALF_PADDLE_HEIGHT, 0.0f,  // bottom left
        RIGHT_PADDLE_BALL_COLLISION_X,                 HALF_PADDLE_HEIGHT, 0.0f   // top left
    };

    float ballStartingPoint[] = {
         0.015f,  0.015f, 0.0f,  // top right
         0.015f, -0.015f, 0.0f,  // bottom right
        -0.0f,   -0.015f, 0.0f,  // bottom left
        -0.0f,    0.015f, 0.0f   // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    unsigned int VBOs[3], VAOs[3], EBOs[3];
    glGenVertexArrays(3, VAOs);
    glGenBuffers(3, VBOs);
    glGenBuffers(3, EBOs);
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

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAOs[2]);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ballStartingPoint), ballStartingPoint, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // END OF SETTING UP STUFF FOR DRAWING

	while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        handleBall();
        if (roundRestartTimer > 0.0f) {
            roundRestartTimer -= deltaTime;
        }

        // Process any input for the game
		processInput(window);

        // Pong has a black background
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

        // Update positioning of left paddle
        leftPaddleShader.use();
        leftPaddleShader.setFloat("offsetY", leftPaddleY);

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
        rightPaddleShader.use();
        rightPaddleShader.setFloat("offsetY", rightPaddleY);

        // Draw our right paddle
        glBindVertexArray(VAOs[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Draw our ball
        ballShader.use();
        ballShader.setFloat("offsetX", ballX);
        ballShader.setFloat("offsetY", ballY);
        glBindVertexArray(VAOs[2]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();

	}

    glDeleteVertexArrays(3, VAOs);
    glDeleteBuffers(3, VBOs);
    glDeleteBuffers(3, EBOs);

	glfwTerminate();

	return 0;

}

void handleBall() {

    if (roundStarted) {

        ballX += ballDirectionX * STARTING_BALL_X_SPEED * deltaTime;
        ballY += ballDirectionY * STARTING_BALL_Y_SPEED * deltaTime;

        // Check wall bounds
        if (ballY <= -BALL_SCREEN_BOUND || ballY >= BALL_SCREEN_BOUND) {
            ballDirectionY = -ballDirectionY;
        }

        // Ball is going left
        if (ballDirectionX < 0) {

            if (ballX <= LEFT_PADDLE_BALL_COLLISION_X) {

                if (ballY <= leftPaddleY + HALF_PADDLE_HEIGHT && ballY >= leftPaddleY - HALF_PADDLE_HEIGHT) {
                    ballDirectionX = -ballDirectionX;
                }

                if (ballX < -1.0f) {
                    newRound();
                }

            }
        }
        else if (ballDirectionX > 0) {

            if (ballX >= RIGHT_PADDLE_BALL_COLLISION_X) {

                if (ballY <= rightPaddleY + HALF_PADDLE_HEIGHT && ballY >= rightPaddleY - HALF_PADDLE_HEIGHT) {
                    ballDirectionX = -ballDirectionX;
                }

                if (ballX > 1.0f) {
                    newRound();
                }

            }

        }
    }

}

void newRound() {
    
    ballX = 0.0f;
    ballY = 0.0f;
    ballDirectionX = -ballDirectionX;
    roundStarted = false;
    roundRestartTimer = ROUND_RESTART_DELAY;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        leftPaddleY += PADDLE_MOVE_SPEED * deltaTime;

        // Make sure our paddle stays on the screen
        if (leftPaddleY >= PADDLE_SCREEN_BOUND) {
            leftPaddleY = PADDLE_SCREEN_BOUND;
        }
        // Would be better to handle this with an event system but I'm not
        // making that for this
        if (!roundStarted && roundRestartTimer <= 0.0f)
            roundStarted = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        leftPaddleY -= PADDLE_MOVE_SPEED * deltaTime;
        if (leftPaddleY <= -PADDLE_SCREEN_BOUND) {
            leftPaddleY = -PADDLE_SCREEN_BOUND;
        }
        if (!roundStarted && roundRestartTimer <= 0.0f)
            roundStarted = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rightPaddleY += PADDLE_MOVE_SPEED * deltaTime;
        if (rightPaddleY >= PADDLE_SCREEN_BOUND) {
            rightPaddleY = PADDLE_SCREEN_BOUND;
        }
        if (!roundStarted && roundRestartTimer <= 0.0f)
            roundStarted = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rightPaddleY -= PADDLE_MOVE_SPEED * deltaTime;
        if (rightPaddleY <= -PADDLE_SCREEN_BOUND) {
            rightPaddleY = -PADDLE_SCREEN_BOUND;
        }
        if (!roundStarted && roundRestartTimer <= 0.0f)
            roundStarted = true;
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
