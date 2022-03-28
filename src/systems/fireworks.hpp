#pragma once

#include <mm/engine_fwd.hpp>

#include "../components/particles.hpp"
#include "../components/firework_particles.hpp"

#include <mm/components/time_delta.hpp>

#include <mm/random/srng.hpp>

void spawn_fireworks_rocket(MM::Scene& scene);

void spawn_fireworks_explosion_full(MM::Scene& scene, const glm::vec2& pos, const uint16_t amount, const float strenth, const glm::vec3& color);

void spawn_fireworks_explosion_circle(MM::Scene& scene, const glm::vec2& pos, const uint16_t amount, const float strenth, const glm::vec3& color);

void spawn_fireworks_explosion(MM::Scene& scene, const glm::vec2& pos, const Components::FireworksExplosion& exp);

namespace Systems {

	void particle_fireworks_rocket(
		entt::registry& scene,
		entt::view<entt::get_t<Components::FireworksRocket, Components::Particle2DPropulsion, const Components::Particle2DVel>> view,
		MM::Random::SRNG& rng, const MM::Components::TimeDelta& ft
	);

} // Systems

