#include <glad/gl.h>
#include <iostream>
#include <vector>
#include "World.h"
#include "../Common.h"

GLuint laneVAO, laneVBO, laneShader;

const int STRIPES_PER_LANE = 8;                  
const int RECTS_PER_LANE = STRIPES_PER_LANE + 2; 
const float EDGE_SIZE = 0.02f;                   
const float SOIL[3]        = {0.45f, 0.30f, 0.18f};
const float LIGHT_GRASS[3] = {0.42f, 0.72f, 0.26f};
const float DARK_GRASS[3]  = {0.33f, 0.61f, 0.20f};
const float GRASS_EDGE[3]  = {0.20f, 0.42f, 0.12f};
// hover and selection
int hoveredLane = -1;
bool laneSelected[GameConfig::LANE_COUNT] = {false};
bool mouseWasPressed = false;
const float HOVER_HIGHLIGHT = 0.2f;
const float SELECTED_HIGHLIGHT = 0.45f;
const char* laneVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main()
{
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* laneFragmentShaderSource = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
uniform float u_Highlight; 
void main()
{
    FragColor = vec4(mix(vColor, vec3(1.0), u_Highlight), 1.0);
}
)";

GLuint compileLaneShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Lane shader compile error:\n" << infoLog << std::endl;
    }
    return shader;
}

