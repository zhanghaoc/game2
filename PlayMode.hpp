#include "Mode.hpp"

#include "Scene.hpp"
#include "Mesh.hpp"

#include <glm/glm.hpp>

#include <stdint.h>
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

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	struct Boundingbox {
		glm::vec3 min = glm::vec3(0.0f);
		glm::vec3 max = glm::vec3(0.0f);
	};

	struct Ray {
		glm::vec3 point = glm::vec3(0.0f);
		glm::vec3 dir = glm::vec3(0.0f);
	};

	struct UFO {
		unsigned int id = 0;
		Boundingbox boundingbox;
		Scene::Transform * transform = nullptr;
	};
	
	bool hit(Ray, Boundingbox);
	void SpawnUFO(unsigned int, float);

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	
	std::vector<UFO> UFOs;
	unsigned int score = 0;
	int lives = 3;
	unsigned int UFO_nums = 0;
	float UFO_spawn_time = 2.0f;
	float elapsed_time = 0.0f;
	bool game_over = false;

	Mesh UFO_mesh;
	Scene::Drawable UFO_instance;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
