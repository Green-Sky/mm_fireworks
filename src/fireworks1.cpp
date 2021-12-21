#include <memory>
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

//#include <glm/ext/scalar_constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include <mm/random/srng.hpp>

#include "render_tasks/particles.hpp"
#include <mm/opengl/render_tasks/imgui.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/organizer.hpp>

#include <mm/opengl/camera_3d.hpp>

#include <mm/components/transform2d.hpp>
#include "components/particles.hpp"

#include <mm/logger.hpp>
#include <iostream>
//#include <random>
#include <queue>
#include <vector>

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

namespace Components {
	struct FireworksExplosion {
		enum exp_type_t : uint16_t {
			FULL = 0,
			FULL_SPLIT, // uses 2
			CIRCLE,
			CIRCLE_2, // uses 2
			exp_type_t_COUNT,
		} type = FULL;

		uint16_t amount = 500;
		uint16_t amount2 = 500;
		float strenth = 80.f;
		float strenth2 = 80.f;
		glm::vec3 color {1.f, 1.f, 1.f};
		glm::vec3 color2 {1.f, 1.f, 1.f};
	};

	struct FireworksRocket {
		float trail_amount = 100.f;
		float trail_amount_accu = 0.f;
		glm::vec3 trail_color {1.f, 0.8f, 0.8f};

		float explosion_timer = 1.f;
		FireworksExplosion explosion;
	};

	struct Particle2DPropulsion {
		float dir = 0.f;
		float amount = 10.f; // m/s^2 ?
	};
}

template<typename RNG>
static int64_t random_int(RNG& rng, int64_t min, int64_t max) {
	return (rng() % ((max - min) + 1)) + min;
}

template<typename RNG>
static float random_float(RNG& rng, float min, float max, float precision_boost = 100.f) {
	return ((rng() % (size_t((max - min) * precision_boost) + 1)) / precision_boost) + min;
}

static const size_t color_list_size = 11;
static const glm::vec3 color_list[color_list_size] {
	{1.f, 1.f, 1.f},
	{1.f, 0.f, 0.f},
	{0.f, 1.f, 0.f},
	{0.f, 0.f, 1.f},
	{1.f, 0.f, 1.f},
	{0.f, 1.f, 1.f},
	{1.f, 1.f, 0.f},
	{0.6f, 1.f, 0.f},
	{0.8f, 0.8f, 1.f},
	{0.8f, 0.0f, 1.f},
	{0.3f, 0.9f, 0.3f},
};

static void spawn_fireworks_rocket(MM::Scene& scene) {
	auto& mt = scene.ctx<MM::Random::SRNG>();

	auto e = scene.create();
	auto& fr = scene.emplace<Components::FireworksRocket>(e);
	fr.trail_amount = 800.f;
	//fr.explosion_timer = 3.6f;
	fr.explosion_timer = random_float(mt, 3.5f, 4.5f);

	fr.explosion.amount = 5000;
	//fr.explosion.strenth =;
	fr.explosion.type = Components::FireworksExplosion::exp_type_t(
		random_int(mt, 0, Components::FireworksExplosion::exp_type_t_COUNT-1)
	);
	fr.explosion.color = color_list[random_int(mt, 0, int64_t(color_list_size)-1)];
	fr.explosion.color2 = color_list[random_int(mt, 0, int64_t(color_list_size)-1)];

	auto& p2dv = scene.emplace<Components::Particle2DVel>(e);
	//p2dv.pos = {0.f, -70.f};
	p2dv.pos = {
		random_float(mt, -100.f, 100.f),
		-70.f
	};
	p2dv.vel = {0.f, 0.f};
	p2dv.dampening = 0.5f;

	auto& p2dp = scene.emplace<Components::Particle2DPropulsion>(e);
	//p2dp.amount = 30.f;
	p2dp.amount = random_float(mt, 27.f, 33.f);
	p2dp.dir = (1.5f + random_float(mt, -0.05f, 0.05f)) * glm::pi<float>();
}

