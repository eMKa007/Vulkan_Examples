#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glfw3.h>

#include <vec4.hpp>
#include <mat4x4.hpp>

#include <algorithm>
#include <iostream>
#include <optional>
#include <cstdint>
#include <fstream>
#include <chrono>

#include <vector>
#include <set>
#include <array>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/hash.hpp>

#include "Camera.h"

#define NDEBUG

#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
#define MAX_FRAMES_IN_FLIGHT 2
#define MODEL_PATH "Models/bunny.obj"