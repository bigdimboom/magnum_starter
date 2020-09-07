
#include <spdlog/spdlog.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColor.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/GlmIntegration/Integration.h>

namespace Magnum
{
	namespace Examples
	{

		class TriangleExample : public Platform::Application {
		public:
			explicit TriangleExample(const Arguments& arguments);

		private:
			void drawEvent() override;

			GL::Mesh _mesh;
			Shaders::VertexColor2D _shader;
		};

		TriangleExample::TriangleExample(const Arguments& arguments) :
			Platform::Application{ arguments, Configuration{}.setTitle("Magnum Triangle Example") }
		{
			spdlog::set_level(spdlog::level::debug);
			spdlog::info("triangle");


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

			swapBuffers();

			redraw();
		}

	}
}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TriangleExample)