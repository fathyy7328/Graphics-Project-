#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../Common.h"
#include <iostream>
#include "../world/World.h"
#include "Menus.h"
#include <glad/gl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <map>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/shader.h>
#include "../enemies/Enemies.h"
#include "../players/Players.h"

GAME_STATE G_STATE = GAME_STATE::Main_Menu;
bool escapeWasPressed = false;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// void processInput(GLFWwindow *window);
void Open_Cutscene(GLFWwindow* window);
void Open_Playing(GLFWwindow* window);
void Open_Pause(GLFWwindow* window);
void Resume_Game(GLFWwindow* window);
void Return_to_Main_Menu(GLFWwindow* window);

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, "REAL MADRID VS BARCELONA", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    if (!initializeFont())
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Shader textShader(
            "assets/shaders/text.vs",
            "assets/shaders/text.fs"
        );

        glm::mat4 textProjection = glm::ortho(
            0.0f, static_cast<float>(GameConfig::WINDOW_WIDTH),
            0.0f, static_cast<float>(GameConfig::WINDOW_HEIGHT)
        );

        textShader.use();
        glUniformMatrix4fv(
            glGetUniformLocation(textShader.ID, "projection"),
            1, GL_FALSE, glm::value_ptr(textProjection)
        );

    createLanes();
    createBackground();
    createDecorations();
    createHomeBase();
    createHealthBar();
    createCards();
    movingTriangle();
    playersInit();
    

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
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
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        0.675f, -0.517f, 0.0f,  // top right
        0.675f, -0.733f, 0.0f,  // bottom right
        -0.675f, -0.733f, 0.0f,  // bottom left
        -0.675f, -0.517f, 0.0f   // top left
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 3, 1,
        1, 3, 2  // second Triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); 


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    double previousTime = glfwGetTime();
    float cutsceneAttackerX = 0.8f;
    float cutsceneContactTimer = 0.0f;
    bool cutsceneAttackerPushedBack = false;
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        // processInput(window);
        const double currentTime = glfwGetTime();
        const float deltaTime =
        static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;


        Open_Cutscene(window);
        Open_Playing(window);
        if ((G_STATE == GAME_STATE::Win || G_STATE == GAME_STATE::Lose) &&
            glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        {
            playersReset();
            worldReset();
            resetEnemies();
            cutsceneAttackerX = 0.8f;
            cutsceneContactTimer = 0.0f;
            cutsceneAttackerPushedBack = false;
            G_STATE = GAME_STATE::Playing;
        }
        updateLanes(window);
        updateCards(window);
        if (G_STATE == GAME_STATE::Playing)
        {
            Open_Pause(window);
        }
        else if (G_STATE == GAME_STATE::Paused)
        {
            Resume_Game(window);
            drawDecorations();
        }

        escapeWasPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

        // render
        // ------
        if (G_STATE == GAME_STATE::Main_Menu)
        {
            glClearColor(0.0f, 0.3f, 0.0f, 1.0f);
        }
        else if (G_STATE == GAME_STATE::Cutscene)
        {
            glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
        }
        else if (G_STATE == GAME_STATE::Playing)
        {
           glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        }
        else if (G_STATE == GAME_STATE::Paused)
        {
            glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
        }
        else if (G_STATE == GAME_STATE::Win)
        {
            glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
        }
        else if (G_STATE == GAME_STATE::Lose)
        {
            glClearColor(0.12f, 0.0f, 0.0f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);
        
        if (G_STATE == GAME_STATE::Main_Menu)
        {   
            drawDecorations();
            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            RenderText(
                textShader,
                "REAL MADRID VS BARCELONA",
                150.0f, 450.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
            RenderText(
                textShader,
                "Press Here to Start The Game",
                150.0f, 100.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)

               
            )
            ;
          
        }
        
        if (G_STATE == GAME_STATE::Playing)
          { 
            updateEnemyPosition(deltaTime);
            playersUpdate(deltaTime);
            drawBackground();
            drawLanes();
            drawDecorations();
            drawHomeBase();
            drawCards();
            drawTriangle();
            playersDraw();

        }
        if (G_STATE == GAME_STATE::Cutscene)
        {   
            drawBackground();
            drawDecorations();
            drawHomeBase();
            
            
            RenderText(
                textShader,
                "The Trophy Garden is under attack!",
                150.0f, 180.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );

            RenderText(
                textShader,
                "Barcelona is coming for the trophies.",
                150.0f, 130.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );

            RenderText(
                textShader,
                "Real Madrid, defend them!",
                150.0f, 80.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );

            RenderText(
                textShader,
                "Press SPACE to begin.",
                150.0f, 30.0f,
                0.7f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
            playersDraw();

            if (!cutsceneAttackerPushedBack)
            {
                cutsceneAttackerX -= 0.35f * deltaTime;

                if (cutsceneAttackerX <= -0.21f)
                {
                    cutsceneAttackerX = -0.21f;
                    cutsceneContactTimer += deltaTime;

                    if (cutsceneContactTimer >= 0.3f)
                    {
                        cutsceneAttackerPushedBack = true;
                    }
                }
            }
            else if (cutsceneAttackerX < 0.0f)
            {
                cutsceneAttackerX += 0.5f * deltaTime;

                if (cutsceneAttackerX > 0.0f)
                {
                    cutsceneAttackerX = 0.0f;
                }
            }

            const bool attackerAtDefender = cutsceneContactTimer > 0.0f && !cutsceneAttackerPushedBack;
            drawHardEnemy(cutsceneAttackerX, 0.0f, static_cast<float>(currentTime), attackerAtDefender);
        }
            if (G_STATE == GAME_STATE::Paused)
        {
            Resume_Game(window);
            
            drawDecorations();
            RenderText(
                textShader,
                "Press the ESC key to resume the game.",
                150.0f, 450.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
            RenderText(
                textShader,
                "Press X to return to the main menu.",
                150.0f, 100.0f,
                0.85f,
                glm::vec3(1.0f, 1.0f, 1.0f)

               
            );
            Return_to_Main_Menu(window);
            

        }
        if(G_STATE == GAME_STATE::Win)
        {
            drawDecorations();
            RenderText(
                textShader,
                "Congratulations! You won!",
                165.0f, 470.0f,
                1.0f,
                glm::vec3(1.0f, 0.84f, 0.0f)
            );
            RenderText(
                textShader,
                "Press G to play again.",
                250.0f, 410.0f,
                0.75f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
        }
        if(G_STATE == GAME_STATE::Lose)
        {
            drawDecorations();
            RenderText(
                textShader,
                "You lost. Hard luck next time.",
                130.0f, 470.0f,
                1.0f,
                glm::vec3(1.0f, 0.0f, 0.0f)
            );
            RenderText(
                textShader,
                "Press G to play again.",
                250.0f, 410.0f,
                0.75f,
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
        }
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    deleteCards();
    deleteHealthBar();
    deleteHomeBase();
    deleteDecorations();
    deleteBackground();
    deleteLanes(); // last, it deletes the shader the others use
    

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
// void processInput(GLFWwindow *window)
// {
//     if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//         glfwSetWindowShouldClose(window, true);
// }

void Open_Cutscene(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && G_STATE == GAME_STATE::Main_Menu)
    {
        G_STATE = GAME_STATE::Cutscene;
        spawnPlayer(PlayerType::Defense, 1);
    }
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    const float mouseX =
        static_cast<float>(2.0 * cursorX / windowWidth - 1.0);

    const float mouseY =
        static_cast<float>(1.0 - 2.0 * cursorY / windowHeight);

    const bool insideStartButton =
        mouseX >= -0.675f && mouseX <= 0.675f &&
        mouseY >= -0.733f && mouseY <= -0.517f;

    const bool leftButtonDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (leftButtonDown && insideStartButton &&
        G_STATE == GAME_STATE::Main_Menu)
    {
        G_STATE = GAME_STATE::Cutscene;
        spawnPlayer(PlayerType::Defense, 1);
    }
}
void Open_Playing(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && G_STATE == GAME_STATE::Cutscene)
    {   playersReset();
        G_STATE = GAME_STATE::Playing;
    }
}
void Open_Pause(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !escapeWasPressed && G_STATE == GAME_STATE::Playing)
    {
        G_STATE = GAME_STATE::Paused;
    }
}
void Resume_Game(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !escapeWasPressed && G_STATE == GAME_STATE::Paused)
    {
        G_STATE = GAME_STATE::Playing;
    }
}
void Return_to_Main_Menu(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && G_STATE == GAME_STATE::Paused)
    {
        G_STATE = GAME_STATE::Main_Menu;
        playersReset();
        worldReset();
        resetEnemies();
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
