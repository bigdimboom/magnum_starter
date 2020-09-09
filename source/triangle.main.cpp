
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

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

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
			ImGuiIntegration::Context _imgui{ NoCreate };
			bool _showDemoWindow = true;
		};

		TriangleExample::TriangleExample(const Arguments& arguments) :
			Platform::Application{ arguments, Configuration{}.setTitle("Magnum Triangle Example") }
		{
			spdlog::set_level(spdlog::level::debug);
			spdlog::info("triangle");


			_imgui = ImGuiIntegration::Context(Vector2{ windowSize() } / dpiScaling(),
				windowSize(), framebufferSize());

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

			_imgui.newFrame();


			/* Enable text input, if needed */
			if (ImGui::GetIO().WantTextInput && !isTextInputActive())
				startTextInput();
			else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
				stopTextInput();

			/* 3. Show the ImGui demo window. Most of the sample code is in
			   ImGui::ShowDemoWindow() */
			if (_showDemoWindow) {
				ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
				ImGui::ShowDemoWindow();
			}


			/* Update application cursor */
			_imgui.updateApplicationCursor(*this);

			/* Set appropriate states. If you only draw ImGui, it is sufficient to
			   just enable blending and scissor test in the constructor. */
			GL::Renderer::enable(GL::Renderer::Feature::Blending);
			GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
			GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
			GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

			_imgui.drawFrame();

			/* Reset state. Only needed if you want to draw something else with
			   different state after. */
			GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
			GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
			GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
			GL::Renderer::disable(GL::Renderer::Feature::Blending);

			_shader.draw(_mesh);

			swapBuffers();

			redraw();
		}

		void TriangleExample::viewportEvent(ViewportEvent& event)
		{
			GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });

			_imgui.relayout(Vector2{ event.windowSize() } / event.dpiScaling(),
				event.windowSize(), event.framebufferSize());
		}

		void TriangleExample::keyPressEvent(KeyEvent& event)
		{
			if (_imgui.handleKeyPressEvent(event)) return;
		}

		void TriangleExample::keyReleaseEvent(KeyEvent& event)
		{
			if (_imgui.handleKeyReleaseEvent(event)) return;
		}

		void TriangleExample::mousePressEvent(MouseEvent& event)
		{
			if (_imgui.handleMousePressEvent(event)) return;
		}

		void TriangleExample::mouseReleaseEvent(MouseEvent& event)
		{
			if (_imgui.handleMouseReleaseEvent(event)) return;
		}

		void TriangleExample::mouseMoveEvent(MouseMoveEvent& event)
		{
			if (_imgui.handleMouseMoveEvent(event)) return;
		}

		void TriangleExample::mouseScrollEvent(MouseScrollEvent& event)
		{
			if (_imgui.handleMouseScrollEvent(event)) {
				/* Prevent scrolling the page */
				event.setAccepted();
				return;
			}
		}

		void TriangleExample::textInputEvent(TextInputEvent& event)
		{
			if (_imgui.handleTextInputEvent(event)) return;
		}

	}
}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TriangleExample)