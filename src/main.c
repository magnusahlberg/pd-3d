//
//  main.c
//  Extension
//
//  Created by Dave Hayden on 7/30/14.
//  Copyright (c) 2014 Panic, Inc. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

//#define M_PI 3.14159265358979323846

static int update(void* userdata);
void _fini(void) {}

struct vec3d 
{
	float x, y, z;
};

struct triangle
{
	struct vec3d p[3];
};

struct mesh {
	struct triangle* triangles;
	size_t numTriangles;
};

struct mat4x4 
{
	float m[4][4];
};

void MultiplyMatrixVector(struct vec3d* out, const struct mat4x4* m, const struct vec3d* v)
{
	out->x = m->m[0][0] * v->x + m->m[0][1] * v->y + m->m[0][2] * v->z + m->m[0][3];
	out->y = m->m[1][0] * v->x + m->m[1][1] * v->y + m->m[1][2] * v->z + m->m[1][3];
	out->z = m->m[2][0] * v->x + m->m[2][1] * v->y + m->m[2][2] * v->z + m->m[2][3];
	float w = m->m[3][0] * v->x + m->m[3][1] * v->y + m->m[3][2] * v->z + m->m[3][3];
	
	if (w != 0.0f) {
		out->x /= w;
		out->y /= w;
		out->z /= w;
	}
}

void destroyMesh(struct mesh* m)
{
	free(m->triangles);
	free(m);
}

struct mesh* createMeshWithTriangles(const struct triangle* triangles, size_t numTriangles)
{
    struct mesh* m = malloc(sizeof(struct mesh));
    if (!m) {
//        fprintf(stderr, "Failed to allocate memory for mesh.\n");
        exit(EXIT_FAILURE);
    }
    m->triangles = malloc(numTriangles * sizeof(struct triangle));
    if (!m->triangles) {
//        fprintf(stderr, "Failed to allocate memory for triangles.\n");
        free(m);
        exit(EXIT_FAILURE);
    }
    m->numTriangles = numTriangles;

    // Copy the provided triangles into the mesh
    for (size_t i = 0; i < numTriangles; i++) {
        m->triangles[i] = triangles[i];
    }

    return m;
}

struct mesh* meshCube;

struct mat4x4 projection;