static void spawn_fireworks_explosion_full(MM::Scene& scene, const glm::vec2& pos, const uint16_t amount, const float strenth, const glm::vec3& color) {
	auto& mt = scene.ctx<MM::Random::SRNG>();

	for (size_t i = 0; i < amount; i++) {
		auto e = scene.create();
		{
			auto& part_2d_vel = scene.emplace<Components::Particle2DVel>(e);

			part_2d_vel.pos = pos;

			float dir = ((mt() % 10001)/10000.f) * glm::two_pi<float>();
			part_2d_vel.vel =
				glm::vec2{glm::cos(dir), glm::sin(dir)}
				* strenth * ((mt() % 10001)/10000.f);

			part_2d_vel.dampening = 1.0f + random_float(mt, 0.f, 0.5f, 1000.f);
		}

		auto& col = scene.emplace<Components::ParticleColor>(e);
		col.color = color;

		scene.emplace<Components::ParticleLifeTime>(e, 0.3f + ((mt() % 101)/100.f) * 5.0f);
	}
}

static void spawn_fireworks_explosion_circle(MM::Scene& scene, const glm::vec2& pos, const uint16_t amount, const float strenth, const glm::vec3& color) {
	auto& mt = scene.ctx<MM::Random::SRNG>();

	const float dampening = 1.0f + random_float(mt, 0.f, 0.5f, 1000.f);

	for (size_t i = 0; i < amount; i++) {
		auto e = scene.create();
		{
			auto& part_2d_vel = scene.emplace<Components::Particle2DVel>(e);

			part_2d_vel.pos = pos;

			float dir = ((mt() % 10001)/10000.f) * glm::two_pi<float>();
			part_2d_vel.vel = glm::vec2{glm::cos(dir), glm::sin(dir)} * strenth;

			part_2d_vel.dampening = dampening;
		}

		auto& col = scene.emplace<Components::ParticleColor>(e);
		col.color = color;

		scene.emplace<Components::ParticleLifeTime>(e, 0.3f + ((mt() % 101)/100.f) * 5.0f);
	}
}

static void spawn_fireworks_explosion(MM::Scene& scene, const glm::vec2& pos, const Components::FireworksExplosion& exp) {
	using exp_type_t = Components::FireworksExplosion::exp_type_t;
	switch (exp.type) {
		case exp_type_t::FULL:
			spawn_fireworks_explosion_full(scene, pos, exp.amount, exp.strenth, exp.color);
			break;
		case exp_type_t::FULL_SPLIT:
			spawn_fireworks_explosion_full(scene, pos, exp.amount, exp.strenth, exp.color);
			spawn_fireworks_explosion_full(scene, pos, exp.amount2, exp.strenth2, exp.color2);
			break;
		case exp_type_t::CIRCLE:
			spawn_fireworks_explosion_circle(scene, pos, exp.amount, exp.strenth, exp.color);
			break;
		case exp_type_t::CIRCLE_2:
			spawn_fireworks_explosion_circle(scene, pos, exp.amount, exp.strenth, exp.color);
			spawn_fireworks_explosion_circle(scene, pos, exp.amount2, exp.strenth2, exp.color2);
			break;
		case exp_type_t::exp_type_t_COUNT:
			break;
	}
}

namespace Systems {

