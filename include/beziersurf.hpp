#pragma once
#include "object.hpp"
#include "bezier.hpp"
#include "surface.hpp"

namespace mini {
	class bezier_surface_c0 : public bicubic_surface {
		public:
			bezier_surface_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader, unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points);

			bezier_surface_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader, unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points,
				const std::vector<GLuint> topology);

			virtual const object_serializer_base & get_serializer () const;
	};

	class bezier_patch_c0_template : public scene_obj_t {
		private:
			enum class build_mode_t {
				mode_default,
				mode_cylinder,
				mode_hat
			};

			std::shared_ptr<bezier_surface_c0> m_patch;
			std::vector<point_ptr> m_points;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_solid_shader;
			std::shared_ptr<shader_t> m_grid_shader;
			std::shared_ptr<shader_t> m_point_shader;
			std::shared_ptr<texture_t> m_point_texture;

			int m_patches_x;
			int m_patches_y;

			float m_radius;

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