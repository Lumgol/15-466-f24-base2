#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <format>
#include <iostream>
#include <string>

// string_format function adapted from this stackoverflow post:
// https://stackoverflow.com/a/26221725
std::string string_format( const std::string& format, uint8_t score, uint8_t high_score )
{
	int size_s = std::snprintf( nullptr, 0, format.c_str(), score, high_score ) + 1; // Extra space for '\0'
	if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
	auto size = static_cast<size_t>( size_s );
	std::unique_ptr<char[]> buf( new char[ size ] );
	std::snprintf( buf.get(), size, format.c_str(), score, high_score);
	return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
};

GLuint game2_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > game2_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("game2.pnct"));
	game2_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > game2_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("game2.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = game2_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = game2_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		if (mesh_name == "Cube.005") {
			drawable.is_paintable = true;
		}
	});
});

PlayMode::PlayMode() : scene(*game2_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Basket") basket = &transform;
		else if (transform.name == "drop1") drops[0] = &transform;
		else if (transform.name == "drop2") drops[1] = &transform;
		else if (transform.name == "drop3") drops[2] = &transform;
		else if (transform.name == "drop4") drops[3] = &transform;
		else if (transform.name == "drop5") drops[4] = &transform;
	}
	if (basket == nullptr) throw std::runtime_error("Basket not found.");
	if (drops[0] == nullptr) throw std::runtime_error("drop1 not found.");
	if (drops[1] == nullptr) throw std::runtime_error("drop2 not found.");

	basket_pos = basket->position;
	// using basket bounding box's max z coordinate for collision-ish stuff
	basket_max_z = game2_meshes->lookup("Sphere").max.z;
	// drop_origin_to_min_z is the distance from the drop's origin z-coord to the drop's minimum z-coord.
	// it is negative.
	Mesh drop_mesh = game2_meshes->lookup("Icosphere.002");
	drop_origin_to_min_z = drop_mesh.min.z;
	drop_reset_height = drops[0]->position.z;

	std::function<void(uint)> randomize_drop_idxs = [this](uint seed) {
		srand(seed);
		for (uint8_t i = 0; i < num_drops; i++) {
			// not a perfectly uniform distribution but I think the game won't suffer for it
			drop_idxs[i] = rand() % 9;
			bool is_unique_idx = true;
			do {
				is_unique_idx = true;
				for (uint8_t j = 0; j < i; j++) {
					if (drop_idxs[j] == drop_idxs[i]) is_unique_idx = false;
				} if (!is_unique_idx) {
					drop_idxs[i] = rand() % 9;
				}
			} while (!is_unique_idx);

			drops[i]->position.x = ((int8_t) drop_idxs[i] / 3 - 1) * grid_width;
			drops[i]->position.y = ((int8_t) drop_idxs[i] % 3 - 1) * grid_width;
		}
	};

	randomize_drop_idxs(0);

	// initialize goal_colors
	// start with a goal mixed from 2 colors
	goal_color_idxs = goal_colors[rand() % 10];
	for (uint8_t i = 0; i < goal_color_idxs.size(); i++) {
		goal_color = goal_color * ((float) i + 1) / ((float) i + 2) + drop_colors[goal_color_idxs[i]] / ((float) i + 2);
	}

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1) {
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	}
	camera = &scene.cameras.front();

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			basket_idx.x = std::max(basket_idx.x - 1, -1);
			basket->position.x = basket_idx.x * grid_width;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			basket_idx.x = std::min(basket_idx.x + 1, 1);
			basket->position.x = basket_idx.x * grid_width;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			basket_idx.y = std::min(basket_idx.y + 1, 1);
			basket->position.y = basket_idx.y * grid_width;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			basket_idx.y = std::max(basket_idx.y - 1, -1);
			basket->position.y = basket_idx.y * grid_width;
			return true;
		}
	} 

	return false;
}

