#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <stdio.h>
#include <vcruntime.h>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("base.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("base.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

// this function is from my Graphics homework
bool PlayMode::hit(Ray ray, Boundingbox box) {
	glm::vec3 invdir;
	//taking some care so that we don't end up with NaN's , just a degenerate matrix, if scale is zero:
	invdir.x = (ray.dir.x == 0.0f ? 0.0f : 1.0f / ray.dir.x);
	invdir.y = (ray.dir.y == 0.0f ? 0.0f : 1.0f / ray.dir.y);
	invdir.z = (ray.dir.z == 0.0f ? 0.0f : 1.0f / ray.dir.z);

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	bool sign[3] = {invdir.x < 0, invdir.y < 0, invdir.z < 0};
	glm::vec3 bounds[2] = {box.min, box.max};

	tmin = (bounds[sign[0]].x - ray.point.x) * invdir.x;
	tmax = (bounds[1 - sign[0]].x - ray.point.x) * invdir.x;
	tymin = (bounds[sign[1]].y - ray.point.y) * invdir.y;
	tymax = (bounds[1 - sign[1]].y - ray.point.y) * invdir.y;

	if ((tmin > tymax) || (tymin > tmax)) return false;

	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[sign[2]].z - ray.point.z) * invdir.z;
	tzmax = (bounds[1 - sign[2]].z - ray.point.z) * invdir.z;

	if ((tmin > tzmax) || (tzmin > tmax)) return false;

	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}

void PlayMode::SpawnUFO(unsigned int num, float distance) {
	for (unsigned int i = 0; i < num; i++) {
		UFOs.emplace_back();
		auto &UFO = UFOs.back();
		UFO.id = UFO_nums;
		UFO_nums += 1;
		// add random position
		scene.transforms.emplace_back();
		auto UFO_transforms = &scene.transforms.back();
		UFO.transform = UFO_transforms;

		Scene::Drawable new_UFO_instance = UFO_instance;
		new_UFO_instance.transform = UFO_transforms;
		scene.drawables.emplace_back(new_UFO_instance);

		UFO_transforms->name = "UFO" + std::to_string(UFO.id);
		UFO_transforms->rotation = UFO_instance.transform->rotation;
		UFO_transforms->scale = UFO_instance.transform->scale;
		UFO_transforms->position = UFO_instance.transform->position;
		glm::vec3 random_dir = distance * glm::normalize(glm::vec3(rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50));
		UFO_transforms->position += random_dir;
		
	}
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	
	// temporarily use a transform to pass assert test
	for (auto drawable : scene.drawables) {
		if (drawable.transform->name == "OUFO") {
			UFO_instance = drawable;
			// printf("car body parent: %s\n", drawable.transform->parent->name.c_str());
			break;
		}
	}
	printf("successfully get UFO\n");

	UFO_mesh = hexapod_meshes->lookup("OUFO");
	
	UFO_nums = 0;

	srand(15666);

	UFOs = std::vector<UFO>();

	SpawnUFO(10, 50.0f);
	
	
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	

	//move camera:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;


	if (game_over) return;

	Ray ray;
	ray.point = camera->transform->position;
	ray.dir = camera->transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f); // camera direction -z

	std::vector<size_t> UFOs_to_erase_index;
	std::vector<size_t> UFOs_to_erase_id;
	size_t index = 0;
	for (auto &UFO : UFOs) {
		UFO.boundingbox.min = UFO.transform->position + UFO.transform->scale * UFO_mesh.min;
		UFO.boundingbox.max = UFO.transform->position + UFO.transform->scale * UFO_mesh.max;
		if (hit(ray, UFO.boundingbox)) {
			// printf("UFO hit! id: %u\n", UFO.id);
			score += 1;
			UFOs_to_erase_id.push_back(UFO.id);
			UFOs_to_erase_index.push_back(index);
		}
		index += 1;
	}
	// erase UFOs that are hit
	for (auto it =  UFOs_to_erase_index.rbegin(); it != UFOs_to_erase_index.rend(); ++it) {
		UFOs.erase(UFOs.begin() + *it);
	}

	unsigned int drawables_erase_index = 1;
	// size_t drawables_size = scene.drawables.size();
	std::vector<size_t> drawables_to_erase_index;
	for (auto it = scene.drawables.begin(); it != scene.drawables.end(); ++it) {
		auto &drawable = *it;
		for (auto UFO_index: UFOs_to_erase_id) {
			if (drawable.transform->name == "UFO" + std::to_string(UFO_index)) {
				scene.drawables.erase(it);
				// drawables_to_erase_index.push_back(drawables_size - drawables_erase_index);
				break;
			}
		}
		drawables_erase_index += 1;
	}
	
	// for (auto &ind: drawables_to_erase_index) {
	// 	scene.drawables.erase(scene.drawables.begin() + ind);
	// }

	{
		constexpr float UFOSpeed = 10.0f;
		for (auto &drawable : scene.drawables) {
			if (std::strncmp(drawable.transform->name.c_str(), "UFO", 3) == 0) {
				// printf("name: %s\n", drawable.transform->name.c_str());
				glm::vec3 normal_dir = glm::normalize(drawable.transform->position);
				drawable.transform->position -= normal_dir * elapsed * UFOSpeed;
				if (glm::length(drawable.transform->position) < 0.5f) {
					// printf("UFO hit player!\n");
					lives -= 1;
					if (lives == 0) {
						game_over = true;
					}
					drawable.transform->position = glm::vec3(0.0f);
				}
			}
		}
	}

	elapsed_time += elapsed;
	if (elapsed_time > UFO_spawn_time) {
		elapsed_time -= UFO_spawn_time;
		SpawnUFO(3, 100.0f);
	}
	

	
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
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
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

		constexpr float H = 0.09f;
		float windows_offset = 1000.0f / drawable_size.y;
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse;",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse;",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			
		lines.draw_text("Score: " + std::to_string(score) + " Lives: " + std::to_string(lives),
			glm::vec3(-aspect + 0.1f * H + windows_offset, -1.0 + 0.1f * H + windows_offset, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Score: " + std::to_string(score) + " Lives: " + std::to_string(lives),
			glm::vec3(-aspect + 0.1f * H + ofs + windows_offset, -1.0 + 0.1f * H + ofs + windows_offset, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		float aim_offset_x = 1.7f;
		float aim_offset_y = 1.0f;
		lines.draw_text("------",
			glm::vec3(-aspect + 0.1f * H + aim_offset_x, -1.0 + 0.1f * H + aim_offset_y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0x00, 0x00, 0x00));
		lines.draw_text("|",
			glm::vec3(-aspect + 0.1f * H + aim_offset_x + 0.1f, -1.0 + 0.1f * H + aim_offset_y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0x00, 0x00, 0x00));

		
	}
	if (game_over)
	{
		// If game ends, draw bounding box for each UFO.
		DrawLines draw_lines(camera->make_projection() * glm::mat4(camera->transform->make_world_to_local()));
		for (auto &UFO: UFOs) {
			glm::vec3 r = 0.5f * (UFO.boundingbox.max - UFO.boundingbox.min);
			glm::vec3 c = 0.5f * (UFO.boundingbox.max + UFO.boundingbox.min);
			glm::mat4x3 mat(
				glm::vec3(r.x,  0.0f, 0.0f),
				glm::vec3(0.0f,  r.y, 0.0f),
				glm::vec3(0.0f, 0.0f,  r.z),
				c
			);
			draw_lines.draw_box(mat, glm::u8vec4(0xdd, 0xdd, 0xdd, 0xff));
		}
	}
}
