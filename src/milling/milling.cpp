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

	inline bool intersect_segment(
		const glm::vec2& s1,
		const glm::vec2& e1,
		const glm::vec2& s2,
		const glm::vec2& e2,
		glm::vec2& p
	) {
		float t, u;
		if (!intersect_segment(s1.x, s1.y, e1.x, e1.y, s2.x, s2.y, e2.x, e2.y, t, u)) {
			return false;
		}

		p = glm::mix(s1, e1, t);
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

	inline std::vector<glm::vec2> join_intersection_curves(
		const std::vector<glm::vec2>& c1,
		const std::vector<glm::vec2>& c2,
		float merge_eps = -1.0f) {

		std::vector<glm::vec2> join;
		join.reserve(c1.size() + c2.size());

		struct intersection_t {
			std::size_t c1_index;
			std::size_t c2_index;
			glm::vec2 point;
		};

		std::vector<intersection_t> intersections;
		glm::vec2 p;

		for (auto i = 0UL; i < c1.size() - 1; ++i) {
			const auto& s1 = c1[i + 0];
			const auto& e1 = c1[i + 1];

			for (auto j = 0UL; j < c2.size() - 1; ++j) {
				const auto& s2 = c2[j + 0];
				const auto& e2 = c2[j + 1];

				if (intersect_segment(s1, e1, s2, e2, p)) {
					if (merge_eps >= 0.0f) {
						bool already_found = false;

						for (const auto& found_intersection : intersections) {
							if (glm::distance(found_intersection.point, p) < merge_eps) {
								already_found = true;
								break;
							}
						}

						if (already_found) {
							continue;
						}

						intersections.push_back({ i, j, p });
					} else {
						intersections.push_back({ i, j, p });
					}
				}
			}
		}

		auto next_int = intersections.begin();
		int surface = 0;

		for (size_t i = 0, j = 0; i < c1.size() && j < c2.size();) {
			if (surface == 0) {
				join.push_back(c1[i]);

				if (next_int != intersections.end() &&
					next_int->c1_index == i) {

					surface = 1;
					join.push_back(next_int->point);

					if (next_int->c2_index + 1 <= j) {
						break;
					}

					j = next_int->c2_index + 1;
					next_int++;
				}

				i++;
			} else {
				join.push_back(c2[j]);

				if (next_int != intersections.end() &&
					next_int->c2_index == j) {

					surface = 0;
					join.push_back(next_int->point);

					if (next_int->c1_index + 1 <= i) {
						break;
					}

					i = next_int->c1_index + 1;
					next_int++;
				}

				j++;
			}
		}

		return join;
	}

	inline std::vector<glm::vec2> extrude_intersection(
		const std::vector<glm::vec2>& curve,
		float sign) {

		constexpr float eps = 0.00001f;

		// extrude the inersection between base and body
		std::vector<glm::vec2> intersection_curve;
		intersection_curve.reserve(curve.size());

		int num_int_pts = static_cast<int>(curve.size());
		for (int i = 0; i < num_int_pts; ++i) {
			auto d1 = glm::vec2{ 0.0f, 0.0f };
			auto d2 = glm::vec2{ 0.0f, 0.0f };

			const auto& curr = curve[i + 0];

			if (i > 0) {
				const auto& prev = curve[i - 1];
				d1 = curr - prev;
			}

			if (i < num_int_pts - 1) {
				const auto& next = curve[i + 1];
				d2 = next - curr;
			}

			if (glm::length(d1) < eps && glm::length(d2) < eps) {
				continue;
			}

			if (glm::length(d1) >= eps) {
				d1 = glm::normalize(d1);
			}

			if (glm::length(d2) >= eps) {
				d2 = glm::normalize(d2);
			}

			auto norm1 = glm::vec2{ -d1.y, d1.x };
			auto norm2 = glm::vec2{ -d2.y, d2.x };

			auto norm = glm::normalize(norm1 + norm2);
			intersection_curve.push_back(curr + norm * sign);
		}

		return intersection_curve;
	}

	inline std::vector<glm::vec3> curve_to_world(
		const std::shared_ptr<differentiable_surface_base>& surf,
		const std::vector<glm::vec2>& curve
	) {
		std::vector<glm::vec3> out;
		out.reserve(curve.size());

		for (const auto& el : curve) {
			out.push_back(surf->sample(el.x, el.y));
		}

		return out;
	}

	inline std::vector<glm::vec2> optimize_curve(
		const std::vector<glm::vec2>& curve,
		float eps = 0.00001f
	) {
		std::vector<glm::vec2> out;
		out.reserve(curve.size());

		glm::vec2 previous;
		for (auto& point : curve) {
			if (glm::distance(point, previous) < eps) {
				continue;
			}

			previous = point;
			out.push_back(point);
		}

		return out;
	}

	inline std::vector<glm::vec3> extrude_intersection_xz(
		const std::shared_ptr<differentiable_surface_base>& surf,
		const std::vector<glm::vec2>& curve,
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
	    //m_gen_path_2();
		//m_gen_path_3();
		m_gen_path_4();

		//m_export_path("1.k16", m_path_1);
		//m_export_path("2.f12", m_path_2);
		//m_export_path("3.f10", m_path_3);
		m_export_path("4.k08", m_path_4);
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

	void padlock_milling_script::m_prepare_hole(float cutter_radius, float eps) {
		glm::vec3 tl{ 1.7f, -cutter_radius, 0.75f };
		glm::vec3 br{ -1.7f, -cutter_radius, -4.0f };

		float step_x = (br.x - tl.x) / 3.0f;
		float step_z = (br.z - tl.z) / 3.0f;

		point_list points;

		for (int sx = 0; sx < 4; ++sx) {
			for (int sz = 0; sz < 4; ++sz) {
				glm::vec3 pos{ tl.x + (sx * step_x), -cutter_radius, tl.z + (sz * step_z) };

				const auto point_obj = std::make_shared<point_object>(m_app,
					m_app.m_store->get_billboard_s_shader(),
					m_app.m_store->get_point_texture());

				point_obj->set_translation(pos);
				point_obj->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });

				m_app.add_object("milling_point_hole", point_obj);
				points.push_back(point_obj);
			}
		}

		const auto base_surf = std::make_shared<bezier_surface_c0>(
			m_app,
			m_app.m_store->get_bezier_surf_shader(),
			m_app.m_store->get_bezier_surf_solid_shader(),
			m_app.m_store->get_line_shader(),
			1, 1,
			points,
			false, false);

		m_app.add_object("milling_hole_base", base_surf);
		m_hole_base = base_surf;

		m_hole_base->integrate(0.0f);
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
		auto body_bound = extrude_intersection_xz(m_model_base, body_curve, hp + eps1);

		auto shackle_curve = merge_intersection_curve(m_int_base_shackle1.s21, m_int_base_shackle1.s22);
		auto shackle_bound = extrude_intersection_xz(m_model_base, shackle_curve, hp + eps1);

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

	void padlock_milling_script::m_gen_path_3() {
		// generate third curve
		const float real_cutter_radius = 0.5f;
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

		// parameter space width for the base is 12 units
		// so scale this by 1/12
		const float extrude_radius = -(hp + eps2) / 12.0f;

		auto body_curve = merge_intersection_curve(m_int_base_body.s21, m_int_base_body.s22);
		auto body_bound = extrude_intersection(body_curve, extrude_radius);

		auto shackle_curve = merge_intersection_curve(m_int_base_shackle1.s21, m_int_base_shackle1.s22);
		auto shackle_bound = extrude_intersection(shackle_curve, extrude_radius);

		auto join_curve = join_intersection_curves(body_bound, shackle_bound);
		auto join_bound = extrude_intersection_xz(m_model_base, join_curve, 0.0f);

		auto first_curve_pt = join_bound.front();

		m_path_3.push_back({ 0.0f, -6.0f, 0.0f });
		m_path_3.push_back({ hh + pw + 0.45f, -6.0f, first_curve_pt.z });
		m_path_3.push_back({ hh + pw + 0.45f, 0.0f, first_curve_pt.z });
		m_path_3.insert(m_path_3.end(), join_bound.begin(), join_bound.end());
		m_path_3.push_back({ hh + pw + 0.45f, 0.0f, first_curve_pt.z });
		m_path_3.push_back({ hh + pw + 0.45f, -6.0f, first_curve_pt.z });
		m_path_3.push_back({ 0.0f, -6.0f, 0.0f });

		auto curve2 = std::make_shared<curve>(m_app, m_app.m_store->get_line_shader(), m_path_3);
		curve2->set_color({ 1.0f, 0.0f, 1.0f, 1.0f });
		curve2->set_line_width(3.0f);
		m_app.add_object("milling_curve_f10", curve2);
	}

	inline std::vector<glm::vec3> create_milling_curve(
		const std::shared_ptr<differentiable_surface_base> & surface,
		const std::vector<std::vector<glm::vec2>*> & bounds,
		const bool vertical,
		const bool invert,
		const float start_level,
		const float end_level,
		const float start_line,
		const float middle_line,
		const float path_width,
		const float line_bound,
		const float accuracy,
		const std::function<float(float)> correction = nullptr) {

		std::vector<glm::vec2> lines;
		int iteration = 0;

		if (invert) {
			iteration++;
		}

		float sgn = glm::sign(path_width);

		for (float shift = start_level; shift*sgn < end_level*sgn; shift += path_width, iteration++) {
			if (correction) {
				shift += path_width*sgn*correction(glm::abs(shift/(end_level - start_level)));
			}

			const float s = (sgn == 1.0f) ? glm::min(shift, end_level) : glm::max(shift, end_level);

			glm::vec2 line_start = vertical ? glm::vec2{ s, start_line } : glm::vec2{ start_line, s };
			glm::vec2 line_end = vertical ? glm::vec2{ s, middle_line } : glm::vec2{ middle_line, s };

			float lbound = line_bound;

			if (iteration % 2 != 0) {
				lbound = 1.0f - lbound;
				std::swap(line_start, line_end);
			}

			float t_start = 0.0f;
			float t_end = 1.0f;

			for (const auto* bound : bounds) {
				for (size_t i = 0; i < bound->size() - 1; ++i) {
					const auto& b_start = (*bound)[i + 0];
					const auto& b_end = (*bound)[i + 1];

					float t, u;

					if (intersect_segment(
						line_start.x, line_start.y, line_end.x, line_end.y,
						b_start.x, b_start.y, b_end.x, b_end.y, t, u)) {
						if (t <= lbound) {
							t_start = glm::max(t, t_start);
						} else {
							t_end = glm::min(t, t_end);
						}
					}
				}
			}

			if (glm::abs(t_start - t_end) < 0.05f) {
				continue;
			}

			for (float t = t_start + 0.005f; t < t_end + accuracy; t += accuracy) {
				float rt = glm::min(t, t_end - 0.005f);
				lines.push_back(glm::mix(line_start, line_end, rt));
			}
		}

		return curve_to_world(surface, lines);
	}

	inline std::vector<glm::vec3> create_milling_curve_2(
		const std::shared_ptr<differentiable_surface_base>& surface,
		const std::vector<std::vector<glm::vec2>*>& bounds,
		const bool vertical,
		const bool invert,
		const float start_level,
		const float end_level,
		const float path_width,
		const float accuracy,
		const std::function<float(float)> correction = nullptr) {

		std::vector<glm::vec2> lines;
		int iteration = 0;

		if (invert) {
			iteration++;
		}

		float sgn = glm::sign(path_width);

		for (float shift = start_level; shift * sgn < end_level * sgn; shift += path_width, iteration++) {
			if (correction) {
				shift += path_width * sgn * correction(glm::abs(shift / (end_level - start_level)));
			}

			const float s = (sgn == 1.0f) ? glm::min(shift, end_level) : glm::max(shift, end_level);

			glm::vec2 line_start = vertical ? glm::vec2{ s, 0.0f } : glm::vec2{ 0.0f, s };
			glm::vec2 line_end = vertical ? glm::vec2{ s, 1.0f } : glm::vec2{ 1.0f, s };

			if (iteration % 2 != 0) {
				std::swap(line_start, line_end);
			}

			float t_start = 0.0f;
			float t_end = 1.0f;

			float c1 = -1.0f, c2 = -1.0f;

			for (const auto* bound : bounds) {
				for (size_t i = 0; i < bound->size() - 1; ++i) {
					const auto& b_start = (*bound)[i + 0];
					const auto& b_end = (*bound)[i + 1];

					float t, u;

					if (intersect_segment(
						line_start.x, line_start.y, line_end.x, line_end.y,
						b_start.x, b_start.y, b_end.x, b_end.y, t, u)) {
						
						if (c1 < 0.0f) {
							c1 = t;
						} else {
							c2 = t;
							break;
						}
					}
				}
			}

			if (c1 < 0.0f || c2 < 0.0f) {
				if (lines.empty()) {
					continue;
				} else {
					break;
				}
			}

			if (glm::abs(t_start - t_end) < 0.05f) {
				continue;
			}

			t_start = glm::min(c1, c2);
			t_end = glm::max(c1, c2);

			for (float t = t_start + 0.005f; t < t_end + accuracy; t += accuracy) {
				float rt = glm::min(t, t_end - 0.005f);
				lines.push_back(glm::mix(line_start, line_end, rt));
			}
		}

		return curve_to_world(surface, lines);
	}

	inline std::vector<glm::vec2> remove_looping(
		const std::vector<glm::vec2>& curve
	) {
		std::vector<glm::vec2> curve_1, curve_2;
		curve_1.reserve(curve.size());
		curve_2.reserve(curve.size());

		glm::vec2 first = curve.front();
		glm::vec2 previous = curve.front();
		auto iter = curve.begin();

		bool looped = false;
		for (; iter != curve.end(); ++iter) {
			if (glm::distance(previous, *iter) > 0.5f) {
				looped = true;
				break;
			}

			previous = *iter;
			curve_1.push_back(*iter);
		}

		if (!looped) {
			return curve_1;
		}

		for (; iter != curve.end(); ++iter) {
			curve_2.push_back(*iter);

			if (glm::distance(first, *iter) < 0.005f) {
				curve_2.push_back(first);
				break;
			}
		}

		std::vector<glm::vec2> res;
		res.reserve(curve_1.size() + curve_2.size());

		for (auto m = curve_2.begin(); m != curve_2.end(); ++m) res.push_back(*m);
		for (auto m = curve_1.begin(); m != curve_1.end(); ++m) res.push_back(*m);

		return res;
	}

	inline std::vector<glm::vec2> trim_curves(
		const std::vector<glm::vec2>& c1,
		const std::vector<glm::vec2>& c2,
		const bool start) {

		std::vector<glm::vec2> trimmed;
		trimmed.reserve(c1.size());
		glm::vec2 p;

		bool add = start, intersected = false;

		if (add) {
			trimmed.push_back(c1.front());
		}

		for (auto i = 0UL; i < c1.size() - 1; ++i) {
			const auto& s1 = c1[i + 0];
			const auto& e1 = c1[i + 1];

			bool intersects = false;

			if (!intersected) {
				for (auto j = 0UL; j < c2.size() - 1; ++j) {
					const auto& s2 = c2[j + 0];
					const auto& e2 = c2[j + 1];

					if (intersect_segment(s1, e1, s2, e2, p)) {
						intersects = true;
					}
				}
			}

			if (intersects) {
				trimmed.push_back(p);
				add = !add;
				intersected = true;

				if (start) {
					break;
				}
			} else if (add) {
				trimmed.push_back(e1);
			}
		}

		return trimmed;
	}

	void padlock_milling_script::m_gen_path_4() {
		const float real_cutter_radius = 0.4f;
		const float cutter_radius = real_cutter_radius * 4.0f / 5.0f;

		const float path_width = 0.65f * cutter_radius * 2.0f;
		const float eps = 0.005f;

		m_body_eqd = std::make_shared<equidistant_surface>(m_padlock_body, cutter_radius + eps);
		m_shackle_eqd = std::make_shared<equidistant_surface>(m_padlock_shackle, cutter_radius + eps);
		m_keyhole_eqd = std::make_shared<equidistant_surface>(m_padlock_keyhole, cutter_radius + eps);
		m_base_eqd = std::make_shared<equidistant_surface>(m_model_base, -cutter_radius - eps);

		intersection_controller controller1(m_app, m_body_eqd, m_keyhole_eqd, m_app.m_store, false);

		// intersect shackle
		m_app.set_cursor_pos({ 0.9f, 0.0f, -1.2f });
		intersection_controller controller2(m_app, m_body_eqd, m_shackle_eqd, m_app.m_store, true);
		m_app.set_cursor_pos({ -0.9f, 0.0f, -1.2f });
		intersection_controller controller3(m_app, m_body_eqd, m_shackle_eqd, m_app.m_store, true);

		intersection_controller controller4(m_app, m_body_eqd, m_base_eqd, m_app.m_store, false);

		// intersect shackle
		m_app.set_cursor_pos({ 1.87f, 0.0f, -1.132f });
		intersection_controller controller5(m_app, m_shackle_eqd, m_base_eqd, m_app.m_store, true);

		m_app.set_cursor_pos({ 0.85f, 0.0f, -1.154f });
		intersection_controller controller6(m_app, m_shackle_eqd, m_base_eqd, m_app.m_store, true);

		// prepare curves for bounds on domains of equidistant surfaces
		auto int_body_base = controller4.get_verbose();
		auto int_body_shackle1 = controller2.get_verbose();
		auto int_body_shackle2 = controller3.get_verbose();
		auto int_body_keyhole = controller1.get_verbose();
		auto int_shackle_base1 = controller5.get_verbose();
		auto int_shackle_base2 = controller6.get_verbose();


		/// GENERATE OUTER LINING FOR BODY
		auto joint_body_outer = join_intersection_curves(
			join_intersection_curves(
				merge_intersection_curve(optimize_curve(int_body_base.s11), optimize_curve(int_body_base.s12)),
				optimize_curve(int_body_shackle1.s11),
				0.0001f
			), optimize_curve(int_body_shackle2.s11), 
			0.0001f
		);

		/// GENERATE OUTER LINING FOR SHACKLE
		auto shackle_loop_1 = remove_looping(optimize_curve(int_body_shackle1.s22));
		auto shackle_loop_2 = remove_looping(optimize_curve(int_body_shackle2.s22));

		auto shackle_inner_lining = optimize_curve(merge_intersection_curve(int_shackle_base2.s11, int_shackle_base2.s12));
		std::reverse(shackle_inner_lining.begin(), shackle_inner_lining.end());

		shackle_inner_lining = trim_curves(shackle_inner_lining, shackle_loop_1, false);
		shackle_inner_lining = trim_curves(shackle_inner_lining, shackle_loop_2, true);

		auto joint_shackle_outer = trim_curves(shackle_loop_1, shackle_inner_lining, false);
		joint_shackle_outer = join_intersection_curves(joint_shackle_outer,
			optimize_curve(merge_intersection_curve(int_shackle_base1.s11, int_shackle_base1.s12)));
		joint_shackle_outer = join_intersection_curves(joint_shackle_outer, 
			trim_curves(shackle_loop_2, shackle_inner_lining, true));

		joint_shackle_outer.insert(joint_shackle_outer.end(), shackle_inner_lining.rbegin(), shackle_inner_lining.rend());

		// EXPORT OUTER LINING MILLING PATH
		auto world_body_outer = curve_to_world(m_body_eqd, joint_body_outer);
		auto world_shackle_outer = curve_to_world(m_shackle_eqd, joint_shackle_outer);

		// BOUNDS FOR SURFACES
		std::vector<std::vector<glm::vec2>*> body_bounds;
		body_bounds.push_back(&joint_body_outer);
		body_bounds.push_back(&int_body_keyhole.s11);

		std::vector<std::vector<glm::vec2>*> shackle_bounds;
		shackle_bounds.push_back(&joint_shackle_outer);

		m_path_4.push_back({0.0f, -6.0f, 0.0f});

		const float safe_height = -3.0f;

		// generate detailed paths for c2 surfaces
		auto body_lines1 = create_milling_curve(m_body_eqd, body_bounds, false, false, 0.2f, 0.75f, 0.3f, 0.58f, 0.007f, 0.5f, 0.005f);
		m_path_4.push_back({ body_lines1.front().x, safe_height, body_lines1.front().z });
		m_path_4.insert(m_path_4.end(), body_lines1.begin(), body_lines1.end());
		m_path_4.push_back({ body_lines1.back().x, safe_height, body_lines1.back().z });

		auto body_lines2 = create_milling_curve(m_body_eqd, body_bounds, false, false, 0.75f, 0.2f, 0.53f, 0.8f, -0.007f, 0.5f, 0.005f);
		m_path_4.push_back({ body_lines2.front().x, safe_height, body_lines2.front().z });
		m_path_4.insert(m_path_4.end(), body_lines2.begin(), body_lines2.end());
		m_path_4.push_back({ body_lines2.back().x, safe_height, body_lines2.back().z });

		auto body_lines3 = create_milling_curve(m_body_eqd, body_bounds, true, false, 0.36f, 0.74f, 0.1f, 0.27f, 0.004f, 0.75f, 0.01f);
		m_path_4.push_back({ body_lines3.front().x, safe_height, body_lines3.front().z });
		m_path_4.insert(m_path_4.end(), body_lines3.begin(), body_lines3.end());
		m_path_4.push_back({ body_lines3.back().x, safe_height, body_lines3.back().z });

		auto body_lines4 = create_milling_curve(m_body_eqd, body_bounds, true, false, 0.36f, 0.74f, 0.7f, 0.9f, 0.004f, 0.05f, 0.01f);
		m_path_4.push_back({ body_lines4.front().x, safe_height, body_lines4.front().z });
		m_path_4.insert(m_path_4.end(), body_lines4.begin(), body_lines4.end());
		m_path_4.push_back({ body_lines4.back().x, safe_height, body_lines4.back().z });

		auto shackle_lines1 = create_milling_curve(m_shackle_eqd, shackle_bounds, false, false, 0.06f, 0.94f, 0.2f, 0.8f, 0.004f, 0.75f, 0.01f, 
		[=](float t)->float {
			float V = 0.1f;
			float E = 0.56f;

			float exponent = (t - E)/V;
			return (4.0f / (12.0f * V)) * glm::exp(-0.5f * exponent * exponent);
		});

		m_path_4.push_back({ shackle_lines1.front().x, safe_height, shackle_lines1.front().z });
		m_path_4.insert(m_path_4.end(), shackle_lines1.begin(), shackle_lines1.end());
		m_path_4.push_back({ shackle_lines1.back().x, safe_height, shackle_lines1.back().z });

		// create surface for the hole
		m_prepare_hole(cutter_radius, eps);

		// generate detailed paths for the HOLE
		std::vector<std::vector<glm::vec2>*> hole_bounds;

		intersection_controller hole_controller(m_app, m_hole_base, m_shackle_eqd, m_app.m_store, true);
		intersection_controller hole_controller2(m_app, m_hole_base, m_body_eqd, m_app.m_store, true);

		auto hole_bound1 = optimize_curve(hole_controller.get_verbose().s12);
		auto hole_bound2 = optimize_curve(merge_intersection_curve(
			hole_controller2.get_verbose().s11, hole_controller2.get_verbose().s12));

		hole_bounds.push_back(&hole_bound1);
		hole_bounds.push_back(&hole_bound2);

		auto hole_lines1 = create_milling_curve(m_hole_base, hole_bounds, false, false, 0.25f, 0.75f, 0.4f, 0.9f, 0.02f, 0.25f, 1.0f);
		auto hole_lines2 = create_milling_curve(m_hole_base, hole_bounds, true, false, 0.5f, 0.78f, 0.1f, 0.9f, 0.02f, 0.5f, 1.0f);
		hole_lines1.insert(hole_lines1.end(), hole_lines2.begin(), hole_lines2.end());

		m_path_4.push_back({ hole_lines1.front().x, safe_height, hole_lines1.front().z });
		m_path_4.insert(m_path_4.end(), hole_lines1.begin(), hole_lines1.end());
		m_path_4.push_back({ hole_lines1.back().x, safe_height, hole_lines1.back().z });

		// keyhole paths, this is the "sharp" part of the model so we need to make it SHARP
		std::vector<std::vector<glm::vec2>*> keyhole_bounds;
		int_body_keyhole.s21 = optimize_curve(int_body_keyhole.s21);
		int_body_keyhole.s22 = optimize_curve(int_body_keyhole.s22);

		keyhole_bounds.push_back(&int_body_keyhole.s21);
		keyhole_bounds.push_back(&int_body_keyhole.s22);

		//auto keyhole_lines1 = create_milling_curve(m_keyhole_eqd, keyhole_bounds, true, false, 0.45f, 0.6666f, 0.05f, 0.5f, 0.008f, 0.95f, 0.02f);

		auto keyhole_lines1 = create_milling_curve_2(m_keyhole_eqd, keyhole_bounds, true, false, 0.45f, 0.6666f, 0.008f, 0.02f);
		m_path_4.push_back({ keyhole_lines1.front().x, safe_height, keyhole_lines1.front().z });
		m_path_4.insert(m_path_4.end(), keyhole_lines1.begin(), keyhole_lines1.end());
		m_path_4.push_back({ keyhole_lines1.back().x, safe_height, keyhole_lines1.back().z });

		auto keyhole_lines2 = create_milling_curve_2(m_keyhole_eqd, keyhole_bounds, true, false, 0.6705f, 0.85f, 0.008f, 0.02f);
		m_path_4.push_back({ keyhole_lines2.front().x, safe_height, keyhole_lines2.front().z });
		m_path_4.insert(m_path_4.end(), keyhole_lines2.begin(), keyhole_lines2.end());
		m_path_4.push_back({ keyhole_lines2.back().x, safe_height, keyhole_lines2.back().z });

		/*auto keyhole_lines2 = create_milling_curve(m_keyhole_eqd, keyhole_bounds, false, false, 0.07f, 0.88f, 0.6705f, 0.85f, 0.008f, 0.05f, 0.02f);
		m_path_4.push_back({ keyhole_lines2.front().x, safe_height, keyhole_lines2.front().z });
		m_path_4.insert(m_path_4.end(), keyhole_lines2.begin(), keyhole_lines2.end());
		m_path_4.push_back({ keyhole_lines2.back().x, safe_height, keyhole_lines2.back().z });*/

		//auto keyhole_lines1 = create_milling_curve(m_keyhole_eqd, keyhole_bounds, false, false, 0.07f, 0.88f, 0.45f, 0.6666f, 0.008f, 0.95f, 0.02f);

		// go over all the intersections as the last stage!
		m_path_4.push_back({ world_body_outer.front().x, safe_height, world_body_outer.front().z });
		m_path_4.insert(m_path_4.end(), world_body_outer.begin(), world_body_outer.end());
		m_path_4.push_back({ world_body_outer.back().x, safe_height, world_body_outer.back().z });

		m_path_4.push_back({ world_shackle_outer.front().x, safe_height, world_shackle_outer.front().z });
		m_path_4.insert(m_path_4.end(), world_shackle_outer.begin(), world_shackle_outer.end());
		m_path_4.push_back({ world_shackle_outer.back().x, safe_height, world_shackle_outer.back().z });

		auto world_keyhole_outer = curve_to_world(m_body_eqd, int_body_keyhole.s11);
		m_path_4.push_back({ world_keyhole_outer.front().x, safe_height, world_keyhole_outer.front().z });
		m_path_4.insert(m_path_4.end(), world_keyhole_outer.begin(), world_keyhole_outer.end());
		m_path_4.push_back({ world_keyhole_outer.back().x, safe_height, world_keyhole_outer.back().z });

		// return to starting position
		m_path_4.push_back({ 0.0f, -6.0f, 0.0f });

		for (auto& el : m_path_4) {
			el.y += cutter_radius;
		}

		// display curve
		//auto curve1 = std::make_shared<curve>(m_app, m_app.m_store->get_line_shader(), curve_to_world(m_body_eqd, int_body_keyhole.s11));
		//curve1->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
		//curve1->set_line_width(3.0f);
		//m_app.add_object("milling_curve_f10", curve1);

		auto curve2 = std::make_shared<curve>(m_app, m_app.m_store->get_line_shader(), m_path_4);
		curve2->set_color({ 0.0f, 1.0f, 1.0f, 1.0f });
		curve2->set_line_width(3.0f);
		m_app.add_object("milling_curve_f10", curve2);
	}

	void padlock_milling_script::m_export_path(const std::string& name, const std::vector<glm::vec3>& path) const {
		std::ofstream fs(name);

		if (fs) {
			int n = 3;
			const float scale = 5.0f / 4.0f;

			for (const auto& point : path) {
				float px = -point.x * 10.0f * scale;
				float py = -point.z * 10.0f * scale;
				float pz = (m_base_depth - point.y) * 10.0f * scale;

				fs << "N" << n 
					<< "G01X" << std::setprecision(3) << std::fixed << px 
					<< "Y" << std::setprecision(3) << std::fixed << py 
					<< "Z" << std::setprecision(3) << std::fixed << pz << std::endl;

				n++;
			}
		} else {
			std::cerr << "failed to export path " << name << std::endl;
		}
	}
}