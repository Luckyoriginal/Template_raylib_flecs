#ifndef MAIN_C
#define MAIN_C

#ifdef IMPL_main
#include <flecs.h>
#include "graphics.c"
#include "physics.c"
#include <raylib.h>
#include "utils.c"
#include "utility.c"
#include <stdio.h>


int main(void) {
	ecs_world_t* world = ecs_init();

	ECS_IMPORT(world,GraphicModule);
	ECS_IMPORT(world,PhysicModule);
	ECS_IMPORT(world,UtilityModule);
	PhysicSetAlgorithm(BROADPHASE_SAP);

	ecs_insert(world, 
		ecs_value(Box, {.x=32,.y=32,.w=32,.h=32,.color=RED}),
		ecs_value(Collider, {.x=32,.y=32,.w=32,.h=32}),
		ecs_value(Velocity, {.x=1,.y=1})
		);
	//ecs_insert(world, ecs_value(Collider, {.x=32,.y=32,.w=32,.h=32,.other=0}));
	//ecs_insert(world, ecs_value(Collider, {.x=52,.y=32,.w=32,.h=32,.other=0}));

	InitWindow(800 , 600 , "hello");
	SetTargetFPS(40);
	while(!WindowShouldClose()){
		BeginDrawing();
		ClearBackground(WHITE);
		ecs_progress(world , 0);
		EndDrawing();
	}
	ecs_fini(world);
	CloseWindow();
	return 0;
}

#endif  
#endif  
