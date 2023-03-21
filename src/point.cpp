#include "point.hpp"

namespace mini {
	point_object::point_object (std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture) : 
		scene_obj_t ("point", true, false, false),
		m_billboard (shader, texture) { 

		m_billboard.set_size ({ 16.0f, 16.0f });
	}

	void point_object::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		m_billboard.render (context, world_matrix);
	}

	void point_object::configure () {
		return scene_obj_t::configure ();
	}

	void point_object::t_on_selection (bool selected) {
		if (selected) {
			m_billboard.set_color_tint (glm::vec4 (0.960f, 0.646f, 0.0192f, 1.0f));
		} else {
			m_billboard.set_color_tint (glm::vec4 (1.0f, 1.0f, 1.0f, 1.0f));
		}
	}
}