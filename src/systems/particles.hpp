#pragma once

#include <mm/engine_fwd.hpp>

#include "../components/particles.hpp"

#include <mm/components/time_delta.hpp>

namespace Systems {

	void particle_2d_vel(entt::view<entt::get_t<Components::Particle2DVel>> view, const MM::Components::TimeDelta& ft);

	void particle_2d_propulsion(entt::view<entt::get_t<Components::Particle2DVel, const Components::Particle2DPropulsion>> view, const MM::Components::TimeDelta& ft);

	void particle_2d_gravity(entt::view<entt::get_t<Components::Particle2DVel>> view, const MM::Components::TimeDelta& ft);

	void particle_life(entt::view<entt::get_t<Components::ParticleLifeTime>> view, const MM::Components::TimeDelta& ft);

	//void particle_death(entt::view<entt::exclude_t<>, const Components::ParticleLifeTime> view, MM::Scene& scene) {
	void particle_death(MM::Scene& scene);

} // Systems

