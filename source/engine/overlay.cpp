//
// Created by bob on 7/21/20.
//

#include "overlay.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <algorithm>

graphics::Overlay::Overlay(Magnum::Platform::Application* app)
	: d_app(app)
{
	assert(d_app);
	using namespace Magnum;
	d_imgui = ImGuiIntegration::Context(
		Vector2{ d_app->windowSize() } / d_app->dpiScaling(),
		d_app->windowSize(),
		d_app->framebufferSize());

#if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
	/* Have some sane speed, please */
	d_app->setMinimalLoopPeriod(16);
#endif
}

Magnum::ImGuiIntegration::Context& graphics::Overlay::imGuiCtx()
{
	return d_imgui;
}

void graphics::Overlay::add(const graphics::Overlay::OverlayCB& cb)
{
	d_functions.push_back(cb);
}

void graphics::Overlay::render()
{
	assert(d_app);

	using namespace Magnum;

	d_imgui.newFrame();

	/* Enable text input, if needed */
	if (ImGui::GetIO().WantTextInput && !d_app->isTextInputActive())
		d_app->startTextInput();
	else if (!ImGui::GetIO().WantTextInput && d_app->isTextInputActive())
		d_app->stopTextInput();

	std::for_each(d_functions.begin(), d_functions.end(), [this](const OverlayCB& cb)
	{ cb(*this); });

	if (d_state.enable_fps)
	{
		printFPS();
	}

	d_imgui.updateApplicationCursor(*d_app);

	GL::Renderer::setBlendEquation(
		GL::Renderer::BlendEquation::Add,
		GL::Renderer::BlendEquation::Add);
	GL::Renderer::setBlendFunction(
		GL::Renderer::BlendFunction::SourceAlpha,
		GL::Renderer::BlendFunction::OneMinusSourceAlpha);

	GL::Renderer::enable(GL::Renderer::Feature::Blending);
	GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
	GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
	GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

	d_imgui.drawFrame();

	GL::Renderer::disable(GL::Renderer::Feature::Blending);
	GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
	GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

}

void graphics::Overlay::clearAll()
{
	d_functions.clear();
}

void graphics::Overlay::enableFPSCounter(bool enable)
{
	d_state.enable_fps = enable;
}

void graphics::Overlay::printFPS()
{
	const glm::vec4 color = { 1.0, 1.0, 1.0, 1.0 };
	const glm::vec2 pos = { 0.0f, 0.0f };
	const ImGuiWindowFlags fpsWindowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar;

	if (!ImGui::Begin("fps window", nullptr, fpsWindowFlags))
	{
		ImGui::End();
		return;
	}

	ImGui::SetWindowPos(*(reinterpret_cast<const ImVec2*>(&pos)));
	ImGui::TextColored(*(reinterpret_cast<const ImVec4*>(&color)),
					   "Average %.3f ms/frame (%.1f FPS)",
					   static_cast<double>(1000.0f / ImGui::GetIO().Framerate),
					   static_cast<double>(ImGui::GetIO().Framerate));

	ImGui::End();
}
