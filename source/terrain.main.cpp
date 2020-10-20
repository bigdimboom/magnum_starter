
#include <spdlog/spdlog.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColor.h>

#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/ArrayView.h>

#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData.h>

#include <SDL2/SDL.h>
#include <glm/ext.hpp>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/GlmIntegration/Integration.h>

#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Corrade/Containers/ArrayViewStl.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>


#include "engine/overlay.h"
#include "engine/free_camera.h"
#include "engine/debug_draw.h"
#include "engine/ImGuizmo.h"
#include "engine/fast_noise.h"

namespace Magnum
{

namespace Examples
{

class TerrainShader : public GL::AbstractShaderProgram
{
public:
	TerrainShader()
	{
		MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL450);

		const Utility::Resource rs{ "shader" };

		GL::Shader vert{ GL::Version::GL450, GL::Shader::Type::Vertex };
		GL::Shader frag{ GL::Version::GL450, GL::Shader::Type::Fragment };

		vert.addSource(rs.get("terrain.vert"));
		frag.addSource(rs.get("terrain.frag"));

		CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({ vert, frag }));

		attachShaders({ vert, frag });

		CORRADE_INTERNAL_ASSERT_OUTPUT(link());

		d_modelMatrix = uniformLocation("uModelMat");
		d_viewProjMatrix = uniformLocation("uCamViewProjMat");
		d_gridRez = uniformLocation("uGridRez");
		d_gridStepSize = uniformLocation("uGridStepSize");
		d_gridHeightBoost = uniformLocation("uGridHeightBoosts");
		d_camPosVec3 = uniformLocation("uCamPos");

		setModelMatrix(glm::mat4(1.0f));
		setViewProjectMatrix(glm::mat4(1.0f));
		setGridRez(1);
		setGridStepSize(1.0f);
		setGridElevationBoost(10.0f);
		setCamPos(glm::vec3(0.0f));

		setUniform(uniformLocation("elevationMap"), TextureUnit);
	}

	TerrainShader& setModelMatrix(const glm::mat4& model)
	{
		setUniform(d_modelMatrix, Matrix4x4(model));
		return *this;
	}

	TerrainShader& setViewProjectMatrix(const glm::mat4& view_proj)
	{
		setUniform(d_viewProjMatrix, Matrix4x4(view_proj));
		return *this;
	}

	TerrainShader& setCamPos(const glm::vec3& campos)
	{
		setUniform(d_camPosVec3, Vector3(campos));
		return *this;
	}

	TerrainShader& setGridRez(uint32_t rez)
	{
		setUniform(d_gridRez, int(rez));
		return *this;
	}

	TerrainShader& setGridStepSize(float size)
	{
		setUniform(d_gridStepSize, size);
		return *this;
	}

	TerrainShader& setGridElevationBoost(float scale)
	{
		setUniform(d_gridHeightBoost, scale);
		return *this;
	}

	TerrainShader& bindElevationTexture(GL::Texture2D& texture) {
		texture.bind(TextureUnit);
		return *this;
	}


private:
	Int d_modelMatrix = 0;
	Int d_viewProjMatrix = 0;
	Int d_camPosVec3 = 0;
	Int d_gridRez = 0;
	Int d_gridStepSize = 0;
	Int d_gridHeightBoost = 0;

	enum : Int { TextureUnit = 0 };
};

