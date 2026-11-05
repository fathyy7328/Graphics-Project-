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

bool isLaneSelected(int lane);   // defined in M2's World.cpp

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
    float animTimer;     // R3.5: drives the walking leg swing
    float swingTimer;    // R3.6: drives the melee weapon swing
    bool inCombat;       // R3.6 / R3.12: true while fighting an enemy
    int strikesDone;     // R3.16: how many melee swings have already dealt damage
    float fireTimer;     // R3.7 / R3.15: time since the ranged player last fired
};

// R3.15: a ball fired by a ranged player
struct PlayerProjectile {
    float x;
    float y;
    int lane;
    int damage;          // always 1 (R3.16)
    bool alive;
    bool hitting;        // true while the hit effect is playing
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
static const float WALK_SWING_SPEED = 6.0f;    // how fast the legs swing
static const float WALK_SWING_ANGLE = 20.0f;   // how far the legs swing (degrees)
static const float LEG_LENGTH = 0.07f;
static const float MELEE_SHOULDER_X = 0.028f;    // where the front arm is attached
static const float MELEE_SHOULDER_Y = 0.035f;
static const float MELEE_SWING_SPEED = 6.0f;     // how fast the weapon swings
static const float MELEE_SWING_CENTER = 25.0f;   // middle of the swing (degrees)
static const float MELEE_SWING_RANGE = 45.0f;    // swings 45 degrees each way: from -20 to 70
static const float MELEE_HOLD_ANGLE = 45.0f;     // arm angle while walking (weapon held still)
static const float RANGED_FIRE_INTERVAL = 1.5f;  // seconds between two shots
static const float RANGED_FIRE_ANIM = 0.25f;     // R3.7: how long the header animation lasts
static const float RANGED_RELOAD_ANIM = 0.35f;   // R3.7: how long the new ball takes to drop back onto the head
static const float PROJECTILE_SPEED = 0.6f;      // how fast the ball flies right
static const float PROJECTILE_SIZE = 0.026f;
static const float HIT_EFFECT_TIME = 0.2f;       // R3.15: how long the hit "pop" lasts

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

// Same idea as M4's drawPart: (cx, cy) = where the player stands,
// (px, py) = where this part sits on the body, w/h = size, angle = rotation, r/g/b = color.
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

// Same as drawPart, but the part rotates around its TOP end (hip / shoulder) instead of its center.
// (px, py) is the top end. Same idea as M4's drawLimb.
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

// R3.6: like drawPart, but first moves to the shoulder and rotates by armAngle,
// so the arm, hand and weapon all turn around the shoulder together.
// (px, py) is measured from the shoulder, as if the arm were hanging straight down.
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

// R3.5: two legs that swing around the hip, with a boot at the end of each leg
static void drawWalkingLegs(float x, float y, float swing,
                            float backR, float backG, float backB,
                            float frontR, float frontG, float frontB) {
    float rad = glm::radians(swing);
    float footX = sin(rad) * LEG_LENGTH;   // where the bottom of the leg ends up
    float footY = cos(rad) * LEG_LENGTH;

    drawLimb(PlayerShape::Square, x, y, -0.014f, -0.050f, 0.020f, LEG_LENGTH, swing, backR, backG, backB);                // back leg
    drawPart(PlayerShape::Square, x, y, -0.014f + footX + 0.006f, -0.050f - footY - 0.005f, 0.032f, 0.015f, swing, 0.08f, 0.08f, 0.08f);  // back boot
    drawLimb(PlayerShape::Square, x, y,  0.014f, -0.050f, 0.020f, LEG_LENGTH, -swing, frontR, frontG, frontB);           // front leg
    drawPart(PlayerShape::Square, x, y,  0.014f - footX + 0.006f, -0.050f - footY - 0.005f, 0.032f, 0.015f, -swing, 0.08f, 0.08f, 0.08f); // front boot
}

// R3.1 Melee: blond midfielder with a corner-flag spear
static void drawMeleePlayer(float x, float y, float swing, float armAngle) {
    drawWalkingLegs(x, y, swing, 0.88f, 0.88f, 0.88f, 0.98f, 0.98f, 0.98f);                                 // legs + boots (R3.5)
    drawPart(PlayerShape::Square, x, y, -0.034f,  0.000f, 0.016f, 0.055f,  10.0f, 0.82f, 0.82f, 0.86f); // back arm
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.040f, 0.060f, 0.030f,   0.0f, 0.96f, 0.96f, 0.96f); // shorts
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.010f, 0.060f, 0.070f,   0.0f, 1.00f, 1.00f, 1.00f); // shirt
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.042f, 0.030f, 0.006f,   0.0f, 0.85f, 0.70f, 0.20f); // gold collar
    drawPart(PlayerShape::Circle, x, y, -0.005f,  0.078f, 0.048f, 0.044f,   0.0f, 0.95f, 0.80f, 0.25f); // blond hair
    drawPart(PlayerShape::Circle, x, y,  0.000f,  0.070f, 0.042f, 0.042f,   0.0f, 0.96f, 0.80f, 0.66f); // head
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.072f, 0.006f, 0.006f,   0.0f, 0.10f, 0.07f, 0.05f); // eye
    // R3.6: these 4 parts turn around the shoulder by armAngle, so the weapon swings in the hand
    drawArmPart(PlayerShape::Square,   x, y, armAngle, 0.000f, -0.025f, 0.016f, 0.050f,   0.0f, 0.82f, 0.82f, 0.86f); // front arm
    drawArmPart(PlayerShape::Square,   x, y, armAngle, 0.035f, -0.050f, 0.090f, 0.007f,   0.0f, 0.75f, 0.75f, 0.78f); // flag pole
    drawArmPart(PlayerShape::Triangle, x, y, armAngle, 0.066f, -0.036f, 0.026f, 0.030f, -90.0f, 0.55f, 0.25f, 0.75f); // pennant
    drawArmPart(PlayerShape::Circle,   x, y, armAngle, 0.000f, -0.050f, 0.017f, 0.017f,   0.0f, 0.96f, 0.80f, 0.66f); // hand
}

