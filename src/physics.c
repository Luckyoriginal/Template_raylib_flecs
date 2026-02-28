#ifndef PHYSICS_C
#define PHYSICS_C

#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void ImportPhysic(ecs_world_t *ecs, int algorithm);

/* ── Broadphase algorithm selector ───────────────────────────────── */
#define BROADPHASE_BRUTE    0   /* O(n²)         — simple, always correct          */
#define BROADPHASE_SAP      1   /* O(n log n + k) — sort & sweep on X axis         */
#define BROADPHASE_GRID     2   /* O(n)           — uniform grid, best same-size   */
#define BROADPHASE_QUADTREE 3   /* O(n log n + k) — best for clustered/varying     */

typedef struct {
    int x, y, w, h;
    ecs_entity_t other;
} Collider;

extern ECS_COMPONENT_DECLARE(Collider);
extern ecs_entity_t OnCollision;

#ifdef IMPL_physics

ECS_COMPONENT_DECLARE(Collider);
ecs_entity_t OnCollision;

/* ── shared helpers ─────────────────────────────────────────────── */

static void emit_collision(ecs_world_t *world,
                           ecs_entity_t a, ecs_entity_t b) {
    ecs_event_desc_t desc = {
        .event  = OnCollision,
        .ids    = &(ecs_type_t){
            .array = (ecs_id_t[]){ ecs_id(Collider) }, .count = 1
        },
        .entity = a,
        .param  = (void *)&b,
    };
    ecs_emit(world, &desc);
}

static int aabb_overlap(const Collider *a, const Collider *b) {
    return abs(a->x - b->x) < (a->w + b->w) / 2 &&
           abs(a->y - b->y) < (a->h + b->h) / 2;
}

/* ══════════════════════════════════════════════════════════════════
 * ALGORITHM 0 — BRUTE FORCE
 * Check every pair. O(n²).
 * Best for: n < ~100, debugging, correctness baseline.
 * ══════════════════════════════════════════════════════════════════ */
static void broadphase_brute(ecs_iter_t *it, Collider *c, int n) {
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (aabb_overlap(&c[i], &c[j]))
                emit_collision(it->world, it->entities[i], it->entities[j]);
}

/* ══════════════════════════════════════════════════════════════════
 * ALGORITHM 1 — SORT & SWEEP (SAP)
 * Sort by min_x, sweep forward until max_x exceeded, then check Y.
 * O(n log n + k) where k = pairs found.
 * Best for: general purpose, spread-out entities, varying sizes.
 * ══════════════════════════════════════════════════════════════════ */
typedef struct { int min_x, max_x; int idx; } sap_entry_t;

static int sap_cmp(const void *a, const void *b) {
    return ((sap_entry_t *)a)->min_x - ((sap_entry_t *)b)->min_x;
}

static void broadphase_sap(ecs_iter_t *it, Collider *c, int n) {
    sap_entry_t *entries = malloc(n * sizeof(*entries));
    if (!entries) return;

    for (int i = 0; i < n; i++) {
        entries[i].min_x = c[i].x - c[i].w / 2;
        entries[i].max_x = c[i].x + c[i].w / 2;
        entries[i].idx   = i;
    }
    qsort(entries, n, sizeof(*entries), sap_cmp);

    for (int i = 0; i < n; i++) {
        int a = entries[i].idx;
        for (int j = i + 1; j < n; j++) {
            if (entries[j].min_x > entries[i].max_x) break;
            int b = entries[j].idx;
            if (aabb_overlap(&c[a], &c[b]))
                emit_collision(it->world, it->entities[a], it->entities[b]);
        }
    }
    free(entries);
}

/* ══════════════════════════════════════════════════════════════════
 * ALGORITHM 2 — UNIFORM GRID
 * Divide world into fixed cells; check only same/adjacent cells.
 * O(n) insert + O(n + k) query.
 * Best for: many same-size entities (bullets, particles).
 * Tune GRID_CELL_SIZE to ~2× average entity size.
 * ══════════════════════════════════════════════════════════════════ */
#define GRID_CELL_SIZE    64
#define GRID_W            64
#define GRID_H            64
#define GRID_CELLS        (GRID_W * GRID_H)
#define GRID_MAX_PER_CELL 64

typedef struct {
    int count;
    int indices[GRID_MAX_PER_CELL];
} grid_cell_t;

