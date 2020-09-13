//
// Created by bob on 7/5/20.
//

#include "debug_draw.h"

#define DEBUG_DRAW_IMPLEMENTATION

#include "debug_draw.hpp"

#include <spdlog/spdlog.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Context.h>
#include <Corrade/Containers/Reference.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/BufferImage.h>
#include <Magnum/GL/PixelFormat.h>
#include <Corrade/Containers/ArrayView.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Mesh.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/Platform/Sdl2Application.h>


namespace graphics
{

using namespace Magnum;

class DrawProgram : public GL::AbstractShaderProgram
{
public:
	DrawProgram()
	{
		spdlog::info("DDRenderer: initializing Point/Line Rendering...");
		MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);
		GL::Shader vs(GL::Version::GL330, GL::Shader::Type::Vertex);
		GL::Shader fs(GL::Version::GL330, GL::Shader::Type::Fragment);
		vs.addSource(vsSource());
		fs.addSource(fsSource());
		CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({ vs, fs }));
		attachShaders({ vs, fs });
		CORRADE_INTERNAL_ASSERT_OUTPUT(link());

		d_mvpLoc = uniformLocation("u_MvpMatrix");
	}

	~DrawProgram() override = default;

	void setUniformMVP(const glm::mat4& mvp)
	{
		setUniform(d_mvpLoc, Magnum::Matrix4x4(mvp));
	}

private:
	Int d_mvpLoc = 0;

	static const char* vsSource()
	{
		static auto str = "layout(location = 0)in vec3 in_Position;\n"
						  "layout(location = 1)in vec4 in_ColorPointSize;\n"
						  "\n"
						  "out vec4 v_Color;\n"
						  "uniform mat4 u_MvpMatrix;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position  = u_MvpMatrix * vec4(in_Position, 1.0);\n"
						  "    gl_PointSize = in_ColorPointSize.w;\n"
						  "    v_Color      = vec4(in_ColorPointSize.xyz, 1.0);\n"
						  "}\n";
		return str;
	}

	static const char* fsSource()
	{
		static auto str = "in  vec4 v_Color;\n"
						  "out vec4 out_FragColor;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    out_FragColor = v_Color;\n"
						  "}\n";
		return str;
	}
};

class TextProgram : public GL::AbstractShaderProgram
{
public:
	TextProgram()
	{
		spdlog::info("DDRenderer: initializing Text Rendering...");
		MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);
		GL::Shader vs(GL::Version::GL330, GL::Shader::Type::Vertex);
		GL::Shader fs(GL::Version::GL330, GL::Shader::Type::Fragment);
		vs.addSource(vsSource());
		fs.addSource(fsSource());
		CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({ vs, fs }));
		attachShaders({ vs, fs });
		CORRADE_INTERNAL_ASSERT_OUTPUT(link());

		setUniform(uniformLocation("u_glyphTexture"), d_textureUnit);
		u_scrn_dim_loc = uniformLocation("u_screenDimensions");
	}

	~TextProgram() override = default;

	void updateScreenRez(int w, int h)
	{
		setUniform(u_scrn_dim_loc, Magnum::Vector2(w, h));
	}

	void bindGlyphTexture(Magnum::GL::Texture2D& tex) const
	{
		tex.bind(d_textureUnit);
	}

