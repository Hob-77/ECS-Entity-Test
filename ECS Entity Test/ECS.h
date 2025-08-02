#pragma once
#include <cstdint>
#include "Array.h"
#include "Vec2.h"
#include <SDL3/SDL.h>

struct Player
{
	float health = 100.0f;
	float speed = 200.0f;
};

// Entity
using Entity = std::uint32_t;
const Entity NULL_ENTITY = 0;
const int MAX_ENTITIES = 10000;

// Components
struct Transform {
	Vec2 position;
	float rotation = 0;
	float scale = 1;
};

struct Sprite {
	SDL_Texture* texture = nullptr;
	SDL_Color color = { 255,255,255,255 };
	uint8_t width = 0;
	uint8_t height = 0;
};

struct Animation {
	SDL_Texture* spriteSheet = nullptr;
	uint8_t frameWidth = 0;
	uint8_t frameHeight = 0;
	uint8_t currentFrame = 0;
	uint8_t totalFrames = 0;
	float frameTime = 0.1f;
	float timer = 0.0f;
	bool loop = true;
	bool playing = true;
};

struct Physics {
	// Motion
	Vec2 velocity;
	Vec2 acceleration;

	// Gravity
	float gravityScale = 1.0f;
	float maxFallSpeed = 600.0f;

	// Friction
	float linearDamping = 0.0f;

	// Type
	bool isKinematic = false;
};

enum CollisionLayers : uint16_t
{
	LAYER_DEFAULT = 1 << 0,
	LAYER_PLAYER = 1 << 1,
	LAYER_ENEMY = 1 << 2,
	LAYER_PLATFORM = 1 << 3,
	LAYER_TRIGGER = 1 << 4,
};

struct Collider
{
	// Size and offset
	Vec2 size;
	Vec2 offset;

	// Collision layers (bitflags)
	uint16_t layer = LAYER_DEFAULT;
	uint16_t collidesWith = 0xFFFF;

	// Type
	bool isTrigger = false;
	bool isStatic = false;

};

struct CollisionState
{
	// Current fram collision info 
	bool isGrounded = false;
	bool isTouchingCeiling = false;
	bool isTouchingWallLeft = false;
	bool isTouchingWallRight = false;

	// What we're colliding with
	Entity groundEntity = NULL_ENTITY;
	Vec2 groundNormal = Vec2(0, -1);

	float timeSinceGrounded = 0.0f;
	float timeSinceWallTouch = 0.0f;

	// Clear all flags (called at start of physics update)
	void Clear() {
		isGrounded = false;
		isTouchingCeiling = false;
		isTouchingWallLeft = false;
		isTouchingWallRight = false;
		groundEntity = NULL_ENTITY;
		groundNormal = Vec2(0, -1);
	}
};

template<typename T>
class SparseSet
{
private:
	static const uint32_t INVALID_INDEX = MAX_ENTITIES + 1;

	Array<uint32_t> sparse; // entity -> dense index
	Array<Entity> dense;
	Array<T> data;
	int count = 0;
	int capacity = 0;

	void Grow()
	{
		int newCapacity = capacity == 0 ? 64 : capacity * 2;

		// Create new arrays
		Array<Entity> newDense(newCapacity);
		Array<T> newData(newCapacity);

		// Copy existing data
		for (int i = 0; i < count; i++)
		{
			newDense[i] = dense[i];
			newData[i] = data[i];
		}

		// Swap arrays
		dense = std::move(newDense);
		data = std::move(newData);
		capacity = newCapacity;
	}

public:
	SparseSet() : sparse(MAX_ENTITIES), dense(0), data(0), count(0), capacity(0)
	{
		// Initialize sparse array to invalid indices
		for (int i = 0; i < MAX_ENTITIES; i++)
		{
			sparse[i] = INVALID_INDEX;
		}
	}

	void Add(Entity entity, const T& component)
	{
		if (entity >= MAX_ENTITIES)
		{
			return;
		}

		if (Has(entity))
		{
			// Update existing component
			data[sparse[entity]] = component;
			return;
		}

		// Grow if needed
		if (count >= capacity)
		{
			Grow();
		}

		// Add new component
		sparse[entity] = count;
		dense[count] = entity;
		data[count] = component;
		count++;
	}

	void Remove(Entity entity)
	{
		if (!Has(entity))
		{
			return;
		}

		uint32_t index = sparse[entity];
		uint32_t lastIndex = count - 1;

		// Swap with last element if not already last
		if (index != lastIndex)
		{
			Entity lastEntity = dense[lastIndex];

			dense[index] = lastEntity;
			data[index] = data[lastIndex];
			sparse[lastEntity] = index;
		}

		// Mark as removed
		sparse[entity] = INVALID_INDEX;
		count--;
	}

	T* Get(Entity entity)
	{
		if (!Has(entity))
		{
			return nullptr;
		}

		return &data[sparse[entity]];
	}

