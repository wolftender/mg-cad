#include "diffdebug.hpp"
#include "gui.hpp"

namespace mini {
	diff_surface_debugger::diff_surface_debugger(
		scene_controller_base& scene, 
		std::shared_ptr<shader_t> line_shader, 
		std::weak_ptr<scene_obj_t> surface,
		diff_surface_ref diff_surface) : 
		scene_obj_t(scene, "differential debugger", false, false, false) {

		m_diff_surface = diff_surface;
		m_surface = surface;
		
		m_rebuild_queued = false;
		m_signals_setup = false;
		m_scale = 1.0f;

		m_res_u = m_res_v = 25U;
		m_color = { 1.0f, 0.0f, 0.0f, 1.0f };

		m_line_shader = line_shader;

		m_rebuild_buffer();

		// signal handlers
		t_set_handler(signal_event_t::changed, std::bind(&diff_surface_debugger::m_changed_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		t_set_handler(signal_event_t::moved, std::bind(&diff_surface_debugger::m_changed_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		t_set_handler(signal_event_t::rotated, std::bind(&diff_surface_debugger::m_changed_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		t_set_handler(signal_event_t::scaled, std::bind(&diff_surface_debugger::m_changed_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		t_set_handler(signal_event_t::topology, std::bind(&diff_surface_debugger::m_topology_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));
	}

	diff_surface_debugger::~diff_surface_debugger() {}

	int diff_surface_debugger::get_res_u() const {
		return m_res_u;
	}

	int diff_surface_debugger::get_res_v() const {
		return m_res_v;
	}

	void diff_surface_debugger::set_res_u(int res_u) {
		m_res_u = res_u;
	}

	void diff_surface_debugger::set_res_v(int res_v) {
		m_res_v = res_v;
	}

	void diff_surface_debugger::integrate(float delta_time) {
		gui::clamp(m_res_u, 0, 64);
		gui::clamp(m_res_v, 0, 64);

		if (!m_signals_setup) {
			m_setup_signals();
		}

		if (m_rebuild_queued) {
			m_rebuild_buffer();
			m_rebuild_queued = false;
		}
	}

	void diff_surface_debugger::render(app_context& context, const glm::mat4x4& world_matrix) const {
		m_segments->set_color(m_color);
		context.draw(m_segments, world_matrix);
	}

	void diff_surface_debugger::configure() {
		if (ImGui::CollapsingHeader("Debugger Settings")) {
			gui::prefix_label("u Resolution ", 250.0f);
			if (ImGui::InputInt("##dsd_res_u", &m_res_u)) {
				m_rebuild_queued = true;
			}

			gui::prefix_label("v Resolution ", 250.0f);
			if (ImGui::InputInt("##dsd_res_v", &m_res_v)) {
				m_rebuild_queued = true;
			}

			gui::prefix_label("Vec. Scale: ", 250.0f);
			if (ImGui::InputFloat("##dsd_scale", &m_scale)) {
				gui::clamp(m_scale, 0.1f, 5.0f);
				m_rebuild_queued = true;
			}

			gui::prefix_label("Color: ", 250.0f);
			gui::color_editor("##dsd_color", m_color);
			ImGui::NewLine();
		}
	}

	void diff_surface_debugger::t_on_object_deleted(std::shared_ptr<scene_obj_t> object) {
		if (object->get_id() == m_surface_id) {
			dispose();
		}
	}

	void diff_surface_debugger::m_setup_signals() {
		auto surface = m_surface.lock();

		if (surface) {
			t_listen(signal_event_t::scaled, *surface);
			t_listen(signal_event_t::rotated, *surface);
			t_listen(signal_event_t::moved, *surface);
			t_listen(signal_event_t::changed, *surface);
			t_listen(signal_event_t::topology, *surface);
		}

		m_signals_setup = true;
	}

	void diff_surface_debugger::m_changed_sighandler(signal_event_t sig, scene_obj_t& sender) {
		m_rebuild_queued = true;
	}

	void diff_surface_debugger::m_topology_sighandler(signal_event_t sig, scene_obj_t& sender) {
		dispose();
	}

	void diff_surface_debugger::m_rebuild_buffer() {
		auto surface = m_diff_surface.lock();

		if (surface) {
			m_segments = std::make_shared<segments_array>(m_line_shader, m_res_u * m_res_v * 2);
			
			const float w = surface->get_max_u() - surface->get_min_u();
			const float h = surface->get_max_v() - surface->get_min_v();
			const float su = w / static_cast<float>(m_res_u);
			const float sv = h / static_cast<float>(m_res_v);

			for (unsigned int iu = 0; iu < m_res_u; ++iu) {
				for (unsigned int iv = 0; iv < m_res_v; ++iv) {
					unsigned int b = ((iu * m_res_v) + iv) * 2;

					const float u = iu * su;
					const float v = iv * sv;

					auto p = surface->sample(u, v);
					auto n = p + m_scale * surface->normal(u, v);

					m_segments->update_point(b + 0, p);
					m_segments->update_point(b + 1, n);

					m_segments->add_segment(b + 0, b + 1);
				}
			}
		}

		m_segments->rebuild_buffers();
	}
}