private:
	Int d_textureUnit = 0;
	Int u_scrn_dim_loc = 0;

	static const char* vsSource()
	{
		static auto str = "layout(location = 0)in vec2 in_Position;\n"
						  "layout(location = 1)in vec2 in_TexCoords;\n"
						  "layout(location = 2)in vec3 in_Color;\n"
						  "\n"
						  "uniform vec2 u_screenDimensions;\n"
						  "\n"
						  "out vec2 v_TexCoords;\n"
						  "out vec4 v_Color;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    // Map to normalized clip coordinates:\n"
						  "    float x = ((2.0 * (in_Position.x - 0.5)) / u_screenDimensions.x) - 1.0;\n"
						  "    float y = 1.0 - ((2.0 * (in_Position.y - 0.5)) / u_screenDimensions.y);\n"
						  "\n"
						  "    gl_Position = vec4(x, y, 0.0, 1.0);\n"
						  "    v_TexCoords = in_TexCoords;\n"
						  "    v_Color     = vec4(in_Color, 1.0);\n"
						  "}\n";
		return str;
	}

	static const char* fsSource()
	{
		static auto str = "in vec2 v_TexCoords;\n"
						  "in vec4 v_Color;\n"
						  "\n"
						  "uniform sampler2D u_glyphTexture;\n"
						  "out vec4 out_FragColor;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    out_FragColor = v_Color;\n"
						  "    out_FragColor.a = texture(u_glyphTexture, v_TexCoords).r;\n"
						  "}\n";
		return str;
	}
};

// IMPL.
dd::GlyphTextureHandle DebugDraw::createGlyphTexture(int width, int height, const void* pixels)
{
	using namespace Magnum;
	d_glyphText = std::make_unique<GL::Texture2D>();

	Containers::ArrayView<const void> data(pixels, width * height);

	GL::BufferImage2D image(
			GL::PixelFormat::Red,
			GL::PixelType::UnsignedByte,
			{ width, height }, data, GL::BufferUsage::StaticDraw);

	(*d_glyphText).setWrapping(GL::SamplerWrapping::MirrorClampToEdge)
			.setMagnificationFilter(GL::SamplerFilter::Linear)
			.setMinificationFilter(GL::SamplerFilter::Linear)
			.setStorage(1, GL::TextureFormat::R8, { width, height })
			.setSubImage(0, {}, image);

	return GLToHandle(d_glyphText->id());
}

void DebugDraw::destroyGlyphTexture(dd::GlyphTextureHandle glyphTex)
{
	assert(d_glyphText->id() == handleToGL(glyphTex));
	d_glyphText = nullptr;
}

void DebugDraw::drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled)
{
	using namespace Magnum;

	if ( (depthEnabled && d_depth == DepthConfig::DONT_CARE) || d_depth == DepthConfig::ENABLED)
	{
		GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	}
	else if((!depthEnabled && d_depth == DepthConfig::DONT_CARE) || d_depth == DepthConfig::DISABLED)
	{
		GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
	}

	d_linePointProgram->setUniformMVP(d_mvp);
	d_linePointVBO->setSubData(0, Magnum::Containers::ArrayView<const dd::DrawVertex>(points, count));
	d_linePoint->setPrimitive(MeshPrimitive::Points).setCount(count);
	d_linePointProgram->draw(*d_linePoint);
}

void DebugDraw::drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled)
{
	using namespace Magnum;

	if ( (depthEnabled && d_depth == DepthConfig::DONT_CARE) || d_depth == DepthConfig::ENABLED)
	{
		GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	}
	else if((!depthEnabled && d_depth == DepthConfig::DONT_CARE) || d_depth == DepthConfig::DISABLED)
	{
		GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
	}

	d_linePointProgram->setUniformMVP(d_mvp);
	d_linePointVBO->setSubData(0, Magnum::Containers::ArrayView<const dd::DrawVertex>(lines, count));
	d_linePoint->setPrimitive(MeshPrimitive::Lines).setCount(count);
	d_linePointProgram->draw(*d_linePoint);
}

void DebugDraw::drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex)
{
	using namespace Magnum;

	d_textProgram->updateScreenRez(d_w, d_h);
	d_textProgram->bindGlyphTexture(*d_glyphText);
	assert(d_glyphText->id() == handleToGL(glyphTex));


	GL::Renderer::setBlendFunction(
			GL::Renderer::BlendFunction::SourceAlpha,
			GL::Renderer::BlendFunction::OneMinusSourceAlpha);
	GL::Renderer::enable(GL::Renderer::Feature::Blending);
	GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

	Containers::ArrayView<const dd::DrawVertex> data(glyphs, count);
	d_textVBO->setSubData(0, data);
	d_text->setPrimitive(GL::MeshPrimitive::Triangles).setCount(count);
	d_textProgram->draw(*d_text);

	GL::Renderer::disable(GL::Renderer::Feature::Blending);
	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
}

