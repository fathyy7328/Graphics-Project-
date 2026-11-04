#pragma once 

enum class PlayerType {
    Melee,
    Ranged,
    Defense
};

void spawnPlayer(PlayerType type, int lane);  
void playersInit() ;
void playersUpdate(float dt);
void playersDraw();
void playersReset();
