#include "bezier.hpp"
#include "gui.hpp"

namespace mini {
	bezier_curve_c0::bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader) : 
		scene_obj_t (scene, "bezier c0", false, false, false) {

		for (auto iter = get_scene ().get_selected_objects (); iter->has (); iter->next ()) {
			auto object = std::dynamic_pointer_cast<point_object> (iter->get_object ());

			if (object) {
				m_points.push_back (object);
			}
		}
		
		m_auto_extend = false;
		m_queue_curve_rebuild = false;

		m_rebuild_curve ();
	}

	bezier_curve_c0::~bezier_curve_c0 () {
		
	}

	void bezier_curve_c0::integrate (float delta_time) { 
		if (m_queue_curve_rebuild) {
			m_rebuild_curve ();
		}
	}

	void bezier_curve_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		
	}

	void bezier_curve_c0::configure () {
		gui::prefix_label ("Auto Extend: ", 250.0f);
		ImGui::Checkbox ("##auto_extend", &m_auto_extend);
		ImGui::NewLine ();
	}

	void bezier_curve_c0::t_on_object_created (std::shared_ptr<scene_obj_t> object) {
		if (!m_auto_extend) {
			return;
		}

		auto point = std::dynamic_pointer_cast<point_object> (object);

		if (point) {
			m_points.push_back (point);
			m_queue_curve_rebuild = true;
		}
	}

	void bezier_curve_c0::t_on_object_deleted (std::shared_ptr<scene_obj_t> object) {
		bool changed = false;

		for (auto iter = m_points.begin (); iter != m_points.end (); ++iter) {
			auto point = iter->lock ();

			if (point) {
				if (object == point) {
					iter = m_points.erase (iter);
					changed = true;
				}
			} else {
				iter = m_points.erase (iter);
				changed = true;
			}

			if (iter == m_points.end ()) {
				break;
			}
		}

		if (changed) {
			m_queue_curve_rebuild = true;
		}
	}

	void bezier_curve_c0::m_rebuild_curve () {
		m_queue_curve_rebuild = false;
	}
}