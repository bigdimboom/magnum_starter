
#include <spdlog/spdlog.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColor.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/GlmIntegration/Integration.h>

#include "engine/overlay.h"

#include <Magnum/Math/Color.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/ArrayView.h>

#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData.h>

#include "engine/free_camera.h"
#include <SDL2/SDL.h>

#include "engine/debug_draw.h"
#include "engine/ImGuizmo.h"
#include <glm/ext.hpp>

namespace Magnum
{
namespace Examples
{

class CubeExample : public Platform::Application {
public:
	explicit CubeExample(const Arguments& arguments);

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

	bool _showDemoWindow = false;
	std::shared_ptr<graphics::Overlay> d_overlay;

	GL::Mesh _mesh;
	Shaders::Phong _shader;
	Color3 _color;

	glm::mat4 d_cubeModelMatrix = glm::mat4(1.0f);
	std::shared_ptr<graphics::FreeCamera> d_cam;
	std::shared_ptr<graphics::DebugDraw> d_dd;
};

CubeExample::CubeExample(const Arguments& arguments) :
	Platform::Application{ arguments, Configuration{}.setTitle("Magnum Triangle Example") }
{
	spdlog::set_level(spdlog::level::debug);
	spdlog::info("triangle");

	//GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	//GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

	///* Set up proper blending to be used by ImGui. There's a great chance
	//   you'll need this exact behavior for the rest of your scene. If not, set
	//   this only for the drawFrame() call. */
	//GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
	//							   GL::Renderer::BlendEquation::Add);
	//GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
	//							   GL::Renderer::BlendFunction::OneMinusSourceAlpha);

	d_overlay = std::make_shared<graphics::Overlay>(this);
	d_overlay->enableFPSCounter(true);
	d_overlay->add([this](graphics::Overlay&)
	{
		ImGui::Checkbox("test chdeck box", &_showDemoWindow);
		if (_showDemoWindow)
		{
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
			ImGui::ShowDemoWindow();
		}

		{
			//ImGui::Begin("transform edit");
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::BeginFrame();
			ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			ImGuizmo::Manipulate(glm::value_ptr(d_cam->view()), glm::value_ptr(d_cam->proj()),
								 ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
								 glm::value_ptr(d_cubeModelMatrix));
			//ImGui::End();
		}
	});

#if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
	/* Have some sane speed, please */
	setMinimalLoopPeriod(16);
#endif

	using namespace Math::Literals;


	Trade::MeshData cube = Primitives::cubeSolid();

	GL::Buffer vertices;
	vertices.setData(MeshTools::interleave(cube.positions3DAsArray(),
					 cube.normalsAsArray()));

	std::pair<Containers::Array<char>, MeshIndexType> compressed = MeshTools::compressIndices(cube.indicesAsArray());
	GL::Buffer indices;
	indices.setData(compressed.first);

	_mesh.setPrimitive(cube.primitive())
		.setCount(cube.indexCount())
		.addVertexBuffer(std::move(vertices), 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
		.setIndexBuffer(std::move(indices), 0, compressed.second);

	_color = Color3::fromHsv({ 35.0_degf, 1.0f, 1.0f });

	graphics::FreeCameraCreateInfo1 ci;
	ci.aspect_ratio = (float)windowSize().x() / (float)windowSize().y();
	ci.spawn_location = { 0.0f,0.0f, 10.0f };
	d_cam = std::make_shared<graphics::FreeCamera>(ci);

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
		dd::sphere(ooo, dd::colors::Bisque, 5.0f);
	});
}

void CubeExample::drawEvent() {

	GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
	GL::defaultFramebuffer.clearColor(Magnum::Color4(0, 0, 0, 0));

	Matrix4 _transformation(d_cam->view() * d_cubeModelMatrix);
	Matrix4 _projection(d_cam->proj());

	_shader.setLightPosition({ 0.0f, 5.0f, 8.0f })
		.setLightColor({ 1.0f, 1.0f, 1.0f })
		.setDiffuseColor(_color)
		.setAmbientColor(Color3::fromHsv({ _color.hue(), 1.0f, 0.3f }))
		.setTransformationMatrix(_transformation)
		.setNormalMatrix(_transformation.normalMatrix())
		.setProjectionMatrix(_projection)
		.draw(_mesh);

	d_dd->updateMVP(d_cam->viewProj());
	d_dd->render();

	d_overlay->render();

	swapBuffers();

	redraw();
}

void CubeExample::viewportEvent(ViewportEvent& event)
{
	GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });

	d_overlay->imGuiCtx().relayout(Vector2{ event.windowSize() } / event.dpiScaling(),
								   event.windowSize(), event.framebufferSize());

	d_dd->resize(event.windowSize().x(), event.windowSize().y());
}

void CubeExample::keyPressEvent(KeyEvent& event)
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

void CubeExample::keyReleaseEvent(KeyEvent& event)
{
	if (d_overlay->imGuiCtx().handleKeyReleaseEvent(event)) return;
}

void CubeExample::mousePressEvent(MouseEvent& event)
{
	if (d_overlay->imGuiCtx().handleMousePressEvent(event)) return;
}

void CubeExample::mouseReleaseEvent(MouseEvent& event)
{
	if (d_overlay->imGuiCtx().handleMouseReleaseEvent(event)) return;
}

void CubeExample::mouseMoveEvent(MouseMoveEvent& event)
{
	if (d_overlay->imGuiCtx().handleMouseMoveEvent(event)) return;

	if (event.buttons() & MouseMoveEvent::Button::Left)
	{
		d_cam->handleMouseMotion(event.relativePosition().x(), -event.relativePosition().y());
	}
}

void CubeExample::mouseScrollEvent(MouseScrollEvent& event)
{
	if (d_overlay->imGuiCtx().handleMouseScrollEvent(event)) {
		/* Prevent scrolling the page */
		event.setAccepted();
		return;
	}
}

void CubeExample::textInputEvent(TextInputEvent& event)
{
	if (d_overlay->imGuiCtx().handleTextInputEvent(event)) return;
}

}
}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CubeExample)