#include "Players.h"
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../Common.h"
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class PlayerAction {
    None,
    Jumping,
    Ducking
};

enum class PlayerShape {
    Square,
    Circle,
    Triangle
};

struct Player {
    PlayerType type;
    int lane;
    float x;
    float yOffset;
    float speed;
    bool alive;
    int health;
    int maxHealth;
    PlayerAction action;
    float actionTimer;
};

static std::vector<Player> players;
static GLuint quadVAO = 0, quadVBO = 0, quadEBO = 0;
static GLuint circleVAO = 0, circleVBO = 0;
static int circleVertexCount = 0;
static GLuint triangleVAO = 0, triangleVBO = 0;
static GLuint playerShader = 0;
static GLint playerTransformLocation = -1;
static GLint playerColorLocation = -1;
static GLint playerDamageLocation = -1;

static const float ACTION_DURATION = 0.6f;
static const float JUMP_HEIGHT = 0.05f;
static const float DUCK_DEPTH = 0.05f;

static bool jumpKeyWasPressed = false;
static bool duckKeyWasPressed = false;

static const char* playerVertexSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 u_Model;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = u_Model * vec4(aPos, 1.0);\n"
    "}\0";

static const char* playerFragmentSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 u_Color;\n"
    "uniform float u_Damage;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(mix(u_Color.rgb, vec3(1.0, 0.0, 0.0), u_Damage), u_Color.a);\n"
    "}\0";

static void createPlayerQuad() {
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void createPlayerCircle() {
    std::vector<float> circleVertices;
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);

    const int segments = 30;
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159f / segments;
        circleVertices.push_back(0.5f * cos(angle));
        circleVertices.push_back(0.5f * sin(angle));
        circleVertices.push_back(0.0f);
    }
    circleVertexCount = (int)circleVertices.size() / 3;

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

static void createPlayerTriangle() {
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &triangleVBO);
    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void drawPart(PlayerShape shape, float cx, float cy, float px, float py,
                     float w, float h, float angle, float r, float g, float b) {
    float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::scale(model, glm::vec3(aspect, 1.0f, 1.0f));
    model = glm::translate(model, glm::vec3(px, py, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    glUniformMatrix4fv(playerTransformLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4f(playerColorLocation, r, g, b, 1.0f);

    if (shape == PlayerShape::Square) {
        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    else if (shape == PlayerShape::Circle) {
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);
    }
    else {
        glBindVertexArray(triangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

// Melee
static void drawMeleePlayer(float x, float y) {
    drawPart(PlayerShape::Square, x, y, -0.014f, -0.085f, 0.020f, 0.070f,   0.0f, 0.88f, 0.88f, 0.88f); // back leg
    drawPart(PlayerShape::Square, x, y, -0.008f, -0.125f, 0.032f, 0.015f,   0.0f, 0.08f, 0.08f, 0.08f); // back boot
    drawPart(PlayerShape::Square, x, y, -0.034f,  0.000f, 0.016f, 0.055f,  10.0f, 0.82f, 0.82f, 0.86f); // back arm
    drawPart(PlayerShape::Square, x, y,  0.014f, -0.085f, 0.020f, 0.070f,   0.0f, 0.98f, 0.98f, 0.98f); // front leg
    drawPart(PlayerShape::Square, x, y,  0.020f, -0.125f, 0.032f, 0.015f,   0.0f, 0.08f, 0.08f, 0.08f); // front boot
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.040f, 0.060f, 0.030f,   0.0f, 0.96f, 0.96f, 0.96f); // shorts
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.010f, 0.060f, 0.070f,   0.0f, 1.00f, 1.00f, 1.00f); // shirt
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.042f, 0.030f, 0.006f,   0.0f, 0.85f, 0.70f, 0.20f); // gold collar
    drawPart(PlayerShape::Circle, x, y, -0.005f,  0.078f, 0.048f, 0.044f,   0.0f, 0.95f, 0.80f, 0.25f); // blond hair
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.070f, 0.042f, 0.042f,   0.0f, 0.96f, 0.80f, 0.66f); // head
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.072f, 0.006f, 0.006f,   0.0f, 0.10f, 0.07f, 0.05f); // eye
    drawPart(PlayerShape::Square, x, y,  0.046f,  0.017f, 0.016f, 0.050f,  45.0f, 0.82f, 0.82f, 0.86f); // front arm
    drawPart(PlayerShape::Square, x, y,  0.088f,  0.024f, 0.090f, 0.007f,  45.0f, 0.75f, 0.75f, 0.78f); // flag pole
    drawPart(PlayerShape::Triangle, x, y, 0.100f, 0.056f, 0.026f, 0.030f, -45.0f, 0.55f, 0.25f, 0.75f); // pennant
    drawPart(PlayerShape::Circle, x, y,  0.063f,  0.000f, 0.017f, 0.017f,   0.0f, 0.96f, 0.80f, 0.66f); // hand
}

// Ranged
static void drawRangedPlayer(float x, float y) {
    drawPart(PlayerShape::Square, x, y, -0.014f, -0.085f, 0.020f, 0.070f,   0.0f, 0.88f, 0.88f, 0.88f); // back leg
    drawPart(PlayerShape::Square, x, y, -0.008f, -0.125f, 0.032f, 0.015f,   0.0f, 0.08f, 0.08f, 0.08f); // back boot
    drawPart(PlayerShape::Square, x, y,  0.014f, -0.085f, 0.020f, 0.070f,   0.0f, 0.98f, 0.98f, 0.98f); // front leg
    drawPart(PlayerShape::Square, x, y,  0.020f, -0.125f, 0.032f, 0.015f,   0.0f, 0.08f, 0.08f, 0.08f); // front boot
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.040f, 0.060f, 0.030f,   0.0f, 0.96f, 0.96f, 0.96f); // shorts
    drawPart(PlayerShape::Square, x, y, -0.036f,  0.005f, 0.016f, 0.050f, -25.0f, 0.82f, 0.82f, 0.86f); // back arm out
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.010f, 0.060f, 0.070f,   0.0f, 1.00f, 1.00f, 1.00f); // shirt
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.042f, 0.030f, 0.006f,   0.0f, 0.85f, 0.70f, 0.20f); // gold collar
    drawPart(PlayerShape::Square, x, y,  0.036f,  0.005f, 0.016f, 0.050f,  25.0f, 0.82f, 0.82f, 0.86f); // front arm out
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.070f, 0.042f, 0.042f,   0.0f, 0.36f, 0.24f, 0.16f); // head
    drawPart(PlayerShape::Square, x, y, -0.002f,  0.088f, 0.042f, 0.012f,   0.0f, 0.05f, 0.05f, 0.05f); // black hair
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.072f, 0.006f, 0.006f,   0.0f, 0.10f, 0.07f, 0.05f); // eye
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.113f, 0.026f, 0.026f,   0.0f, 1.00f, 1.00f, 1.00f); // ball
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.113f, 0.010f, 0.010f,   0.0f, 0.15f, 0.15f, 0.15f); // ball patch
}

