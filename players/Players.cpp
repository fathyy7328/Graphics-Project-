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

bool isLaneSelected(int lane);   //world

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
    float animTimer;     
    float swingTimer;    
    bool inCombat;       
    int strikesDone;     
    float fireTimer;     
};

// ranged attack
struct PlayerProjectile {
    float x;
    float y;
    int lane;
    int damage;          
    bool alive;
    bool hitting;       
    float hitTimer;
};

static std::vector<Player> players;
static std::vector<PlayerProjectile> playerProjectiles;
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
static const float WALK_SWING_SPEED = 6.0f;    
static const float WALK_SWING_ANGLE = 20.0f;   
static const float LEG_LENGTH = 0.07f;
static const float MELEE_SHOULDER_X = 0.028f;    
static const float MELEE_SHOULDER_Y = 0.035f;
static const float MELEE_SWING_SPEED = 6.0f;     
static const float MELEE_SWING_CENTER = 25.0f;   
static const float MELEE_SWING_RANGE = 45.0f;    
static const float MELEE_HOLD_ANGLE = 45.0f;     
static const float RANGED_FIRE_INTERVAL = 1.5f;  
static const float RANGED_FIRE_ANIM = 0.25f;     
static const float RANGED_RELOAD_ANIM = 0.35f;   
static const float PROJECTILE_SPEED = 0.6f;      
static const float PROJECTILE_SIZE = 0.026f;
static const float HIT_EFFECT_TIME = 0.2f;       

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

static void drawLimb(PlayerShape shape, float cx, float cy, float px, float py,
    float w, float h, float angle, float r, float g, float b) {
    float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::scale(model, glm::vec3(aspect, 1.0f, 1.0f));
    model = glm::translate(model, glm::vec3(px, py, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(0.0f, -h / 2.0f, 0.0f));
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


static void drawArmPart(PlayerShape shape, float cx, float cy, float armAngle, float px, float py,
    float w, float h, float angle, float r, float g, float b) {
    float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::scale(model, glm::vec3(aspect, 1.0f, 1.0f));
    model = glm::translate(model, glm::vec3(MELEE_SHOULDER_X, MELEE_SHOULDER_Y, 0.0f));
    model = glm::rotate(model, glm::radians(armAngle), glm::vec3(0.0f, 0.0f, 1.0f));
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

static void drawWalkingLegs(float x, float y, float swing,
    float backR, float backG, float backB,
    float frontR, float frontG, float frontB) {
    float rad = glm::radians(swing);
    float footX = sin(rad) * LEG_LENGTH;  
    float footY = cos(rad) * LEG_LENGTH;

    drawLimb(PlayerShape::Square, x, y, -0.014f, -0.050f, 0.020f, LEG_LENGTH, swing, backR, backG, backB);                // back leg
    drawPart(PlayerShape::Square, x, y, -0.014f + footX + 0.006f, -0.050f - footY - 0.005f, 0.032f, 0.015f, swing, 0.08f, 0.08f, 0.08f);  // back boot
    drawLimb(PlayerShape::Square, x, y,  0.014f, -0.050f, 0.020f, LEG_LENGTH, -swing, frontR, frontG, frontB);           // front leg
    drawPart(PlayerShape::Square, x, y,  0.014f - footX + 0.006f, -0.050f - footY - 0.005f, 0.032f, 0.015f, -swing, 0.08f, 0.08f, 0.08f); // front boot
}

// Melee
static void drawMeleePlayer(float x, float y, float swing, float armAngle) {
    drawWalkingLegs(x, y, swing, 0.88f, 0.88f, 0.88f, 0.98f, 0.98f, 0.98f); // legs + boots 
    drawPart(PlayerShape::Square, x, y, -0.034f,  0.000f, 0.016f, 0.055f,  10.0f, 0.82f, 0.82f, 0.86f); // back arm
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.040f, 0.060f, 0.030f,   0.0f, 0.96f, 0.96f, 0.96f); // shorts
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.010f, 0.060f, 0.070f,   0.0f, 1.00f, 1.00f, 1.00f); // shirt
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.042f, 0.030f, 0.006f,   0.0f, 0.85f, 0.70f, 0.20f); // ya2a
    drawPart(PlayerShape::Circle, x, y, -0.005f,  0.078f, 0.048f, 0.044f,   0.0f, 0.95f, 0.80f, 0.25f); // blond hair
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.070f, 0.042f, 0.042f,   0.0f, 0.96f, 0.80f, 0.66f); // head
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.072f, 0.006f, 0.006f,   0.0f, 0.10f, 0.07f, 0.05f); // eye
   
    drawArmPart(PlayerShape::Square,   x, y, armAngle, 0.000f, -0.025f, 0.016f, 0.050f,   0.0f, 0.82f, 0.82f, 0.86f); // arm
    drawArmPart(PlayerShape::Square,   x, y, armAngle, 0.035f, -0.050f, 0.090f, 0.007f,   0.0f, 0.75f, 0.75f, 0.78f); // corner flag
    drawArmPart(PlayerShape::Triangle, x, y, armAngle, 0.066f, -0.036f, 0.026f, 0.030f, -90.0f, 0.55f, 0.25f, 0.75f); // pennant
    drawArmPart(PlayerShape::Circle,   x, y, armAngle, 0.000f, -0.050f, 0.017f, 0.017f,   0.0f, 0.96f, 0.80f, 0.66f); // hand
}

