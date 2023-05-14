#pragma once
#include "object.hpp"
#include "bezier.hpp"

namespace mini {
	class bezier_patch_c0 : public scene_obj_t {
		private:
			std::vector<point_wptr> m_points;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_grid_shader;

			const unsigned int m_patches_x;
			const unsigned int m_patches_y;
			bool m_show_polygon;

		protected:
			const std::vector<point_wptr> & t_get_points () const;

		public: 
			bool is_showing_polygon () const;
			void set_showing_polygon (bool show);

			unsigned int get_patches_x () const;
			unsigned int get_patches_y () const;

			bezier_patch_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> grid_shader, 
				unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points);

			bezier_patch_c0 (const bezier_patch_c0 &) = delete;
			bezier_patch_c0 & operator= (const bezier_patch_c0 &) = delete;
	};

	class bezier_patch_c0_template : public scene_obj_t {
		private:
			std::unique_ptr<bezier_patch_c0> m_patch;
			std::vector<point_ptr> m_points;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_grid_shader;
			std::shared_ptr<shader_t> m_point_shader;
			std::shared_ptr<texture_t> m_point_texture;

			unsigned int m_patches_x;
			unsigned int m_patches_y;

		public:
			bezier_patch_c0_template (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> grid_shader, 
				std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, unsigned int patches_x, unsigned int patches_y);

			bezier_patch_c0_template (const bezier_patch_c0_template &) = delete;
			bezier_patch_c0_template & operator= (const bezier_patch_c0_template &) = delete;

			virtual void configure () override;
			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		private:
			void m_rebuild_surface ();
	};
}