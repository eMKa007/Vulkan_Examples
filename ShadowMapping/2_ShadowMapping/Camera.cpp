#include "Camera.h"


void Camera::updateCameraVectors()
{
    this->front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    this->front.y = sin(glm::radians(this->pitch));
    this->front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

    this->front = glm::normalize(this->front);
    this->right = glm::normalize(glm::cross(this->front, this->worldUp));
    this->up = glm::normalize(glm::cross(this->right, this->front));
}

Camera::Camera(glm::vec3 position, glm::vec3 direction, glm::vec3 worldUp)
{
    this->ViewMatrix = glm::mat4(1.f);

    this->movementSpeed = 3.f;
    this->sensitivity = 5.f;

    this->worldUp = worldUp;
    this->position = position;
    this->right = glm::vec3(0.f);
    this->up = worldUp;

    this->pitch = direction.x;
    this->yaw = direction.y;
    this->roll = direction.z;

    this->updateCameraVectors();
}


Camera::~Camera()
{
}

void Camera::move(const float& dt, const int direction)
{
    // Update Position vector
    switch (direction)
    {
    case FORWARD:
        this->position += this->front * this->movementSpeed * dt;
        break;

    case BACKWARD:
        this->position -= this->front * this->movementSpeed * dt;
        break;

    case LEFT:
        this->position -= this->right * this->movementSpeed * dt;
        break;

    case RIGTH:
        this->position += this->right * this->movementSpeed * dt;
        break;

    case UPWARD:
        this->position += this->up * this->movementSpeed * dt;
        break;

    case DOWNWARD:
        this->position -= this->up * this->movementSpeed * dt;
        break;

    default:
        break;
    }
}


void Camera::updateMouseInput(const float& dt, const double& offsetX, const double& offsetY)
{
    // Update Pitch, Yaw and Roll
    this->pitch -= static_cast<GLfloat>(offsetY) * this->sensitivity * dt;
    this->yaw += static_cast<GLfloat>(offsetX) * this->sensitivity * dt;

    if(this->pitch >= 80.f )
        this->pitch = 80.f;
    else if ( this->pitch < -80.f )
        this->pitch = -80.f;

    if (this->yaw > 360.f || this->yaw < -360.f)
        this->yaw = 0.f;
}