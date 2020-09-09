
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

namespace Magnum
{
	namespace Examples
	{

		class TriangleExample : public Platform::Application {
		public:
			explicit TriangleExample(const Arguments& arguments);

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

			GL::Mesh _mesh;
			Shaders::VertexColor2D _shader;
			bool _showDemoWindow = false;

			std::shared_ptr<graphics::Overlay> d_overlay;

		};

		TriangleExample::TriangleExample(const Arguments& arguments) :
			Platform::Application{ arguments, Configuration{}.setTitle("Magnum Triangle Example") }
		{
			spdlog::set_level(spdlog::level::debug);
			spdlog::info("triangle");

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
			});

			/* Set up proper blending to be used by ImGui. There's a great chance
			   you'll need this exact behavior for the rest of your scene. If not, set
			   this only for the drawFrame() call. */
			GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
				GL::Renderer::BlendEquation::Add);
			GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
				GL::Renderer::BlendFunction::OneMinusSourceAlpha);

#if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
			/* Have some sane speed, please */
			setMinimalLoopPeriod(16);
#endif

			using namespace Math::Literals;

			// glm test
			glm::vec3 a{ 1.0f, 2.0f, 3.0f };
			Vector3 b(a);
			auto c = Matrix4::rotation(35.0_degf, Vector3(glm::normalize(a)));

			struct TriangleVertex {
				Vector2 position;
				Color3 color;
			};
			const TriangleVertex data[]{
				{{-0.5f, -0.5f}, 0xff0000_rgbf},    /* Left vertex, red color */
				{{ 0.5f, -0.5f}, 0x00ff00_rgbf},    /* Right vertex, green color */
				{{ 0.0f,  0.5f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
			};

			GL::Buffer buffer;
			buffer.setData(data);

			_mesh.setCount(3)
				.addVertexBuffer(std::move(buffer), 0,
					Shaders::VertexColor2D::Position{},
					Shaders::VertexColor2D::Color3{});
		}

		void TriangleExample::drawEvent() {
			GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

			_shader.draw(_mesh);

			d_overlay->render();

			swapBuffers();

			redraw();
		}

		void TriangleExample::viewportEvent(ViewportEvent& event)
		{
			GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });

			d_overlay->imGuiCtx().relayout(Vector2{ event.windowSize() } / event.dpiScaling(),
				event.windowSize(), event.framebufferSize());
		}

		void TriangleExample::keyPressEvent(KeyEvent& event)
		{
			if (d_overlay->imGuiCtx().handleKeyPressEvent(event)) return;
		}

		void TriangleExample::keyReleaseEvent(KeyEvent& event)
		{
			if (d_overlay->imGuiCtx().handleKeyReleaseEvent(event)) return;
		}

		void TriangleExample::mousePressEvent(MouseEvent& event)
		{
			if (d_overlay->imGuiCtx().handleMousePressEvent(event)) return;
		}

		void TriangleExample::mouseReleaseEvent(MouseEvent& event)
		{
			if (d_overlay->imGuiCtx().handleMouseReleaseEvent(event)) return;
		}

		void TriangleExample::mouseMoveEvent(MouseMoveEvent& event)
		{
			if (d_overlay->imGuiCtx().handleMouseMoveEvent(event)) return;
		}

		void TriangleExample::mouseScrollEvent(MouseScrollEvent& event)
		{
			if (d_overlay->imGuiCtx().handleMouseScrollEvent(event)) {
				/* Prevent scrolling the page */
				event.setAccepted();
				return;
			}
		}

		void TriangleExample::textInputEvent(TextInputEvent& event)
		{
			if (d_overlay->imGuiCtx().handleTextInputEvent(event)) return;
		}

	}
}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TriangleExample)