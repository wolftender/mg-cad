#pragma once
#include "object.hpp"
#include "bezier.hpp"

namespace mini {
	class bezier_patch_c0 : public scene_obj_t {
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

			bezier_patch_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader, 
				std::shared_ptr<shader_t> grid_shader, unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points);
			~bezier_patch_c0 ();

			bezier_patch_c0 (const bezier_patch_c0 &) = delete;
			bezier_patch_c0 & operator= (const bezier_patch_c0 &) = delete;

			virtual void configure () override;
			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		private:
			void m_bind_shader (app_context & context, shader_t & shader, const glm::mat4x4 & world_matrix) const;
			void m_rebuild_buffers ();
			bool m_calc_pos_buffer ();
			void m_calc_idx_buffer ();
			void m_update_buffers ();
			void m_destroy_buffers ();
			void m_moved_sighandler (signal_event_t sig, scene_obj_t & sender);
			void m_setup_signals ();
	};

	class bezier_patch_c0_template : public scene_obj_t {
		private:
			enum class build_mode_t {
				mode_default,
				mode_cylinder,
				mode_hat
			};

			std::shared_ptr<bezier_patch_c0> m_patch;
			std::vector<point_ptr> m_points;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_solid_shader;
			std::shared_ptr<shader_t> m_grid_shader;
			std::shared_ptr<shader_t> m_point_shader;
			std::shared_ptr<texture_t> m_point_texture;

			int m_patches_x;
			int m_patches_y;

			build_mode_t m_build_mode;
			int m_combo_item;
			bool m_rebuild, m_added;

		public:
			bezier_patch_c0_template (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader, 
				std::shared_ptr<shader_t> grid_shader, std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, 
				unsigned int patches_x, unsigned int patches_y);

			bezier_patch_c0_template (const bezier_patch_c0_template &) = delete;
			bezier_patch_c0_template & operator= (const bezier_patch_c0_template &) = delete;

			virtual void configure () override;
			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		private:
			void m_add_to_scene ();
			void m_rebuild_surface (build_mode_t mode);
	};
}