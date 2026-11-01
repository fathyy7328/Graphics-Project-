#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>

void createLanes();
void drawLane(int i);
void drawLanes();
void deleteLanes();
void updateLanes(GLFWwindow* window);
bool isLaneSelected(int lane);
void createHealthBar();
void drawHealthBar(float x, float y, int hp, int maxHp);
void deleteHealthBar();
void addCoins(int n);
int getCoins();
bool spendCoins(int n);
void resetCoins();
void createBackground();
void drawBackground();
void deleteBackground();
void createDecorations();
void drawDecorations();
void deleteDecorations();
void createHomeBase();
void drawHomeBase();
void deleteHomeBase();
void worldReset();