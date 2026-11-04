#pragma once 

void movingTriangle();
void drawTriangle();
void updateEnemyPosition(float deltaTime);
void resetEnemies();
void drawEasyEnemy(float x, float y, float animTime, bool attacking);
void drawMediumEnemy(float x, float y, float animTime, bool attacking);
void drawHardEnemy(float x, float y, float animTime, bool attacking);