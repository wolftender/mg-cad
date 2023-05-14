#pragma once
#include <unordered_map>
#include <memory>

#include "shader.hpp"
#include "texture.hpp"

namespace mini {
	class resource_store final {
		private:
			std::shared_ptr<shader_t> m_basic_shader, m_grid_xz_shader, m_grid_xy_shader;
			std::shared_ptr<shader_t> m_billboard_shader, m_billboard_shader_s;
			std::shared_ptr<shader_t> m_mesh_shader, m_alt_mesh_shader;
			std::shared_ptr<shader_t> m_bezier_shader, m_bezier_poly_shader;
			std::shared_ptr<shader_t> m_line_shader;
			std::shared_ptr<shader_t> m_bezier_surf_shader;

			std::shared_ptr<texture_t> m_cursor_texture, m_point_texture;

		public:
			std::shared_ptr<shader_t> get_basic_shader () const;
			std::shared_ptr<shader_t> get_grid_xz_shader () const;
			std::shared_ptr<shader_t> get_grid_xy_shader () const;
			std::shared_ptr<shader_t> get_billboard_shader () const;
			std::shared_ptr<shader_t> get_billboard_s_shader () const;
			std::shared_ptr<shader_t> get_mesh_shader () const;
			std::shared_ptr<shader_t> get_alt_mesh_shader () const;
			std::shared_ptr<shader_t> get_bezier_shader () const;
			std::shared_ptr<shader_t> get_bezier_poly_shader () const;
			std::shared_ptr<shader_t> get_line_shader () const;
			std::shared_ptr<shader_t> get_bezier_surf_shader () const;

			std::shared_ptr<texture_t> get_cursor_texture () const;
			std::shared_ptr<texture_t> get_point_texture () const;

		public:
			resource_store ();
			~resource_store () = default;

			resource_store (const resource_store &) = delete;
			resource_store & operator= (const resource_store &) = delete;

		private:
			std::string m_read_file_content (const std::string & path) const;
			std::shared_ptr<shader_t> m_load_shader (const std::string & vs_file, const std::string & ps_file) const;
			std::shared_ptr<shader_t> m_load_shader (const std::string & vs_file, const std::string & ps_file, const std::string & gs_file) const;
			std::shared_ptr<shader_t> m_load_shader (const std::string & vs_file, const std::string & ps_file, 
				const std::string & tcs_file, const std::string & tes_file) const;
	};
}