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

struct Player {
    PlayerType type;
    int lane;
    float x;
    float yOffset;
    float speed;
    bool alive;
    int health;
    int maxHealth;
};

static std::vector<Player> players;
static GLuint quadVAO = 0, quadVBO = 0, quadEBO = 0;
static GLuint playerShader = 0;
static GLint locModel = -1, locColor = -1;

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
    "void main()\n"
    "{\n"
    "   FragColor = u_Color;\n"
    "}\0";

void spawnPlayer(PlayerType type, int lane) {
    Player p;
    p.type = type;
    p.lane = lane;
    p.x = GameConfig::PLAYER_START_X;
    p.yOffset = 0.0f;
    p.speed = 0.1f;
    p.alive = true;

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

    locModel = glGetUniformLocation(playerShader, "u_Model");
    locColor = glGetUniformLocation(playerShader, "u_Color");

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

    spawnPlayer(PlayerType::Melee, 0);
    spawnPlayer(PlayerType::Ranged, 1);
    spawnPlayer(PlayerType::Defense, 2);
}

void playersUpdate(float dt) {
    for (Player& p : players) {
        if (p.type != PlayerType::Defense) {
            p.x += p.speed * dt;
        }

        float maxX = GameConfig::PLAYFIELD_RIGHT - 0.05f;
        if (p.x > maxX) {
            p.x = maxX;
        }
    }
}

void playersDraw() {
    glUseProgram(playerShader);
    glBindVertexArray(quadVAO);

    for (Player& p : players) {
        float y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(p.x, y, 0.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.2f, 1.0f));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

        if (p.type == PlayerType::Melee) {
            glUniform4f(locColor, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        else if (p.type == PlayerType::Ranged) {
            glUniform4f(locColor, 0.2f, 0.4f, 1.0f, 1.0f);
        }
        else {
            glUniform4f(locColor, 0.9f, 0.8f, 0.2f, 1.0f);
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    
    for (Player& p : players) {
        float y = GameConfig::LANE_CENTER_Y[p.lane] + p.yOffset;
        drawHealthBar(p.x, y + 0.14f, p.health, p.maxHealth);
    }
}

void playersReset() {
    players.clear();
}