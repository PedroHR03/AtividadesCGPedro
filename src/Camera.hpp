#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    // Posição e orientação
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    // Ângulos de Euler
    float yaw;    // rotação em Y (olhar para os lados)
    float pitch;  // rotação em X (olhar para cima/baixo)

    // Configurações
    float speed;
    float sensitivity;
    float fov;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f),
           float startYaw   = -90.0f,
           float startPitch =   0.0f)
        : position(startPos)
        , yaw(startYaw)
        , pitch(startPitch)
        , speed(5.0f)
        , sensitivity(0.05f)
        , fov(45.0f)
    {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        recalcularVetores();
    }

    // Retorna a View Matrix atual
    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, position + front, up);
    }

    // Move a câmera nas 4 direções relativas à sua orientação
    // direction: 0=frente, 1=trás, 2=esquerda, 3=direita
    void mover(int direction, float deltaTime)
    {
        float vel = speed * deltaTime;
        if (direction == 0) position += front * vel;
        if (direction == 1) position -= front * vel;
        if (direction == 2) position -= right * vel;
        if (direction == 3) position += right * vel;
    }

    // Rotaciona a câmera a partir do deslocamento do mouse
    void rotacionar(float xOffset, float yOffset)
    {
        yaw   += xOffset * sensitivity;
        pitch += yOffset * sensitivity;

        // Limita o pitch para não virar de cabeça para baixo
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        recalcularVetores();
    }

    // Zoom via scroll — altera o FOV
    void zoom(float yOffset)
    {
        fov -= yOffset;
        if (fov <  1.0f) fov =  1.0f;
        if (fov > 45.0f) fov = 45.0f;
    }

private:
    void recalcularVetores()
    {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);

        // Recalcula right e up a partir do world-up (0,1,0)
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up    = glm::normalize(glm::cross(right, front));
    }
};
