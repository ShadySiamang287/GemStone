#include "Camera.h"

glm::mat4 Camera::getMatrix() {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::Update(GEM::Input& input, float dt) {
    if (input.isKeyDown('W')) {
        cameraPos += cameraSpeed * cameraFront * dt;
    }
    if (input.isKeyDown('S')) {
        cameraPos -= cameraSpeed * cameraFront * dt;
    }
    if (input.isKeyDown('A')) {
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * dt;
    }
    if (input.isKeyDown('D')) {
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * dt;
    }
    if (input.isKeyDown(VK_SPACE)) {
        cameraPos += cameraUp * cameraSpeed * dt;
    }
    if (input.isKeyDown(VK_SHIFT)){
        cameraPos -= cameraUp * cameraSpeed * dt;
    }

    if (input.isRightMouseDown()) {
        float xPos = input.getMouseX();
        float yPos = input.getMouseY();
        float xOffset = xPos - lastX;
        float yOffset = lastY - yPos;

        xOffset *= sensitivity;
        yOffset *= sensitivity;

        yaw += xOffset;
        pitch += yOffset;

        if (pitch > 89.f) {
            pitch = 89.f;
        }
        if (pitch < -89.f) {
            pitch = -89.f;
        }

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);
    }

    lastX =  input.getMouseX();
    lastY = input.getMouseY();
}