#pragma once

#include <mm/opengl/render_task.hpp>
#include <mm/services/opengl_renderer.hpp>

#include <mm/opengl/camera_3d.hpp>

//#include <glm/fwd.hpp>
#include <glm/vec4.hpp>

#include <mm/opengl/instance_buffer.hpp>

// fwd
namespace MM::OpenGL {
	class Shader;
	class Buffer;
	class VertexArrayObject;
}


namespace MM::OpenGL::RenderTasks {

	class Particles : public RenderTask {
		private:
			std::shared_ptr<Shader> _shader;
			std::shared_ptr<Shader> _sprites_shader;
			std::unique_ptr<Buffer> _vertexBuffer;
			std::unique_ptr<VertexArrayObject> _vao;

			struct gl_instance_data {
				glm::vec3 pos;
				glm::vec3 color;
			};
			std::unique_ptr<MM::OpenGL::InstanceBuffer<gl_instance_data>> gl_inst_buffer;

			struct gl_instance_data_sp {
				glm::mat4 pos_trans;
				glm::vec4 color;
				uint32_t tile_index;
			};
			std::unique_ptr<MM::OpenGL::InstanceBuffer<gl_instance_data_sp>> gl_inst_buffer_sp;


		public:
			glm::vec4 default_color {1.f, 1.f, 1.f, 1.f};

			OpenGL::Camera3D default_cam;

			Particles(Engine& engine);
			~Particles(void);

			void renderParticles(Services::OpenGLRenderer& rs, Engine& engine);
			void renderParticlesSprite(Services::OpenGLRenderer& rs, Engine& engine);
			void render(Services::OpenGLRenderer& rs, Engine& engine) override;

		public:
			const char* vertexPath = "shader/particles/vert.glsl";
			const char* fragmentPath = "shader/particles/frag.glsl";
			const char* vertexPathSprites = "shader/particles/sprites_vert.glsl";
			const char* fragmentPathSprites = "shader/particles/sprites_frag.glsl";

			std::string target_fbo = "display";

		private:
			void setupShaderFiles(void);
	};

} // MM::OpenGL::RenderTasks

