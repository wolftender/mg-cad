#include <vector>
#include <fstream>

#include "app.hpp"
#include "surface.hpp"
#include "curve.hpp"
#include "beziersurf.hpp"
#include "intersection.hpp"

#include "milling/milling.hpp"

namespace mini {
	inline bool intersect_segment(
		float x1, float y1,
		float x2, float y2,
		float x3, float y3,
		float x4, float y4,
		float& t, float& u) {

		float den = ((x1 - x2) * (y3 - y4)) - ((y1 - y2) * (x3 - x4));

		if (den == 0.0f) {
			t = u = 0.0f;
			return false;
		}

		float num_t = ((x1 - x3) * (y3 - y4)) - ((y1 - y3) * (x3 - x4));
		float num_u = ((x1 - x3) * (y1 - y2)) - ((y1 - y3) * (x1 - x2));

		// 0 < num_t/den < 1
		if (num_t * den < 0.0f || num_u * den < 0.0f) {
			t = u = 0.0f;
			return false;
		}

		num_u = fabsf(num_u);
		num_t = fabsf(num_t);
		den = fabsf(den);

		if (num_u > den || num_t > den) {
			t = u = 0.0f;
			return false;
		}

		t = num_t / den;
		u = num_u / den;

		return true;
	}

	inline std::vector<glm::vec2> merge_intersection_curve(
		const std::vector<glm::vec2>& c1,
		const std::vector<glm::vec2>& c2) {

		std::vector<glm::vec2> join;
		join.reserve(c1.size() + c2.size());

		for (auto iter = c1.rbegin(); iter != c1.rend(); ++iter) {
			join.push_back(*iter);
		}

		for (auto iter = c2.begin(); iter != c2.end(); ++iter) {
			join.push_back(*iter);
		}

		return join;
	}

	inline std::vector<glm::vec3> extrude_intersection_xz(
		const std::shared_ptr<differentiable_surface_base>& surf,
		const std::vector<glm::vec2> curve,
		float sign) {

		// extrude the inersection between base and body
		std::vector<glm::vec3> intersection_curve;
		intersection_curve.reserve(curve.size());

		int num_int_pts = static_cast<int>(curve.size());
		for (int i = 0; i < num_int_pts; ++i) {
			auto d1 = glm::vec3{ 0.0f, 0.0f, 0.0f };
			auto d2 = glm::vec3{ 0.0f, 0.0f, 0.0f };

			const auto& curr = curve[i + 0];
			auto ccurr = surf->sample(curr.x, curr.y);

			if (i > 0) {
				const auto& prev = curve[i - 1];
				auto cprev = surf->sample(prev.x, prev.y);
				d1 = glm::normalize(ccurr - cprev);
			}

			if (i < num_int_pts - 1) {
				const auto& next = curve[i + 1];
				auto cnext = surf->sample(next.x, next.y);
				d2 = glm::normalize(cnext - ccurr);
			}

			auto norm1 = glm::vec3{ -d1.z, 0.0f, d1.x };
			auto norm2 = glm::vec3{ -d2.z, 0.0f, d2.x };

			auto norm = glm::normalize(norm1 + norm2);
			intersection_curve.push_back(ccurr + norm * sign);
		}

		return intersection_curve;
	}

	padlock_milling_script::padlock_milling_script(application& app) : m_app(app) { 
		m_padlock_body = nullptr;
		m_padlock_keyhole = nullptr;
		m_padlock_shackle = nullptr;

		m_base_width = 12.0f;
		m_base_height = 12.0f;
		m_base_depth = 1.5f;
		m_scale = 12.0f / 15.0f;
	}

	padlock_milling_script::~padlock_milling_script() { }

	void padlock_milling_script::run() {
		// prepare data
		m_prepare_base();
		m_identify_model();
		m_prepare_heightmap();

		// path generation
		//m_gen_path_1();
		m_gen_path_2();
	}

