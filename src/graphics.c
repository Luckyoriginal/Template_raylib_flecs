#ifndef GRAPHICS_C
#define GRAPHICS_C
#include <flecs.h>
#include <flecs/addons/flecs_c.h>
#include <raylib.h>
#include "utils.c"

typedef struct {
	int x,y,w,h;
	Color color;
}Box;

extern ECS_COMPONENT_DECLARE(Box);

void GraphicModuleImport(ecs_world_t* ecs);
void DrawBoxSystem(ecs_iter_t* it);

#ifdef IMPL_graphics
ECS_COMPONENT_DECLARE(Box);

void GraphicModuleImport(ecs_world_t* ecs){
	ECS_MODULE(ecs, GraphicModule);
	ECS_COMPONENT_DEFINE(ecs, Box);
	ECS_SYSTEM(ecs,DrawBoxSystem,EcsOnUpdate, Box);
}

void DrawBoxSystem(ecs_iter_t* it ){
	Box* box = ecs_field(it, Box, 0);
	iter(i, it->count){
		DrawRectangle(box[i].x, box[i].y, box[i].h, box[i].w, box[i].color);
	}
}
#endif  
#endif 
