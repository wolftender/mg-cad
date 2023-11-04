#include <vector>

#include "app.hpp"
#include "surface.hpp"
#include "beziersurf.hpp"
#include "intersection.hpp"

#include "milling/milling.hpp"

namespace mini {
	padlock_milling_script::padlock_milling_script(application& app) : m_app(app) { 
		m_padlock_body = nullptr;
		m_padlock_keyhole = nullptr;
		m_padlock_shackle = nullptr;
	}

	padlock_milling_script::~padlock_milling_script() { }

	void padlock_milling_script::run() {
		m_prepare_base();
		m_identify_model();
	}

	void padlock_milling_script::m_prepare_base() {
		auto& store = m_app.m_store;

		// first create a new surface in the scene, this will be the base of the model
		point_list points;

		for (int x = 0; x < 4; ++x) {
			for (int y = 0; y < 4; ++y) {
				glm::vec3 position{
					-6.0f + x * 4.0f,
					0.0f,
					-6.0f + y * 4.0f,
				};

				const auto point_obj = std::make_shared<point_object>(m_app,
					store->get_billboard_s_shader(),
					store->get_point_texture());

				point_obj->set_translation(position);
				point_obj->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });

				m_app.add_object("milling_point", point_obj);
				points.push_back(point_obj);
			}
		}

		const auto base_surf = std::make_shared<bezier_surface_c0>(
			m_app,
			store->get_bezier_surf_shader(),
			store->get_bezier_surf_solid_shader(),
			store->get_line_shader(),
			1, 1,
			points,
			false, false);

		m_app.add_object("milling_base", base_surf);
		m_model_base = base_surf;

		m_model_base->integrate(0.0f);
	}

	void padlock_milling_script::m_identify_model() {
		std::shared_ptr<application::object_wrapper_t> body;
		std::shared_ptr<application::object_wrapper_t> shackle;
		std::shared_ptr<application::object_wrapper_t> keyhole;

		for (auto& object_wrapper : m_app.m_objects) {
			auto object = object_wrapper->object;

			if (object->get_name() == "padlock_body") {
				m_padlock_body = std::dynamic_pointer_cast<bspline_surface>(object);
				body = object_wrapper;
			} else if (object->get_name() == "padlock_shackle") {
				m_padlock_shackle = std::dynamic_pointer_cast<bspline_surface>(object);
				shackle = object_wrapper;
			} else if (object->get_name() == "padlock_keyhole") {
				m_padlock_keyhole = std::dynamic_pointer_cast<bezier_surface_c0>(object);
				keyhole = object_wrapper;
			}
		}

		// display settings
		m_padlock_body->set_showing_polygon(false);
		m_padlock_shackle->set_showing_polygon(false);
		m_padlock_keyhole->set_showing_polygon(false);
		m_app.m_points_enabled = false;

		intersection_controller controller1(m_app, m_padlock_body, m_padlock_keyhole, m_app.m_store, false);

		// intersect shackle
		m_app.set_cursor_pos({ 0.9f, 0.0f, -1.2f });
		intersection_controller controller2(m_app, m_padlock_body, m_padlock_shackle, m_app.m_store, true);
		m_app.set_cursor_pos({ -0.9f, 0.0f, -1.2f });
		intersection_controller controller3(m_app, m_padlock_body, m_padlock_shackle, m_app.m_store, true);

		m_app.clear_selection();

		// trim surfaces automatically from hardcoded points
		m_padlock_keyhole->get_domain().trim(0.22f, 0.45f);

		m_padlock_body->get_domain().trim(0.55f, 0.4f);
		m_padlock_body->get_domain().trim(0.42f, 0.134f);
		m_padlock_body->get_domain().trim(0.676f, 0.134f);

		m_padlock_shackle->get_domain().trim(0.226f, 0.015f);
		m_padlock_shackle->get_domain().trim(0.089f, 0.970f);

		intersection_controller controller4(m_app, m_padlock_body, m_model_base, m_app.m_store, false);
		m_padlock_body->get_domain().trim(0.14f, 0.14f);

		// intersect shackle
		m_app.set_cursor_pos({ 1.87f, 0.0f, -1.132f });
		intersection_controller controller5(m_app, m_padlock_shackle, m_model_base, m_app.m_store, true);

		m_app.set_cursor_pos({ 0.85f, 0.0f, -1.154f });
		intersection_controller controller6(m_app, m_padlock_shackle, m_model_base, m_app.m_store, true);

		m_padlock_shackle->get_domain().trim(0.148f, 0.475f);
		m_padlock_shackle->get_domain().trim(0.869f, 0.475f);

		m_app.clear_selection();
	}
}