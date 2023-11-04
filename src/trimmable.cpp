#include "trimmable.hpp"
#include "gui.hpp"

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

	void trimmable_surface_domain::trim_directions(const glm::vec2& start, const std::vector<glm::vec2>& directions) {
		auto p1 = start;
		for (const auto& direction : directions) {
			auto p2 = p1 + direction;

			uint32_t x0 = static_cast<uint32_t>(p1.x * m_domain_width);
			uint32_t y0 = static_cast<uint32_t>(p1.y * m_domain_height);
			uint32_t x1 = static_cast<uint32_t>(p2.x * m_domain_width);
			uint32_t y1 = static_cast<uint32_t>(p2.y * m_domain_height);

			m_draw_line(x0, y0, x1, y1, CURVE);
			p1 = p2;
		}
	}

	void trimmable_surface_domain::update_texture() {
		if (m_texture) {
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexSubImage2D(
				GL_TEXTURE_2D,
				0, 0, 0, m_domain_width, m_domain_height,
				GL_RGB,
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

	void trimmable_surface_domain::configure() {
		if (ImGui::CollapsingHeader("Surface Trimming")) {
			auto min = ImGui::GetWindowContentRegionMin();
			auto max = ImGui::GetWindowContentRegionMax();
			auto window_pos = ImGui::GetWindowPos();

			int width = static_cast<int> (max.x - min.x);
			int height = static_cast<int> (max.y - min.y);

			auto cursor_pos = ImGui::GetCursorScreenPos();

			if (ImGui::ImageButton(reinterpret_cast<ImTextureID> (m_texture), ImVec2(width, width), ImVec2(0, 0), ImVec2(1, 1), 0)) {
				auto mouse_pos = ImGui::GetMousePos();

				auto pos_x = static_cast<float>(mouse_pos.x - cursor_pos.x) / static_cast<float>(width);
				auto pos_y = static_cast<float>(mouse_pos.y - cursor_pos.y) / static_cast<float>(width);

				std::cout << "trim " << pos_x << ", " << pos_y << std::endl;

				int32_t pixel_pos_x = pos_x * m_domain_width;
				int32_t pixel_pos_y = pos_y * m_domain_height;

				m_flood_fill(pixel_pos_x, pixel_pos_y);

				update_texture();
			}

			if (ImGui::Button("Flip Domain", ImVec2(ImGui::GetWindowWidth() * 0.95f, 24.0f))) {
				for (auto& el : m_domain) {
					if (el == VISIBLE) {
						el = HIDDEN;
					} else if (el == HIDDEN) {
						el = VISIBLE;
					}
				}

				update_texture();
			}

			if (ImGui::Button("Reset Domain", ImVec2(ImGui::GetWindowWidth() * 0.95f, 24.0f))) {
				std::fill(m_domain.begin(), m_domain.end(), VISIBLE);
				update_texture();
			}

			ImGui::NewLine();
		}
	}

	uint8_t trimmable_surface_domain::at(int32_t x, int32_t y) const {
		auto ry = y % m_domain_height;
		auto rx = x % m_domain_width;
		auto base = 3 * (ry * m_domain_width + rx);

		return m_domain[base];
	}

	void trimmable_surface_domain::trim(float u, float v) {
		int32_t pixel_pos_x = u * m_domain_width;
		int32_t pixel_pos_y = v * m_domain_height;

		m_flood_fill(pixel_pos_x, pixel_pos_y);
		update_texture();
	}

	void trimmable_surface_domain::m_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color) {
		int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = (dx > dy ? dx : -dy) / 2, e2;

		for (;;) {
			auto ry = y0 % m_domain_height;
			auto rx = x0 % m_domain_width;
			auto base = 3 * (ry * m_domain_width + rx);

			m_domain[base + 0] = color;
			m_domain[base + 1] = color;
			m_domain[base + 2] = color;

			if (x0 == x1 && y0 == y1) break;
			e2 = err;
			if (e2 > -dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}

	void trimmable_surface_domain::m_flood_fill(int32_t x, int32_t y) {
		auto color = at(x, y);
		if (color != VISIBLE && color != HIDDEN) {
			return;
		}

		auto inv = (color == VISIBLE) ? HIDDEN : VISIBLE;
	
		std::vector<bool> visited;
		visited.resize(m_domain_width * m_domain_height);
		std::fill(visited.begin(), visited.end(), false);

		std::vector<std::pair<int32_t, int32_t>> stack;
		stack.reserve(m_domain_width * m_domain_height);

		stack.push_back({x, y});

		while (!stack.empty()) {
			auto curr = stack.back();
			stack.pop_back();

			auto x = curr.first;
			auto y = curr.second;

			auto base = 3 * (y * m_domain_width + x);
			m_domain[base + 0] = inv;
			m_domain[base + 1] = inv;
			m_domain[base + 2] = inv;

			if (x >= 1 && at(x - 1, y) == color) {
				stack.push_back({ x - 1, y });
			}

			if (y >= 1 && at(x, y - 1) == color) {
				stack.push_back({ x, y - 1 });
			}

			if (x < m_domain_width - 1 && at(x + 1, y) == color) {
				stack.push_back({ x + 1, y });
			}

			if (y < m_domain_height - 1 && at(x, y + 1) == color) {
				stack.push_back({ x, y + 1 });
			}
		}
	}

	void trimmable_surface_domain::m_init_texture() {
		m_domain.resize(m_domain_width * m_domain_height * 3);
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

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_domain_width, m_domain_height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_domain.data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void trimmable_surface_domain::m_free_texture() {
		if (m_texture) {
			glDeleteTextures(1, &m_texture);
			m_texture = 0;
		}
	}
}