// Ranged
static void drawRangedPlayer(float x, float y, float swing, float nod, float ballLift, bool showBall) {
    float headX = 0.010f * nod;    
    float headY = -0.006f * nod;    
    float armOut = 25.0f * nod;    

    drawWalkingLegs(x, y, swing, 0.88f, 0.88f, 0.88f, 0.98f, 0.98f, 0.98f); // legs + boots
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.040f, 0.060f, 0.030f,   0.0f, 0.96f, 0.96f, 0.96f); // shorts
    drawPart(PlayerShape::Square, x, y, -0.036f,  0.005f, 0.016f, 0.050f, -25.0f - armOut, 0.82f, 0.82f, 0.86f); // arm 
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.010f, 0.060f, 0.070f,   0.0f, 1.00f, 1.00f, 1.00f); // shirt
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.042f, 0.030f, 0.006f,   0.0f, 0.85f, 0.70f, 0.20f); // ya2a
    drawPart(PlayerShape::Square, x, y,  0.036f,  0.005f, 0.016f, 0.050f,  25.0f + armOut, 0.82f, 0.82f, 0.86f); // front arm out
    drawPart(PlayerShape::Circle, x, y,  0.000f + headX,  0.070f + headY, 0.042f, 0.042f, 0.0f, 0.36f, 0.24f, 0.16f); // head
    drawPart(PlayerShape::Square, x, y, -0.002f + headX,  0.088f + headY, 0.042f, 0.012f, 0.0f, 0.05f, 0.05f, 0.05f); //  hair
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.072f, 0.006f, 0.006f,   0.0f, 0.10f, 0.07f, 0.05f); // eye
    if (showBall) {
        drawPart(PlayerShape::Circle, x, y, 0.000f, 0.113f + ballLift, 0.026f, 0.026f, 0.0f, 1.00f, 1.00f, 1.00f); // ball
        drawPart(PlayerShape::Circle, x, y, 0.000f, 0.113f + ballLift, 0.010f, 0.010f, 0.0f, 0.15f, 0.15f, 0.15f); // ball patch
    }
}

static void drawPlayerProjectile(const PlayerProjectile& b) {
    if (!b.hitting) {
        drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, PROJECTILE_SIZE * 1.2f, PROJECTILE_SIZE * 1.2f, 0.0f, 0.15f, 0.15f, 0.15f); 
        drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, PROJECTILE_SIZE, PROJECTILE_SIZE, 0.0f, 1.00f, 1.00f, 1.00f);               
        drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, PROJECTILE_SIZE * 0.4f, PROJECTILE_SIZE * 0.4f, 0.0f, 0.15f, 0.15f, 0.15f); 
        return;
    }

    float t = b.hitTimer / HIT_EFFECT_TIME;         
    float size = PROJECTILE_SIZE * (1.0f + t);
    float dist = 0.02f + 0.04f * t;
    drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, size, size, 0.0f, 1.00f, 1.00f, 1.00f);
    for (int i = 0; i < 4; i++) {
        float dir = 45.0f + i * 90.0f;           
        float rad = glm::radians(dir);
        drawPart(PlayerShape::Triangle, b.x, b.y, cos(rad) * dist, sin(rad) * dist,
                 0.012f, 0.024f, dir - 90.0f, 1.00f, 0.90f, 0.30f);
    }
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
    p.animTimer = 0.0f;
    p.swingTimer = 0.0f;
    p.inCombat = false;
    p.strikesDone = 0;
    p.fireTimer = RANGED_FIRE_ANIM + RANGED_RELOAD_ANIM;   // start with the ball resting on the head

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

bool isPlayerAt(int lane, float x) {
    for (Player& p : players) {
        if (p.lane == lane && p.health > 0 && fabs(p.x - x) <= 0.05f) {
            return true;
        }
    }
    return false;
}

bool hitPlayer(int lane, float x, int damage, AttackHeight height) {
    Player* target = nullptr;
    for (Player& p : players) {
        if (p.lane == lane && p.health > 0 && fabs(p.x - x) <= 0.05f) {
            if (target == nullptr || p.x > target->x) {
                target = &p;
            }
        }
    }
    if (target == nullptr) {
        return false;                                  
    }

    if (height == AttackHeight::High && target->action == PlayerAction::Ducking) {
        return false;
    }
    if (height == AttackHeight::Low && target->action == PlayerAction::Jumping) {
        return false;
    }

    target->health -= damage;                          
    return true;
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
}

