#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	void randomize_drop_idxs(uint seed);

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	static const uint8_t num_drops = 5;

	//pointers to meshes
	Scene::Transform *basket = nullptr;
	Scene::Transform *drop1 = nullptr;
	Scene::Transform *drop2 = nullptr;
	Scene::Transform *drop3 = nullptr;
	Scene::Transform *drop4 = nullptr;
	Scene::Transform *drop5 = nullptr;

	// storing positions of meshes
	glm::vec3 basket_pos;
	std::array<Scene::Transform *, num_drops> drops;
	float basket_max_z;
	float drop_origin_to_min_z;
	float drop_reset_height;

	float wobble = 0.0f;

	// putting meshes on a grid :O
	float grid_width = 0.8 / 3.;
	uint8_t num_grid_tiles = 9;
	glm::i8vec2 basket_idx = glm::i8vec2(0, 0);
	std::array<uint8_t, num_drops> drop_idxs;

	// timing
	float total_time = 0.;
	float speed_bonus = 0;

	// coloring
	glm::vec4 paint_color = glm::vec4(1., 1., 1., 1.);
	glm::vec4 goal_color = glm::vec4(1., 1., 1., 1.);
	std::vector<uint8_t> goal_color_idxs;
	uint8_t accumulated_colors = 1;

	const std::array<glm::vec4, num_drops> drop_colors = {
		glm::vec4(1., 0., 0., 1.),
		glm::vec4(0., 1., 0., 1.),
		glm::vec4(0., 0., 1., 1.),
		glm::vec4(1., 1., 1., 1.),
		glm::vec4(0., 0., 0., 1.),};

	// each entry of the outer vector is a vector of indices into drop_colors
	// so each goal color is the sum of drop_colors entries at those indices
	std::vector<std::vector<uint8_t>> goal_colors = {
		{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 2}, 
		{1, 3}, {1, 4}, {2, 3}, {2, 4}, {3, 4},
		{0, 0, 1}, {0, 0, 2}, {0, 0, 3}, {0, 0, 4}, {0, 1, 1},
		{0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 2, 2}, {0, 2, 3},
		{0, 2, 4}, {0, 3, 3}, {0, 3, 4}, {0, 4, 4}, {1, 1, 2},
		{1, 1, 3}, {1, 1, 4}, {1, 2, 2}, {1, 2, 3}, {1, 2, 4},
		{1, 3, 3}, {1, 3, 4}, {1, 4, 4}, {2, 2, 3}, {2, 2, 4},
		{2, 3, 3}, {2, 3, 4}, {2, 4, 4}, {3, 3, 4}, {3, 4, 4},
		{0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3}, {0, 0, 0, 4}, {0, 0, 1, 2}, 
		{0, 0, 1, 3}, {0, 0, 1, 4}, {0, 0, 2, 3}, {0, 0, 2, 4}, {0, 0, 3, 4}, 
		{0, 1, 1, 1}, {0, 1, 1, 2}, {0, 1, 1, 3}, {0, 1, 1, 4}, {0, 1, 2, 2}, 
		{0, 1, 2, 3}, {0, 1, 2, 4}, {0, 1, 3, 3}, {0, 1, 3, 4}, {0, 1, 4, 4}, 
		{0, 2, 2, 3}, {0, 2, 3, 3}, {0, 2, 3, 4}, {0, 2, 4, 4}, {0, 3, 3, 3},
		{0, 3, 3, 4}, {0, 3, 4, 4}, {0, 4, 4, 4}, {1, 1, 1, 2}, {1, 1, 1, 3},
		{1, 1, 1, 4}, {1, 1, 2, 3}, {1, 1, 2, 4}, {1, 1, 3, 4}, {1, 2, 2, 2},
		{1, 2, 2, 3}, {1, 2, 2, 4}, {1, 2, 3, 3}, {1, 2, 3, 4}, {1, 3, 3, 3},
		{1, 3, 3, 4}, {1, 3, 4, 4}, {1, 4, 4, 4}, {2, 2, 2, 3}, {2, 2, 2, 4},
		{2, 2, 3, 4}, {2, 3, 3, 3}, {2, 3, 3, 4}, {2, 3, 4, 4}, {2, 4, 4, 4},
		{3, 3, 3, 4}, {3, 4, 4, 4}
	};

	// scoring
	uint8_t score = 0;
	uint8_t high_score = 0;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
