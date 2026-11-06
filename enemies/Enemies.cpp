#include "Enemies.h"
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../Common.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

enum class enemyType {
    Easy,
    Medium,
    Hard
};

enum class Shape {
    Square,
    Circle,
    Triangle
};

struct enemy {
    float x;
    int lane;
    float speed;
    bool alive;
    enemyType type;
    int health;
    int maxHealth;
    float attackTimer;
    float attackAnim;
};

struct projectile {
    float x;
    float y;
    int lane;
    float speed;
    int damage;
    bool alive;
    AttackHeight height;
};

struct droppedCoin{
    float x;
    float y;
    int lane;
    float timer;
    bool alive;
};

static GLuint squareVAO = 0, squareVBO = 0;
static GLuint circleVAO = 0, circleVBO = 0;
static int circleVertexCount = 0;
static GLuint triangleVAO = 0, triangleVBO = 0;
static GLuint enemyShaderProgram = 0;
static GLint enemyTransformLocation = -1;
static GLint enemyColorLocation = -1;
static GLint enemyDamageLocation = -1;
static std::vector<enemy> enemies;
static float spawnTimer = 0.0f;
static const float spawnInterval = 4.0f;
static const float ENEMY_SPEED = 0.08f;
static int currentWave = 0; 
static int spawnedEnemies = 0;
static const int waves[] = { 3, 5, 11 }; 
static bool gameLost = false;
static const float ATTACK_INTERVAL = 5.0f;
static std::vector<projectile> projectiles;
static std::vector<droppedCoin> droppedCoins;


static const char* enemyVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 transform;
void main()
{
gl_Position = transform * vec4(aPos, 1.0);
}
)";

static const char* enemyFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
uniform float damage;
void main()
{
FragColor = vec4(mix(color, vec3(1.0, 0.0, 0.0), damage), 1.0);
}
)";