int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed

    if (event == kEventInit)
    {
        // Define all triangles in one go
        struct triangle cubeTriangles[] = {
            // SOUTH
            {{ {0, 0, 0}, {0, 1, 0}, {1, 1, 0} }},
            {{ {0, 0, 0}, {1, 1, 0}, {1, 0, 0} }},
            // EAST
            {{ {1, 0, 0}, {1, 1, 0}, {1, 1, 1} }},
            {{ {1, 0, 0}, {1, 1, 1}, {1, 0, 1} }},
            // NORTH
            {{ {1, 0, 1}, {1, 1, 1}, {0, 1, 1} }},
            {{ {1, 0, 1}, {0, 1, 1}, {0, 0, 1} }},
            // WEST
            {{ {0, 0, 1}, {0, 1, 1}, {0, 1, 0} }},
            {{ {0, 0, 1}, {0, 1, 0}, {0, 0, 0} }},
            // TOP
            {{ {0, 1, 0}, {0, 1, 1}, {1, 1, 1} }},
            {{ {0, 1, 0}, {1, 1, 1}, {1, 1, 0} }},
            // BOTTOM
            {{ {1, 0, 0}, {1, 0, 1}, {0, 0, 1} }},
            {{ {1, 0, 0}, {0, 0, 1}, {0, 0, 0} }},
        };

        // Create the mesh using the predefined triangles
        meshCube = createMeshWithTriangles(cubeTriangles, sizeof(cubeTriangles) / sizeof(cubeTriangles[0]));

		float fNear = 0.1f;
		float fFar = 1000.0f;
		float fFov = 90.0f;
		float fAspect = (float)pd->display->getHeight() / (float)pd->display->getWidth();
		float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * M_PI);
		
		memset(&projection, 0, sizeof(projection));
		projection.m[0][0] = fAspect * fFovRad;
        projection.m[1][1] = fFovRad;
        projection.m[2][2] = fFar / (fFar - fNear);
        projection.m[2][3] = 1.0f;
        projection.m[3][2] = (-fFar * fNear) / (fFar - fNear);
        projection.m[3][3] = 0.0f;

		pd->display->setRefreshRate(50);
		// Set the update callback
        pd->system->setUpdateCallback(update, pd);
    }

    return 0;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;
	
	pd->graphics->clear(kColorWhite);

	struct mat4x4 matRotZ, matRotX;
	float fTheta = 2.0f * pd->system->getElapsedTime();
	float fTheta2 = 0.5f * pd->system->getElapsedTime();

	// Rotation matrices
	memset(&matRotZ, 0, sizeof(matRotZ));
	matRotZ.m[0][0] = cosf(fTheta); 
	matRotZ.m[0][1] = sinf(fTheta);
	matRotZ.m[1][0] = -sinf(fTheta);
	matRotZ.m[1][1] = cosf(fTheta);
	matRotZ.m[2][2] = 1.0f; 
	matRotZ.m[3][3] = 1.0f;

	memset(&matRotX, 0, sizeof(matRotX));
	matRotX.m[0][0] = 1.0f;
	matRotX.m[1][1] = cosf(fTheta2);
	matRotX.m[1][2] = sinf(fTheta2);
	matRotX.m[2][1] = -sinf(fTheta2);
	matRotX.m[2][2] = cosf(fTheta2);
	matRotX.m[3][3] = 1.0f;

	// Draw the mesh
	for (size_t i = 0; i < meshCube->numTriangles; i++)
	{
		struct triangle tProjected, tTranslated, tRotatedZ, tRotatedXZ;
		struct triangle* t = &meshCube->triangles[i];

		// Rotate around the Z axis
		MultiplyMatrixVector(&tRotatedZ.p[0], &matRotZ, &t->p[0]);
		MultiplyMatrixVector(&tRotatedZ.p[1], &matRotZ, &t->p[1]);
		MultiplyMatrixVector(&tRotatedZ.p[2], &matRotZ, &t->p[2]);
		
		// Rotate around the X axis
 		MultiplyMatrixVector(&tRotatedXZ.p[0], &matRotX, &tRotatedZ.p[0]);
		MultiplyMatrixVector(&tRotatedXZ.p[1], &matRotX, &tRotatedZ.p[1]);
		MultiplyMatrixVector(&tRotatedXZ.p[2], &matRotX, &tRotatedZ.p[2]);
		
		// Translate the triangle
		tTranslated = tRotatedXZ;
		tTranslated.p[0].z += 10.0f;
		tTranslated.p[1].z += 10.0f;
		tTranslated.p[2].z += 10.0f;

		MultiplyMatrixVector(&tProjected.p[0], &projection, &tTranslated.p[0]);
		MultiplyMatrixVector(&tProjected.p[1], &projection, &tTranslated.p[1]);
		MultiplyMatrixVector(&tProjected.p[2], &projection, &tTranslated.p[2]);
		
		tProjected.p[0].x += 1.0f; tProjected.p[0].y += 1.0f;
		tProjected.p[1].x += 1.0f; tProjected.p[1].y += 1.0f;
		tProjected.p[2].x += 1.0f; tProjected.p[2].y += 1.0f;

		tProjected.p[0].x *= 0.5f * (float)pd->display->getWidth();
		tProjected.p[0].y *= 0.5f * (float)pd->display->getHeight();
		tProjected.p[1].x *= 0.5f * (float)pd->display->getWidth();
		tProjected.p[1].y *= 0.5f * (float)pd->display->getHeight();
		tProjected.p[2].x *= 0.5f * (float)pd->display->getWidth();
		tProjected.p[2].y *= 0.5f * (float)pd->display->getHeight();

		pd->graphics->drawLine(tProjected.p[0].x, tProjected.p[0].y, tProjected.p[1].x, tProjected.p[1].y, 1, kColorBlack);
		pd->graphics->drawLine(tProjected.p[1].x, tProjected.p[1].y, tProjected.p[2].x, tProjected.p[2].y, 1, kColorBlack);
		pd->graphics->drawLine(tProjected.p[2].x, tProjected.p[2].y, tProjected.p[0].x, tProjected.p[0].y, 1, kColorBlack);
	}
        
	pd->system->drawFPS(0,0);

	return 1;
}
