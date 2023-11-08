#pragma once
#include "surface.hpp"

namespace mini {
	class equidistant_surface final : public differentiable_surface_base {
		private:
			std::shared_ptr<const differentiable_surface_base> m_surface;
			float m_dist;

		public:
			equidistant_surface(std::shared_ptr<const differentiable_surface_base> surface,float dist);
			~equidistant_surface();

			equidistant_surface(const equidistant_surface&) = delete;
			equidistant_surface& operator=(const equidistant_surface&) = delete;

			float get_min_u() const override;
			float get_max_u() const override;
			float get_min_v() const override;
			float get_max_v() const override;

			glm::vec3 sample(float u, float v) const override;
			glm::vec3 normal(float u, float v) const override;

			glm::vec3 ddu(float u, float v) const override;
			glm::vec3 ddv(float u, float v) const override;

			bool is_u_wrapped() const override;
			bool is_v_wrapped() const override;
	};
}