static void broadphase_grid(ecs_iter_t *it, Collider *c, int n) {
    grid_cell_t *grid = calloc(GRID_CELLS, sizeof(*grid));
    if (!grid) return;
    char *visited = calloc(n * n, 1);
    if (!visited) { free(grid); return; }

    for (int i = 0; i < n; i++) {
        int cx0 = (c[i].x - c[i].w / 2) / GRID_CELL_SIZE;
        int cy0 = (c[i].y - c[i].h / 2) / GRID_CELL_SIZE;
        int cx1 = (c[i].x + c[i].w / 2) / GRID_CELL_SIZE;
        int cy1 = (c[i].y + c[i].h / 2) / GRID_CELL_SIZE;
        if (cx0 < 0) cx0 = 0; if (cx0 >= GRID_W) cx0 = GRID_W - 1;
        if (cy0 < 0) cy0 = 0; if (cy0 >= GRID_H) cy0 = GRID_H - 1;
        if (cx1 < 0) cx1 = 0; if (cx1 >= GRID_W) cx1 = GRID_W - 1;
        if (cy1 < 0) cy1 = 0; if (cy1 >= GRID_H) cy1 = GRID_H - 1;

        for (int gy = cy0; gy <= cy1; gy++)
            for (int gx = cx0; gx <= cx1; gx++) {
                grid_cell_t *cell = &grid[gy * GRID_W + gx];
                if (cell->count < GRID_MAX_PER_CELL)
                    cell->indices[cell->count++] = i;
            }
    }

    for (int cell = 0; cell < GRID_CELLS; cell++) {
        grid_cell_t *gc = &grid[cell];
        for (int i = 0; i < gc->count; i++)
            for (int j = i + 1; j < gc->count; j++) {
                int a = gc->indices[i], b = gc->indices[j];
                if (a > b) { int t = a; a = b; b = t; }
                if (visited[a * n + b]) continue;
                visited[a * n + b] = 1;
                if (aabb_overlap(&c[a], &c[b]))
                    emit_collision(it->world, it->entities[a], it->entities[b]);
            }
    }
    free(visited);
    free(grid);
}

/* ══════════════════════════════════════════════════════════════════
 * ALGORITHM 3 — QUADTREE
 * Recursively subdivide space. Entities that don't fit a child stay
 * in the parent and are checked against everything below.
 * O(n log n + k).
 * Best for: clustered entities, wildly varying sizes.
 * ══════════════════════════════════════════════════════════════════ */
#define QT_MAX_DEPTH    6
#define QT_MAX_PER_NODE 8
#define QT_MAX_NODES    4096

typedef struct {
    int x, y, w, h;
    int children[4];
    int entities[QT_MAX_PER_NODE];
    int count;
} qt_node_t;

typedef struct {
    qt_node_t nodes[QT_MAX_NODES];
    int       used;
} qt_ctx_t;

static int qt_new_node(qt_ctx_t *qt, int x, int y, int w, int h) {
    if (qt->used >= QT_MAX_NODES) return -1;
    int idx = qt->used++;
    qt->nodes[idx] = (qt_node_t){
        .x = x, .y = y, .w = w, .h = h,
        .children = {-1,-1,-1,-1}, .count = 0
    };
    return idx;
}

static void qt_subdivide(qt_ctx_t *qt, int node_idx) {
    qt_node_t *n = &qt->nodes[node_idx];
    int hw = n->w / 2, hh = n->h / 2;
    n->children[0] = qt_new_node(qt, n->x - hw/2, n->y - hh/2, hw, hh);
    n->children[1] = qt_new_node(qt, n->x + hw/2, n->y - hh/2, hw, hh);
    n->children[2] = qt_new_node(qt, n->x - hw/2, n->y + hh/2, hw, hh);
    n->children[3] = qt_new_node(qt, n->x + hw/2, n->y + hh/2, hw, hh);
}

static int qt_fits(qt_node_t *n, Collider *c) {
    return abs(c->x - n->x) + c->w / 2 <= n->w &&
           abs(c->y - n->y) + c->h / 2 <= n->h;
}

static void qt_insert(qt_ctx_t *qt, int node_idx,
                      int entity_idx, Collider *c, int depth) {
    qt_node_t *n = &qt->nodes[node_idx];

    if (depth < QT_MAX_DEPTH) {
        if (n->children[0] == -1 && n->count >= QT_MAX_PER_NODE)
            qt_subdivide(qt, node_idx);

        if (n->children[0] != -1) {
            for (int i = 0; i < 4; i++) {
                int ci = n->children[i];
                if (ci != -1 && qt_fits(&qt->nodes[ci], c)) {
                    qt_insert(qt, ci, entity_idx, c, depth + 1);
                    return;
                }
            }
        }
    }
    if (n->count < QT_MAX_PER_NODE)
        n->entities[n->count++] = entity_idx;
}