class Clipmap
{
public:
	Clipmap(uint32_t dim_n, uint32_t levels, float stepsize)
	{
		typedef GL::Attribute<0, Vector2> Position;

		size_t m = (dim_n + 1) / 4;

		{
			// init block
			std::vector<glm::vec2> blockV;
			std::vector<glm::uint16> blockI;
			blockV.resize(m * m);
			blockI.resize((m - 1) * 6 * (m - 1));

			for (size_t row = 0; row < m; ++row)
			{
				for (size_t col = 0; col < m; ++col)
				{
					blockV[row * m + col] = glm::vec2(col, row);
				}
			}

			for (size_t row = 0; row < m - 1; ++row)
			{
				for (size_t col = 0; col < m - 1; ++col)
				{
					size_t id = row * m + col;
					size_t id_dy = (row + 1) * m + col;
					size_t width = 6 * (m - 1);

					blockI[row * width + col * 6 + 0] = id;
					blockI[row * width + col * 6 + 1] = id_dy;
					blockI[row * width + col * 6 + 2] = id_dy + 1;

					blockI[row * width + col * 6 + 3] = id;
					blockI[row * width + col * 6 + 4] = id_dy + 1;
					blockI[row * width + col * 6 + 5] = id + 1;
				}
			}

			GL::Buffer blockVBuffer, blockIBuffer;
			blockVBuffer.setData(blockV);
			blockIBuffer.setData(blockI);

			d_blockMesh
				.addVertexBuffer(std::move(blockVBuffer), 0, Position{})
				.setIndexBuffer(std::move(blockIBuffer), 0, MeshIndexType::UnsignedShort)
				.setCount(blockI.size())
				.setPrimitive(MeshPrimitive::Triangles);
		}

		// init ring fix up
		{
			std::vector<glm::vec2> blockV;
			std::vector<glm::uint16> blockI;
			blockV.resize(m * 3);
			blockI.resize((3 - 1) * 6 * (m - 1));

			for (size_t row = 0; row < m; ++row)
			{
				for (size_t col = 0; col < 3; ++col)
				{
					blockV[row * 3 + col] = glm::vec2(col, row);
				}
			}

			for (size_t row = 0; row < m - 1; ++row)
			{
				for (size_t col = 0; col < 3 - 1; ++col)
				{
					size_t id = row * 3 + col;
					size_t id_dy = (row + 1) * 3 + col;
					size_t width = 6 * (3 - 1);

					blockI[row * width + col * 6 + 0] = id;
					blockI[row * width + col * 6 + 1] = id_dy;
					blockI[row * width + col * 6 + 2] = id_dy + 1;

					blockI[row * width + col * 6 + 3] = id;
					blockI[row * width + col * 6 + 4] = id_dy + 1;
					blockI[row * width + col * 6 + 5] = id + 1;
				}
			}

			GL::Buffer blockVBuffer, blockIBuffer;
			blockVBuffer.setData(blockV);
			blockIBuffer.setData(blockI);

			d_ringFixUpMesh
				.addVertexBuffer(std::move(blockVBuffer), 0, Position{})
				.setIndexBuffer(std::move(blockIBuffer), 0, MeshIndexType::UnsignedShort)
				.setCount(blockI.size())
				.setPrimitive(MeshPrimitive::Triangles);
		}


	}

	void draw()
	{

	}



private:
	GL::Mesh d_blockMesh;
	GL::Mesh d_ringFixUpMesh;


};


class TerrainExample : public Platform::Application {
public:
	explicit TerrainExample(const Arguments& arguments);

private:
	void drawEvent() override;

	void viewportEvent(ViewportEvent& event) override;

	void keyPressEvent(KeyEvent& event) override;
	void keyReleaseEvent(KeyEvent& event) override;

	void mousePressEvent(MouseEvent& event) override;
	void mouseReleaseEvent(MouseEvent& event) override;
	void mouseMoveEvent(MouseMoveEvent& event) override;
	void mouseScrollEvent(MouseScrollEvent& event) override;
	void textInputEvent(TextInputEvent& event) override;


	std::shared_ptr<graphics::Overlay> d_overlay;
	std::shared_ptr<graphics::FreeCamera> d_cam;
	std::shared_ptr<graphics::DebugDraw> d_dd;

	GL::Texture2D d_elevationMap;
	GL::Mesh d_terrainMesh;
	TerrainShader d_terrainShader;
};

