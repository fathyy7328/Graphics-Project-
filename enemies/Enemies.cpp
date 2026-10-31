#include "Enemies.h"
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../Common.h"
#include <vector>
#include <cstdlib>
#include <ctime>

struct enemy {
    float x;
    int lane;
    float speed;
    bool alive;
};

static GLuint enemyVAO = 0, enemyVBO = 0;
static GLuint enemyShaderProgram = 0;
static GLint enemyOffsetLocation = -1;
static std::vector<enemy> enemies;
static float spawnTimer = 0.0f;
static const float spawnInterval = 2.0f;
static const float ENEMY_SPEED = 0.2f;
static int currentWave = 0;
static int spawnedEnemies = 0;
static const int waves[] = { 3, 5, 10 };

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
    
    srand(time(nullptr));
}

void drawTriangle(){
    glUseProgram(enemyShaderProgram);
    glBindVertexArray(enemyVAO);
    for (auto& enemy : enemies) {
        if (!enemy.alive) continue;
        glUniform2f(enemyOffsetLocation, enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

void spawnEnemy() {
    enemy newEnemy;
    newEnemy.x = GameConfig::ENEMY_SPAWN_X;
    newEnemy.lane = rand() % GameConfig::LANE_COUNT;
    newEnemy.speed = ENEMY_SPEED;
    newEnemy.alive = true;
    enemies.push_back(newEnemy);
}

void updateEnemyPosition(float deltaTime) {
    
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval && currentWave < sizeof(waves) / sizeof(waves[0]) && spawnedEnemies < waves[currentWave]) {
        spawnEnemy();
        spawnedEnemies++;
        std::cout << "Spawned " << spawnedEnemies << " of " << waves[currentWave] << " in wave " << currentWave + 1 << std::endl;
        spawnTimer = 0.0f;
    }
    
    for (auto& enemy : enemies) {
        if (!enemy.alive) continue;
        enemy.x -= enemy.speed * deltaTime;
        if (enemy.x < -1.0f) {
            enemy.alive = false;
        }
    }
    bool anyAlive = false;
    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            anyAlive = true;
            break;
        }
    }
    if (currentWave < sizeof(waves) / sizeof(waves[0]) && !anyAlive && spawnedEnemies >= waves[currentWave]) {
        currentWave++;
        if (currentWave < sizeof(waves) / sizeof(waves[0])) {
            std::cout << "Wave " << currentWave + 1 << " starting" << std::endl;        
        } else {
            std::cout << "All waves cleared" << std::endl;          }
        spawnedEnemies = 0;
        spawnTimer = 0.0f;
    }
}


