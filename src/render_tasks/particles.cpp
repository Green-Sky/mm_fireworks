#include "./particles.hpp"

#include <memory>

#include <mm/opengl/shader.hpp>
#include <mm/opengl/buffer.hpp>
#include <mm/opengl/vertex_array_object.hpp>

#include <mm/fs_const_archiver.hpp>

#include <mm/engine.hpp>

#include <mm/services/scene_service_interface.hpp>
#include <entt/entity/registry.hpp>

#include <mm/components/color.hpp>

//#include "./spritesheet_renderable.hpp"
#include <mm/services/opengl_renderer.hpp>

#include "../components/particles.hpp"

#include <tracy/Tracy.hpp>
#ifndef MM_OPENGL_3_GLES
	#include <tracy/TracyOpenGL.hpp>
#else
	#define TracyGpuContext
	#define TracyGpuCollect
	#define TracyGpuZone(...)
#endif

#include <mm/logger.hpp>

namespace MM::OpenGL::RenderTasks {

Particles::Particles(Engine& engine) {
	float vertices[] = {
		-0.5f, 0.5f,
		-0.5f, -0.5f,
		0.5f, -0.5f,
		0.5f, -0.5f,
		0.5f, 0.5f,
		-0.5f, 0.5f,
	};

	_vertexBuffer = std::make_unique<Buffer>(vertices, 2 * 6 * sizeof(float), GL_STATIC_DRAW);
	_vao = std::make_unique<VertexArrayObject>();
	_vao->bind();
	_vertexBuffer->bind(GL_ARRAY_BUFFER);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

	gl_inst_buffer = std::make_unique<MM::OpenGL::InstanceBuffer<gl_instance_data>>();
	gl_inst_buffer_sp = std::make_unique<MM::OpenGL::InstanceBuffer<gl_instance_data_sp>>();

	_vertexBuffer->unbind(GL_ARRAY_BUFFER);
	_vao->unbind();

	setupShaderFiles();
	_shader = Shader::createF(engine, vertexPath, fragmentPath);
	assert(_shader != nullptr);
	_sprites_shader = Shader::createF(engine, vertexPathSprites, fragmentPathSprites);
	assert(_sprites_shader != nullptr);
}

Particles::~Particles(void) {
}

void Particles::renderParticles(Services::OpenGLRenderer& rs, Engine& engine) {
	auto* scene_ss = engine.tryService<MM::Services::SceneServiceInterface>();
	// no scene
	if (scene_ss == nullptr) {
		return; // nothing to render
	}

	auto& scene = scene_ss->getScene();

	if (!scene.ctx().contains<Camera3D>()) {
		return; // nothing to draw
	}

	auto view = scene.view<::Components::Particle2DVel, ::Components::ParticleColor>();
	if (view.begin() == view.end()) {
		return; // nothing to render
	}

	rs.targets[target_fbo]->bind(FrameBufferObject::W);

	glDisable(GL_DEPTH_TEST); // the simple Particles really dont need this
	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);

	_shader->bind();
	_vertexBuffer->bind(GL_ARRAY_BUFFER);
	_vao->bind();

	Camera3D& cam = scene.ctx().at<Camera3D>();
	auto vp = cam.getViewProjection();
	_shader->setUniformMat4f("_VP", vp);

	const size_t size_hint = view.size_hint();
	//gl_inst_buffer->resize(size_hint, GL_DYNAMIC_DRAW);
	size_t size_actual = 0;
	{
		auto* mapped_buffer = gl_inst_buffer->map(size_hint, GL_DYNAMIC_DRAW);
		assert(mapped_buffer);

		size_t i = 0;
		for (const auto& [e, p, col] : view.each()) {
			(void)e;

			mapped_buffer[i].pos = {p.pos.x, p.pos.y, 1.f}; // TODO: z plane
			mapped_buffer[i].color = col.color;

			i++;
		}

		gl_inst_buffer->unmap();
		size_actual = i;
	}


	static_assert(std::is_standard_layout<gl_instance_data>::value); // check if offsetof() is usable

