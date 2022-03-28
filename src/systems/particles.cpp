#include "./particles.hpp"

#include <entt/entity/registry.hpp>

#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>

namespace Systems {

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

} // Systems

