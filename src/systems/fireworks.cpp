#include "./fireworks.hpp"

#include <mm/scalar_range2.hpp>

#include <entt/entity/registry.hpp>

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>

void spawn_fireworks_rocket(MM::Scene& scene) {
	auto& rng = scene.ctx().at<MM::Random::SRNG>();
	const auto& color_list = scene.ctx().at<Components::ColorList>();

	auto e = scene.create();
	auto& fr = scene.emplace<Components::FireworksRocket>(e);
	fr.trail_amount = 800.f;
	//fr.explosion_timer = 3.6f;
	fr.explosion_timer = rng.range(MM::ScalarRange2{3.5f, 4.5f});

	fr.explosion.amount = 5000;
	//fr.explosion.strenth =;
	fr.explosion.type = Components::FireworksExplosion::exp_type_t(
		rng.minMax(0, Components::FireworksExplosion::exp_type_t_COUNT-1)
	);
	fr.explosion.color = color_list.list[rng.minMax(0, color_list.list.size()-1)];
	fr.explosion.color2 = color_list.list[rng.minMax(0, color_list.list.size()-1)];

	auto& p2dv = scene.emplace<Components::Particle2DVel>(e);
	//p2dv.pos = {0.f, -70.f};
	p2dv.pos = {
		rng.negOneToOne() * 100.f,
		-70.f
	};
	p2dv.vel = {0.f, 0.f};
	p2dv.dampening = 0.5f;

	auto& p2dp = scene.emplace<Components::Particle2DPropulsion>(e);
	//p2dp.amount = 30.f;
	p2dp.amount = rng.range(MM::ScalarRange2{27.f, 33.f});
	p2dp.dir = (1.5f + rng.negOneToOne() * 0.05f) * glm::pi<float>();
}

void spawn_fireworks_explosion_full(MM::Scene& scene, const glm::vec2& pos, const uint16_t amount, const float strenth, const glm::vec3& color) {
	auto& rng = scene.ctx().at<MM::Random::SRNG>();

	for (size_t i = 0; i < amount; i++) {
		auto e = scene.create();
		{
			auto& part_2d_vel = scene.emplace<Components::Particle2DVel>(e);

			part_2d_vel.pos = pos;

			float dir = rng.zeroToOne() * glm::two_pi<float>();
			part_2d_vel.vel =
				glm::vec2{glm::cos(dir), glm::sin(dir)} * strenth * rng.zeroToOne();

			part_2d_vel.dampening = 1.0f + rng.zeroToOne() * 0.5f;
		}

		auto& col = scene.emplace<Components::ParticleColor>(e);
		col.color = color;

		scene.emplace<Components::ParticleLifeTime>(e, 0.3f + rng.zeroToOne() * 5.0f);
	}
}

void spawn_fireworks_explosion_circle(MM::Scene& scene, const glm::vec2& pos, const uint16_t amount, const float strenth, const glm::vec3& color) {
	auto& rng = scene.ctx().at<MM::Random::SRNG>();

	const float dampening = 1.0f + rng.zeroToOne() * 0.5f;

	for (size_t i = 0; i < amount; i++) {
		auto e = scene.create();
		{
			auto& part_2d_vel = scene.emplace<Components::Particle2DVel>(e);

			part_2d_vel.pos = pos;

			float dir = rng.zeroToOne() * glm::two_pi<float>();
			part_2d_vel.vel = glm::vec2{glm::cos(dir), glm::sin(dir)} * strenth;

			part_2d_vel.dampening = dampening;
		}

		auto& col = scene.emplace<Components::ParticleColor>(e);
		col.color = color;

		scene.emplace<Components::ParticleLifeTime>(e, 0.3f + rng.zeroToOne() * 5.0f);
	}
}

void spawn_fireworks_explosion(MM::Scene& scene, const glm::vec2& pos, const Components::FireworksExplosion& exp) {
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
	MM::Random::SRNG& rng, const MM::Components::TimeDelta& ft
) {
	view.each(
		[&scene, &rng, &ft](
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
				+ rng.negOneToOne() * 0.1f; // add a bit of random to trail to simulate cone

			dir = glm::mod(dir + glm::two_pi<float>(), glm::two_pi<float>());
			part_2d_vel.vel = glm::vec2{glm::cos(dir), glm::sin(dir)} * rocket_propulsion.amount * 1.5f;

			part_2d_vel.dampening = 5.0f;

			auto& col = scene.emplace<Components::ParticleColor>(e);
			col.color = rocket.trail_color;

			auto& lt = scene.emplace<Components::ParticleLifeTime>(e);
			lt.time_remaining = 0.3f + rng.zeroToOne() * 0.3f;
		}
	});
}

} // Systems

