//
// Created by bob on 7/5/20.
//

#pragma once

#include "debug_draw.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <Magnum/GlmIntegration/Integration.h>
#include <Magnum/Platform/GLContext.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Platform/Sdl2Application.h>

namespace graphics
{

class DrawProgram;
class TextProgram;

class DebugDraw : public dd::RenderInterface
{
public:

	enum DepthConfig
	{
		DONT_CARE,
		ENABLED,
		DISABLED
	};

	// MEMBERS
	explicit DebugDraw(int w, int h, Magnum::GL::Context& ctx= Magnum::Platform::GLContext::current());

	~DebugDraw() override;

	void depth(DepthConfig enabled);
	void updateMVP(const glm::mat4& mvp);
	void render();
	void clearDraws();
	void registerDraws(std::function<void()> cb);
	void resize(int w, int h);

private:
	// IMPL.
	dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void* pixels) override;
	void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override;
	void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override;
	void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override;
	void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex) override;

	// HELPERS
	static uint32_t handleToGL(dd::GlyphTextureHandle handle);
	static dd::GlyphTextureHandle GLToHandle(uint32_t id);
	static void setGLStates();

private:

	Magnum::GL::Context& d_ctx;

	// VARS
	int d_w = 0, d_h = 0;
	glm::mat4 d_mvp = glm::mat4(1.0f);
	std::vector<std::function<void()>> d_draws;

	std::unique_ptr<DrawProgram> d_linePointProgram;
	std::unique_ptr<TextProgram> d_textProgram;

	std::unique_ptr<Magnum::GL::Texture2D> d_glyphText = nullptr;

	std::unique_ptr<Magnum::GL::Mesh> d_linePoint = nullptr;
	std::unique_ptr<Magnum::GL::Mesh> d_text = nullptr;
	std::unique_ptr<Magnum::GL::Buffer> d_linePointVBO = nullptr;
	std::unique_ptr<Magnum::GL::Buffer> d_textVBO = nullptr;

	DepthConfig d_depth = DONT_CARE;
};

} // end namespace graphics