#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glfw3.h>

#include <vec4.hpp>
#include <mat4x4.hpp>

#include <iostream>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <optional>
#include <set>

#include "TutorialApp.h"

#define NDEBUG