	bool Has(Entity entity) const
	{
		if (entity >= MAX_ENTITIES)
		{
			return false;
		}
		uint32_t index = sparse[entity];
		return index < count && dense[index] == entity;
	}

	int Count() const
	{
		return count;
	}

	T& GetData(int index)
	{
		return data[index];
	}

	Entity GetEntity(int index) const
	{
		return dense[index];
	}

	// Clear all components
	void Clear()
	{
		for (int i = 0; i < count; i++)
		{
			sparse[dense[i]] = INVALID_INDEX;
		}
		count = 0;
	}

};

class World {
private:
	Entity nextEntity = 1;

	// Sparse sets for each component type
	SparseSet<Transform> transforms;
	SparseSet<Sprite> sprites;
	SparseSet<Animation> animations;
	SparseSet<Physics> physics;
	SparseSet<Collider> colliders;
	SparseSet<CollisionState> collisionStates;
	SparseSet<Player> players;

public:
	World()
	{

	}
	// Get sparse sets by type
	template<typename T>
	SparseSet<T>* GetSparseSet();

	// Make Entity
	Entity CreateEntity()
	{
		if (nextEntity >= MAX_ENTITIES)
		{
			return NULL_ENTITY;
		}
		return nextEntity++;
	}

	// Add components

	void AddTransform(Entity entity, const Transform& transform)
	{
		transforms.Add(entity, transform);
	}

	void AddSprite(Entity entity, const Sprite& sprite)
	{
		sprites.Add(entity, sprite);
	}

	void AddAnimation(Entity entity, const Animation& animation)
	{
		animations.Add(entity, animation);
	}

	void AddPhysics(Entity entity, const Physics& phys)
	{
		physics.Add(entity, phys);
	}

	void AddCollider(Entity entity, const Collider& collider)
	{
		colliders.Add(entity, collider);
	}

	void AddCollisionState(Entity entity, const CollisionState& state)
	{
		collisionStates.Add(entity, state);
	}

	void AddPlayer(Entity entity, const Player& player)
	{
		players.Add(entity, player);
	}

	// Get components
	Transform* GetTransform(Entity entity)
	{
		return transforms.Get(entity);
	}

	Sprite* GetSprite(Entity entity)
	{
		return sprites.Get(entity);
	}

	Animation* GetAnimation(Entity entity)
	{
		return animations.Get(entity);
	}

	Physics* GetPhysics(Entity entity)
	{
		return physics.Get(entity);
	}

	Collider* GetCollider(Entity entity)
	{
		return colliders.Get(entity);
	}

	CollisionState* GetCollisionState(Entity entity)
	{
		return collisionStates.Get(entity);
	}

	Player* GetPlayer(Entity entity)
	{
		return players.Get(entity);
	}

	// Remove components
	void RemoveTransform(Entity entity)
	{
		transforms.Remove(entity);
	}

	void RemoveSprite(Entity entity)
	{
		sprites.Remove(entity);
	}

	void RemoveAnimation(Entity entity)
	{
		animations.Remove(entity);
	}

	void RemovePhysics(Entity entity)
	{
		physics.Remove(entity);
	}

	void RemoveCollider(Entity entity)
	{
		colliders.Remove(entity);
	}

	void RemoveCollisionState(Entity entity)
	{
		collisionStates.Remove(entity);
	}

	void RemovePlayer(Entity entity)
	{
		players.Remove(entity);
	}

	bool HasTransform(Entity entity)
	{
		return transforms.Has(entity);
	}

	bool HasSprite(Entity entity)
	{
		return sprites.Has(entity);
	}

	bool HasAnimation(Entity entity)
	{
		return animations.Has(entity);
	}

	bool HasPhysics(Entity entity)
	{
		return physics.Has(entity);
	}

	bool HasCollider(Entity entity)
	{
		return colliders.Has(entity);
	}

	bool HasCollisionState(Entity entity)
	{
		return collisionStates.Has(entity);
	}

	bool HasPlayer(Entity entity)
	{
		return players.Has(entity);
	}

	// Remove components from entity
	void DestroyEntity(Entity entity)
	{
		transforms.Remove(entity);
		sprites.Remove(entity);
		animations.Remove(entity);
		physics.Remove(entity);
		colliders.Remove(entity);
		collisionStates.Remove(entity);
		players.Remove(entity);
	}

	// Query for entities with 1 component
	template<typename T1, typename Func>
	void Query(Func func)
	{
		SparseSet<T1>* set1 = GetSparseSet<T1>();

		for (int i = 0; i < set1->Count(); i++)
		{
			Entity entity = set1->GetEntity(i);
			func(entity, set1->GetData(i));
		}
	}