// HELPERS
uint32_t DebugDraw::handleToGL(dd::GlyphTextureHandle handle)
{
	const auto temp = reinterpret_cast<std::size_t>(handle);
	return static_cast<GLuint>(temp);
}

dd::GlyphTextureHandle DebugDraw::GLToHandle(uint32_t id)
{
	const auto temp = static_cast<std::size_t>(id);
	return reinterpret_cast<dd::GlyphTextureHandle>(temp);
}

void DebugDraw::setGLStates()
{
//	GL::Renderer::setBlendFunction(
//			GL::Renderer::BlendFunction::SourceAlpha,
//			GL::Renderer::BlendFunction::OneMinusSourceAlpha);
	GL::Renderer::enable(GL::Renderer::Feature::ProgramPointSize);
	GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

// MEMBERS
DebugDraw::DebugDraw(int w, int h, Magnum::GL::Context& ctx)
		: d_ctx(ctx)
		, d_w(w), d_h(h)
{
	auto& curr = Magnum::Platform::GLContext::current();

	spdlog::info("GL_VENDOR{}", curr.vendorString());
	spdlog::info("GL_RENDERER{}", curr.rendererString());
	spdlog::info("GL_VERSION{}", curr.versionString());
	spdlog::info("GLSL_VERSION{}", curr.shadingLanguageVersionString());
	spdlog::info("DDRender: InterfaceCoreGL Initializing ...");

	setGLStates();

	d_linePointProgram = std::make_unique<DrawProgram>();
	d_textProgram = std::make_unique<TextProgram>();

	d_linePoint = std::make_unique<GL::Mesh>();
	d_text = std::make_unique<GL::Mesh>();

	d_linePointVBO = std::make_unique<GL::Buffer>();
	d_textVBO = std::make_unique<GL::Buffer>();

	{
		std::size_t offset = 0;
		d_linePointVBO->setData({ nullptr, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex) },
				GL::BufferUsage::StreamDraw);
		d_linePoint->addVertexBuffer(*d_linePointVBO, offset, GL::Attribute<0, Vector3>(sizeof(dd::DrawVertex)));
		offset += sizeof(float) * 3;
		d_linePoint->addVertexBuffer(*d_linePointVBO, offset, GL::Attribute<1, Vector4>(sizeof(dd::DrawVertex)));
	}

	{
		d_textVBO->setData({ nullptr, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex) },
				GL::BufferUsage::StreamDraw);
		std::size_t offset = 0;
		d_text->addVertexBuffer(*d_textVBO, offset, GL::Attribute<0, Vector2>(sizeof(dd::DrawVertex)));
		offset += sizeof(float) * 2;
		d_text->addVertexBuffer(*d_textVBO, offset, GL::Attribute<1, Vector2>(sizeof(dd::DrawVertex)));
		offset += sizeof(float) * 2;
		d_text->addVertexBuffer(*d_textVBO, offset, GL::Attribute<2, Vector4>(sizeof(dd::DrawVertex)));
	}

	// finally
	dd::initialize(this);
}

void DebugDraw::updateMVP(const glm::mat4& mvp)
{
	d_mvp = mvp;
}

DebugDraw::~DebugDraw()
{
	spdlog::info("DDRender: InterfaceCoreGL Shutting down");
	dd::shutdown();
}

void DebugDraw::render()
{
	setGLStates();
	std::for_each(d_draws.begin(), d_draws.end(), [](std::function<void()> cb)
	{ cb(); });
	dd::flush();
}

void DebugDraw::registerDraws(std::function<void()> cb)
{
	assert(cb);
	d_draws.push_back(cb);
}

void DebugDraw::clearDraws()
{
	d_draws.clear();
}

void DebugDraw::resize(int w, int h)
{
	d_w = w, d_h = h;
}

void DebugDraw::depth(DepthConfig enabled)
{
	d_depth = enabled;
}

} // end namespace graphics