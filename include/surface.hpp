#pragma once
#include "object.hpp"
#include "bezier.hpp"

namespace mini {
	class bicubic_surface : public scene_obj_t {
		private:
			static constexpr unsigned int num_control_points = 16;

			std::vector<point_ptr> m_points;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_solid_shader;
			std::shared_ptr<shader_t> m_grid_shader;

			const unsigned int m_patches_x;
			const unsigned int m_patches_y;

			int m_res_u;
			int m_res_v;

			bool m_use_solid;
			bool m_use_wireframe;
			bool m_queued_update;

			bool m_show_polygon;
			bool m_ready;
			bool m_signals_setup;

			GLuint m_vao, m_grid_vao;
			GLuint m_pos_buffer, m_index_buffer, m_grid_index_buffer;

			glm::vec4 m_color;

			std::vector<float> m_positions;
			std::vector<GLuint> m_indices;
			std::vector<GLuint> m_grid_indices;

		protected:
			const std::vector<point_ptr> & t_get_points () const;

		public:
			bool is_showing_polygon () const;
			void set_showing_polygon (bool show);

			unsigned int get_patches_x () const;
			unsigned int get_patches_y () const;

			unsigned int get_num_points () const;
			unsigned int get_num_patches () const;

			int get_res_u () const;
			int get_res_v () const;

			void set_res_u (int u);
			void set_res_v (int v);

			bicubic_surface (
				const std::string & type_name, 
				scene_controller_base & scene, 
				std::shared_ptr<shader_t> shader, 
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader, 
				unsigned int patches_x, 
				unsigned int patches_y, 
				const std::vector<point_ptr> & points
			);

			bicubic_surface (
				const std::string & type_name, 
				scene_controller_base & scene, 
				std::shared_ptr<shader_t> shader, 
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader, 
				unsigned int patches_x, 
				unsigned int patches_y, 
				const std::vector<point_ptr> & points,
				const std::vector<GLuint> topology
			);

			virtual ~bicubic_surface ();

			bicubic_surface (const bicubic_surface &) = delete;
			bicubic_surface & operator= (const bicubic_surface &) = delete;

			virtual void configure () override;
			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

			using serialized_patch = std::array<uint64_t, 16>;

			std::vector<uint64_t> serialize_points ();
			std::vector<serialized_patch> serialize_patches ();

		private:
			void m_bind_shader (app_context & context, shader_t & shader, const glm::mat4x4 & world_matrix) const;
			void m_rebuild_buffers (bool recalculate_indices);
			bool m_calc_pos_buffer ();
			void m_calc_idx_buffer ();
			void m_update_buffers ();
			void m_destroy_buffers ();
			void m_moved_sighandler (signal_event_t sig, scene_obj_t & sender);
			void m_setup_signals ();
	};
}