	// Query for entities with 2 components
	template<typename T1, typename T2, typename Func>
	void Query(Func func)
	{
		SparseSet<T1>* set1 = GetSparseSet<T1>();
		SparseSet<T2>* set2 = GetSparseSet<T2>();

		// Iterate smaller set for performance
		if (set1->Count() <= set2->Count())
		{
			for (int i = 0; i < set1->Count(); i++)
			{
				Entity entity = set1->GetEntity(i);
				T2* comp2 = set2->Get(entity);

				if (comp2)
				{
					func(entity, set1->GetData(i), *comp2);
				}
			}
		}
		else
		{
			for (int i = 0; i < set2->Count(); i++)
			{
				Entity entity = set2->GetEntity(i);
				T1* comp1 = set1->Get(entity);

				if (comp1)
				{
					func(entity, *comp1, set2->GetData(i));
				}
			}
		}
	}

	// Query for entities with 3 components
	template<typename T1, typename T2, typename T3, typename Func>
	void Query(Func func)
	{
		SparseSet<T1>* set1 = GetSparseSet<T1>();
		SparseSet<T2>* set2 = GetSparseSet<T2>();
		SparseSet<T3>* set3 = GetSparseSet<T3>();

		for (int i = 0; i < set1->Count(); i++)
		{
			Entity entity = set1->GetEntity(i);
			T2* comp2 = set2->Get(entity);
			T3* comp3 = set3->Get(entity);

			if (comp2 && comp3)
			{
				func(entity, set1->GetData(i), *comp2, *comp3);
			}
		}
	}

	// Query for entities with 4 components
	template<typename T1, typename T2, typename T3, typename T4, typename Func>
	void Query(Func func)
	{
		SparseSet<T1>* set1 = GetSparseSet<T1>();
		SparseSet<T2>* set2 = GetSparseSet<T2>();
		SparseSet<T3>* set3 = GetSparseSet<T3>();
		SparseSet<T4>* set4 = GetSparseSet<T4>();

		for (int i = 0; i < set1->Count(); i++)
		{
			Entity entity = set1->GetEntity(i);
			T2* comp2 = set2->Get(entity);
			T3* comp3 = set3->Get(entity);
			T4* comp4 = set4->Get(entity);

			if (comp2 && comp3 && comp4)
			{
				func(entity, set1->GetData(i), *comp2, *comp3, *comp4);
			}
		}
	}

	// Query for entities with 5 components
	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename Func>
	void Query(Func func)
	{
		SparseSet<T1>* set1 = GetSparseSet<T1>();
		SparseSet<T2>* set2 = GetSparseSet<T2>();
		SparseSet<T3>* set3 = GetSparseSet<T3>();
		SparseSet<T4>* set4 = GetSparseSet<T4>();
		SparseSet<T5>* set5 = GetSparseSet<T5>();

		for (int i = 0; i < set1->Count(); i++)
		{
			Entity entity = set1->GetEntity(i);
			T2* comp2 = set2->Get(entity);
			T3* comp3 = set3->Get(entity);
			T4* comp4 = set4->Get(entity);
			T5* comp5 = set5->Get(entity);

			if (comp2 && comp3 && comp4 && comp5)
			{
				func(entity, set1->GetData(i), *comp2, *comp3, *comp4, *comp5);
			}
		}
	}

	// Query for entities with 6 components
	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename Func>
	void Query(Func func)
	{
		SparseSet<T1>* set1 = GetSparseSet<T1>();
		SparseSet<T2>* set2 = GetSparseSet<T2>();
		SparseSet<T3>* set3 = GetSparseSet<T3>();
		SparseSet<T4>* set4 = GetSparseSet<T4>();
		SparseSet<T5>* set5 = GetSparseSet<T5>();
		SparseSet<T6>* set6 = GetSparseSet<T6>();

		for (int i = 0; i < set1->Count(); i++)
		{
			Entity entity = set1->GetEntity(i);
			T2* comp2 = set2->Get(entity);
			T3* comp3 = set3->Get(entity);
			T4* comp4 = set4->Get(entity);
			T5* comp5 = set5->Get(entity);
			T6* comp6 = set6->Get(entity);

			if (comp2 && comp3 && comp4 && comp5 && comp6)
			{
				func(entity, set1->GetData(i), *comp2, *comp3, *comp4, *comp5, *comp6);
			}
		}
	}

};

template<> inline SparseSet<Transform>* World::GetSparseSet<Transform>() { return &transforms; }
template<> inline SparseSet<Physics>* World::GetSparseSet<Physics>() { return &physics; }
template<> inline SparseSet<Sprite>* World::GetSparseSet<Sprite>() { return &sprites; }
template<> inline SparseSet<Animation>* World::GetSparseSet<Animation>() { return &animations; }
template<> inline SparseSet<Collider>* World::GetSparseSet<Collider>() { return &colliders; }
template<> inline SparseSet<CollisionState>* World::GetSparseSet<CollisionState>() { return &collisionStates; }
template<> inline SparseSet<Player>* World::GetSparseSet<Player>() { return &players; }