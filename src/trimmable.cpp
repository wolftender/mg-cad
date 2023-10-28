#include "trimmable.hpp"

namespace mini {
	constexpr uint8_t VISIBLE = 255;
	constexpr uint8_t CURVE = 128;
	constexpr uint8_t HIDDEN = 0;

	trimmable_surface_domain::trimmable_surface_domain(
		uint32_t domain_width,
		uint32_t domain_height,
		float min_u,
		float min_v,
		float max_u,
		float max_v) {

		m_domain_width = domain_width;
		m_domain_height = domain_height;

		m_texture = 0;
		m_init_texture();
	}

	trimmable_surface_domain::~trimmable_surface_domain() {
		m_free_texture();
	}

	void trimmable_surface_domain::trim_curve(const std::vector<glm::vec2>& curve_points) {
		for (int i = 0; i < curve_points.size() - 1; ++i) {
			const auto& p1 = curve_points[i + 0];
			const auto& p2 = curve_points[i + 1];

			uint32_t x0 = static_cast<uint32_t>(p1.x * m_domain_width);
			uint32_t y0 = static_cast<uint32_t>(p1.y * m_domain_height);
			uint32_t x1 = static_cast<uint32_t>(p2.x * m_domain_width);
			uint32_t y1 = static_cast<uint32_t>(p2.y * m_domain_height);

			m_draw_line(x0, y0, x1, y1, CURVE);
		}
	}

	void trimmable_surface_domain::update_texture() {
		if (m_texture) {
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexSubImage2D(
				GL_TEXTURE_2D,
				0, 0, 0, m_domain_width, m_domain_height,
				GL_RED,
				GL_UNSIGNED_BYTE,
				m_domain.data());

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void trimmable_surface_domain::bind(uint32_t slot) const {
		if (m_texture) {
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, m_texture);
		}
	}

	void trimmable_surface_domain::m_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color) {
		int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = (dx > dy ? dx : -dy) / 2, e2;

		for (;;) {
			m_domain[y0 * m_domain_width + x0] = color;

			if (x0 == x1 && y0 == y1) break;
			e2 = err;
			if (e2 > -dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}

	void trimmable_surface_domain::m_init_texture() {
		m_domain.resize(m_domain_width * m_domain_height);
		std::fill(m_domain.begin(), m_domain.end(), VISIBLE);

		if (m_texture) {
			m_free_texture();
		}

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_domain_width, m_domain_height, 0, GL_RED, GL_UNSIGNED_BYTE, m_domain.data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void trimmable_surface_domain::m_free_texture() {
		if (m_texture) {
			glDeleteTextures(1, &m_texture);
			m_texture = 0;
		}
	}
}