// R3.2 Ranged: striker with a ball above his head
// R3.7: nod (0..1) = how far into the header he is; ballLift = how high the new ball still is; showBall = false right after firing
static void drawRangedPlayer(float x, float y, float swing, float nod, float ballLift, bool showBall) {
    float headX = 0.010f * nod;     // the head moves forward ...
    float headY = -0.006f * nod;    // ... and a little down, like heading a ball
    float armOut = 25.0f * nod;     // the arms flare out for balance

    drawWalkingLegs(x, y, swing, 0.88f, 0.88f, 0.88f, 0.98f, 0.98f, 0.98f);                                 // legs + boots (R3.5)
    drawPart(PlayerShape::Square, x, y,  0.000f, -0.040f, 0.060f, 0.030f,   0.0f, 0.96f, 0.96f, 0.96f); // shorts
    drawPart(PlayerShape::Square, x, y, -0.036f,  0.005f, 0.016f, 0.050f, -25.0f - armOut, 0.82f, 0.82f, 0.86f); // back arm out
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.010f, 0.060f, 0.070f,   0.0f, 1.00f, 1.00f, 1.00f); // shirt
    drawPart(PlayerShape::Square, x, y,  0.000f,  0.042f, 0.030f, 0.006f,   0.0f, 0.85f, 0.70f, 0.20f); // gold collar
    drawPart(PlayerShape::Square, x, y,  0.036f,  0.005f, 0.016f, 0.050f,  25.0f + armOut, 0.82f, 0.82f, 0.86f); // front arm out
    drawPart(PlayerShape::Circle, x, y,  0.000f + headX,  0.070f + headY, 0.042f, 0.042f, 0.0f, 0.36f, 0.24f, 0.16f); // head
    drawPart(PlayerShape::Square, x, y, -0.002f + headX,  0.088f + headY, 0.042f, 0.012f, 0.0f, 0.05f, 0.05f, 0.05f); // black hair
    drawPart(PlayerShape::Circle, x, y,  0.012f,  0.072f, 0.006f, 0.006f,   0.0f, 0.10f, 0.07f, 0.05f); // eye (stays still)
    if (showBall) {
        drawPart(PlayerShape::Circle, x, y, 0.000f, 0.113f + ballLift, 0.026f, 0.026f, 0.0f, 1.00f, 1.00f, 1.00f); // ball
        drawPart(PlayerShape::Circle, x, y, 0.000f, 0.113f + ballLift, 0.010f, 0.010f, 0.0f, 0.15f, 0.15f, 0.15f); // ball patch
    }
}