void playersUpdate(float dt) {
    // w jump and s duck
    GLFWwindow* window = glfwGetCurrentContext();
    bool jumpKey = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool duckKey = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    if (jumpKey && !jumpKeyWasPressed) {
        for (int lane = 0; lane < GameConfig::LANE_COUNT; lane++) {
            if (isLaneSelected(lane)) {
                jumpLane(lane);
            }
        }
    }
    if (duckKey && !duckKeyWasPressed) {
        for (int lane = 0; lane < GameConfig::LANE_COUNT; lane++) {
            if (isLaneSelected(lane)) {
                duckLane(lane);
            }
        }
    }
    jumpKeyWasPressed = jumpKey;
    duckKeyWasPressed = duckKey;

    for (Player& p : players) {
        float oldX = p.x;

        p.inCombat = isEnemyAt(p.lane, p.x);

        if (p.type != PlayerType::Defense && !p.inCombat) {
            p.x += p.speed * dt;
        }

        float maxX = GameConfig::PLAYFIELD_RIGHT - 0.05f;
        if (p.x >= maxX) {
            p.alive = false;
        }

        if (p.x != oldX) {
            p.animTimer += dt;
        }

        if (p.type == PlayerType::Melee && p.inCombat) {
            p.swingTimer += dt;

            float phase = p.swingTimer * MELEE_SWING_SPEED;
            int strikes = (int)((phase + 3.14159f) / (2.0f * 3.14159f));
            if (strikes > p.strikesDone) {
                p.strikesDone = strikes;
                hitEnemy(p.lane, p.x, 1);
            }
        }
        else {
            p.swingTimer = 0.0f;
            p.strikesDone = 0;
        }

        collectCoinsAt(p.lane, p.x);

        if (p.type == PlayerType::Ranged) {
            p.fireTimer += dt;
            if (p.fireTimer >= RANGED_FIRE_INTERVAL) {
                p.fireTimer = 0.0f;
                float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
                PlayerProjectile b;
                b.x = p.x + 0.03f * aspect;                                 
                b.y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset + 0.07f; 
                b.lane = p.lane;
                b.damage = 1;
                b.alive = true;
                b.hitting = false;
                b.hitTimer = 0.0f;
                playerProjectiles.push_back(b);
            }
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
        if (players[i].health <= 0 || !players[i].alive) {
            players.erase(players.begin() + i);
        }
    }

    for (PlayerProjectile& b : playerProjectiles) {
        if (b.hitting) {
            b.hitTimer += dt;
            if (b.hitTimer >= HIT_EFFECT_TIME) {
                b.alive = false;                     
            }
            continue;
        }

        b.x += PROJECTILE_SPEED * dt;              

        if (hitEnemy(b.lane, b.x, b.damage)) {
            b.hitting = true;
        }
        else if (b.x > GameConfig::PLAYFIELD_RIGHT) {
            b.alive = false;
        }
    }

    for (int i = (int)playerProjectiles.size() - 1; i >= 0; i--) {
        if (!playerProjectiles[i].alive) {
            playerProjectiles.erase(playerProjectiles.begin() + i);
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

        float swing = sin(p.animTimer * WALK_SWING_SPEED) * WALK_SWING_ANGLE; 

        if (p.type == PlayerType::Melee) {
            float armAngle = MELEE_HOLD_ANGLE;   
            if (p.inCombat) {
                armAngle = MELEE_SWING_CENTER + cos(p.swingTimer * MELEE_SWING_SPEED) * MELEE_SWING_RANGE;
            }
            drawMeleePlayer(p.x, y, swing, armAngle);
        }
        else if (p.type == PlayerType::Ranged) {
            float nod = 0.0f;
            float ballLift = 0.0f;
            bool showBall = true;
            if (p.fireTimer < RANGED_FIRE_ANIM) {
                nod = sin(p.fireTimer / RANGED_FIRE_ANIM * 3.14159f);   
                showBall = false;                                     
            }
            else if (p.fireTimer < RANGED_FIRE_ANIM + RANGED_RELOAD_ANIM) {
                float t = (p.fireTimer - RANGED_FIRE_ANIM) / RANGED_RELOAD_ANIM;
                ballLift = 0.05f * (1.0f - t);                         
            }
            drawRangedPlayer(p.x, y, swing, nod, ballLift, showBall);
        }
        else {
            drawDefensePlayer(p.x, y);
        }
    }
    
    glUniform1f(playerDamageLocation, 0.0f);
    for (const PlayerProjectile& b : playerProjectiles) {
        drawPlayerProjectile(b);
    }

    glBindVertexArray(0);

    for (Player& p : players) {
        float y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset;
        drawHealthBar(p.x, y + 0.14f, p.health, p.maxHealth);
    }
}

void playersReset() {
    players.clear();
    playerProjectiles.clear();
    jumpKeyWasPressed = false;
    duckKeyWasPressed = false;
}