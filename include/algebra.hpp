#pragma once
#include <array>
#include <iostream>
#include <type_traits>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace mini {
	// wrappers required by the task
	glm::mat4x4 make_rotation_x (float angle);
	glm::mat4x4 make_rotation_y (float angle);
	glm::mat4x4 make_rotation_z (float angle);
	glm::mat4x4 make_translation (const glm::vec3 & vector);
	glm::mat4x4 make_scale (const glm::vec3 & vector);
}

namespace deprecated_mini {
	constexpr const float pi_f = 3.14159265359f;

	template <typename T> class vector_t {
		static_assert (std::is_arithmetic <T>::value, "T needs to be an arithmetic value");

		private:
			std::array <T, 4> m_buffer;

		public:
			vector_t () {
				for (int index = 0; index < 4; ++index) {
					m_buffer[index] = T ();
				}
			}

			vector_t (std::initializer_list<T> list) {
				int index = 0;
				auto iter = list.begin ();

				while (index < 4 && iter != list.end ()) {
					m_buffer[index] = (*iter);
					index++;
					iter++;
				}

				while (index < 4) m_buffer[index++] = T ();
			}

			vector_t (T a0, T a1, T a2, T a3) {
				m_buffer[0] = a0;
				m_buffer[1] = a1;
				m_buffer[2] = a2;
				m_buffer[3] = a3;
			}

			vector_t (const vector_t<T> &) = default;
			vector_t<T> & operator= (const vector_t<T> &) = default;
			~vector_t () = default;

			T & operator[] (int index) {
				return m_buffer[index];
			}

			const T & operator[] (int index) const {
				return m_buffer[index];
			}

			const T * get_buffer () const {
				return m_buffer.data ();
			}

			T length () const {
				return static_cast<T> (sqrt (
					m_buffer[0] * m_buffer[0] + 
					m_buffer[1] * m_buffer[1] + 
					m_buffer[2] * m_buffer[2]
				));
			}

			// static methods
			static T dot (const vector_t<T> & a, const vector_t<T> & b) {
				return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
			}

			static vector_t<T> cross (const vector_t<T> & a, const vector_t<T> & b) {
				return vector_t<T> (
					((a[1] * b[2]) - (a[2] * b[1])),
					((a[2] * b[0]) - (a[0] * b[2])),
					((a[0] * b[1]) - (a[1] * b[0])), a[3]
				);
			}
	};

	template <typename T> class matrix_t {
		static_assert (std::is_arithmetic <T>::value, "T needs to be an arithmetic value");

		private:
			std::array <T, 16> m_buffer;

		public:
			matrix_t () {
				for (int index = 0; index < 16; ++index) {
					m_buffer[index] = T();
				}
			}

			matrix_t (std::initializer_list<T> list) {
				int index = 0;
				auto iter = list.begin ();

				while (index < 16 && iter != list.end ()) {
					m_buffer[index] = (*iter);
					index++;
					iter++;
				}

				while (index < 16) m_buffer[index++] = T();
			}

			matrix_t (
				T a00, T a01, T a02, T a03,
				T a10, T a11, T a12, T a13,
				T a20, T a21, T a22, T a23,
				T a30, T a31, T a32, T a33
			) {
				m_buffer[0]		= a00;
				m_buffer[1]		= a01;
				m_buffer[2]		= a02;
				m_buffer[3]		= a03;
				m_buffer[4]		= a10;
				m_buffer[5]		= a11;
				m_buffer[6]		= a12;
				m_buffer[7]		= a13;
				m_buffer[8]		= a20;
				m_buffer[9]		= a21;
				m_buffer[10]	= a22;
				m_buffer[11]	= a23;
				m_buffer[12]	= a30;
				m_buffer[13]	= a31;
				m_buffer[14]	= a32;
				m_buffer[15]	= a33;
			}

			matrix_t (const matrix_t<T> &) = default;
			matrix_t<T> & operator= (const matrix_t<T> &) = default;
			~matrix_t () = default;

			T & operator[] (int index) {
				return m_buffer[index];
			}

			const T & operator[] (int index) const {
				return m_buffer[index];
			}

			const T * get_buffer () const {
				return m_buffer.data ();
			}

			static matrix_t<T> transpose (const matrix_t<T> & a) {
				return {
					a[0], a[4], a[8],  a[12],
					a[1], a[5], a[9],  a[13],
					a[2], a[6], a[10], a[14],
					a[3], a[7], a[11], a[15]
				};
			}
	};

	template <typename T> vector_t<T> operator+ (const vector_t<T> & a, const vector_t<T> & b) {
		return vector_t<T> (a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3]);
	}

	template <typename T> vector_t<T> operator- (const vector_t<T> & a, const vector_t<T> & b) {
		return vector_t<T> (a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3]);
	}

	template <typename T> vector_t<T> operator* (T a, const vector_t<T> & b) {
		return vector_t<T> (a * b[0], a * b[1], a * b[2], b[3]);
	}

	template <typename T> matrix_t<T> operator* (const T a, const matrix_t<T> & b) {
		return matrix_t<T> {
			a*b[0],  a*b[1],  a*b[2],  a*b[3],
			a*b[4],  a*b[5],  a*b[6],  a*b[7],
			a*b[8],  a*b[9],  a*b[10], a*b[11],
			a*b[12], a*b[13], a*b[14], a*b[15],
		};
	}

	template <typename T> vector_t<T> operator* (const matrix_t<T> & t, const vector_t<T> & v) {
		return vector_t<T> {
			t[0] * v[0] + t[1] * v[1] + t[2] * v[2] + t[3] * v[3],
			t[4] * v[0] + t[5] * v[1] + t[6] * v[2] + t[7] * v[3],
			t[8] * v[0] + t[9] * v[1] + t[10] * v[2] + t[11] * v[3],
			t[12] * v[0] + t[13] * v[1] + t[14] * v[2] + t[15] * v[3]
		};
	}

	template <typename T> matrix_t<T> operator* (const matrix_t<T> & a, const matrix_t<T> & b) {
		// this is (probably) faster than loops, i don't know if -O2 woulkd optimize it this way
		return matrix_t<T> {
			a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12],
			a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13],
			a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14],
			a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15],
			a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12],
			a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13],
			a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14],
			a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15],
			a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12],
			a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13],
			a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14],
			a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15],
			a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12],
			a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13],
			a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14],
			a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15]
		};
	}

	template <typename T> std::ostream & operator<< (std::ostream & stream, const vector_t<T> & v) {
		stream << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
		return stream;
	}
	
	using double_vector_t = vector_t<double>;
	using float_vector_t = vector_t<float>;
	using int_vector_t = vector_t<int>;

	using double_matrix_t = matrix_t<double>;
	using float_matrix_t = matrix_t<float>;
	using int_matrix_t = matrix_t<int>;

	// common transforms
	float_vector_t normalize (const float_vector_t & vector);
	float_matrix_t make_identity ();
	float_matrix_t make_scale (float s);
	float_matrix_t make_scale (float sx, float sy, float sz);
	float_matrix_t make_scale (const float_vector_t & vector);
	float_matrix_t make_translation (const float_vector_t & vector);
	float_matrix_t make_translation (float tx, float ty, float tz);
	float_matrix_t make_rotation_x (float angle);
	float_matrix_t make_rotation_y (float angle);
	float_matrix_t make_rotation_z (float angle);
	float_matrix_t make_rotation (const float_vector_t & axis, float angle);
	float_matrix_t make_rotation (float x, float y, float z, float angle);
	float_matrix_t invert (const float_matrix_t & m);
}