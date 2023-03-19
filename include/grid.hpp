#pragma once
#include "context.hpp"

namespace mini {
	class grid_object : public graphics_obj_t {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_t> m_shader;
			float m_spacing;

		public:
			void set_spacing (float spacing);
			float get_spacing () const;

			grid_object (std::shared_ptr<shader_t> shader);
			~grid_object ();

			grid_object (const grid_object &) = delete;
			grid_object & operator= (const grid_object &) = delete;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
	};
}