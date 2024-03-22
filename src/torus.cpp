#include "torus.hpp"
#include "gui.hpp"
#include "serializer.hpp"

#include <iostream>

namespace mini {
	torus_object::torus_object(
		scene_controller_base& scene,
		std::shared_ptr<shader_t> shader,
		float inner_radius,
		float outer_radius) :
		scene_obj_t(scene, "torus"),
		m_domain(2048U, 2048U, 0.0f, 0.0f, 1.0f, 1.0f) {

		m_shader = shader;
		m_inner_radius = inner_radius;
		m_outer_radius = outer_radius;

		m_div_u = 64;
		m_div_v = 64;
		m_is_wireframe = false;

		m_requires_rebuild = false;
		m_generate_geometry();
	}

	torus_object::~torus_object() {
		m_free_geometry();
	}

	void torus_object::render(app_context& context, const glm::mat4x4& world_matrix) const {
		glBindVertexArray(m_vao);

		m_domain.bind(0);
		m_shader->bind();

		if (!is_selected()) {
			m_shader->set_uniform("u_color", glm::vec4{ 1.0f, 1.0f, 1.0f, 0.75f });
		} else {
			m_shader->set_uniform("u_color", glm::vec4{ 0.960f, 0.646f, 0.0192f, 0.4f });
		}

		// set uniforms
		const auto& view_matrix = context.get_view_matrix();
		const auto& proj_matrix = context.get_projection_matrix();

		m_shader->set_uniform_int("u_domain_sampler", 0);
		m_shader->set_uniform("u_world", world_matrix);
		m_shader->set_uniform("u_view", view_matrix);
		m_shader->set_uniform("u_projection", proj_matrix);

		if (m_is_wireframe) {
			glDisable(GL_DEPTH_TEST);
		}

		glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
		glBindVertexArray(static_cast<GLuint>(NULL));
		glEnable(GL_DEPTH_TEST);
	}

	void torus_object::configure() {
		scene_obj_t::configure();

		bool changed = false;

		ImGui::NewLine();
		if (ImGui::CollapsingHeader("Torus Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Inner Radius: ", 100.0f);
			changed = ImGui::InputFloat("##innerradius", &m_inner_radius) || changed;

			gui::prefix_label("Outer Radius: ", 100.0f);
			changed = ImGui::InputFloat("##outerradius", &m_outer_radius) || changed;

			gui::prefix_label("u Resolution: ", 100.0f);
			changed = ImGui::InputInt("##ures", &m_div_u) || changed;

			gui::prefix_label("v Resolution: ", 100.0f);
			changed = ImGui::InputInt("##vres", &m_div_v) || changed;
		}

		m_domain.configure();

		if (changed) {
			m_rebuild();
		}
	}

	const object_serializer_base& torus_object::get_serializer() const {
		return generic_object_serializer<torus_object>::get_instance();
	}

	int torus_object::get_div_u() const {
		return m_div_u;
	}

	int torus_object::get_div_v() const {
		return m_div_v;
	}

	float torus_object::get_inner_radius() const {
		return m_inner_radius;
	}

	float torus_object::get_outer_radius() const {
		return m_outer_radius;
	}

	void torus_object::set_div_u(int div_u) {
		m_div_u = div_u;
		m_rebuild();
	}

	void torus_object::set_div_v(int div_v) {
		m_div_v = div_v;
		m_rebuild();
	}

	void torus_object::set_inner_radius(float r) {
		m_inner_radius = r;
		m_rebuild();
	}

	void torus_object::set_outer_radius(float r) {
		m_outer_radius = r;
		m_rebuild();
	}

	void torus_object::m_rebuild() {
		gui::clamp(m_outer_radius, 0.2f, 30.0f);
		gui::clamp(m_inner_radius, 0.1f, m_outer_radius - 0.5f);
		gui::clamp(m_div_u, 4, 100);
		gui::clamp(m_div_v, 4, 100);

		m_generate_geometry();
	}

	void torus_object::m_generate_geometry() {
		m_requires_rebuild = false;

		// parametric equation of torus is
		// A = (R+r)/2
		// B = (R-r)/2
		// x(u,v) = cos(v) * (A + B * cos(u))
		// y(u,v) = sin(v) * (A + B * cos(u))
		// z(u,v) = B * sin(u)
		// u "controls" the small circle
		// v "controls" the big circle

		std::vector<float> vertices;
		std::vector<float> uv;
		std::vector<float> domain_uv;
		std::vector<GLuint> indices;

		vertices.reserve(m_div_u * m_div_v * 3);
		uv.reserve(m_div_u * m_div_v * 2);
		domain_uv.reserve(m_div_u * m_div_v * 2);
		indices.reserve(m_div_u * m_div_v * 6);

		// u \in [0, 2pi)
		// v \in [0, 2pi)

		constexpr float double_pi = (2.0f * glm::pi<float>());

		float du = double_pi / static_cast<float>(m_div_u);
		float dv = double_pi / static_cast<float>(m_div_v);

		float A = (m_outer_radius + m_inner_radius) / 2.0f;
		float B = (m_outer_radius - m_inner_radius) / 2.0f;

		for (int vi = 0; vi <= m_div_v; ++vi) {
			for (int ui = 0; ui <= m_div_u; ++ui) {
				float u = du * ui;
				float v = dv * vi;

				float x = cos(v) * (A + B * cos(u));
				float y = sin(v) * (A + B * cos(u));
				float z = B * sin(u);

				vertices.push_back(x);
				vertices.push_back(y);
				vertices.push_back(z);

				int tu = ui % 2;
				int tv = vi % 2;

				float tu_f = static_cast<float> (tu);
				float tv_f = static_cast<float> (tv);

				uv.push_back(tu_f);
				uv.push_back(tv_f);

				domain_uv.push_back(u / double_pi);
				domain_uv.push_back(v / double_pi);
			}
		}

		int vertex_count = (m_div_u + 1) * (m_div_v + 1);
		int loop_size = m_div_u + 1;

		// construct torus topology
		for (int vi = 0; vi < m_div_v; ++vi) {
			for (int ui = 0; ui < m_div_u; ++ui) {
				// face is a quad
				int v0 = (loop_size * vi) + ui;
				int v1 = v0 + 1;
				int v2 = v0 + loop_size;
				int v3 = v2 + 1;

				assert(v0 < vertex_count);
				assert(v1 < vertex_count);
				assert(v2 < vertex_count);
				assert(v3 < vertex_count);

				indices.push_back(v0);
				indices.push_back(v1);
				indices.push_back(v2);

				indices.push_back(v1);
				indices.push_back(v3);
				indices.push_back(v2);
			}
		}

		m_free_geometry();
		m_build_geometry(vertices, uv, domain_uv, indices);
	}

