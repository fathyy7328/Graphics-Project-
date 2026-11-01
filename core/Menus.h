#pragma once

#include <glm/glm.hpp>
#include <string>

class Shader;

bool initializeFont();
void RenderText(Shader& shader, std::string text,
                float x, float y, float scale, glm::vec3 color);
