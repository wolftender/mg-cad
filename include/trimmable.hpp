#pragma once
#include <vector>
#include <cstdint>

#include "context.hpp"
#include "algebra.hpp"

namespace mini {
	class trimmable_surface_domain {
		private:
			std::vector<uint8_t> m_domain;

			uint32_t m_domain_width;
			uint32_t m_domain_height;

			float m_min_u, m_min_v;
			float m_max_u, m_max_v;

			GLuint m_texture;

		public:
			trimmable_surface_domain(
				uint32_t domain_width, 
				uint32_t domain_height, 
				float min_u, 
				float min_v, 
				float max_u, 
				float max_v);

			~trimmable_surface_domain();

			trimmable_surface_domain(const trimmable_surface_domain&) = delete;
			trimmable_surface_domain& operator=(const trimmable_surface_domain&) = delete;

			void trim_curve(const std::vector<glm::vec2>& curve_points);
			void trim_directions(const glm::vec2& start, const std::vector<glm::vec2>& directions);
			void update_texture();

			void bind(uint32_t slot) const;
			void configure();

			uint8_t at(int32_t x, int32_t y) const;

		private:
			void m_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color);
			void m_flood_fill(int32_t x, int32_t y);

			void m_init_texture();
			void m_free_texture();
	};
}