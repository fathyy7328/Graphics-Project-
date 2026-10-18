#include "Enemies.h"
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../Common.h"

static GLuint enemyVAO = 0, enemyVBO = 0;
static GLuint enemyShaderProgram = 0;
static GLint enemyOffsetLocation = -1;
static float enemyOffsetX = GameConfig::ENEMY_SPAWN_X;
static int enemyLane = 0;
static float enemySpeed = 0.03f;

static const char* enemyVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 offset;
void main()
{
gl_Position = vec4(aPos.x + offset.x, aPos.y + offset.y, aPos.z, 1.0);
}
)";

static const char* enemyFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
FragColor = vec4(1.0, 0.8, 0.2, 1.0);
}
)";

void movingTriangle(){
    
    float vertices[] = {
    0.1f, 0.1f, 0.0f, // top
    -0.1f, 0.0f, 0.0f, // mid
    0.1f, -0.1f, 0.0f // bottom
    };

    glGenVertexArrays(1, &enemyVAO);
    glGenBuffers(1, &enemyVBO);
    glBindVertexArray(enemyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, enemyVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &enemyVertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &enemyFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    enemyShaderProgram = glCreateProgram();
    glAttachShader(enemyShaderProgram, vertexShader);
    glAttachShader(enemyShaderProgram, fragmentShader);
    glLinkProgram(enemyShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    enemyOffsetLocation = glGetUniformLocation(enemyShaderProgram, "offset");
    

}

void drawTriangle(){

    glUseProgram(enemyShaderProgram);
    glUniform2f(enemyOffsetLocation, enemyOffsetX, GameConfig::LANE_CENTER_Y[enemyLane]);
    glBindVertexArray(enemyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

}

void updateEnemyPosition(float deltaTime) {
    enemyOffsetX -= enemySpeed * deltaTime;
    if (enemyOffsetX < -1.0f) {
        enemyOffsetX = GameConfig::ENEMY_SPAWN_X;
        enemyLane = (enemyLane + 1) % GameConfig::LANE_COUNT;
    }
}


