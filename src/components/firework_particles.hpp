#pragma once

#include <glm/vec3.hpp>

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

} // Components

