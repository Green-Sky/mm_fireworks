#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace Components {

	struct Particle2DVel {
		glm::vec2 pos {0.f, 0.f};
		glm::vec2 vel {0.f, 0.f};
		float dampening = 0.f;
	};

	struct ParticleColor {
		glm::vec3 color {1.f, 1.f, 1.f};
	};

	struct ParticleLifeTime {
		float time_remaining = 0.f;
	};

	// ???
	struct ParticleEmitter {
		uint16_t amount_per_tick = 1;
		float dir = 0.f;
		float dir_range = 0.f; // for random, +-range
		float init_vel = 1.f;
	};

	struct Particle2DPropulsion {
		float dir = 0.f;
		float amount = 10.f; // m/s^2 ?
	};

} // Components

