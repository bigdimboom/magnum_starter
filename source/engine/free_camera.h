#pragma once
#include "camera.h"
#include <array>

namespace graphics
{

struct FreeCameraCreateInfo1
{
	float fovy = 45.0f;
	float aspect_ratio = 4.0f / 3.0f;
	float near = 1.0f;
	float far = 5000.0f;
	float pitch_angle = 0.0f; // in degree
	float yaw_angle = 0.0f; // in degree
	glm::vec3 spawn_location = glm::vec3(0.0f, 0.0f, 0.0f); // initial location
	float movementSpeed = 1.0f;
};

class FreeCamera
{
public:
	FreeCamera(const FreeCameraCreateInfo1& ci);
	~FreeCamera();
	FreeCamera(const FreeCamera&) = delete;
	FreeCamera(FreeCamera&&) = delete;
	void operator=(const FreeCamera&) = delete;
	void operator=(FreeCamera&&) = delete;

	void setView(const glm::mat4& view);
	void setProj(const glm::mat4& proj, ProjType type);

	[[nodiscard]] const glm::mat4& view() const;
	[[nodiscard]] const glm::mat4& proj() const;
	[[nodiscard]] const glm::mat4& viewInv() const;
	[[nodiscard]] const glm::mat4& projInv() const;
	[[nodiscard]] const glm::mat4& viewProj() const;
	[[nodiscard]] const glm::mat4& viewProjInv() const;
	[[nodiscard]] ProjType projType() const;

	[[nodiscard]] const glm::vec3& pos() const;
	[[nodiscard]] const glm::vec3& vDir() const;
	[[nodiscard]] const glm::vec3& up() const;
	[[nodiscard]] const glm::vec3& right() const;

	void setMovementSpeed(float speed);
	void setPitchSpeed(float speed);
	void setYawSpeed(float speed);

	[[nodiscard]] float moveSpeed() const;

	/*void handleKeyReleased(char key);*/
	void handleKeyPressed(char key);
	void handleMouseMotion(float deltaX, float deltaY);
	void handleResizeEvent(int w, int h);

private:

	ProjType d_projType = ProjType::Perspective;
	float d_moveSpeed = 1.0f;

	struct CameraMatrices
	{
		Camera basic;
		glm::mat4 view_proj;
		glm::mat4 proj_inv;
		glm::mat4 view_inv;
		glm::mat4 view_proj_inv;

		void update()
		{
			view_proj = basic.proj * basic.view;
			view_proj_inv = glm::inverse(view_proj);
		}
	}d_matrices;

	struct Basis
	{
		glm::vec3 eye = glm::vec3(0, 0, 0);
		glm::vec3 right = glm::vec3(1, 0, 0);
		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 dir = glm::vec3(0, 0, -1);
	}d_basis;

	struct EulerAngles
	{
		float pitch = 0.0f;
		float yaw = 0.0f;
		float roll = 0.0f;
		float pitch_speed = 0.08f;
		float yaw_speed = 0.08f;
	}d_anglesDeg;

	struct ClipSpace
	{
		float nearVal = 1.0f;
		float farVal = 5000.0f;
	}d_clip;

	// planes for frustrum
	//std::array<glm::vec4, 6> d_frustrumPlanes{};

	// HELPERS
	void extractBasisFromViewMatrix();
	glm::mat4 buildView(const glm::vec3& r, const glm::vec3& u, const glm::vec3& v, const glm::vec3& eye);
	void extractRange(const glm::mat4& perspectiveProj, float& n, float& f);

};

} // end namespace graphics