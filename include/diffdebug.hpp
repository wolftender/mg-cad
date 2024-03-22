#pragma once
#include "object.hpp"
#include "segments.hpp"
#include "surface.hpp"

namespace mini {
	using diff_surface_ref = std::weak_ptr<differentiable_surface_base>;

	class diff_surface_debugger : public scene_obj_t {
		private:
			std::weak_ptr<scene_obj_t> m_surface;
			diff_surface_ref m_diff_surface;

			uint64_t m_surface_id;

			std::shared_ptr<shader_t> m_line_shader;
			std::shared_ptr<segments_array> m_segments;

			int m_res_u;
			int m_res_v;

			glm::vec4 m_color;
			float m_scale;

			bool m_rebuild_queued, m_signals_setup;

		public:
			diff_surface_debugger(
				scene_controller_base& scene, 
				std::shared_ptr<shader_t> line_shader,
				std::weak_ptr<scene_obj_t> surface,
				diff_surface_ref diff_surface);

			~diff_surface_debugger();
			
			diff_surface_debugger(const diff_surface_debugger&) = delete;
			diff_surface_debugger& operator=(const diff_surface_debugger&) = delete;

			int get_res_u() const;
			int get_res_v() const;

			void set_res_u(int res_u);
			void set_res_v(int res_v);

			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context, const glm::mat4x4& world_matrix) const override;
			virtual void configure() override;

		protected:
			virtual void t_on_object_deleted(std::shared_ptr<scene_obj_t> object) override;

		private:
			void m_setup_signals();
			void m_changed_sighandler(signal_event_t sig, scene_obj_t& sender);
			void m_topology_sighandler(signal_event_t sig, scene_obj_t& sender);

			void m_rebuild_buffer();
	};
}