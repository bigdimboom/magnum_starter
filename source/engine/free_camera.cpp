#include "free_camera.h"
#include <glm/ext.hpp>
#include <spdlog/spdlog.h>
#include <glm/gtx/string_cast.hpp>

namespace graphics
{

FreeCamera::FreeCamera(const FreeCameraCreateInfo1& ci)
{
	d_moveSpeed = ci.movementSpeed;
	d_basis.eye = ci.spawn_location;
	d_anglesDeg.pitch = ci.pitch_angle;
	d_anglesDeg.yaw = ci.yaw_angle;

	d_projType = ProjType::Perspective;
	d_matrices.basic.proj = glm::perspective(glm::radians(ci.fovy), ci.aspect_ratio, ci.near, ci.far);
	d_matrices.proj_inv = glm::inverse(d_matrices.basic.proj);
	d_clip.nearVal = ci.near;
	d_clip.farVal = ci.far;

	//spdlog::info("{}", glm::to_string(d_matrices.basic.proj));

	auto trans = glm::translate(glm::mat4(1.0), -d_basis.eye);
	auto yaw = glm::rotate(glm::mat4(1.0f), glm::radians(d_anglesDeg.yaw), glm::vec3(0, 1, 0));
	auto pitch = glm::rotate(glm::mat4(1.0f), glm::radians(d_anglesDeg.pitch), glm::vec3(1, 0, 0));
	auto roll = glm::rotate(glm::mat4(1.0f), glm::radians(d_anglesDeg.roll), glm::vec3(0, 0, 1));
	setView(roll * pitch * yaw * trans);
}

FreeCamera::~FreeCamera() = default;

void FreeCamera::setView(const glm::mat4& view)
{
	d_matrices.basic.view = view;
	d_matrices.view_inv = glm::affineInverse(view);
	d_matrices.update();
	extractBasisFromViewMatrix();

	//spdlog::info("{}", glm::to_string(d_matrices.basic.view));
}

void FreeCamera::setProj(const glm::mat4& proj, ProjType type)
{
	d_projType = type;
	d_matrices.basic.proj = proj;
	d_matrices.proj_inv = glm::inverse(proj);
	d_matrices.update();
	extractRange(d_matrices.basic.proj, d_clip.nearVal, d_clip.farVal);
}

const glm::mat4& FreeCamera::view() const
{
	return d_matrices.basic.view;
}

const glm::mat4& FreeCamera::proj() const
{
	return d_matrices.basic.proj;
}

const glm::mat4& FreeCamera::viewInv() const
{
	return d_matrices.view_inv;
}

const glm::mat4& FreeCamera::projInv() const
{
	return d_matrices.proj_inv;
}

const glm::mat4& FreeCamera::viewProj() const
{
	return d_matrices.view_proj;
}

const glm::mat4& FreeCamera::viewProjInv() const
{
	return d_matrices.view_proj_inv;
}

ProjType FreeCamera::projType() const
{
	return d_projType;
}

const glm::vec3& FreeCamera::pos() const
{
	return d_basis.eye;
}

const glm::vec3& FreeCamera::vDir() const
{
	return d_basis.dir;
}

const glm::vec3& FreeCamera::up() const
{
	return d_basis.up;
}

const glm::vec3& FreeCamera::right() const
{
	return d_basis.right;
}

void FreeCamera::setMovementSpeed(float speed)
{
	d_moveSpeed = speed;
}

void FreeCamera::setPitchSpeed(float speed)
{
	d_anglesDeg.pitch_speed = speed;
}

void FreeCamera::setYawSpeed(float speed)
{
	d_anglesDeg.yaw = speed;
}

float FreeCamera::moveSpeed() const
{
	return d_moveSpeed;
}

void FreeCamera::handleKeyPressed(char key)
{
	auto& eye = d_basis.eye;
	const auto& vdir = this->vDir();
	const auto& rdir = this->right();

	switch (key)
	{
	case 'w':
		eye += vdir * d_moveSpeed;
		break;
	case 's':
		eye -= vdir * d_moveSpeed;
		break;
	case 'a':
		eye -= rdir * d_moveSpeed;
		break;
	case 'd':
		eye += rdir * d_moveSpeed;
		break;
	default:
		return;
	}

	setView(buildView(d_basis.right, d_basis.up, d_basis.dir, d_basis.eye));
}

void FreeCamera::handleMouseMotion(float deltaX, float deltaY)
{
	const float PITCH_MAX = 89.9f;
	const float PITCH_MIN = -89.9f;

	d_anglesDeg.yaw += d_anglesDeg.yaw_speed * static_cast<float>(deltaX);
	d_anglesDeg.pitch += d_anglesDeg.pitch_speed * static_cast<float>(-deltaY);
	d_anglesDeg.pitch = glm::clamp(d_anglesDeg.pitch, PITCH_MIN, PITCH_MAX);

	auto trans = glm::translate(glm::mat4(1.0), -d_basis.eye);
	auto yaw = glm::rotate(glm::mat4(1.0f), glm::radians(d_anglesDeg.yaw), glm::vec3(0, 1, 0));
	auto pitch = glm::rotate(glm::mat4(1.0f), glm::radians(d_anglesDeg.pitch), glm::vec3(1, 0, 0));
	auto roll = glm::rotate(glm::mat4(1.0f), glm::radians(d_anglesDeg.roll), glm::vec3(0, 0, 1));
	setView(roll * pitch * yaw * trans);

	// TODO: update frustrum planes
}

void FreeCamera::handleResizeEvent(int w, int h)
{
	if (projType() == ProjType::Perspective)
	{
		auto& proj = d_matrices.basic.proj;

		float fov, n, f, asp;
		fov = 2.0f * glm::atan(1.0f / proj[1][1]);
		asp = (1.0f / proj[0][0]) / (1.0f / proj[1][1]);
		n = -0.5f * (proj[3][2] - (proj[2][2] + 1.0f) / (proj[2][2] - 1.0f));
		f = n * (proj[2][2] - 1.0f) / (proj[2][2] + 1.0f);

		asp = static_cast<float>(w) / static_cast<float>(h);
		setProj(glm::perspective(fov, asp, n, f), ProjType::Perspective);
		return;
	}
	assert(0);
}

// HELPERS
void FreeCamera::extractBasisFromViewMatrix()
{
	const auto& v = d_matrices.basic.view;
	d_basis.right = glm::normalize(glm::vec3(v[0][0], v[1][0], v[2][0]));
	d_basis.up = glm::normalize(glm::vec3(v[0][1], v[1][1], v[2][1]));
	d_basis.dir = -glm::normalize(glm::vec3(v[0][2], v[1][2], v[2][2]));

	//auto rotM = glm::mat4(glm::mat3(v));
	//auto tM = glm::affineInverse(rotM) * v;
	//d_basis.eye = tM[3];
}

glm::mat4 FreeCamera::buildView(const glm::vec3& r, const glm::vec3& u, const glm::vec3& v, const glm::vec3& eye)
{
	glm::mat4 rot(1.0f);
	glm::mat4 t = glm::translate(glm::mat4(1.0f), -eye);

	rot[0][0] = r.x, rot[1][0] = r.y, rot[2][0] = r.z;
	rot[0][1] = u.x, rot[1][1] = u.y, rot[2][1] = u.z;
	rot[0][2] = -v.x, rot[1][2] = -v.y, rot[2][2] = -v.z;

	return rot * t;
}

void FreeCamera::extractRange(const glm::mat4& proj, float& n, float& f)
{
	float fovDeg = glm::degrees(2.0f * glm::atan(1.0f / proj[1][1]));
	float aspectRatio = (1.0f / proj[0][0]) / (1.0f / proj[1][1]);
	n = -0.5f * (proj[3][2] - (proj[2][2] + 1.0f) / (proj[2][2] - 1.0f));
	f = n * (proj[2][2] - 1.0f) / (proj[2][2] + 1.0f);
}

} // end namespace graphics