static void qt_collide_node(qt_ctx_t *qt, int node_idx,
                             ecs_iter_t *it, Collider *c,
                             int *ancestors, int anc_count) {
    qt_node_t *n = &qt->nodes[node_idx];

    /* Pairs within this node */
    for (int i = 0; i < n->count; i++)
        for (int j = i + 1; j < n->count; j++) {
            int a = n->entities[i], b = n->entities[j];
            if (aabb_overlap(&c[a], &c[b]))
                emit_collision(it->world, it->entities[a], it->entities[b]);
        }

    /* This node's entities vs all ancestor entities */
    for (int i = 0; i < n->count; i++)
        for (int j = 0; j < anc_count; j++) {
            int a = n->entities[i], b = ancestors[j];
            if (aabb_overlap(&c[a], &c[b]))
                emit_collision(it->world, it->entities[a], it->entities[b]);
        }

    if (n->children[0] == -1) return;

    int *new_anc = malloc((anc_count + n->count) * sizeof(int));
    if (!new_anc) return;
    memcpy(new_anc, ancestors, anc_count * sizeof(int));
    memcpy(new_anc + anc_count, n->entities, n->count * sizeof(int));

    for (int i = 0; i < 4; i++)
        if (n->children[i] != -1)
            qt_collide_node(qt, n->children[i], it, c,
                            new_anc, anc_count + n->count);
    free(new_anc);
}

static void broadphase_quadtree(ecs_iter_t *it, Collider *c, int n) {
    int min_x = c[0].x, max_x = c[0].x;
    int min_y = c[0].y, max_y = c[0].y;
    for (int i = 1; i < n; i++) {
        if (c[i].x < min_x) min_x = c[i].x;
        if (c[i].x > max_x) max_x = c[i].x;
        if (c[i].y < min_y) min_y = c[i].y;
        if (c[i].y > max_y) max_y = c[i].y;
    }

    qt_ctx_t *qt = calloc(1, sizeof(*qt));
    if (!qt) return;

    int root = qt_new_node(qt,
        (min_x + max_x) / 2, (min_y + max_y) / 2,
        (max_x - min_x) / 2 + 64, (max_y - min_y) / 2 + 64);

    for (int i = 0; i < n; i++)
        qt_insert(qt, root, i, c, 0);

    qt_collide_node(qt, root, it, c, NULL, 0);
    free(qt);
}

/* ══════════════════════════════════════════════════════════════════
 * SYSTEM DISPATCH
 * ══════════════════════════════════════════════════════════════════ */

static int _algorithm = BROADPHASE_BRUTE;

void BroadPhaseCollision(ecs_iter_t *it) {
    Collider *c = ecs_field(it, Collider, 0);
    int n = it->count;
    if (n < 2) return;

    switch (_algorithm) {
        case BROADPHASE_SAP:      broadphase_sap(it, c, n);      break;
        case BROADPHASE_GRID:     broadphase_grid(it, c, n);     break;
        case BROADPHASE_QUADTREE: broadphase_quadtree(it, c, n); break;
        default:                  broadphase_brute(it, c, n);    break;
    }
}

void DebugCollision(ecs_iter_t *it) {
    ecs_entity_t *other = it->param;
    for (int i = 0; i < it->count; i++) {
        printf("collision: entity %llu hit %llu\n",
               (unsigned long long)it->entities[i],
               (unsigned long long)*other);
    }
}

void ImportPhysic(ecs_world_t *ecs, int algorithm) {
    _algorithm = algorithm;

    ECS_MODULE(ecs, PhysicModule);
    ECS_COMPONENT_DEFINE(ecs, Collider);

    OnCollision = ecs_new(ecs);

    ECS_SYSTEM(ecs, BroadPhaseCollision, EcsOnUpdate, Collider);

    ecs_observer(ecs, {
        .query.terms = {{ ecs_id(Collider) }},
        .events      = { OnCollision },
        .callback    = DebugCollision,
    });
}

#endif  /* IMPL_physics */
#endif  /* PHYSICS_C */