void createSquare() {
    float squareVertices[] = {
        -0.5f, -0.5f, 0.0f,   // bottom left
         0.5f, -0.5f, 0.0f,   // bottom right
         0.5f,  0.5f, 0.0f,   // top right

        -0.5f, -0.5f, 0.0f,   // bottom left
         0.5f,  0.5f, 0.0f,   // top right
        -0.5f,  0.5f, 0.0f    // top left
    };

    glGenVertexArrays(1, &squareVAO); 
    glGenBuffers(1, &squareVBO); 
    glBindVertexArray(squareVAO); 
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void createCircle() {
    std::vector<float> circleVertices;
    circleVertices.push_back(0.0f); //x
    circleVertices.push_back(0.0f); //y
    circleVertices.push_back(0.0f); //z
    
    const int segments = 30;                          

    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159f / segments; 

        circleVertices.push_back(0.5f * cos(angle));  // x
        circleVertices.push_back(0.5f * sin(angle));  // y
        circleVertices.push_back(0.0f);               // z 

    }

    circleVertexCount = circleVertices.size() / 3; 

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void createTriangle() {
    float triangleVertices[] = {
        0.0f,  0.5f, 0.0f,  
       -0.5f, -0.5f, 0.0f,  
        0.5f, -0.5f, 0.0f   
    };

    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &triangleVBO);
    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void movingTriangle(){

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

    enemyTransformLocation = glGetUniformLocation(enemyShaderProgram, "transform");
    enemyColorLocation = glGetUniformLocation(enemyShaderProgram, "color");
    enemyDamageLocation = glGetUniformLocation(enemyShaderProgram, "damage");
    srand(time(nullptr));



    createSquare();
    createCircle();
    createTriangle();
}

static void drawPart(Shape shape, float cx, float cy, float px, float py, float w, float h, float angle, float r, float g, float b){
    float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::scale(model, glm::vec3(aspect, 1.0f, 1.0f));
    model = glm::translate(model, glm::vec3(px, py, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    glUniformMatrix4fv(enemyTransformLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(enemyColorLocation, r, g, b);
    if (shape == Shape::Square) {
        glBindVertexArray(squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } else if (shape == Shape::Circle) {
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);
    } else if (shape == Shape::Triangle) {
        glBindVertexArray(triangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

static void drawLimb(Shape shape, float cx, float cy, float px, float py, float w, float h, float angle, float r, float g, float b){
    float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::scale(model, glm::vec3(aspect, 1.0f, 1.0f));
    model = glm::translate(model, glm::vec3(px, py, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(0.0f, -h/2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    glUniformMatrix4fv(enemyTransformLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(enemyColorLocation, r, g, b);
    if (shape == Shape::Square) {
        glBindVertexArray(squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } else if (shape == Shape::Circle) {
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);
    } else if (shape == Shape::Triangle) {
        glBindVertexArray(triangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

void drawEasyEnemy(float x, float y, float animTime, bool attacking){
    glUseProgram(enemyShaderProgram);
    float swing = sin(animTime *6.0f) * 20.0f;
    float rad = glm::radians(swing);
    float footX = sin(rad) * 0.08f;
    float footY = cos(rad) * 0.08f;
    float leftArmAngle = -20.0f;
    float rightArmAngle = 20.0f;
    if (attacking) {
        leftArmAngle = -160.0f;
        rightArmAngle = 160.0f;
    }
    float leftHandX  = sin(glm::radians(leftArmAngle))  * 0.06f;
    float leftHandY  = cos(glm::radians(leftArmAngle))  * 0.06f;
    float rightHandX = sin(glm::radians(rightArmAngle)) * 0.06f;
    float rightHandY = cos(glm::radians(rightArmAngle)) * 0.06f;
    drawLimb(Shape::Square, x, y, -0.015f, -0.04f, 0.02f, 0.08f, swing, 0.78f, 0.88f, 0.23f); // left leg
    drawLimb(Shape::Square, x, y, 0.015f, -0.04f, 0.02f, 0.08f, -swing, 0.78f, 0.88f, 0.23f); // right leg
    drawPart(Shape::Square, x, y, -0.015f + footX, -0.04f - footY, 0.032f, 0.015f, swing, 0.07f, 0.07f, 0.07f); // left boot
    drawPart(Shape::Square, x, y, 0.015f - footX, -0.04f - footY, 0.032f, 0.015f, -swing, 0.07f, 0.07f, 0.07f); // right boot
    drawPart(Shape::Square, x, y, 0.0f, -0.035f, 0.06f, 0.03f, 0.0f, 0.12f, 0.16f, 0.27f); // shorts
    drawPart(Shape::Square, x, y, 0.0f, 0.01f, 0.06f, 0.07f, 0.0f, 0.78f, 0.88f, 0.23f); // torso
    drawPart(Shape::Square, x, y, 0.0f, 0.03f, 0.06f, 0.006f, 0.0f, 0.24f, 0.29f, 0.10f); // chest stripe
    drawLimb(Shape::Square, x, y, -0.028f, 0.038f, 0.016f, 0.06f, leftArmAngle, 0.78f, 0.88f, 0.23f); // left arm
    drawLimb(Shape::Square, x, y, 0.028f, 0.038f, 0.016f, 0.06f, rightArmAngle, 0.78f, 0.88f, 0.23f); // right arm
    drawPart(Shape::Square, x, y, -0.028f + leftHandX, 0.038f - leftHandY, 0.032f, 0.032f, 0.0f, 0.95f, 0.95f, 0.95f); // left glove
    drawPart(Shape::Square, x, y, 0.028f + rightHandX, 0.038f - rightHandY, 0.032f, 0.032f, 0.0f, 0.95f, 0.95f, 0.95f); // right glove
    drawPart(Shape::Circle, x, y, 0.0f, 0.07f, 0.04f, 0.04f, 0.0f, 0.79f, 0.56f, 0.40f); // head
    drawPart(Shape::Square, x, y, 0.0f, 0.06f, 0.038f, 0.014f, 0.0f, 0.17f, 0.11f, 0.08f); // beard
    drawPart(Shape::Circle, x, y, 0.0f, 0.054f, 0.026f, 0.026f, 0.0f, 0.17f, 0.11f, 0.08f); // chin
    drawPart(Shape::Square, x, y, 0.0f, 0.063f, 0.012f, 0.003f, 0.0f, 0.79f, 0.56f, 0.40f); // mouth
    drawPart(Shape::Square, x, y, 0.0f, 0.085f, 0.04f, 0.012f, 0.0f, 0.17f, 0.11f, 0.08f); // hair
    if (attacking)
        drawPart(Shape::Circle, x, y, 0.0f, 0.11f, 0.032f, 0.032f, 0.0f, 1.0f, 1.0f, 1.0f);     // ball above head
    else
        drawPart(Shape::Circle, x, y, -0.055f, -0.04f, 0.032f, 0.032f, 0.0f, 1.0f, 1.0f, 1.0f); // ball at his side
}

void drawMediumEnemy(float x, float y, float animTime, bool attacking){
    glUseProgram(enemyShaderProgram);
    float swing = sin(animTime *6.0f) * 20.0f;
    float leftLegAngle = swing;
    float rightLegAngle = -swing;
    if (attacking) {
        leftLegAngle = -60.0f;
        rightLegAngle = 0.0f;
    }
    float leftFootX  = sin(glm::radians(leftLegAngle))  * 0.08f;
    float leftFootY  = cos(glm::radians(leftLegAngle))  * 0.08f;
    float rightFootX = sin(glm::radians(rightLegAngle)) * 0.08f;
    float rightFootY = cos(glm::radians(rightLegAngle)) * 0.08f;
    float leftShinX  = sin(glm::radians(leftLegAngle))  * 0.055f;
    float leftShinY  = cos(glm::radians(leftLegAngle))  * 0.055f;
    float rightShinX = sin(glm::radians(rightLegAngle)) * 0.055f;
    float rightShinY = cos(glm::radians(rightLegAngle)) * 0.055f;
    drawLimb(Shape::Square, x, y, -0.014f, -0.04f, 0.02f, 0.08f, leftLegAngle, 0.64f, 0.07f, 0.23f); // left leg
    drawLimb(Shape::Square, x, y, 0.014f, -0.04f, 0.02f, 0.08f, rightLegAngle, 0.64f, 0.07f, 0.23f); // right leg
    drawPart(Shape::Square, x, y, -0.014f + leftShinX, -0.04f - leftShinY, 0.024f, 0.03f, leftLegAngle, 0.94f, 0.94f, 0.94f); // left shin guard
    drawPart(Shape::Square, x, y, 0.014f + rightShinX, -0.04f - rightShinY, 0.024f, 0.03f, rightLegAngle, 0.94f, 0.94f, 0.94f); // right shin guard
    drawPart(Shape::Square, x, y, 0.014f + rightFootX, -0.04f - rightFootY, 0.034f, 0.015f, rightLegAngle, 0.95f, 0.76f, 0.0f); // right boot
    drawPart(Shape::Square, x, y, -0.014f + leftFootX, -0.04f - leftFootY, 0.034f, 0.015f, leftLegAngle, 0.95f, 0.76f, 0.0f); // left boot
    drawPart(Shape::Square, x, y, 0.0f, -0.03f, 0.058f, 0.03f, 0.0f, 0.11f, 0.24f, 0.56f); // shorts
    drawPart(Shape::Square, x, y, 0.0f, 0.015f, 0.058f, 0.07f, 0.0f, 0.11f, 0.24f, 0.56f); // torso
    drawPart(Shape::Square, x, y, -0.018f, 0.015f, 0.009f, 0.07f, 0.0f, 0.64f, 0.07f, 0.23f); // stripe 1
    drawPart(Shape::Square, x, y, 0.0f, 0.015f, 0.009f, 0.07f, 0.0f, 0.64f, 0.07f, 0.23f); // stripe 2
    drawPart(Shape::Square, x, y, 0.018f, 0.015f, 0.009f, 0.07f, 0.0f, 0.64f, 0.07f, 0.23f); // stripe 3
    drawPart(Shape::Square, x, y, -0.04f, 0.02f, 0.015f, 0.055f, -45.0f, 0.11f, 0.24f, 0.56f); // left arm
    drawPart(Shape::Square, x, y, 0.04f, 0.02f, 0.015f, 0.055f, 45.0f, 0.11f, 0.24f, 0.56f); // right arm
    drawPart(Shape::Square, x, y, -0.054f, 0.006f, 0.016f, 0.007f, -45.0f, 0.94f, 0.94f, 0.94f); // left wristband
    drawPart(Shape::Square, x, y, 0.054f, 0.006f, 0.016f, 0.007f, 45.0f, 0.94f, 0.94f, 0.94f); // right wristband
    drawPart(Shape::Circle, x, y, -0.06f, 0.0f, 0.016f, 0.016f, 0.0f, 0.72f, 0.48f, 0.32f); // left hand
    drawPart(Shape::Circle, x, y, 0.06f, 0.0f, 0.016f, 0.016f, 0.0f, 0.72f, 0.48f, 0.32f); // right hand
    drawPart(Shape::Square, x, y, 0.027f, 0.078f, 0.012f, 0.016f, 50.0f, 0.10f, 0.07f, 0.05f); // ponytail
    drawPart(Shape::Circle, x, y, 0.032f, 0.072f, 0.014f, 0.014f, 0.0f, 0.10f, 0.07f, 0.05f); // ponytail tip
    drawPart(Shape::Circle, x, y, 0.0f, 0.075f, 0.04f, 0.04f, 0.0f, 0.72f, 0.48f, 0.32f); // head
    drawPart(Shape::Square, x, y, 0.0f, 0.091f, 0.04f, 0.012f, 0.0f, 0.10f, 0.07f, 0.05f); // hair
    drawPart(Shape::Square, x, y, 0.0f, 0.082f, 0.042f, 0.006f, 0.0f, 0.94f, 0.94f, 0.94f); // headband
    drawPart(Shape::Circle, x, y, 0.021f, 0.084f, 0.012f, 0.012f, 0.0f, 0.94f, 0.94f, 0.94f); // hair tie
    if (!attacking)
        drawPart(Shape::Circle, x, y, -0.062f, -0.122f, 0.032f, 0.032f, 0.0f, 1.0f, 1.0f, 1.0f); // ball
}

void drawHardEnemy(float x, float y, float animTime, bool attacking){
    glUseProgram(enemyShaderProgram);
    float swing = sin(animTime * 14.0f) * 15.0f;
    float backAngle = 45.0f + swing;
    float frontAngle = -40.0f - swing;

    if (attacking) {            
        frontAngle = -85.0f;
        backAngle = 30.0f;
        y -= 0.05f;
    }

    float backFootX = sin(glm::radians(backAngle)) * 0.08f;
    float backFootY = cos(glm::radians(backAngle)) * 0.08f;
    float frontFootX = sin(glm::radians(frontAngle)) * 0.08f;
    float frontFootY = cos(glm::radians(frontAngle)) * 0.08f;
    drawPart(Shape::Square, x, y, 0.075f, 0.05f, 0.04f, 0.004f, 0.0f, 0.95f, 0.95f, 0.95f); // speed line 1
    drawPart(Shape::Square, x, y, 0.085f, 0.02f, 0.04f, 0.004f, 0.0f, 0.95f, 0.95f, 0.95f); // speed line 2
    drawPart(Shape::Square, x, y, 0.075f, -0.01f, 0.04f, 0.004f, 0.0f, 0.95f, 0.95f, 0.95f); // speed line 3
    drawPart(Shape::Square, x, y, -0.005f, 0.045f, 0.016f, 0.045f, 115.0f, 0.08f, 0.16f, 0.42f); // back upper arm
    drawPart(Shape::Square, x, y, 0.025f, 0.037f, 0.014f, 0.04f, 30.0f, 0.54f, 0.35f, 0.24f); // back forearm
    drawPart(Shape::Circle, x, y, 0.035f, 0.02f, 0.016f, 0.016f, 0.0f, 0.54f, 0.35f, 0.24f); // back hand
    drawLimb(Shape::Square, x, y, 0.0f, -0.035f, 0.022f, 0.08f, backAngle, 0.64f, 0.07f, 0.23f); // back leg
    drawPart(Shape::Square, x, y, backFootX, -0.035f - backFootY, 0.036f, 0.016f, backAngle - 15.0f, 1.0f, 0.23f, 0.19f); // back boot
    drawLimb(Shape::Square, x, y, 0.0f, -0.035f, 0.022f, 0.08f, frontAngle, 0.64f, 0.07f, 0.23f); // front leg
    drawPart(Shape::Square, x, y, frontFootX, -0.035f - frontFootY, 0.036f, 0.016f, frontAngle - 15.0f, 1.0f, 0.23f, 0.19f); // front boot
    drawPart(Shape::Square, x, y, -0.002f, -0.03f, 0.06f, 0.03f, 20.0f, 0.11f, 0.24f, 0.56f); // shorts
    drawPart(Shape::Square, x, y, -0.017f, 0.012f, 0.062f, 0.075f, 20.0f, 0.11f, 0.24f, 0.56f); // torso
    drawPart(Shape::Square, x, y, -0.036f, 0.005f, 0.01f, 0.075f, 20.0f, 0.64f, 0.07f, 0.23f); // stripe 1
    drawPart(Shape::Square, x, y, -0.017f, 0.012f, 0.01f, 0.075f, 20.0f, 0.64f, 0.07f, 0.23f); // stripe 2
    drawPart(Shape::Square, x, y, 0.002f, 0.019f, 0.01f, 0.075f, 20.0f, 0.64f, 0.07f, 0.23f); // stripe 3
    drawPart(Shape::Circle, x, y, -0.039f, 0.073f, 0.042f, 0.042f, 0.0f, 0.54f, 0.35f, 0.24f); // head
    drawPart(Shape::Square, x, y, -0.043f, 0.084f, 0.044f, 0.012f, 20.0f, 1.0f, 0.83f, 0.0f); // hair base
    drawPart(Shape::Triangle, x, y, -0.062f, 0.089f, 0.014f, 0.024f, -15.0f, 1.0f, 0.83f, 0.0f); // spike 1
    drawPart(Shape::Triangle, x, y, -0.054f, 0.096f, 0.014f, 0.024f, -15.0f, 1.0f, 0.83f, 0.0f); // spike 2
    drawPart(Shape::Triangle, x, y, -0.045f, 0.1f, 0.014f, 0.024f, -15.0f, 1.0f, 0.83f, 0.0f); // spike 3
    drawPart(Shape::Triangle, x, y, -0.034f, 0.1f, 0.014f, 0.024f, -15.0f, 1.0f, 0.83f, 0.0f); // spike 4
    drawPart(Shape::Square, x, y, -0.051f, 0.075f, 0.013f, 0.003f, 0.0f, 0.16f, 0.10f, 0.06f); // angry brow
    drawPart(Shape::Circle, x, y, -0.05f, 0.069f, 0.006f, 0.006f, 0.0f, 0.16f, 0.10f, 0.06f); // eye
    drawPart(Shape::Square, x, y, -0.047f, 0.028f, 0.016f, 0.045f, -70.0f, 0.08f, 0.16f, 0.42f); // front upper arm
    drawPart(Shape::Square, x, y, -0.073f, 0.04f, 0.014f, 0.04f, -165.0f, 0.54f, 0.35f, 0.24f); // front forearm
    drawPart(Shape::Circle, x, y, -0.078f, 0.059f, 0.016f, 0.016f, 0.0f, 0.54f, 0.35f, 0.24f); // front hand
}

void drawTriangle(){
    glUseProgram(enemyShaderProgram);
    
    glUniform1f(enemyDamageLocation, 0.0f);
    for (auto& proj : projectiles) {
        if (!proj.alive) continue;
        drawPart(Shape::Circle, proj.x, proj.y, 0.0f, 0.0f, 0.025f, 0.025f, 0.0f, 1.0f, 1.0f, 1.0f);

    }

    for (auto& coin : droppedCoins) {
        if (!coin.alive) continue;
        drawPart(Shape::Circle, coin.x, coin.y, 0, 0, 0.05f,  0.05f,  0, 0.72f, 0.55f, 0.12f);
        drawPart(Shape::Circle, coin.x, coin.y, 0, 0, 0.036f, 0.036f, 0, 0.90f, 0.72f, 0.20f);
    }

    float t = (float)glfwGetTime();

    for (auto& enemy : enemies) {
        if (!enemy.alive) continue;
        
        float damage = 1.0f - (float)enemy.health / enemy.maxHealth;
        if (damage > 0.7f) 
            damage = 0.7f;
        glUseProgram(enemyShaderProgram);
        glUniform1f(enemyDamageLocation, damage);
        if (enemy.type == enemyType::Easy)
            drawEasyEnemy(enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane], t, enemy.attackAnim > 0.0f);        
        else if (enemy.type == enemyType::Medium)
            drawMediumEnemy(enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane], t, enemy.attackAnim > 0.0f);
        else
            drawHardEnemy(enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane], t, enemy.attackAnim > 0.0f);    
        drawHealthBar(enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane] + 0.13f, enemy.health, enemy.maxHealth);        
    }
}

void spawnEnemy() {
    enemy newEnemy;
    newEnemy.x = GameConfig::ENEMY_SPAWN_X;
    newEnemy.lane = rand() % GameConfig::LANE_COUNT;
    newEnemy.speed = ENEMY_SPEED;
    newEnemy.alive = true;
    newEnemy.attackTimer = ATTACK_INTERVAL; 
    newEnemy.attackAnim = 0.0f;

    if (currentWave == 0){
        newEnemy.type = enemyType::Easy;
        newEnemy.health = 5;
        newEnemy.maxHealth = 5;
    }      
    else if (currentWave == 1){
        int pick = rand() % 2;
            if (pick == 0) 
                newEnemy.type = enemyType::Easy;
            else
                newEnemy.type = enemyType::Medium;
        if (spawnedEnemies == 0)
            newEnemy.type = enemyType::Medium;
        newEnemy.health = 5;
        newEnemy.maxHealth = newEnemy.health;
        
    }      
    else {
        int pick = rand() % 3;
            if (pick == 0) 
                newEnemy.type = enemyType::Easy;
            else
                if (pick == 1)
                    newEnemy.type = enemyType::Medium;
                else{
                    newEnemy.type = enemyType::Hard;
                    newEnemy.speed = ENEMY_SPEED * 2.0f;
                }       
        if (spawnedEnemies == 0){
            newEnemy.type = enemyType::Easy;
            newEnemy.speed = ENEMY_SPEED;
        }
        else {
            if (spawnedEnemies == 1){
                newEnemy.type = enemyType::Medium;
                newEnemy.speed = ENEMY_SPEED;
            }
            else{
                if (spawnedEnemies == 2){
                    newEnemy.type = enemyType::Hard;
                    newEnemy.speed = ENEMY_SPEED * 2.0f;
                }
            }
        }    
        newEnemy.health = (newEnemy.type == enemyType::Easy || newEnemy.type == enemyType::Medium) ? 5 : 7;
        newEnemy.maxHealth = newEnemy.health;   
    }

    enemies.push_back(newEnemy);
}

void updateEnemyPosition(float deltaTime) {
    
    if (gameLost) return;

    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval && currentWave < sizeof(waves) / sizeof(waves[0]) && spawnedEnemies < waves[currentWave]) {
        spawnEnemy();
        spawnedEnemies++;
        std::cout << "Spawned " << spawnedEnemies << " of " << waves[currentWave] << " in wave " << currentWave + 1 << std::endl;
        spawnTimer = 0.0f;
    }
    
    for (auto& enemy : enemies) {
        
        if (!enemy.alive) continue;
        
        if (enemy.health <= 0) {
            enemy.alive = false;
            droppedCoins.push_back({enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane], enemy.lane, 0.0f, true});
            std::cout << "Enemy in lane " << enemy.lane << " defeated" << std::endl;
            continue;
        }

        if (enemy.attackAnim > 0) {
            enemy.attackAnim -= deltaTime;
        }


        bool playerHere = isPlayerAt(enemy.lane, enemy.x);   // every enemy type stops when it touches a player

        if (enemy.type == enemyType::Hard && playerHere) {
            enemy.attackTimer += deltaTime;
            if (enemy.attackTimer >= ATTACK_INTERVAL*2) {
                enemy.attackTimer = 0.0f;
                enemy.attackAnim = 0.3f;
                hitPlayer(enemy.lane, enemy.x, 2, AttackHeight::Melee);
            }
        }
        else if (!playerHere) {
            enemy.x -= enemy.speed * deltaTime;
        }


        if (enemy.x <= GameConfig::HOME_BASE_RIGHT) {
            gameLost = true;  //Test
            std::cout << "Game Over!" << std::endl;  
            G_STATE = GAME_STATE::Lose; 

        }

        if (enemy.type == enemyType::Easy || enemy.type == enemyType::Medium){
            enemy.attackTimer += deltaTime;
            if (enemy.attackTimer >= ATTACK_INTERVAL) {
                enemy.attackTimer = 0.0f;
                enemy.attackAnim = 0.3f;
                if (enemy.type == enemyType::Easy)
                    projectiles.push_back({enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane] + 0.05f, enemy.lane, 0.25f, 1, true, AttackHeight::High});
                else if (enemy.type == enemyType::Medium)
                    projectiles.push_back({enemy.x, GameConfig::LANE_CENTER_Y[enemy.lane] - 0.12f, enemy.lane, 0.25f, 1, true, AttackHeight::Low});
                }
        } 
    }

    for (auto& proj : projectiles) {
        if (!proj.alive) continue;
        proj.x -= proj.speed * deltaTime;

        if (hitPlayer(proj.lane, proj.x, proj.damage, proj.height)) {   // hit a player (misses if dodged)
            proj.alive = false;
            continue;
        }

        if (proj.x <= GameConfig::HOME_BASE_RIGHT) {
            proj.alive = false;
        }
    }
    
    for (auto& coin : droppedCoins) {
        if (!coin.alive) continue;
        coin.timer += 0.1f *deltaTime;
        coin.y += 0.01f * deltaTime; 
        if (coin.timer >= 1.5f) 
            coin.alive = false;
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
            std::cout << "All waves cleared" << std::endl;  
            G_STATE = GAME_STATE::Win;
        }
        spawnedEnemies = 0;
        spawnTimer = 0.0f;
    }
}

void resetEnemies() {
    enemies.clear();
    currentWave = 0;
    spawnedEnemies = 0;
    spawnTimer = 0.0f;
    gameLost = false;
    droppedCoins.clear();
    projectiles.clear();
}

bool isEnemyAt(int lane, float x) {
    for (auto& enemy : enemies){
        if (!enemy.alive) continue;
        if (enemy.lane == lane && fabs(enemy.x - x) <= 0.05f) {
            return true;
        }
    }    
    return false;
}

bool hitEnemy(int lane, float x, int damage){
    for (auto& enemy : enemies){
        if (!enemy.alive || enemy.health <= 0) continue;
        if (enemy.lane == lane && fabs(enemy.x - x) <= 0.05f) {
            enemy.health -= damage;
            std::cout << "Enemy in lane " << lane << " hit for " << damage << ", remaining health: " << enemy.health << std::endl;
            return true;
        }
    }
    return false;
}

void collectCoinsAt(int lane, float x){
    for (auto& coin : droppedCoins){
        if (!coin.alive) continue;
        if (coin.lane == lane && fabs(coin.x - x) <= 0.05f) {
            addCoins(GameConfig::COIN_VALUE);
            coin.alive = false;
            std::cout << "Collected coins at lane " << lane << ", x: " << x << std::endl;
            return;
        }
    }
}