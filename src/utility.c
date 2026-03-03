#ifndef UTILITY_C
#define UTILITY_C
#include <flecs.h>
#include "physics.c"
#include "graphics.c"


void UtilityModuleImport(ecs_world_t* ecs);

#ifdef IMPL_utility

void PhysicGraphic(ecs_iter_t* it){
	Box* box = ecs_field(it, Box, 0);
	Collider* collider = ecs_field(it,Collider, 1);
	iter(i,it->count){
		box[i].x = collider[i].x;
		box[i].y = collider[i].y;
	}
}

void UtilityModuleImport(ecs_world_t* ecs){
	ECS_MODULE(ecs, UtilityModule);
	ECS_IMPORT(ecs, PhysicModule);
	ECS_IMPORT(ecs, GraphicModule);
	ECS_SYSTEM(ecs , PhysicGraphic, EcsOnUpdate, graphic.module.Box, physic.module.Collider);
}


#endif  /* IMPL_utility */
#endif  /* UTILITY_C */
