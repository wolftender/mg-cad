#pragma once
#include "object.hpp"
#include "bezier.hpp"
#include "surface.hpp"

namespace mini {
	class surface_template_base : public scene_obj_t {
		public:
			enum class build_mode_t {
				mode_default,
				mode_cylinder,
				mode_hat
			};

		private:
			int m_patches_x;
			int m_patches_y;

			float m_radius;

			build_mode_t m_build_mode;
			int m_combo_item;
			bool m_rebuild, m_added;

		public:
			int get_patches_x () const;
			int get_pathces_y () const;
			float get_radius () const;
			bool is_rebuild_needed () const;
			bool is_added () const;
			build_mode_t get_build_mode () const;

			void set_patches_x (int patches_x);
			void set_patches_y (int patches_y);
			void set_radius (float radius);
			void set_rebuild_needed (bool rebuild);
			void set_build_mode (build_mode_t mode);

			surface_template_base (
				const std::string & type_name,
				scene_controller_base & scene,
				unsigned int patches_x, 
				unsigned int patches_y);

			surface_template_base (const surface_template_base &) = delete;
			surface_template_base & operator= (const surface_template_base &) = delete;

			virtual ~surface_template_base () { }

			virtual void configure () override;
			virtual void integrate (float delta_time) override;

		protected:
			virtual void t_rebuild (build_mode_t mode) = 0;
			virtual void t_add_to_scene () = 0;

		private:
			void m_rebuild_surface (build_mode_t mode);
	};

	template<typename T> class surface_template : public surface_template_base {
		static_assert (std::is_base_of<bicubic_surface, T> {});
		private:
			std::shared_ptr<T> m_patch;
			std::vector<point_ptr> m_points;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_solid_shader;
			std::shared_ptr<shader_t> m_grid_shader;
			std::shared_ptr<shader_t> m_point_shader;
			std::shared_ptr<texture_t> m_point_texture;

		public:
			surface_template (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader,
				std::shared_ptr<shader_t> point_shader,
				std::shared_ptr<texture_t> point_texture,
				unsigned int patches_x,
				unsigned int patches_y
			) : surface_template_base ("surface_builder", scene, patches_x, patches_y) {

				m_shader = shader;
				m_solid_shader = solid_shader;
				m_grid_shader = grid_shader;
				m_point_shader = point_shader;
				m_point_texture = point_texture;
			}

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override {
				if (m_patch) {
					m_patch->render (context, world_matrix);
				}

				for (const auto & point : m_points) {
					point->render (context, point->get_matrix ());
				}
			}

			virtual void configure () override {
				surface_template_base::configure ();

				if (m_patch) {
					m_patch->configure ();
				}
			}

		protected:
			virtual void t_rebuild (build_mode_t mode) override;
			virtual void t_add_to_scene () override;
	};
}