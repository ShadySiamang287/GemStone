#pragma once

#include <Core/Input.h>

class Camera{
public:
    void Update(GEM::Input& input, float dt);


    glm::mat4 getMatrix();
private:
    glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

    float cameraSpeed = 5.f;
    float sensitivity = 0.15;
    float lastX = 0;
    float lastY = 0;

    float yaw = -90.f;
    float pitch = 0.f;
};