// R3.15: a flying ball, or the "pop" effect when it hits
static void drawPlayerProjectile(const PlayerProjectile& b) {
    if (!b.hitting) {
        drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, PROJECTILE_SIZE * 1.2f, PROJECTILE_SIZE * 1.2f, 0.0f, 0.15f, 0.15f, 0.15f); // outline
        drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, PROJECTILE_SIZE, PROJECTILE_SIZE, 0.0f, 1.00f, 1.00f, 1.00f);               // ball
        drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, PROJECTILE_SIZE * 0.4f, PROJECTILE_SIZE * 0.4f, 0.0f, 0.15f, 0.15f, 0.15f); // patch
        return;
    }

    // hit effect: the ball grows while 4 small sparks fly outward
    float t = b.hitTimer / HIT_EFFECT_TIME;          // 0 -> 1
    float size = PROJECTILE_SIZE * (1.0f + t);
    float dist = 0.02f + 0.04f * t;
    drawPart(PlayerShape::Circle, b.x, b.y, 0.0f, 0.0f, size, size, 0.0f, 1.00f, 1.00f, 1.00f);
    for (int i = 0; i < 4; i++) {
        float dir = 45.0f + i * 90.0f;               // 45, 135, 225, 315 degrees
        float rad = glm::radians(dir);
        drawPart(PlayerShape::Triangle, b.x, b.y, cos(rad) * dist, sin(rad) * dist,
                 0.012f, 0.024f, dir - 90.0f, 1.00f, 0.90f, 0.30f);
    }
}

// R3.3 Defense: commander with a big shield
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

// ---------- called by M4 (declared in Common.h) ----------

// Is any living player in this lane close to x? M4 uses it so enemies stop at players (R4.20).
bool isPlayerAt(int lane, float x) {
    for (Player& p : players) {
        if (p.lane == lane && p.health > 0 && fabs(p.x - x) <= 0.05f) {
            return true;
        }
    }
    return false;
}