// Defense
static void drawDefensePlayer(float x, float y) {
    drawPart(PlayerShape::Square, x, y, -0.016f, -0.085f, 0.020f, 0.070f,  0.0f, 0.10f, 0.12f, 0.25f); // back leg
    drawPart(PlayerShape::Square, x, y, -0.012f, -0.120f, 0.030f, 0.025f,  0.0f, 0.08f, 0.08f, 0.08f); // back boot
    drawPart(PlayerShape::Square, x, y,  0.016f, -0.085f, 0.020f, 0.070f,  0.0f, 0.10f, 0.12f, 0.25f); // front leg
    drawPart(PlayerShape::Square, x, y,  0.018f, -0.120f, 0.030f, 0.025f,  0.0f, 0.08f, 0.08f, 0.08f); // front boot
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.005f, 0.070f, 0.100f,  0.0f, 0.13f, 0.16f, 0.33f); // long coat
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.034f, 0.070f, 0.008f,  0.0f, 0.85f, 0.70f, 0.20f); // gold belt
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.070f, 0.042f, 0.042f,  0.0f, 0.45f, 0.30f, 0.22f); // head
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.071f, 0.006f, 0.006f,  0.0f, 0.10f, 0.07f, 0.05f); // eye
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.096f, 0.050f, 0.020f,  0.0f, 0.13f, 0.16f, 0.33f); // cap
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.088f, 0.050f, 0.005f,  0.0f, 0.85f, 0.70f, 0.20f); // cap band
    drawPart(PlayerShape::Square, x, y,  0.016f,  0.084f, 0.032f, 0.005f, -8.0f, 0.08f, 0.08f, 0.08f); // visor
    drawPart(PlayerShape::Square, x, y,  0.050f, -0.020f, 0.076f, 0.170f,  0.0f, 0.85f, 0.70f, 0.20f); // shield rim
    drawPart(PlayerShape::Square, x, y,  0.050f, -0.020f, 0.062f, 0.156f,  0.0f, 1.00f, 1.00f, 1.00f); // shield face
    drawPart(PlayerShape::Square, x, y,  0.050f,  0.010f, 0.062f, 0.018f,  0.0f, 0.55f, 0.25f, 0.75f); // purple band
    drawPart(PlayerShape::Circle, x, y,  0.016f, -0.020f, 0.017f, 0.017f,  0.0f, 0.45f, 0.30f, 0.22f); // hand on shield
}

