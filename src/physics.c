#ifndef PHYSICS_C
#define PHYSICS_C

#include "flecs.h"
#include <flecs/addons/flecs_c.h>
#include <stdio.h>
#include <stdlib.h>       /* abs() */

void ImportPhysic(ecs_world_t *ecs);

typedef struct {
    int x, y, w, h;
    ecs_entity_t other;
} Collider;

extern ECS_COMPONENT_DECLARE(Collider);
extern ecs_entity_t OnCollision;
#ifdef IMPL_physics

ECS_COMPONENT_DECLARE(Collider);
ecs_entity_t OnCollision;

void BroadPhaseCollision(ecs_iter_t *it) {
    Collider *collider = ecs_field(it, Collider, 0);

    ecs_defer_begin(it->world);        /* buffer ecs_set — can't mutate during iteration */
    for (int i = 0; i < it->count; i++) {
        for (int j = 0; j < it->count; j++) {
            if (i == j) continue;
            if (abs(collider[i].x - collider[j].x) < collider[i].w) {
		    ecs_event_desc_t desc = {
			    .event = OnCollision,
			    .ids = &(ecs_type_t){.array = (ecs_id_t[]){ecs_id(Collider)}, .count=1},
			    .entity = it->entities[i],
			    .param = (void *)&it->entities[j],
		    };
		    ecs_emit(it->world, &desc);
            }
        }
    }
    ecs_defer_end(it->world);
}

void DebugCollision(ecs_iter_t *it) {
	ecs_entity_t *other = it->param;
    for (int i = 0; i < it->count; i++) {
        printf("collision: entity %llu hit %llu\n",
               (unsigned long long)it->entities[i],
               (unsigned long long)*other);  /* was collider->other */
    }
}

void ImportPhysic(ecs_world_t *ecs) {
    ECS_MODULE(ecs, PhysicModule);
    ECS_COMPONENT_DEFINE(ecs, Collider);
	
    OnCollision = ecs_new(ecs);
    ECS_SYSTEM(ecs, BroadPhaseCollision, EcsOnUpdate, Collider);
    ECS_OBSERVER(ecs, DebugCollision, OnCollision, Collider);
}

#endif  /* IMPL_physics */
#endif  /* PHYSICS_C */
