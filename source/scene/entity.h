#ifndef _ENTITY_H
#define _ENTITY_H

#include "../utility/types.h"
#include "../utility/dense_queue.h"

#define MAX_ENTITY_COMPONENTS	64


typedef h32 EntityId;
typedef h32 ComponentId;
typedef size_t ComponentType;

DenseQueueTyped(EntityId, EntityIdQueue);


/**
 * Macro to create a ComponentType value for use with component masks, a *Component and
 * DenseMap to store the component along with its parent EntityId.
 * TypeId should be unique and sequential, starting at 0. This is converted into a pwr-2
 * ComponentType value for use with component masks.
 */
#define ComponentStore(Type, StoreTypeName, HndType, TypeId, StoreName, _capacity) \
	static const ComponentType Type##ComponentType = 1UL << TypeId;\
	struct Type##Component {\
		Type		data;\
		EntityId	entityId;\
	};\
	DenseHandleMap16TypedWithBuffer(Type##Component, StoreTypeName, HndType, TypeId, _capacity);\
	StoreTypeName StoreName;


struct ComponentSet {
	u64			mask;
	u32			componentsSize;
	ComponentId	components[MAX_ENTITY_COMPONENTS];
};


// TODO: Entity can be a messaging (event) hub, both sending and handling events, allowing for direct
// dispatch, broadcast, observer, pub/sub, and event bubbling up and down
struct Entity {
	ComponentSet	sceneComponents;
	ComponentSet	renderComponents;
	ComponentSet	gameComponents;
};


/**
 * Uses a component mask to quickly see if all components in the mask exist in this entity
 * @return true if all component types exist at least once, false if not
 */
inline bool entity_hasAllComponents(ComponentSet& set, u64 mask)
{
	return ((set.mask & mask) == mask);
}

/**
 * Uses a component mask to quickly see if any components in the mask exist in this entity
 * @return true if any component type exists at least once, false if not
 */
inline bool entity_hasAnyComponents(ComponentSet& set, u64 mask)
{
	return ((set.mask & mask) != 0);
}

/**
 * Quickly check if any components of a single type exist in this entity
 * @return true if any component type exists at least once, false if not
 */
inline bool entity_hasComponentType(ComponentSet& set, ComponentType ct)
{
	return ((set.mask >> ct) & 1UL);
}


/**
 * Returns true if the component exists in the entity.
 */
bool entity_hasComponent(
	ComponentSet& set,
	ComponentId id)
{
	for (u32 c = 0;
		c < set.componentsSize;
		++c)
	{
		if (set.components[c] == id) {
			return true;
		}
	}
	return false;
}


ComponentId entity_getFirstComponent(
	ComponentSet& set,
	ComponentType ct)
{
	if (entity_hasComponentType(set, ct))
	{
		for (u32 c = 0;
			c < set.componentsSize;
			++c)
		{
			ComponentId cmpId = set.components[c];
			if (ct == 1ULL << cmpId.typeId) {
				return cmpId;
			}
		}
	}
	return null_h32;
}


/**
 * Adds a scene component id to the set. The corresponding component should already exist in
 * the component store.
 * @return true if the id is added, false if the id is already present
 */
bool entity_addComponent(
	ComponentSet& set,
	ComponentId id)
{
	assert(set.componentsSize < MAX_ENTITY_COMPONENTS
			&& "max entity components reached, consider raising the limit and expanding the bit mask, or combine components");

	if (!entity_hasComponent(set, id)) {
		ComponentType ct = 1ULL << id.typeId;
		set.mask |= ct;

		set.components[set.componentsSize] = id;
		++set.componentsSize;
		
		return true;
	}
	return false;
}


/**
 * Removes id from the set. Does not remove the component from the store.
 * @return true if component was removed, false if not present
 */
bool entity_removeComponent(
	ComponentSet& set,
	ComponentId id)
{
	ComponentType ct = 1ULL << id.typeId;
	bool hasAnotherMatchingComponentType = false;
	bool removed = false;

	for (u32 c = 0;
		c < set.componentsSize;
		++c)
	{
		// remove by swap and pop
		if (set.components[c] == id) {
			if (c != set.componentsSize - 1) {
				set.components[c] = set.components[set.componentsSize - 1];
			}
			--set.componentsSize;
			removed = true;
		}
		
		if (set.components[c].typeId == id.typeId) {
			hasAnotherMatchingComponentType = true;
		}
	}

	// if no more components of this type, clear it in the mask
	if (removed && !hasAnotherMatchingComponentType) {
		set.mask &= ~(1ULL << ct);
	}

	return removed;
}


/**
 * Removes all components of every type in the mask. Does not remove components from the store.
 * @return true if at least one component was removed
 */
bool entity_removeComponentsByType(
	ComponentSet& set,
	ComponentType mask)
{
	if (entity_hasAnyComponents(set, mask)) {
		u32 c = 0;
		while (c < set.componentsSize)
		{
			ComponentType ct = 1ULL << set.components[c].typeId;
			// remove by swap and pop
			if ((ct & mask) != 0ULL) {
				if (c != set.componentsSize - 1) {
					set.components[c] = set.components[set.componentsSize - 1];
				}
				--set.componentsSize;
			}
			else {
				++c;
			}
		}

		// clear bits for removed component types
		set.mask &= ~mask;

		return true;
	}
	return false;
}


#endif