TerrainExample::TerrainExample(const Arguments& arguments) :
	Platform::Application{ arguments, Configuration{}.setTitle("Terrain").setSize({1024, 768}) }
{
	spdlog::set_level(spdlog::level::debug);
	spdlog::info("terrain");

#if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
	/* Have some sane speed, please */
	setMinimalLoopPeriod(16);
#endif

	using namespace Math::Literals;

	// TODO: prepare terrain
	// Create and configure FastNoise object
	size_t dim = 512;
	std::vector<float> noiseData(dim* dim);

	FastNoiseLite noise;
	noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	noise.SetFrequency(0.01f);

	noise.SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
	noise.SetFractalOctaves(5);
	noise.SetFractalLacunarity(2.0f);
	noise.SetFractalGain(0.6f);
	noise.SetFractalWeightedStrength(0.0f);
	noise.SetFractalPingPongStrength(2.0f);

	noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_EuclideanSq);
	noise.SetCellularReturnType(FastNoiseLite::CellularReturnType::CellularReturnType_Distance);
	noise.SetCellularJitter(1.0);


	int index = 0;
	for (int y = 0; y < dim; y++)
	{
		for (int x = 0; x < dim; x++)
		{
			noiseData[index++] = noise.GetNoise((float)x, (float)y);
		}
	}

	int levels = Math::log2((int)dim) + 1;
	ImageView2D image(PixelFormat::R32F, { (int)dim, (int)dim }, noiseData);
	d_elevationMap
		.setMagnificationFilter(GL::SamplerFilter::Linear)
		.setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
		.setWrapping(GL::SamplerWrapping::ClampToEdge)
		.setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
		.setStorage(levels, GL::TextureFormat::R32F, { (int)dim, (int)dim })
		.setSubImage(0, {}, image)
		.generateMipmap();

	size_t meshres = 255;
	std::vector<uint32_t> indices((meshres - 1) * 6 * (meshres - 1));
	for (size_t row = 0; row < meshres - 1; ++row)
	{
		for (size_t col = 0; col < meshres - 1; ++col)
		{
			size_t id = row * meshres + col;
			size_t id_dy = (row + 1) * meshres + col;
			size_t width = 6 * (meshres - 1);

			indices[row * width + col * 6 + 0] = id;
			indices[row * width + col * 6 + 1] = id_dy;
			indices[row * width + col * 6 + 2] = id_dy + 1;

			indices[row * width + col * 6 + 3] = id;
			indices[row * width + col * 6 + 4] = id_dy + 1;
			indices[row * width + col * 6 + 5] = id + 1;
		}
	}

	GL::Buffer indexBuffer;
	indexBuffer.setData(indices);
	d_terrainMesh
		.setIndexBuffer(std::move(indexBuffer), 0, MeshIndexType::UnsignedInt)
		.setPrimitive(MeshPrimitive::Triangles)
		.setCount(indices.size());
	d_terrainShader
		.setGridRez(meshres)
		.setGridStepSize(1.0)
		.setGridElevationBoost(10.0f);

	graphics::FreeCameraCreateInfo1 ci;
	ci.near = 0.1;
	ci.aspect_ratio = (float)windowSize().x() / (float)windowSize().y();
	ci.spawn_location = { 0.0f,0.0f, 10.0f };
	d_cam = std::make_shared<graphics::FreeCamera>(ci);

	d_overlay = std::make_shared<graphics::Overlay>(this);
	d_overlay->enableFPSCounter(true);
	d_overlay->add([this, dim](graphics::Overlay& overlay)
	{
		ImGuiIntegration::image(d_elevationMap, { (float)dim, (float)dim });
	});

	d_dd = std::make_shared<graphics::DebugDraw>(windowSize().x(), windowSize().y());
	d_dd->registerDraws([]() {

		const ddMat4x4 transform = { // The identity matrix
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		ddVec3_In ooo = { 0.0f,0.0f,0.0f };
		dd::axisTriad(transform, 1.0f, 10.0f);
	});

}

void TerrainExample::drawEvent() {

	GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
	GL::defaultFramebuffer.clearColor(Magnum::Color4(0, 0, 0, 0));

	// TODO: render terrain
	GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Line);
	d_terrainShader
		.setViewProjectMatrix(d_cam->viewProj())
		.bindElevationTexture(d_elevationMap)
		.setCamPos(d_cam->pos())
		.draw(d_terrainMesh);
	GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Fill);

	d_dd->updateMVP(d_cam->viewProj());
	d_dd->render();
	d_overlay->render();

	swapBuffers();
	redraw();
}

void TerrainExample::viewportEvent(ViewportEvent& event)
{
	GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });

	d_overlay->imGuiCtx().relayout(Vector2{ event.windowSize() } / event.dpiScaling(),
								   event.windowSize(), event.framebufferSize());

	d_dd->resize(event.windowSize().x(), event.windowSize().y());
}

void TerrainExample::keyPressEvent(KeyEvent& event)
{
	if (d_overlay->imGuiCtx().handleKeyPressEvent(event)) return;

	if (event.keyName() == "Escape")
	{
		event.setAccepted();
		exit();
		return;
	}

	if (!this->isTextInputActive() /*&& !d_fileDialog.IsOpened()*/)
	{
		d_cam->handleKeyPressed(static_cast<char>(event.key()));
	}
}

void TerrainExample::keyReleaseEvent(KeyEvent& event)
{
	if (d_overlay->imGuiCtx().handleKeyReleaseEvent(event)) return;
}

void TerrainExample::mousePressEvent(MouseEvent& event)
{
	if (d_overlay->imGuiCtx().handleMousePressEvent(event)) return;
}

void TerrainExample::mouseReleaseEvent(MouseEvent& event)
{
	if (d_overlay->imGuiCtx().handleMouseReleaseEvent(event)) return;
}

void TerrainExample::mouseMoveEvent(MouseMoveEvent& event)
{
	if (d_overlay->imGuiCtx().handleMouseMoveEvent(event)) return;

	if (event.buttons() & MouseMoveEvent::Button::Left)
	{
		d_cam->handleMouseMotion(event.relativePosition().x(), -event.relativePosition().y());
	}
}

void TerrainExample::mouseScrollEvent(MouseScrollEvent& event)
{
	if (d_overlay->imGuiCtx().handleMouseScrollEvent(event)) {
		/* Prevent scrolling the page */
		event.setAccepted();
		return;
	}
}

void TerrainExample::textInputEvent(TextInputEvent& event)
{
	if (d_overlay->imGuiCtx().handleTextInputEvent(event)) return;
}

}
}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TerrainExample)