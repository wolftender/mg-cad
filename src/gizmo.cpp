#include "gizmo.hpp"
#include "algebra.hpp"

namespace mini {
	gizmo::gizmo_mode_t gizmo::get_mode () const {
		return m_mode;
	}

	void gizmo::set_mode (gizmo_mode_t mode) {
		m_mode;
	}

	gizmo::gizmo (std::shared_ptr<shader_t> shader_mesh, std::shared_ptr<shader_t> shader_line) {
		m_shader_mesh = shader_mesh;
		m_shader_line = shader_line;
		m_mode = gizmo_mode_t::translation;

		m_gen_arrow_mesh (m_mesh_arrow);
	}

	gizmo::~gizmo () {
		m_free_mesh (m_mesh_arrow);
	}

	void gizmo::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_mode == gizmo_mode_t::translation) {
			m_render_translation (context, world_matrix);
		}
	}

	inline float line_distance (const glm::vec3 & a1, const glm::vec3 & b1, const glm::vec3 & a2, const glm::vec3 & b2) {
		glm::vec3 c = glm::cross (b1, b2);
		float num = glm::dot (c, a2 - a1);
		float den = glm::length (c);

		return glm::abs (num / den);
	}

	inline float line_distance (const glm::vec3 & a, const glm::vec3 & n, const glm::vec3 & p) {
		return glm::length (glm::cross (p - a, n)) / glm::length (n);
	}

	gizmo::gizmo_action_t gizmo::get_action (hit_test_data_t & hit_data, const glm::vec3 & center) const {
		constexpr glm::vec3 axis_x = { 1.0f, 0.0f, 0.0f };
		constexpr glm::vec3 axis_y = { 0.0f, 1.0f, 0.0f };
		constexpr glm::vec3 axis_z = { 0.0f, 0.0f, 1.0f };

		if (m_mode == gizmo_mode_t::translation) {
			// check collision with arrows, if collides then return correct translate
			bool collides_x = false, collides_y = false, collides_z = false;

			const auto & cam_pos = hit_data.camera.get_position ();

			float cam_dist = glm::distance (cam_pos, center);
			float world_size = glm::tan (glm::pi<float> () / 3.0f) * cam_dist * 0.018f;

			constexpr float off = 1.9f;

			float center_x_dist = line_distance (cam_pos, hit_data.mouse_ray, center + world_size * axis_x * off);
			float center_y_dist = line_distance (cam_pos, hit_data.mouse_ray, center - world_size * axis_y * off);
			float center_z_dist = line_distance (cam_pos, hit_data.mouse_ray, center - world_size * axis_z * off);

			collides_x = (line_distance (center, { 1.0f, 0.0f, 0.0f }, cam_pos, hit_data.mouse_ray) < 0.3f * world_size) && (center_x_dist < world_size * off);
			collides_y = (line_distance (center, { 0.0f, 1.0f, 0.0f }, cam_pos, hit_data.mouse_ray) < 0.3f * world_size) && (center_y_dist < world_size * off);
			collides_z = (line_distance (center, { 0.0f, 0.0f, 1.0f }, cam_pos, hit_data.mouse_ray) < 0.3f * world_size) && (center_z_dist < world_size * off);

			if (collides_x) {
				return gizmo_action_t::translate_x;
			} else if (collides_y) {
				return gizmo_action_t::translate_y;
			} else if (collides_z) {
				return gizmo_action_t::translate_z;
			}
		}

		return gizmo_action_t::none;
	}

	void gizmo::m_render_translation (app_context & context, const glm::mat4x4 & world_matrix) const {
		glBindVertexArray (m_mesh_arrow.vao);
		m_shader_mesh->bind ();

		const auto & view_matrix = context.get_view_matrix ();
		const auto & proj_matrix = context.get_projection_matrix ();

		auto local = make_translation ({ 0.0f, 1.0f, 0.0f });

		glm::vec3 center = world_matrix * glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f };
		float dist = glm::distance (context.get_camera ().get_position (), center);
		float world_size = 2.0f * glm::tan (glm::pi<float>() / 3.0f) * dist * 0.018f;

		auto scale = make_scale ({ world_size, world_size, world_size });

		glm::mat4x4 up = scale * make_rotation_x (glm::pi<float> ()) * local;
		glm::mat4x4 forward = scale * make_rotation_z (-glm::pi<float> () * 0.5f) * local;
		glm::mat4x4 left = scale * make_rotation_x (-glm::pi<float> () * 0.5f) * local;

		m_shader_mesh->set_uniform ("u_view", view_matrix);
		m_shader_mesh->set_uniform ("u_projection", proj_matrix);

		m_shader_mesh->set_uniform ("u_world", world_matrix * up);
		m_shader_mesh->set_uniform ("u_color", glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
		glDrawElements (GL_TRIANGLES, m_mesh_arrow.indices.size (), GL_UNSIGNED_INT, NULL);

		m_shader_mesh->set_uniform ("u_world", world_matrix * forward);
		m_shader_mesh->set_uniform ("u_color", glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
		glDrawElements (GL_TRIANGLES, m_mesh_arrow.indices.size (), GL_UNSIGNED_INT, NULL);

		m_shader_mesh->set_uniform ("u_world", world_matrix * left);
		m_shader_mesh->set_uniform ("u_color", glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f });
		glDrawElements (GL_TRIANGLES, m_mesh_arrow.indices.size (), GL_UNSIGNED_INT, NULL);

		glBindVertexArray (0);
	}

	void gizmo::m_gen_arrow_mesh (gizmo_mesh_t & mesh) {
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		std::vector<float> positions;
		std::vector<GLuint> indices;

		// arrow consists of 1 vertex on top and 3 rings of vertices and one on the bottom
		const int res = 20;
		const float r1 = 0.15f;
		const float r2 = 0.05f;
		const float r3 = 0.05f;
		const float h = 0.5f;

		int num_vertices = res * 3 + 2;

		positions.resize (num_vertices * 3);
		positions[0] = 0.0f;
		positions[1] = 1.0f;
		positions[2] = 0.0f;

		int v = 1;
		for (int i = 0; i < res; ++i, ++v) {
			int b = 3 * v;
			float t = glm::pi<float>() * 2.0f / static_cast<float> (res) * static_cast<float>(i);

			positions[b + 0] = r1 * glm::cos(t);
			positions[b + 1] = h;
			positions[b + 2] = r1 * glm::sin(t);
		}

		for (int i = 0; i < res; ++i, ++v) {
			int b = 3 * v;
			float t = glm::pi<float> () * 2.0f / static_cast<float> (res) * static_cast<float>(i);

			positions[b + 0] = r2 * glm::cos (t);
			positions[b + 1] = h;
			positions[b + 2] = r2 * glm::sin (t);
		}

		for (int i = 0; i < res; ++i, ++v) {
			int b = 3 * v;
			float t = glm::pi<float> () * 2.0f / static_cast<float> (res) * static_cast<float>(i);

			positions[b + 0] = r3 * glm::cos (t);
			positions[b + 1] = -1.0f;
			positions[b + 2] = r3 * glm::sin (t);
		}

		int b = 3 * v;
		positions[b + 0] = 0.0f;
		positions[b + 1] = -1.0f;
		positions[b + 2] = 0.0f;

		// construct indices
		int b1 = 1;
		int b2 = 1 + res;
		int b3 = 1 + (res * 2);
		int f = num_vertices - 1;

		indices.reserve (res * 6);
		for (int i = 0; i < res; ++i) {
			indices.push_back (b1 + ((i + 1) % res));
			indices.push_back (b1 + i);
			indices.push_back (0);
		}

		for (int i = 0; i < res; ++i) {
			indices.push_back (b2 + i);
			indices.push_back (b1 + i);
			indices.push_back (b1 + ((i + 1) % res));

			indices.push_back (b2 + ((i + 1) % res));
			indices.push_back (b2 + i);
			indices.push_back (b1 + ((i + 1) % res));
		}

		for (int i = 0; i < res; ++i) {
			indices.push_back (b3 + i);
			indices.push_back (b2 + i);
			indices.push_back (b2 + ((i + 1) % res));

			indices.push_back (b3 + ((i + 1) % res));
			indices.push_back (b3 + i);
			indices.push_back (b2 + ((i + 1) % res));
		}

		for (int i = 0; i < res; ++i) {
			indices.push_back (b3 + ((i + 1) % res));
			indices.push_back (f);
			indices.push_back (b3 + i);
		}

		glGenVertexArrays (1, &mesh.vao);
		glGenBuffers (1, &mesh.pos_buffer);
		glGenBuffers (1, &mesh.index_buffer);

		glBindVertexArray (mesh.vao);
		glBindBuffer (GL_ARRAY_BUFFER, mesh.pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * positions.size (), positions.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * indices.size (), indices.data (), GL_STATIC_DRAW);

		glBindVertexArray (0);
		
		mesh.indices = indices;
		mesh.positions = positions;
	}

	void gizmo::m_free_mesh (gizmo_mesh_t & mesh) {
		if (mesh.vao) {
			glDeleteVertexArrays (1, &mesh.vao);
			mesh.vao = 0;
		}

		if (mesh.pos_buffer) {
			glDeleteBuffers (1, &mesh.pos_buffer);
			mesh.pos_buffer = 0;
		}

		if (mesh.index_buffer) {
			glDeleteBuffers (1, &mesh.index_buffer);
			mesh.index_buffer = 0;
		}
	}
}