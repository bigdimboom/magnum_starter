//
// Created by bob on 7/21/20.
//

#pragma once

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Widgets.h>
#include <functional>

namespace graphics
{

class Overlay
{
public:

	using OverlayCB = std::function<void(Overlay&)>;

	explicit Overlay(Magnum::Platform::Application* app);
	~Overlay() = default;
	Overlay(const Overlay&) = delete;
	Overlay(Overlay&&) = delete;
	void operator=(const Overlay&) = delete;
	void operator=(Overlay&&) = delete;
	void add(const OverlayCB& cb);
	void render();
	void clearAll();
	void enableFPSCounter(bool enable);

	// internal access
	Magnum::ImGuiIntegration::Context& imGuiCtx();

private:
	Magnum::Platform::Application* d_app = nullptr;
	Magnum::ImGuiIntegration::Context d_imgui{ Magnum::NoCreate };
	std::vector<OverlayCB> d_functions;

	struct State
	{
		bool enable_fps = false;

	}d_state;

	// HELPERS
	void printFPS();

};

}
