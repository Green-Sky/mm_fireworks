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

#include "./render_tasks/particles.hpp"
#include <mm/opengl/render_tasks/imgui.hpp>

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

	sdl_ss.createGLWindow("Fireworks1", 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	engine.addService<MM::Services::FilesystemService>(argv_0, "mm_fireworks1");
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

	rs.addRenderTask<MM::OpenGL::RenderTasks::Particles>(engine);
	rs.addRenderTask<MM::OpenGL::RenderTasks::ImGuiRT>(engine);

	return true;
}

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

	scene.ctx().emplace<Components::ColorList>();

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

