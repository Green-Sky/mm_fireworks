#include <mm/engine.hpp>

#include <mm/services/filesystem.hpp>
#include <mm/services/sdl_service.hpp>
#include <mm/services/opengl_renderer.hpp>

#include <mm/services/imgui_s.hpp>
#include <mm/services/imgui_menu_bar.hpp>
#include <mm/services/engine_tools.hpp>
#include <mm/services/scene_tools.hpp>

#include <mm/services/scene_service_interface.hpp>
#include <mm/services/organizer_scene.hpp>
#include <mm/components/time_delta.hpp>

#include <mm/opengl/render_tasks/clear.hpp>
#include "./render_tasks/particles.hpp"
#include <mm/opengl/bloom.hpp>
#include <mm/opengl/render_tasks/composition.hpp>
#include <mm/opengl/render_tasks/imgui.hpp>

#include <mm/opengl/fbo_builder.hpp>
#include <mm/opengl/texture_loader.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/organizer.hpp>

#include <mm/random/srng.hpp>

#include <mm/opengl/camera_3d.hpp>

#include "./components/particles.hpp"
#include "./components/firework_particles.hpp"

#include "./systems/particles.hpp"
#include "./systems/fireworks.hpp"

#include <mm/logger.hpp>

#include <iostream>
//#include <random>

#define ENABLE_BAIL(x) if (!x) return false;
bool setup(MM::Engine& engine, const char* argv_0) {
	auto& sdl_ss = engine.addService<MM::Services::SDLService>(
		SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS
	);
	ENABLE_BAIL(engine.enableService<MM::Services::SDLService>());

	sdl_ss.createGLWindow("Fireworks2", 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	engine.addService<MM::Services::FilesystemService>(argv_0, "mm_fireworks2");
	ENABLE_BAIL(engine.enableService<MM::Services::FilesystemService>());

	engine.addService<MM::Services::ImGuiService>();
	ENABLE_BAIL(engine.enableService<MM::Services::ImGuiService>());

	engine.addService<MM::Services::ImGuiMenuBar>();
	ENABLE_BAIL(engine.enableService<MM::Services::ImGuiMenuBar>());

	engine.addService<MM::Services::ImGuiEngineTools>();
	ENABLE_BAIL(engine.enableService<MM::Services::ImGuiEngineTools>());

	engine.addService<MM::Services::OrganizerSceneService>(1.f/64.f);
	ENABLE_BAIL(engine.enableService<MM::Services::OrganizerSceneService>());
	engine.provide<MM::Services::SceneServiceInterface, MM::Services::OrganizerSceneService>();

	engine.addService<MM::Services::ImGuiSceneToolsService>();

	auto& rs = engine.addService<MM::Services::OpenGLRenderer>();
	ENABLE_BAIL(engine.enableService<MM::Services::OpenGLRenderer>());

	// enable after renderer
	ENABLE_BAIL(engine.enableService<MM::Services::ImGuiSceneToolsService>());

	{ // rendering
		using namespace entt::literals;
		{ // textures
			auto& rm_t = MM::ResourceManager<MM::OpenGL::Texture>::ref();
			auto [w, h] = engine.getService<MM::Services::SDLService>().getWindowSize();

			const float render_scale = 1.f;

			// depth
#ifdef MM_OPENGL_3_GLES
			rm_t.reload<MM::OpenGL::TextureLoaderEmpty>(
				"depth",
				GL_DEPTH_COMPONENT24, // d16 is the onlyone for gles 2 (TODO: test for 3)
				w, h,
				GL_DEPTH_COMPONENT, GL_UNSIGNED_INT
			);
#else
			rm_t.reload<MM::OpenGL::TextureLoaderEmpty>(
				"depth",
				GL_DEPTH_COMPONENT32F, // TODO: stencil?
				w, h,
				GL_DEPTH_COMPONENT, GL_FLOAT
			);
#endif


			// hdr color gles3 / webgl2
			rm_t.reload<MM::OpenGL::TextureLoaderEmpty>(
				"hdr_color",
				GL_RGBA16F,
				w * render_scale, h * render_scale,
				GL_RGBA,
				GL_HALF_FLOAT
			);
			{ // filter
				rm_t.get("hdr_color"_hs)->bind(0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			// fbo
			rs.targets["game_view"] = MM::OpenGL::FBOBuilder::start()
				.attachTexture(rm_t.get("hdr_color"_hs), GL_COLOR_ATTACHMENT0)
				.attachTexture(rm_t.get("depth"_hs), GL_DEPTH_ATTACHMENT)
				.setResizeFactors(render_scale, render_scale)
				.setResize(true)
				.finish();
			assert(rs.targets["game_view"]);
		}

		// clear
		auto& clear_opaque = rs.addRenderTask<MM::OpenGL::RenderTasks::Clear>(engine);
		clear_opaque.target_fbo = "game_view";
		// clears all color attachments
		clear_opaque.r = 0.f;
		clear_opaque.g = 0.f;
		clear_opaque.b = 0.f;
		clear_opaque.a = 1.f;
		clear_opaque.mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

		rs.addRenderTask<MM::OpenGL::RenderTasks::Particles>(engine).target_fbo = "game_view";

		// TODO: on window size change, phases max(log(min(w, h), 2)-1, 1);
		MM::OpenGL::setup_bloom(engine, "hdr_color", 7, 0.5);

		// not part of setup_bloom
		auto& comp = rs.addRenderTask<MM::OpenGL::RenderTasks::Composition>(engine);
		comp.color_tex = "hdr_color";
		comp.bloom_tex = "blur_tmp1";
		comp.target_fbo = "display";

		rs.addRenderTask<MM::OpenGL::RenderTasks::ImGuiRT>(engine);
	}

	return true;
}

// TODO: extract this
std::unique_ptr<MM::Scene> setup_sim(MM::Engine& engine) {
	auto new_scene = std::make_unique<MM::Scene>();
	auto& scene = *new_scene;
	scene.ctx().emplace<MM::Engine&>(engine);

	auto& org = scene.ctx().emplace<entt::organizer>();

	//scene.ctx().emplace<MM::Random::SRNG>(std::random_device{}());
	scene.ctx().emplace<MM::Random::SRNG>();

	{
		auto& cam = scene.ctx().emplace<MM::OpenGL::Camera3D>();
		cam.horizontalViewPortSize = 300.f;
		cam.setOrthographic();
		cam.updateView();
	}

	{
		auto& cl = scene.ctx().emplace<Components::ColorList>();
		for (auto& col : cl.list) {
			col *= 8.f;
		}
	}

	org.emplace<Systems::particle_fireworks_rocket>("particle_fireworks_rocket");
	org.emplace<Systems::particle_2d_propulsion>("particle_2d_propulsion");
	org.emplace<Systems::particle_2d_gravity>("particle_2d_gravity");
	org.emplace<Systems::particle_2d_vel>("particle_2d_vel");
	org.emplace<Systems::particle_life>("particle_life");
	org.emplace<Systems::particle_death>("particle_death");

	spawn_fireworks_rocket(scene);
	spawn_fireworks_rocket(scene);
	spawn_fireworks_rocket(scene);
	spawn_fireworks_rocket(scene);

	return new_scene;
}

static MM::Engine engine;

int main(int argc, char** argv) {
	assert(argc > 0);
	if (!setup(engine, argv[0])) {
		std::cerr << "error: setting up the engine\n";
		return 1;
	}

	engine.tryService<MM::Services::SceneServiceInterface>()->changeSceneNow(setup_sim(engine));

	engine.run();

	engine.cleanup();
	return 0;
}