	{
		glVertexAttribPointer(
			1,
			decltype(gl_instance_data::pos)::length(),
			GL_FLOAT, GL_FALSE,
			sizeof(gl_instance_data),
			(void*) offsetof(gl_instance_data, pos)
		);
		glVertexAttribDivisor(1, 1);
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(
			2,
			decltype(gl_instance_data::color)::length(),
			GL_FLOAT, GL_FALSE,
			sizeof(gl_instance_data),
			(void*) offsetof(gl_instance_data, color)
		);
		glVertexAttribDivisor(2, 1);
		glEnableVertexAttribArray(2);

		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, size_actual);
		//SPDLOG_INFO("rendered {} Particles", size_actual);
	}

	_vao->unbind();
	_vertexBuffer->unbind(GL_ARRAY_BUFFER);
	_shader->unbind();
}

void Particles::renderParticlesSprite(Services::OpenGLRenderer& rs, Engine& engine) {



	//_vao->unbind();
	//_vertexBuffer->unbind(GL_ARRAY_BUFFER);
	//_sprites_shader->unbind();
}

void Particles::render(Services::OpenGLRenderer& rs, Engine& engine) {
	ZoneScopedN("MM::OpenGL::RenderTasks::Particles::render");

	auto* scene_ss = engine.tryService<MM::Services::SceneServiceInterface>();
	// no scene
	if (scene_ss == nullptr) {
		return;
	}

	renderParticles(rs, engine);
	renderParticlesSprite(rs, engine);
}

void Particles::setupShaderFiles(void) {
	FS_CONST_MOUNT_FILE(vertexPath,
GLSL_VERSION_STRING
R"(
#ifdef GL_ES
	precision mediump float;
#endif

uniform mat4 _VP;

layout(location = 0) in vec2 _vertexPosition;
layout(location = 1) in vec3 _pos;
layout(location = 2) in vec3 _color;

out vec3 _frag_color;

void main() {
	// fwd
	_frag_color = _color;

	float scale = 0.2;

	// position
	gl_Position = _VP * vec4(_vertexPosition * scale + _pos.xy, _pos.z, 1);
})")

	FS_CONST_MOUNT_FILE(fragmentPath,
GLSL_VERSION_STRING
R"(
#ifdef GL_ES
	precision mediump float;
#endif

in vec3 _frag_color;

out vec4 _out_color;

void main() {
	_out_color = vec4(_frag_color, 1.0);
})")

	FS_CONST_MOUNT_FILE(vertexPathSprites,
GLSL_VERSION_STRING
R"(
#ifdef GL_ES
	precision mediump float;
#endif

uniform mat4 _VP;
uniform uvec2 _tileCount;

layout(location = 0) in vec2 _vertexPosition;
layout(location = 1) in mat4 _pos_trans;
layout(location = 5) in uint _atlasIndex;
layout(location = 6) in vec4 _color;

out vec2 _tex_pos;
out vec4 _tex_color;

void main() {
	// fwd
	_tex_color = _color;

	// position
	gl_Position = _VP * _pos_trans * vec4(_vertexPosition, 0, 1);


	// uv
	uint row = _atlasIndex / _tileCount.x;
	uint column = _atlasIndex % _tileCount.x;

	_tex_pos.x = (float(column) + 0.5 + _vertexPosition.x) / float(_tileCount.x);
	_tex_pos.y = 1.0 - (float(row) + 0.5 - _vertexPosition.y) / float(_tileCount.y);
})")

	FS_CONST_MOUNT_FILE(fragmentPathSprites,
GLSL_VERSION_STRING
R"(
#ifdef GL_ES
	precision mediump float;
#endif

uniform sampler2D _tex0;

in vec2 _tex_pos;
in vec4 _tex_color;

out vec4 _out_color;

void main() {
	vec4 tmp_col = texture(_tex0, _tex_pos) * _tex_color;

	if (tmp_col.a == 0.0) {
		discard;
	}

	_out_color = tmp_col;
})")

}

} // MM::OpenGL::RenderTasks

