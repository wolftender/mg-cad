#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

#include <glad/glad.h>

namespace mini {
	class texture_t {
		private:
			uint32_t m_width, m_height, m_mipmap_levels;
			GLenum m_min_filter, m_mag_filter, m_format;

			GLuint m_texture;

			std::vector<uint8_t> m_data;

		public:
			uint32_t get_width () const;
			uint32_t get_height () const;
			uint32_t get_mipmap_levels () const;

			GLenum get_min_filter () const;
			GLenum get_mag_filter () const;
			GLenum get_format () const;

			texture_t (uint32_t width, uint32_t height, unsigned char * data);
			texture_t (uint32_t width, uint32_t height, unsigned char * data, GLenum format, 
				unsigned int mipmap_levels = 0, GLenum min_filter = GL_LINEAR, GLenum mag_filter = GL_LINEAR);

			texture_t (const texture_t &) = delete;
			texture_t & operator= (const texture_t &) = delete;

			~texture_t ();

			void bind (GLenum slot = GL_TEXTURE0) const;

		private:
			void m_initialize ();

		public:
			static std::shared_ptr<texture_t> load_from_file (const std::string & file);
	};
}