void createLaneShader()
{
    GLuint vertexShader = compileLaneShader(GL_VERTEX_SHADER, laneVertexShaderSource);
    GLuint fragmentShader = compileLaneShader(GL_FRAGMENT_SHADER, laneFragmentShaderSource);
    laneShader = glCreateProgram();
    glAttachShader(laneShader, vertexShader);
    glAttachShader(laneShader, fragmentShader);
    glLinkProgram(laneShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void addRect(std::vector<float>& v, float left, float bottom, float right, float top, const float color[3])
{
    float corners[6][2] = {
        {left, bottom}, {right, bottom}, {right, top}, // first triangle
        {left, bottom}, {right, top},    {left, top}   // second triangle
    };

    for (int i = 0; i < 6; i++)
    {
        v.push_back(corners[i][0]);
        v.push_back(corners[i][1]);
        v.push_back(0.0f);
        v.push_back(color[0]);
        v.push_back(color[1]);
        v.push_back(color[2]);
    }
}

void createLanes()
{
    createLaneShader();

    std::vector<float> vertices;
    float soilTop = GameConfig::LANE_CENTER_Y[0] + GameConfig::LANE_HALF_HEIGHT + 0.04f;
    float soilBottom = GameConfig::LANE_CENTER_Y[GameConfig::LANE_COUNT - 1] - GameConfig::LANE_HALF_HEIGHT - 0.04f;
    addRect(vertices, GameConfig::PLAYFIELD_LEFT, soilBottom, GameConfig::PLAYFIELD_RIGHT, soilTop, SOIL);

    for (int i = 0; i < GameConfig::LANE_COUNT; i++)
    {
        float left = GameConfig::PLAYFIELD_LEFT;
        float right = GameConfig::PLAYFIELD_RIGHT;
        float bottom = GameConfig::LANE_CENTER_Y[i] - GameConfig::LANE_HALF_HEIGHT;
        float top = GameConfig::LANE_CENTER_Y[i] + GameConfig::LANE_HALF_HEIGHT;
        float stripeWidth = (right - left) / STRIPES_PER_LANE;
        for (int s = 0; s < STRIPES_PER_LANE; s++)
        {
            float x1 = left + s * stripeWidth;
            float x2 = x1 + stripeWidth;
            if (s % 2 == 0)
                addRect(vertices, x1, bottom, x2, top, LIGHT_GRASS);
            else
                addRect(vertices, x1, bottom, x2, top, DARK_GRASS);
        }

        addRect(vertices, left, bottom, right, bottom + EDGE_SIZE, GRASS_EDGE);
        addRect(vertices, left, top - EDGE_SIZE, right, top, GRASS_EDGE);
    }

    glGenVertexArrays(1, &laneVAO);
    glGenBuffers(1, &laneVBO);
    glBindVertexArray(laneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, laneVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawLane(int i)
{
    glUseProgram(laneShader);
    glBindVertexArray(laneVAO);
    float highlight = 0.0f;
    if (laneSelected[i])
        highlight = SELECTED_HIGHLIGHT;
    else if (hoveredLane == i)
        highlight = HOVER_HIGHLIGHT;
    GLint highlightLoc = glGetUniformLocation(laneShader, "u_Highlight");
    glUniform1f(highlightLoc, highlight);
    int firstVertex = 6 + i * RECTS_PER_LANE * 6;
    glDrawArrays(GL_TRIANGLES, firstVertex, RECTS_PER_LANE * 6);
}

void drawLanes()
{
    glUseProgram(laneShader);
    glBindVertexArray(laneVAO);
    GLint highlightLoc = glGetUniformLocation(laneShader, "u_Highlight");
    glUniform1f(highlightLoc, 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    for (int i = 0; i < GameConfig::LANE_COUNT; i++)
    {
        drawLane(i);
    }
}

void deleteLanes()
{
    glDeleteVertexArrays(1, &laneVAO);
    glDeleteBuffers(1, &laneVBO);
    glDeleteProgram(laneShader);
}

void updateLanes(GLFWwindow* window)
{
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (G_STATE != GAME_STATE::Playing)
    {
        hoveredLane = -1;
        mouseWasPressed = pressed;
        return;
    }
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float x = (float)mouseX / (width / 2.0f) - 1.0f;
    float y = 1.0f - (float)mouseY / (height / 2.0f);
    hoveredLane = -1;
    if (x >= GameConfig::PLAYFIELD_LEFT && x <= GameConfig::PLAYFIELD_RIGHT)
    {
        for (int i = 0; i < GameConfig::LANE_COUNT; i++)
        {
            float distance = y - GameConfig::LANE_CENTER_Y[i];
            if (distance < 0)
                distance = -distance;
            if (distance <= GameConfig::LANE_HALF_HEIGHT)
                hoveredLane = i;
        }
    }
    if (pressed && !mouseWasPressed && hoveredLane != -1)
        laneSelected[hoveredLane] = !laneSelected[hoveredLane];
    mouseWasPressed = pressed;
}

bool isLaneSelected(int lane)
{
    if (lane < 0 || lane >= GameConfig::LANE_COUNT)
        return false;
    return laneSelected[lane];
}

// ---------------- Health bar ----------------

GLuint barVAO, barVBO, barShader;
const float BAR_WIDTH = 0.12f;
const float BAR_HEIGHT = 0.02f;
const float BAR_BACKGROUND[3] = {0.15f, 0.15f, 0.15f};
const float BAR_GREEN[3]      = {0.20f, 0.80f, 0.20f};
const float BAR_YELLOW[3]     = {0.95f, 0.80f, 0.10f};
const float BAR_RED[3]        = {0.85f, 0.15f, 0.15f};
const char* barVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 u_Position;
uniform vec2 u_Size;
void main()
{
    gl_Position = vec4(aPos.x * u_Size.x + u_Position.x, aPos.y * u_Size.y + u_Position.y, 0.0, 1.0);
}
)";

const char* barFragmentShaderSource = R"(
#version 330 core
uniform vec3 u_Color;
out vec4 FragColor;
void main()
{
    FragColor = vec4(u_Color, 1.0);
}
)";

void createHealthBar()
{
    GLuint vertexShader = compileLaneShader(GL_VERTEX_SHADER, barVertexShaderSource);
    GLuint fragmentShader = compileLaneShader(GL_FRAGMENT_SHADER, barFragmentShaderSource);
    barShader = glCreateProgram();
    glAttachShader(barShader, vertexShader);
    glAttachShader(barShader, fragmentShader);
    glLinkProgram(barShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    float vertices[] = {
        // first triangle
        0.0f, 0.0f, 0.0f, // bottom left
        1.0f, 0.0f, 0.0f, // bottom right
        1.0f, 1.0f, 0.0f, // top right
        // second triangle
        0.0f, 0.0f, 0.0f, // bottom left
        1.0f, 1.0f, 0.0f, // top right
        0.0f, 1.0f, 0.0f  // top left
    };
    glGenVertexArrays(1, &barVAO);
    glGenBuffers(1, &barVBO);
    glBindVertexArray(barVAO);
    glBindBuffer(GL_ARRAY_BUFFER, barVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
// x, y is the center of the bar
void drawHealthBar(float x, float y, int hp, int maxHp)
{
    if (hp < 0)
        hp = 0;
    float percent = (float)hp / maxHp;
    float left = x - BAR_WIDTH / 2;
    float bottom = y - BAR_HEIGHT / 2;
    glUseProgram(barShader);
    glBindVertexArray(barVAO);
    GLint positionLoc = glGetUniformLocation(barShader, "u_Position");
    GLint sizeLoc = glGetUniformLocation(barShader, "u_Size");
    GLint colorLoc = glGetUniformLocation(barShader, "u_Color");
    // background bar (full width)
    glUniform2f(positionLoc, left, bottom);
    glUniform2f(sizeLoc, BAR_WIDTH, BAR_HEIGHT);
    glUniform3f(colorLoc, BAR_BACKGROUND[0], BAR_BACKGROUND[1], BAR_BACKGROUND[2]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    if (percent > 0.6f)
        glUniform3f(colorLoc, BAR_GREEN[0], BAR_GREEN[1], BAR_GREEN[2]);
    else if (percent > 0.3f)
        glUniform3f(colorLoc, BAR_YELLOW[0], BAR_YELLOW[1], BAR_YELLOW[2]);
    else
        glUniform3f(colorLoc, BAR_RED[0], BAR_RED[1], BAR_RED[2]);
    glUniform2f(sizeLoc, BAR_WIDTH * percent, BAR_HEIGHT);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
void deleteHealthBar()
{
    glDeleteVertexArrays(1, &barVAO);
    glDeleteBuffers(1, &barVBO);
    glDeleteProgram(barShader);
}

// ---------------- Coins ----------------

int coins = GameConfig::STARTING_COINS;
void addCoins(int n)
{
    if (n <= 0)
        return;
    coins += n;
    std::cout << "Coins: " << coins << std::endl;
}
int getCoins()
{
    return coins;
}
// returns true if the player had enough coins and they were taken
bool spendCoins(int n)
{
    if (coins < n)
        return false;
    coins -= n;
    std::cout << "Coins: " << coins << std::endl;
    return true;
}
void resetCoins()
{
    coins = GameConfig::STARTING_COINS;
}

// ---------------- Background ----------------

GLuint bgVAO, bgVBO;
int bgVertexCount = 0;
const float WOOD[3]         = {0.50f, 0.32f, 0.17f};
const float WOOD_DARK[3]    = {0.33f, 0.20f, 0.10f};
const float WOOD_LIGHT[3]   = {0.65f, 0.45f, 0.26f};
const float STONE_GAP[3]    = {0.45f, 0.44f, 0.42f};
const float STONE_LIGHT[3]  = {0.78f, 0.76f, 0.72f};
const float STONE_DARK[3]   = {0.70f, 0.68f, 0.64f};
const float GROUND_GRASS[3] = {0.30f, 0.55f, 0.18f};
const float HEDGE[3]        = {0.13f, 0.33f, 0.10f};
const float HEDGE_LIGHT[3]  = {0.20f, 0.45f, 0.15f};

// call after createLanes() because it uses the lane shader
void createBackground()
{
    std::vector<float> vertices;
    float playTop = GameConfig::LANE_CENTER_Y[0] + GameConfig::LANE_HALF_HEIGHT + 0.04f;
    float playBottom = GameConfig::LANE_CENTER_Y[GameConfig::LANE_COUNT - 1] - GameConfig::LANE_HALF_HEIGHT - 0.04f;
    // 1. wooden bar at the top: 3 long planks with a dark line between them
    addRect(vertices, -1.0f, playTop, 1.0f, 1.0f, WOOD);
    float plankHeight = (1.0f - playTop) / 3;
    addRect(vertices, -1.0f, playTop + plankHeight, 1.0f, playTop + plankHeight + 0.006f, WOOD_DARK);
    addRect(vertices, -1.0f, playTop + 2 * plankHeight, 1.0f, playTop + 2 * plankHeight + 0.006f, WOOD_DARK);
    addRect(vertices, -1.0f, playTop, 1.0f, playTop + 0.012f, WOOD_LIGHT); // light edge at the bottom of the bar
    // 2. grass ground at the bottom
    addRect(vertices, -1.0f, -1.0f, 1.0f, playBottom, GROUND_GRASS);
    addRect(vertices, -1.0f, playBottom - 0.02f, 1.0f, playBottom, GRASS_EDGE);
    // 3. stone patio for the home base (dark gaps first, then the tiles on top)
    float patioLeft = GameConfig::HOME_BASE_LEFT;
    float patioRight = GameConfig::HOME_BASE_RIGHT;
    addRect(vertices, patioLeft, playBottom, patioRight, playTop, STONE_GAP);
    int columns = 4;
    int rows = 12;
    float tileWidth = (patioRight - patioLeft) / columns;
    float tileHeight = (playTop - playBottom) / rows;
    float gap = 0.008f;
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < columns; c++)
        {
            float x1 = patioLeft + c * tileWidth + gap;
            float x2 = patioLeft + (c + 1) * tileWidth - gap;
            float y1 = playBottom + r * tileHeight + gap;
            float y2 = playBottom + (r + 1) * tileHeight - gap;
            if ((r + c) % 2 == 0)
                addRect(vertices, x1, y1, x2, y2, STONE_LIGHT);
            else
                addRect(vertices, x1, y1, x2, y2, STONE_DARK);
        }
    }

    // 4. hedge between the patio and the lanes
    addRect(vertices, GameConfig::HOME_BASE_RIGHT, playBottom, GameConfig::PLAYFIELD_LEFT, playTop, HEDGE);
    addRect(vertices, GameConfig::HOME_BASE_RIGHT + 0.01f, playBottom, GameConfig::PLAYFIELD_LEFT - 0.01f, playTop, HEDGE_LIGHT);

    bgVertexCount = vertices.size() / 6; // 6 numbers per vertex (x, y, z, r, g, b)
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// draw this first, before the lanes, so everything else is on top of it
void drawBackground()
{
    glUseProgram(laneShader);
    glBindVertexArray(bgVAO);
    GLint highlightLoc = glGetUniformLocation(laneShader, "u_Highlight");
    glUniform1f(highlightLoc, 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, bgVertexCount);
}
void deleteBackground()
{
    glDeleteVertexArrays(1, &bgVAO);
    glDeleteBuffers(1, &bgVBO);
}