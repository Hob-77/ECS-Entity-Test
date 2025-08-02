#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <iostream>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <vector>

#include "ECS.h"
#include "Renderer.h"

int main(int argc, char* argv[])
{

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		std::cerr << "Failed to init SDL: " << SDL_GetError() << "\n";
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow
	(
		"ECS Entity Testing",
		1920,
		1080,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
	);

	if (!window)
	{
		std::cerr << "Window could not be created! SDL_ERROR: " << SDL_GetError() << "\n";
		SDL_Quit();
		return -1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (!gl_context)
	{
		std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << "\n";
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	SDL_GL_MakeCurrent(window, gl_context);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD\n";
		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	Renderer renderer;
	renderer.Init(1920, 1080);

	World world;

	// Random number generators
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> posX(0.0f, 1920.0f);
	std::uniform_real_distribution<float> posY(0.0f, 1080.0f);
	std::uniform_real_distribution<float> size(5.0f, 20.0f);
	std::uniform_real_distribution<float> gravity(200.0f, 800.0f);
	std::uniform_real_distribution<float> colorDist(0.2f, 1.0f);

	// Create boxes
	for (int i = 0; i < 10000; i++)
	{
		Entity box = world.CreateEntity();

		Transform t;
		t.position = Vec2(posX(gen), posY(gen));
		t.scale = size(gen);
		world.AddTransform(box, t);

		Physics p;
		p.acceleration = Vec2(0, gravity(gen));
		world.AddPhysics(box, p);
	}

	// ===== ADD THE CACHE TEST HERE =====
	std::cout << "Running cache performance comparison...\n";

	// Test 1: Current ECS (cache-friendly)
	auto start = std::chrono::high_resolution_clock::now();
	for (int frame = 0; frame < 1000; frame++) {
		world.Query<Transform, Physics>([](Entity e, Transform& t, Physics& p) {
			p.velocity += p.acceleration * 0.016f;
			t.position += p.velocity * 0.016f;
			});
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto ecsTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

	// Test 2: Simulate bad cache access
	struct BadEntity {
		Transform t;
		Physics p;
		char padding[64]; // Force cache misses
	};
	BadEntity* badEntities = new BadEntity[10000];

	// Initialize bad entities with same data
	{
		int i = 0;
		world.Query<Transform, Physics>([&](Entity e, Transform& t, Physics& p) {
			badEntities[i].t = t;
			badEntities[i].p = p;
			i++;
			});
	}

	// Random access pattern (cache unfriendly)
	start = std::chrono::high_resolution_clock::now();
	for (int frame = 0; frame < 1000; frame++) {
		// Create random access pattern
		std::vector<int> indices(10000);
		std::iota(indices.begin(), indices.end(), 0);
		std::shuffle(indices.begin(), indices.end(), gen);

		for (int idx : indices) {
			badEntities[idx].p.velocity += badEntities[idx].p.acceleration * 0.016f;
			badEntities[idx].t.position += badEntities[idx].p.velocity * 0.016f;
		}
	}
	end = std::chrono::high_resolution_clock::now();
	auto badTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

	std::cout << "=== Cache Performance Results ===\n";
	std::cout << "ECS Time (cache-friendly): " << ecsTime / 1000.0f << " ms\n";
	std::cout << "Bad Time (cache-unfriendly): " << badTime / 1000.0f << " ms\n";
	std::cout << "Speedup: " << (float)badTime / ecsTime << "x faster\n\n";

	delete[] badEntities;
	// ===== END OF CACHE TEST =====

	// Allocate arrays
	float* positions = new float[20000];
	float* scales = new float[20000];
	float* colors = new float[40000];

	// Generate random colors once
	for (int i = 0; i < 10000; i++)
	{
		colors[i * 4] = colorDist(gen);
		colors[i * 4 + 1] = colorDist(gen);
		colors[i * 4 + 2] = colorDist(gen);
		colors[i * 4 + 3] = 1.0f;
	}

	Uint64 lastTime = SDL_GetTicks();

	bool quit = false;
	SDL_Event event;

	while (!quit)
	{
		// Delta time
		Uint64 currentTime = SDL_GetTicks();
		float deltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;

		while (SDL_PollEvent(&event))
		{

			if (event.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}

			if (event.type == SDL_EVENT_KEY_DOWN)
			{
				if (event.key.key == SDLK_ESCAPE)
				{
					quit = true;
				}
			}
		}

		world.Query<Transform, Physics>([deltaTime](Entity e, Transform& t, Physics& p)
			{
				p.velocity += p.acceleration * deltaTime;
				t.position += p.velocity * deltaTime;

				// Wrap at bottom
				if (t.position.y > 1080.0f)
				{
					t.position.y = -1000.0f;
				}
			});

		// Collect render data from ECS
		int count = 0;
		world.Query<Transform>([&](Entity e, Transform& t)
			{
				positions[count * 2] = t.position.x;
				positions[count * 2 + 1] = t.position.y;
				scales[count * 2] = t.scale;
				scales[count * 2 + 1] = t.scale;
				count++;
			});

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		renderer.DrawInstanced(positions, scales, colors, count);

		SDL_GL_SwapWindow(window);

	}

	delete[] positions;
	delete[] scales;
	delete[] colors;

	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

}