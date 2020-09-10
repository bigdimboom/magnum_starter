#pragma once
#include <glm/glm.hpp>

namespace graphics
{

enum class ProjType
{
	Perspective,
	Orthogonal
};

struct Camera
{
	glm::mat4 view;
	glm::mat4 proj;
	Camera() : view(1.0f), proj(1.0f) {}
	Camera(const glm::mat4& view, const glm::mat4& proj) : view(view), proj(proj) {}
};

} // end namespace graphics