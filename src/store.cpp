#include "store.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <ios>

namespace fs = std::filesystem;

namespace mini {
	std::shared_ptr<shader_t> resource_store::m_load_shader (const std::string & vs_file, const std::string & ps_file) const {
		const std::string vs_source = m_read_file_content (vs_file);
		const std::string ps_source = m_read_file_content (ps_file);

		auto shader = std::make_shared<shader_t> (vs_source, ps_source);
		
		try {
			shader->compile ();
		} catch (const shader_error_t & error) {
			std::cerr << "failed to build shader! log: " << std::endl << error.get_log () << std::endl;
			return nullptr;
		}

		return shader;
	}

	std::string resource_store::m_read_file_content (const std::string & path) const {
		std::ifstream stream (path);

		if (stream) {
			std::stringstream ss;
			ss << stream.rdbuf ();

			return ss.str ();
		}

		throw std::runtime_error ("failed to read file " + path);
	}

	std::shared_ptr<shader_t> resource_store::get_basic_shader () const {
		return m_basic_shader;
	}

	std::shared_ptr<shader_t> resource_store::get_grid_xz_shader () const {
		return m_grid_xz_shader;
	}

	std::shared_ptr<shader_t> resource_store::get_grid_xy_shader () const {
		return m_grid_xy_shader;
	}

	std::shared_ptr<shader_t> resource_store::get_billboard_shader () const {
		return m_billboard_shader;
	}

	std::shared_ptr<shader_t> resource_store::get_billboard_s_shader () const {
		return m_billboard_shader_s;
	}

	std::shared_ptr<shader_t> resource_store::get_mesh_shader () const {
		return m_mesh_shader;
	}

	std::shared_ptr<shader_t> resource_store::get_alt_mesh_shader () const {
		return m_alt_mesh_shader;
	}

	resource_store::resource_store () {
		m_basic_shader = m_load_shader ("shaders/vs_basic.glsl", "shaders/fs_basic.glsl");
		m_mesh_shader = m_load_shader ("shaders/vs_meshgrid.glsl", "shaders/fs_meshgrid.glsl");
		m_alt_mesh_shader = m_load_shader ("shaders/vs_meshgrid.glsl", "shaders/fs_meshgrid_s.glsl");
		m_grid_xz_shader = m_load_shader ("shaders/vs_grid.glsl", "shaders/fs_grid_xz.glsl");
		m_grid_xy_shader = m_load_shader ("shaders/vs_grid.glsl", "shaders/fs_grid_xy.glsl");
		m_billboard_shader = m_load_shader ("shaders/vs_billboard.glsl", "shaders/fs_billboard.glsl");
		m_billboard_shader_s = m_load_shader ("shaders/vs_billboard_s.glsl", "shaders/fs_billboard.glsl");
	}
}