	void particle_fireworks_rocket(
		entt::registry& scene,
		entt::view<entt::get_t<Components::FireworksRocket, Components::Particle2DPropulsion, const Components::Particle2DVel>> view,
		MM::Random::SRNG& mt, const MM::Components::TimeDelta& ft
	) {
		view.each(
			[&scene, &mt, &ft](
			MM::Entity me,
			Components::FireworksRocket& rocket,
			Components::Particle2DPropulsion& rocket_propulsion,
			const Components::Particle2DVel rocket_particle
		) {
			// 1. update timer
			rocket.explosion_timer -= ft.tickDelta;

			//   ? and explode
			if (rocket.explosion_timer <= 0.f) {
				//// add to delete
				// explode
				spawn_fireworks_explosion(scene, rocket_particle.pos, rocket.explosion);

				// HACK: add empty life to be killed
				scene.emplace<Components::ParticleLifeTime>(me, 0.f);
				// HACK: add new rocket
				spawn_fireworks_rocket(scene);
				return; // early out
			}


			// 2. update propulsion
			float tmp_vel_dir = glm::atan(rocket_particle.vel.y, rocket_particle.vel.x) + glm::pi<float>();
			rocket_propulsion.dir = glm::mix(rocket_propulsion.dir, tmp_vel_dir, 4.5f * ft.tickDelta);

			// 3. emit trail
			rocket.trail_amount_accu += rocket.trail_amount * ft.tickDelta;
			while (rocket.trail_amount_accu >= 1.f) {
				rocket.trail_amount_accu -= 1.f;

				auto e = scene.create();

				auto& part_2d_vel = scene.emplace<Components::Particle2DVel>(e);
				part_2d_vel.pos = rocket_particle.pos;

				float dir = rocket_propulsion.dir
					+ (((mt() % 2001)-1000.f)/1000.f) * 0.1f; // add a bit of random to trail to simulate cone

				dir = glm::mod(dir + glm::two_pi<float>(), glm::two_pi<float>());
				part_2d_vel.vel = glm::vec2{glm::cos(dir), glm::sin(dir)} * rocket_propulsion.amount * 1.5f;

				part_2d_vel.dampening = 5.0f;

				auto& col = scene.emplace<Components::ParticleColor>(e);
				col.color = rocket.trail_color;

				auto& lt = scene.emplace<Components::ParticleLifeTime>(e);
				lt.time_remaining = 0.3f + ((mt() % 101)/100.f) * 0.3f;
			}
		});
	}

	void particle_2d_vel(entt::view<entt::get_t<Components::Particle2DVel>> view, const MM::Components::TimeDelta& ft) {
		view.each([&ft](Components::Particle2DVel& p) {
			p.vel -= p.vel * p.dampening * ft.tickDelta;
			p.pos += p.vel * ft.tickDelta;
		});
	}

	void particle_2d_propulsion(entt::view<entt::get_t<Components::Particle2DVel, const Components::Particle2DPropulsion>> view, const MM::Components::TimeDelta& ft) {
		view.each([&ft](Components::Particle2DVel& p, const Components::Particle2DPropulsion& prop) {
			float tmp_dir = prop.dir + glm::pi<float>();
			p.vel +=
				glm::vec2{glm::cos(tmp_dir), glm::sin(tmp_dir)}
				* prop.amount * ft.tickDelta;
		});
	}

	void particle_2d_gravity(entt::view<entt::get_t<Components::Particle2DVel>> view, const MM::Components::TimeDelta& ft) {
		view.each([&ft](Components::Particle2DVel& p) {
			p.vel += glm::vec2{0.f, -10.f} * ft.tickDelta;
		});
	}

	void particle_life(entt::view<entt::get_t<Components::ParticleLifeTime>> view, const MM::Components::TimeDelta& ft) {
		view.each([&ft](Components::ParticleLifeTime& lt) {
			lt.time_remaining -= ft.tickDelta;
		});
	}

	//void particle_death(entt::view<entt::exclude_t<>, const Components::ParticleLifeTime> view, MM::Scene& scene) {
	void particle_death(MM::Scene& scene) {
		auto view = scene.view<Components::ParticleLifeTime>();

		std::vector<MM::Entity> to_delete;
		to_delete.reserve(view.size()); // overkill?

		view.each([&to_delete](MM::Entity e, const Components::ParticleLifeTime& lt) {
			if (lt.time_remaining <= 0.f) {
				to_delete.push_back(e);
			}
		});

		scene.destroy(to_delete.begin(), to_delete.end());
	}
}


std::unique_ptr<MM::Scene> setup_sim(MM::Engine& engine) {
	auto new_scene = std::make_unique<MM::Scene>();
	//auto& scene = engine.tryService<MM::Services::SceneServiceInterface>()->getScene();
	auto& scene = *new_scene;
	scene.set<MM::Engine*>(&engine);

	auto& org = scene.ctx_or_set<entt::organizer>();

	//scene.set<MM::Random::SRNG>(std::random_device{}());
	scene.set<MM::Random::SRNG>();

	{
		auto& cam = scene.set<MM::OpenGL::Camera3D>();
		cam.horizontalViewPortSize = 300.f;
		cam.setOrthographic();
		cam.updateView();
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