void PlayMode::update(float elapsed) {

	// copied from earlier in the code. haven't figured out how to create a helper accessible by both 
	// the constructor and update and stuff. Stupid code!
	std::function<void(uint)> randomize_drop_idxs = [this](uint seed) {
		srand(seed);
		for (uint8_t i = 0; i < num_drops; i++) {
			// not a perfectly uniform distribution but I think the game won't suffer for it
			drop_idxs[i] = rand() % 9;
			bool is_unique_idx = true;
			do {
				is_unique_idx = true;
				for (uint8_t j = 0; j < i; j++) {
					if (drop_idxs[j] == drop_idxs[i]) is_unique_idx = false;
				} if (!is_unique_idx) drop_idxs[i] = rand() % 9;
			} while (!is_unique_idx);

			drops[i]->position.x = ((int8_t) drop_idxs[i] / 3 - 1) * grid_width;
			drops[i]->position.y = ((int8_t) drop_idxs[i] % 3 - 1) * grid_width;
		}
	};

	// move drops down:
	float drop_speed = 0.2f + speed_bonus;
	float drop_delta = elapsed * drop_speed;
	float new_height = drops[0]->position.z - drop_delta;
	if (new_height + drop_origin_to_min_z < basket_max_z) {
		// update the paint color!
		for (uint8_t i = 0; i < num_drops; i++) {
			if (drop_idxs[i] == (basket_idx.x + 1) * 3 + basket_idx.y + 1) {
				accumulated_colors++;
				paint_color = (paint_color * (float) (accumulated_colors - 1) + drop_colors[i]) / (float) accumulated_colors;
				
				bool color_is_correct = 0;
				for (uint8_t j = 0; j < goal_color_idxs.size(); j++) {
					if (i == goal_color_idxs[j]) color_is_correct = true;
				}
				if (paint_color == goal_color) {
					// correct color!
					// get points equal to the number of colors mixed
					score += goal_color_idxs.size();
					if (score > high_score) high_score = score;

					// reset paint color
					paint_color = glm::vec4(1.);
					accumulated_colors = 1;

					// get a new goal color
					srand((uint) (total_time * 10.));
					goal_color = glm::vec4(1.);
					uint8_t rand_size_idx = rand() % 3;
					uint8_t rand_color_idx;
					if (rand_size_idx == 0) rand_color_idx = rand() % 10;
					else if (rand_size_idx == 1) rand_color_idx = rand() % 30 + 10;
					else rand_color_idx = rand() % 52 + 40;
					goal_color_idxs = goal_colors[rand_color_idx];
					for (uint8_t i = 0; i < goal_color_idxs.size(); i++) {
						goal_color = goal_color * ((float) i + 1) / ((float) i + 2) 
							+ drop_colors[goal_color_idxs[i]] / ((float) i + 2);
					}
				} else if (!color_is_correct) {
					// we have an incorrect color

					// reset score
					score = 0;

					// reset paint color & accumulated colors
					paint_color = glm::vec4(1.);
					accumulated_colors = 1;

					// get a new goal color
					srand((uint) (total_time * 10.));
					goal_color = glm::vec4(1.);
					uint8_t rand_size_idx = rand() % 3;
					uint8_t rand_color_idx;
					if (rand_size_idx == 0) rand_color_idx = rand() % 10;
					else if (rand_size_idx == 1) rand_color_idx = rand() % 30 + 10;
					else rand_color_idx = rand() % 52 + 40;
					goal_color_idxs = goal_colors[rand_color_idx];
					for (uint8_t i = 0; i < goal_color_idxs.size(); i++) {
						goal_color = goal_color * ((float) i + 1) / ((float) i + 2) 
							+ drop_colors[goal_color_idxs[i]] / ((float) i + 2);
					}

					// reset drop speed
					speed_bonus = 0.;
				}
			}
		}

		// move drops back up to the top
		for (Scene::Transform *drop : drops) {
			drop->position.z = drop_reset_height;
		}
		// randomize drop indices
		randomize_drop_idxs((uint) (total_time * 10.));

		// increase speed
		speed_bonus += elapsed / 4.;
	} 
	else {
		for (Scene::Transform *drop : drops) {
			drop->position.z = new_height;
		}
	}

	total_time += elapsed;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));

	for (std::list<Scene::Drawable>::iterator d = scene.drawables.begin(); d != scene.drawables.end(); d++) {
		if (d->is_paintable) d->paint_color1 = paint_color;
	} 
	glUseProgram(0);

	glClearColor(0.15f, 0.13f, 0.2f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		for (uint i = 0; i < 1000; i++) {
			lines.draw(
				glm::vec3(1.5, (float(i) / 1000.) - 1., 0.), 
				glm::vec3(1.8, (float(i) / 1000.) - 1., 0.), 
				(glm::u8vec4) (paint_color * 255.f));
			lines.draw(
				glm::vec3(1.5, (float(i) / 1000.), 0.), 
				glm::vec3(1.8, (float(i) / 1000.), 0.), 
				(glm::u8vec4) (goal_color * 255.f));
		}

		constexpr float H = 0.12f;

		lines.draw_text(string_format("Score: %u. High score: %u.", score, high_score),
			glm::vec3(-aspect + 0.3f * H, -1.0 + 0.3f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 5.0f / drawable_size.y;
		lines.draw_text(string_format("Score: %u. High score: %u.", score, high_score),
			glm::vec3(-aspect + 0.3f * H + ofs, -1.0 + 0.3f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xf0, 0xff, 0xff, 0x00));

		lines.draw_text("Goal color", 
			glm::vec3(aspect - 0.88, 0.85, 0.),
			glm::vec3(H, 0., 0.), glm::vec3(0., H, 0.), 
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Goal color", 
			glm::vec3(aspect - 0.88 + ofs, 0.85 + ofs, 0.),
			glm::vec3(H, 0., 0.), glm::vec3(0., H, 0.), 
			glm::u8vec4(0xf0, 0xff, 0xff, 0x00));

		lines.draw_text("Your color", 
			glm::vec3(aspect - 0.89, -0.95, 0.),
			glm::vec3(H, 0., 0.), glm::vec3(0., H, 0.), 
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Your color", 
			glm::vec3(aspect - 0.89 + ofs, -0.95 + ofs, 0.),
			glm::vec3(H, 0., 0.), glm::vec3(0., H, 0.), 
			glm::u8vec4(0xf0, 0xff, 0xff, 0x00));
	}
}