void spawnPlayer(PlayerType type, int lane) {
    Player p;
    p.type = type;
    p.lane = lane;
    p.x = GameConfig::PLAYER_START_X;
    p.yOffset = 0.0f;
    p.speed = 0.1f;
    p.alive = true;
    p.action = PlayerAction::None;
    p.actionTimer = 0.0f;

    if (type == PlayerType::Melee) {
        p.health = 5;
    }
    else if (type == PlayerType::Ranged) {
        p.health = 5;
    }
    else if (type == PlayerType::Defense) {
        p.health = 7;
        p.speed = 0.0f;
    }

    p.maxHealth = p.health;
    players.push_back(p);
}

void jumpLane(int lane) {
    for (Player& p : players) {
        if (p.lane == lane && p.type != PlayerType::Defense && p.action == PlayerAction::None) {
            p.action = PlayerAction::Jumping;
            p.actionTimer = 0.0f;
        }
    }
}

void duckLane(int lane) {
    for (Player& p : players) {
        if (p.lane == lane && p.type != PlayerType::Defense && p.action == PlayerAction::None) {
            p.action = PlayerAction::Ducking;
            p.actionTimer = 0.0f;
        }
    }
}

void playersInit() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &playerVertexSource, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &playerFragmentSource, NULL);
    glCompileShader(fs);

    playerShader = glCreateProgram();
    glAttachShader(playerShader, vs);
    glAttachShader(playerShader, fs);
    glLinkProgram(playerShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    playerTransformLocation = glGetUniformLocation(playerShader, "u_Model");
    playerColorLocation = glGetUniformLocation(playerShader, "u_Color");
    playerDamageLocation = glGetUniformLocation(playerShader, "u_Damage");

    createPlayerQuad();
    createPlayerCircle();
    createPlayerTriangle();

    // TEST ONLY: remove when M2's cards call spawnPlayer
    spawnPlayer(PlayerType::Melee, 0);
    spawnPlayer(PlayerType::Ranged, 1);
    spawnPlayer(PlayerType::Defense, 2);
}

void playersUpdate(float dt) {
    // TEST ONLY: remove when M2 calls jumpLane / duckLane
    GLFWwindow* window = glfwGetCurrentContext();
    bool jumpKey = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool duckKey = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    if (jumpKey && !jumpKeyWasPressed) {
        for (int lane = 0; lane < GameConfig::LANE_COUNT; lane++) jumpLane(lane);
    }
    if (duckKey && !duckKeyWasPressed) {
        for (int lane = 0; lane < GameConfig::LANE_COUNT; lane++) duckLane(lane);
    }
    jumpKeyWasPressed = jumpKey;
    duckKeyWasPressed = duckKey;

    for (Player& p : players) {
        if (p.type != PlayerType::Defense) {
            p.x += p.speed * dt;
        }

        float maxX = GameConfig::PLAYFIELD_RIGHT - 0.05f;
        if (p.x > maxX) {
            p.x = maxX;
        }

        if (p.action != PlayerAction::None) {
            p.actionTimer += dt;
            float progress = p.actionTimer / ACTION_DURATION;

            if (progress >= 1.0f) {
                p.action = PlayerAction::None;
                p.actionTimer = 0.0f;
                p.yOffset = 0.0f;
            }
            else if (p.action == PlayerAction::Jumping) {
                p.yOffset = sin(progress * 3.14159f) * JUMP_HEIGHT;
            }
            else {
                p.yOffset = -sin(progress * 3.14159f) * DUCK_DEPTH;
            }
        }
    }

    for (int i = (int)players.size() - 1; i >= 0; i--) {
        if (players[i].health <= 0) {
            players.erase(players.begin() + i);
        }
    }
}

void playersDraw() {
    glUseProgram(playerShader);

    for (Player& p : players) {
        float y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset;

        float damage = 1.0f - (float)p.health / (float)p.maxHealth;
        if (damage > 0.7f) {
            damage = 0.7f;
        }
        glUniform1f(playerDamageLocation, damage);

        if (p.type == PlayerType::Melee) {
            drawMeleePlayer(p.x, y);
        }
        else if (p.type == PlayerType::Ranged) {
            drawRangedPlayer(p.x, y);
        }
        else {
            drawDefensePlayer(p.x, y);
        }
    }

    glBindVertexArray(0);

    for (Player& p : players) {
        float y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset;
        drawHealthBar(p.x, y + 0.14f, p.health, p.maxHealth);
    }
}

void playersReset() {
    players.clear();
    jumpKeyWasPressed = false;
    duckKeyWasPressed = false;
}