	void torus_object::m_build_geometry(
		const std::vector<float>& positions,
		const std::vector<float>& uv,
		const std::vector<float>& domain_uv,
		const std::vector<GLuint>& indices) {

		constexpr GLuint a_position = 0;
		constexpr GLuint a_uv = 1;
		constexpr GLuint a_tex_uv = 2;

		m_indices = indices;
		m_positions = positions;
		m_domain_uv = domain_uv;
		m_uv = uv;

		int num_vertices = (m_div_u + 1) * (m_div_v + 1);
		m_pos_buffer = m_uv_buffer = m_tex_buffer = m_index_buffer = m_vao = 0;

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_pos_buffer);
		glGenBuffers(1, &m_uv_buffer);
		glGenBuffers(1, &m_tex_buffer);
		glGenBuffers(1, &m_index_buffer);
		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 3, positions.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_position, 3, GL_FLOAT, false, sizeof(float) * 3, (void*)0);
		glEnableVertexAttribArray(a_position);

		glBindBuffer(GL_ARRAY_BUFFER, m_uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 2, uv.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_uv, 2, GL_FLOAT, false, sizeof(float) * 2, (void*)0);
		glEnableVertexAttribArray(a_uv);

		glBindBuffer(GL_ARRAY_BUFFER, m_tex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 2, domain_uv.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(a_tex_uv, 2, GL_FLOAT, false, sizeof(float) * 2, (void*)0);
		glEnableVertexAttribArray(a_tex_uv);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(static_cast<GLuint>(NULL));
	}

	void torus_object::m_free_geometry() {
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_pos_buffer);
		glDeleteBuffers(1, &m_uv_buffer);
		glDeleteBuffers(1, &m_tex_buffer);
		glDeleteBuffers(1, &m_index_buffer);
	}

	// differentiable surface interface
	constexpr float DPI = 2.0f * glm::pi<float>();

	float torus_object::get_min_u() const {
		return 0.0f;
	}

	float torus_object::get_max_u() const {
		return 1.0f;
	}

	float torus_object::get_min_v() const {
		return 0.0f;
	}

	float torus_object::get_max_v() const {
		return 1.0f;
	}

	// TODO: optimize world matrix being recaltulated each step
	glm::vec3 torus_object::sample(float u, float v) const {
		u = u * DPI;
		v = v * DPI;

		float A = (m_outer_radius + m_inner_radius) / 2.0f;
		float B = (m_outer_radius - m_inner_radius) / 2.0f;

		float x = cos(v) * (A + B * cos(u));
		float y = sin(v) * (A + B * cos(u));
		float z = B * sin(u);

		auto world_matrix = get_matrix();
		auto world_pos = world_matrix * glm::vec4{ x, y, z, 1.0f }; // affine

		return world_pos;
	}

	glm::vec3 torus_object::normal(float u, float v) const {
		auto d1 = ddv(u, v);
		auto d2 = ddu(u, v);

		return glm::cross(d1, d2);
	}

	glm::vec3 torus_object::ddu(float u, float v) const {
		u = u * DPI;
		v = v * DPI;

		float A = (m_outer_radius + m_inner_radius) / 2.0f;
		float B = (m_outer_radius - m_inner_radius) / 2.0f;

		float grad_x = -B * sin(u) * cos(v);
		float grad_y = -B * sin(u) * sin(v);
		float grad_z = B * cos(u);

		auto world_matrix = get_matrix();
		auto world_grad = world_matrix * glm::vec4{ grad_x, grad_y, grad_z, 0.0f };

		return world_grad;
	}

	glm::vec3 torus_object::ddv(float u, float v) const {
		u = u * DPI;
		v = v * DPI;

		float A = (m_outer_radius + m_inner_radius) / 2.0f;
		float B = (m_outer_radius - m_inner_radius) / 2.0f;

		float grad_x = -sin(v) * (A + B * cos(u));
		float grad_y = cos(v) * (A + B * cos(u));
		float grad_z = 0.0f;

		auto world_matrix = get_matrix();
		auto world_grad = world_matrix * glm::vec4{ grad_x, grad_y, grad_z, 0.0f };

		return world_grad;
	}

	bool torus_object::is_u_wrapped() const {
		return true;
	}

	bool torus_object::is_v_wrapped() const {
		return true;
	}

	bool torus_object::is_trimmable() const {
		return true;
	}

	trimmable_surface_domain& torus_object::get_trimmable_domain() {
		return m_domain;
	}
}