	void padlock_milling_script::m_prepare_base() {
		auto& store = m_app.m_store;

		// first create a new surface in the scene, this will be the base of the model
		point_list points;

		const float hw = m_base_width * 0.5f;
		const float hh = m_base_height * 0.5f;
		const float ws = m_base_width / 3.0f;
		const float hs = m_base_height / 3.0f;

		for (int x = 0; x < 4; ++x) {
			for (int y = 0; y < 4; ++y) {
				glm::vec3 position{
					-hw + x * ws,
					0.0f,
					-hh + y * hs,
				};

				const auto point_obj = std::make_shared<point_object>(m_app,
					store->get_billboard_s_shader(),
					store->get_point_texture());

				point_obj->set_translation(position);
				point_obj->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });

				m_app.add_object("milling_point", point_obj);
				points.push_back(point_obj);
			}
		}

		const auto base_surf = std::make_shared<bezier_surface_c0>(
			m_app,
			store->get_bezier_surf_shader(),
			store->get_bezier_surf_solid_shader(),
			store->get_line_shader(),
			1, 1,
			points,
			false, false);

		m_app.add_object("milling_base", base_surf);
		m_model_base = base_surf;

		m_model_base->integrate(0.0f);
	}

	void padlock_milling_script::m_identify_model() {
		std::shared_ptr<application::object_wrapper_t> body;
		std::shared_ptr<application::object_wrapper_t> shackle;
		std::shared_ptr<application::object_wrapper_t> keyhole;

		for (auto& object_wrapper : m_app.m_objects) {
			auto object = object_wrapper->object;

			if (object->get_name() == "padlock_body") {
				m_padlock_body = std::dynamic_pointer_cast<bspline_surface>(object);
				body = object_wrapper;
			} else if (object->get_name() == "padlock_shackle") {
				m_padlock_shackle = std::dynamic_pointer_cast<bspline_surface>(object);
				shackle = object_wrapper;
			} else if (object->get_name() == "padlock_keyhole") {
				m_padlock_keyhole = std::dynamic_pointer_cast<bezier_surface_c0>(object);
				keyhole = object_wrapper;
			}
		}

		// display settings
		m_padlock_body->set_showing_polygon(false);
		m_padlock_shackle->set_showing_polygon(false);
		m_padlock_keyhole->set_showing_polygon(false);
		m_app.m_points_enabled = false;

		intersection_controller controller1(m_app, m_padlock_body, m_padlock_keyhole, m_app.m_store, false);

		// intersect shackle
		m_app.set_cursor_pos({ 0.9f, 0.0f, -1.2f });
		intersection_controller controller2(m_app, m_padlock_body, m_padlock_shackle, m_app.m_store, true);
		m_app.set_cursor_pos({ -0.9f, 0.0f, -1.2f });
		intersection_controller controller3(m_app, m_padlock_body, m_padlock_shackle, m_app.m_store, true);

		m_app.clear_selection();

		// trim surfaces automatically from hardcoded points
		m_padlock_keyhole->get_domain().trim(0.22f, 0.45f);

		m_padlock_body->get_domain().trim(0.55f, 0.4f);
		m_padlock_body->get_domain().trim(0.42f, 0.134f);
		m_padlock_body->get_domain().trim(0.676f, 0.134f);

		m_padlock_shackle->get_domain().trim(0.226f, 0.015f);
		m_padlock_shackle->get_domain().trim(0.089f, 0.970f);

		intersection_controller controller4(m_app, m_padlock_body, m_model_base, m_app.m_store, false);
		m_padlock_body->get_domain().trim(0.14f, 0.14f);

		// intersect shackle
		m_app.set_cursor_pos({ 1.87f, 0.0f, -1.132f });
		intersection_controller controller5(m_app, m_padlock_shackle, m_model_base, m_app.m_store, true);

		m_app.set_cursor_pos({ 0.85f, 0.0f, -1.154f });
		intersection_controller controller6(m_app, m_padlock_shackle, m_model_base, m_app.m_store, true);

		m_padlock_shackle->get_domain().trim(0.148f, 0.475f);
		m_padlock_shackle->get_domain().trim(0.869f, 0.475f);

		m_app.clear_selection();

		m_int_body_hole = controller1.get_verbose();
		m_int_body_shackle1 = controller2.get_verbose();
		m_int_body_shackle2 = controller3.get_verbose();

		m_int_base_body = controller4.get_verbose();
		m_int_base_shackle1 = controller5.get_verbose();
		m_int_base_shackle2 = controller6.get_verbose();
	}

	void padlock_milling_script::m_prepare_heightmap() {
		m_hm_width = 762;
		m_hm_height = 762;
		m_heightmap.resize(m_hm_width * m_hm_height);

		m_hm_units_x = m_base_width;
		m_hm_units_y = m_base_height;

		float offset_x = -m_hm_units_x / 2;
		float offset_y = -m_hm_units_y / 2;

		std::fill(m_heightmap.begin(), m_heightmap.end(), 0.0f);

		const auto trace_surface = [&](const std::shared_ptr<differentiable_surface_base>& surf) {
			for (int xi = 0; xi < 1024; ++xi) {
				for (int yi = 0; yi < 1024; ++yi) {
					float u = xi * (1.0f / 1024.0f);
					float v = yi * (1.0f / 1024.0f);

					auto point = surf->sample(u, v);

					float hm_x = (point.x - offset_x) / m_hm_units_x;
					float hm_y = (point.z - offset_y) / m_hm_units_y;

					if (hm_x < 0.0f || hm_y < 0.0f || hm_x > 1.0f || hm_y > 1.0f) {
						continue;
					}

					uint32_t hm_pixel_x = static_cast<uint32_t>(hm_x * static_cast<float>(m_hm_width));
					uint32_t hm_pixel_y = static_cast<uint32_t>(hm_y * static_cast<float>(m_hm_height));
					auto index = hm_pixel_y * m_hm_width + hm_pixel_x;

					if (-point.y > m_heightmap[index]) {
						m_heightmap[index] = -point.y;
					}
				}
			}
		};

		trace_surface(m_padlock_body);
		trace_surface(m_padlock_keyhole);
		trace_surface(m_padlock_shackle);

		std::ofstream out("heightmap.txt");
		for (const auto& el : m_heightmap) {
			out << el << " ";
		}
	}

	float padlock_milling_script::m_query_bitmap(float x, float y) const {
		float offset_x = -m_hm_units_x / 2;
		float offset_y = -m_hm_units_y / 2;

		float hm_x = (x - offset_x) / m_hm_units_x;
		float hm_y = (y - offset_y) / m_hm_units_y;

		if (hm_x < 0.0f || hm_y < 0.0f || hm_x > 1.0f || hm_y > 1.0f) {
			return 0.0f;
		}

		uint32_t hm_pixel_x = static_cast<uint32_t>(hm_x * static_cast<float>(m_hm_width));
		uint32_t hm_pixel_y = static_cast<uint32_t>(hm_y * static_cast<float>(m_hm_height));
		auto index = hm_pixel_y * m_hm_width + hm_pixel_x;

		return m_heightmap[index];
	}

	void padlock_milling_script::m_gen_path_1() {
		// this is the first stage of path generation
		// in this stage we only use the heightmap

		const float real_cutter_radius = 0.8f;

		const float cutter_radius = real_cutter_radius * 4.0f / 5.0f;
		const float path_width = 0.9f * cutter_radius * 2.0f;
		const float veps = 0.1f;

		m_path_1.push_back({0.0f, -6.0f, 0.0f});

		const float hw = m_base_width / 2.0f;
		const float hh = m_base_height / 2.0f;
		const float pw = path_width;
		const float hp = path_width / 2.0f;
		const float depth = 5.0f - m_base_depth;
		const float hd = depth / 2.0f;

		const int path_count = static_cast<int>(m_base_width / hp) + 4;

		glm::vec3 position = { -hw - pw, -6.0f, -hh - pw };
		m_path_1.push_back(position);

		position = { -hw - pw, -veps - hd, -hh - pw };
		m_path_1.push_back(position);

		bool is_left = true;
		for (int i = 0; i < path_count; ++i) {
			float dz = i * hp;
			float ndz = (i+1) * hp;

			if (is_left) {
				position = { hw + pw, -veps - hd, -hh - pw + dz };
				m_path_1.push_back(position);
				position = { hw + pw, -veps - hd, -hh - pw + ndz };
				m_path_1.push_back(position);

				is_left = false;
			} else {
				position = { -hw - pw, -veps - hd, -hh - pw + dz };
				m_path_1.push_back(position);
				position = { -hw - pw, -veps - hd, -hh - pw + ndz };
				m_path_1.push_back(position);

				is_left = true;
			}
		}

		std::cout << "ended on the left side? " << is_left << std::endl;

		const auto heightmap_trace = [&](const glm::vec3& start, const glm::vec3& end) {
			for (int i = 0; i < 1000; ++i) {
				float t = i * 0.001f;

				// check collision with heightmap
				auto current = glm::mix(start, end, t);

				std::vector<float> distances(32*32);
				float cut_size = cutter_radius;
				float dcut_size = cut_size / 16.0f;
				float curvature = 1.0f / cut_size;

				// paraboloid has equation
				// z(x,y) = K * (x^2 + y^2) / 2
				float max_eps = 0.0f;

				for (int x = 0, i = 0; x < 32; ++x) {
					for (int y = 0; y < 32; ++y) {
						float dcx = -cut_size + (x * dcut_size);
						float dcy = -cut_size + (y * dcut_size);

						float sx = current.x + dcx;
						float sy = current.z + dcy;

						// surface value
						float zs = m_query_bitmap(sx, sy);

						// paraboloid value
						float zp = curvature * 0.5f * (dcx * dcx + dcy * dcy);
						max_eps = glm::max(zs - zp, max_eps);
					}
				}

				if (max_eps > 0.0f) {
					m_path_1.push_back({current.x, -max_eps -veps, current.z});
				}
			}

			position = end;
			m_path_1.push_back(end);
		};

		glm::vec3 start, end;

		for (int i = 0; i < path_count; ++i) {
			float dz = i * hp;
			float ndz = (i + 1) * hp;

			if (is_left) {
				start = position;
				end = { -hw - pw, -veps, hh + pw - dz };

				heightmap_trace(start, end);

				start = end;
				end = { hw + pw, -veps, hh + pw - dz };

				heightmap_trace(start, end);

				is_left = false;
			} else {
				start = position;
				end = { hw + pw, -veps, hh + pw - dz };

				heightmap_trace(start, end);

				start = end;
				end = { -hw - pw, -veps, hh + pw - dz };

				heightmap_trace(start, end);

				is_left = true;
			}
		}

		m_path_1.push_back({ 0.0f, -6.0f, 0.0f });

		auto curve1 = std::make_shared<curve>(m_app, m_app.m_store->get_line_shader(), m_path_1);
		curve1->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
		curve1->set_line_width(3.0f);
		m_app.add_object("milling_curve_k16", curve1);
	}

	void padlock_milling_script::m_gen_path_2() {
		// this is the second stage of generation for paths
		// here the base is evened out to be nice and smooth

		const float real_cutter_radius = 0.6f;

		const float cutter_radius = real_cutter_radius * 4.0f / 5.0f;
		const float path_width = 0.9f * cutter_radius * 2.0f;
		const float eps1 = 0.2f;
		const float eps2 = 0.05f;

		m_path_2.push_back({ 0.0f, -6.0f, 0.0f });

		const float hw = m_base_width / 2.0f;
		const float hh = m_base_height / 2.0f;
		const float pw = path_width;
		const float hp = path_width / 2.0f;
		const float depth = 5.0f - m_base_depth;
		const float hd = depth / 2.0f;

		const int path_count = static_cast<int>(m_base_width / hp) + 4;

		// connect and extrude paths, this makes a nice border that we can use to not intersect the model
		// when carving out our base
		auto body_curve = merge_intersection_curve(m_int_base_body.s21, m_int_base_body.s22);
		auto body_bound = extrude_intersection_xz(m_model_base, body_curve, hp + eps2);

		auto shackle_curve = merge_intersection_curve(m_int_base_shackle1.s21, m_int_base_shackle1.s22);
		auto shackle_bound = extrude_intersection_xz(m_model_base, shackle_curve, hp + eps2);

		// begin generating path for the base of the model
		m_path_2.push_back({ 0.0f, -6.0f, -hh - pw - 0.45f });
		m_path_2.push_back({ 0.0f, 0.0f, -hh - pw - eps1 });

		const auto collide_path = [&](const glm::vec3& start, const glm::vec3& end) -> glm::vec3 {
			float min_u = 1.0f;

			for (int i = 0; i < body_bound.size() - 1; ++i) {
				auto cstart = body_bound[i];
				auto cend = body_bound[i + 1];

				float t, u;
				if (!intersect_segment(
					cstart.x, cstart.z, cend.x, cend.z,
					start.x, start.z, end.x, end.z, 
					t, u)) {
					continue;
				}

				if (u < min_u) {
					min_u = u;
				}
			}

			for (int i = 0; i < shackle_bound.size() - 1; ++i) {
				auto cstart = shackle_bound[i];
				auto cend = shackle_bound[i + 1];

				float t, u;
				if (!intersect_segment(
					cstart.x, cstart.z, cend.x, cend.z,
					start.x, start.z, end.x, end.z,
					t, u)) {
					continue;
				}

				if (u < min_u) {
					min_u = u;
				}
			}

			return glm::mix(start, end, min_u);
		};

		bool is_left = true;
		for (int i = 0; i < path_count; ++i) {
			float dz = i * hp;
			float ndz = (i + 1) * hp;

			if (is_left) {
				m_path_2.push_back({ hw + pw, 0.0f, -hh - pw + dz });
				m_path_2.push_back({ hw + pw, 0.0f, -hh - pw + ndz });

				is_left = false;
			} else {
				m_path_2.push_back(collide_path({ hw + pw, 0.0f, -hh - pw + dz }, { 0.0f, 0.0f, -hh - pw + dz }));
				m_path_2.push_back(collide_path({ hw + pw, 0.0f, -hh - pw + ndz }, { 0.0f, 0.0f, -hh - pw + ndz }));

				is_left = true;
			}
		}

		float rem = (path_count * hp) - (hw + pw)*2.0f;
		m_path_2.push_back({ -hw - pw, 0.0f, hh + pw - hp + rem });

		for (int i = 0; i < path_count; ++i) {
			float dz = (i+1) * hp - rem;
			float ndz = (i+2) * hp - rem;

			if (is_left) {
				m_path_2.push_back({ -hw - pw, 0.0f, hh + pw - dz });
				m_path_2.push_back({ -hw - pw, 0.0f, hh + pw - ndz });

				is_left = false;
			} else {
				m_path_2.push_back(collide_path({ -hw - pw, 0.0f, hh + pw - dz }, { 0.0f, 0.0f, hh + pw - dz }));
				m_path_2.push_back(collide_path({ -hw - pw, 0.0f, hh + pw - ndz }, { 0.0f, 0.0f, hh + pw - ndz }));

				is_left = true;
			}
		}

		m_path_2.push_back({ 0.0f, -6.0f, -hh - pw - 0.45f });
		m_path_2.push_back({ 0.0f, -6.0f, 0.0f });

		auto curve1 = std::make_shared<curve>(m_app, m_app.m_store->get_line_shader(), m_path_2);
		curve1->set_color({ 0.0f, 1.0f, 0.0f, 1.0f });
		curve1->set_line_width(3.0f);
		m_app.add_object("milling_curve_f12", curve1);
	}
}