// An enemy attack reaches x in this lane. Returns true if a player was hit, false if it missed or was dodged.
bool hitPlayer(int lane, float x, int damage, AttackHeight height) {
    // the RIGHTMOST player close to x takes the hit: a Defense standing in front protects the players behind it
    Player* target = nullptr;
    for (Player& p : players) {
        if (p.lane == lane && p.health > 0 && fabs(p.x - x) <= 0.05f) {
            if (target == nullptr || p.x > target->x) {
                target = &p;
            }
        }
    }
    if (target == nullptr) {
        return false;                                    // nobody there
    }

    // dodges: duck under high attacks, jump over low attacks (Defense never jumps or ducks, melee can't be dodged)
    if (height == AttackHeight::High && target->action == PlayerAction::Ducking) {
        return false;
    }
    if (height == AttackHeight::Low && target->action == PlayerAction::Jumping) {
        return false;
    }

    target->health -= damage;                            // R3.19 tint and R3.20 removal follow automatically
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
    // W / Up = jump, S / Down = duck, only in the lanes selected with the mouse (R2.7 / R3.14)
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

        // R3.12: is an enemy touching this player? (M4's isEnemyAt)
        p.inCombat = isEnemyAt(p.lane, p.x);

        // R3.9 / R3.12: walk right, but stop and fight while touching an enemy
        if (p.type != PlayerType::Defense && !p.inCombat) {
            p.x += p.speed * dt;
        }

        float maxX = GameConfig::PLAYFIELD_RIGHT - 0.05f;
        if (p.x > maxX) {
            p.x = maxX;
        }

        // R3.5: legs only swing while the player is actually moving
        if (p.x != oldX) {
            p.animTimer += dt;
        }

        // R3.6: the melee weapon swings only during combat; otherwise the arm stays still
        if (p.type == PlayerType::Melee && p.inCombat) {
            p.swingTimer += dt;

            // R3.16: 1 damage each time the swing reaches the bottom (once per swing, not every frame)
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

        // R3.21: a player touching a dropped coin collects it (M4's function adds the coins)
        collectCoinsAt(p.lane, p.x);

        // R3.7 / R3.15 / R3.9: the ranged player fires a ball every RANGED_FIRE_INTERVAL seconds, enemy or not
        if (p.type == PlayerType::Ranged) {
            p.fireTimer += dt;
            if (p.fireTimer >= RANGED_FIRE_INTERVAL) {
                p.fireTimer = 0.0f;
                float aspect = (float)GameConfig::WINDOW_HEIGHT / GameConfig::WINDOW_WIDTH;
                PlayerProjectile b;
                b.x = p.x + 0.03f * aspect;                                  // just in front of the head
                b.y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset + 0.07f; // head height
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
        if (players[i].health <= 0) {
            players.erase(players.begin() + i);
        }
    }

    // R3.15: move the balls, play the hit effect, then remove them
    for (PlayerProjectile& b : playerProjectiles) {
        if (b.hitting) {
            b.hitTimer += dt;
            if (b.hitTimer >= HIT_EFFECT_TIME) {
                b.alive = false;                     // vanishes after the hit effect
            }
            continue;
        }

        b.x += PROJECTILE_SPEED * dt;                // moves right, stays in its lane

        // R3.15 / R3.16: touching an enemy in the same lane -> 1 damage, then play the hit effect
        if (hitEnemy(b.lane, b.x, b.damage)) {
            b.hitting = true;
        }
        // off the right side of the field without hitting anything -> just vanish
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

        float swing = sin(p.animTimer * WALK_SWING_SPEED) * WALK_SWING_ANGLE;   // R3.5

        if (p.type == PlayerType::Melee) {
            float armAngle = MELEE_HOLD_ANGLE;   // walking: weapon held still
            if (p.inCombat) {
                // R3.6: swing starts raised (70) and strikes down (-20), again and again
                armAngle = MELEE_SWING_CENTER + cos(p.swingTimer * MELEE_SWING_SPEED) * MELEE_SWING_RANGE;
            }
            drawMeleePlayer(p.x, y, swing, armAngle);
        }
        else if (p.type == PlayerType::Ranged) {
            // R3.7: header animation right after firing, then a new ball drops back onto the head
            float nod = 0.0f;
            float ballLift = 0.0f;
            bool showBall = true;
            if (p.fireTimer < RANGED_FIRE_ANIM) {
                nod = sin(p.fireTimer / RANGED_FIRE_ANIM * 3.14159f);   // 0 -> 1 -> 0
                showBall = false;                                       // the ball just left
            }
            else if (p.fireTimer < RANGED_FIRE_ANIM + RANGED_RELOAD_ANIM) {
                float t = (p.fireTimer - RANGED_FIRE_ANIM) / RANGED_RELOAD_ANIM;
                ballLift = 0.05f * (1.0f - t);                          // new ball falls back onto the head
            }
            drawRangedPlayer(p.x, y, swing, nod, ballLift, showBall);
        }
        else {
            drawDefensePlayer(p.x, y);
        }
    }

    // R3.15: the balls fired by ranged players (never tinted red)
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