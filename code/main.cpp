
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/**
 * TODO(talha)
 * Understanding how texture rendering works:
 * * Look into glfw since raylib uses glfw for any subsequent
 * * drawing as a means of using opengl
 * Understanding how to do collision detection
 * Understanding triangle coloring
 */
const int screenWidth = 800;
const int screenHeight = 450;
// random triangle bounds
int triangleMinWidth = (screenHeight / 2) - 100;
int triangleMaxHeight = (screenHeight / 2) + 100;
int triangleMinHeight = (screenWidth / 4) - 50;
int triangleMaxWidth = (screenWidth / 4) + 50;
int heightRange = triangleMaxHeight - triangleMinHeight;
int widthRange = triangleMaxWidth - triangleMinWidth;
const int trianglesSz = 8;

#define Abs(x) (x < 0 ? x*-1: x)
#define Square(x) (x*x)
#define Det(u, v) ((u.x*v.y) - (u.y*v.x))
#define ORIENT_TOP_BOT 0
#define ORIENT_BOT_TOP 1

/**
 * Note(Talha): 
 * * Basic Triangle struct(self implemented), just contains info of width and height
 * * This is meant to be used in conjunction with extracting raw coords on screen
 */
struct NaviTriangle {
	int id;
	float width; 
	float height;
	float inset; // how much is triangle towards played ( for shifting triangle to give a rolling window effect )
	bool orientation; // how is triangle placed
	bool draw; // should draw or not?
	Vector2 v1; // Meant to store coordinates that are computed from width and height
	Vector2 v2;
	Vector2 v3; // 38 bytes
};

/**
 * NOTE(Talha) DDA Line Drawing Algorithm, SIMPLE VERSION
 * compute dy, dx
 * compute number of steps required for placing pixels:
 * -> MAX(ABS(dy), ABS(dx))
 * compute the increment for each coordinate
 * -> x_inc = dx/steps;
 * -> y_inc = dy/steps;
 * do
 * x_k+1 = x_k (starting point) + x_inc
 * y_k+1 = y_k (starting point) + y_inc
 * draw a pixel at point (x_k+1, y_k+1)
 * until run steps times i.e ${iterator} == steps
 */
void DDALineDrawing(Vector2 startPoint, Vector2 endPoint) {
	float dy = endPoint.y - startPoint.y;
	float dx = endPoint.x - startPoint.x;
	float x = startPoint.x;
	float y = startPoint.y;
	float y_k, x_k;
	int steps = Abs(dy) > Abs(dx) ? Abs(dy) : Abs(dx);
	float incX = dx / (float) steps;
	float incY = dy / (float) steps;
	int step = 0;
	do {
		y_k = y + incY;
		x_k = x + incX;
		y = y_k;
		x = x_k;
		DrawPixel(round(x), round(y), BLACK);
		step++;
	} while (step < steps);
}

/**
	* Computes centroid from 3 triangle points
	*/
Vector2 computeTriangleCentroid(NaviTriangle triangle) {
	float x = (triangle.v1.x + triangle.v2.x + triangle.v3.x)/3;
	float y = (triangle.v1.y + triangle.v2.y + triangle.v3.y)/3;
	Vector2 centroid = {x, y};
	return centroid;
}

void computeRawTriangleCoords(NaviTriangle *triangle) {
	float midpoint;
	float xMax = screenWidth - triangle->inset;
	Vector2 v1, v2, v3;

	if (triangle->orientation == ORIENT_TOP_BOT) {
		v1 = { xMax - triangle->width, 0 };
		v2 = { xMax, 0 };
		midpoint = (v2.x + v1.x) / 2.0f;
		v3 = { midpoint, triangle->height };
		triangle->v1 = v1; 
		triangle->v2 = v2;
		triangle->v3 = v3;
	}
	if (triangle->orientation == ORIENT_BOT_TOP) {
		v1 = { xMax - triangle->width, screenHeight };
		v2 = { xMax, screenHeight };
		midpoint = (v2.x + v1.x) / 2.0f;
		v3 = { midpoint, screenHeight - triangle->height };
		triangle->v1 = v1; 
		triangle->v2 = v2;
		triangle->v3 = v3;
	}
}

void drawTriangle(NaviTriangle *triangle) {
	// counter clock-wise line-drawing
	// a->b, b->c, c->a
	DDALineDrawing(triangle->v1, triangle->v2);
	DDALineDrawing(triangle->v2, triangle->v3);
	DDALineDrawing(triangle->v3, triangle->v1);
}

void randomizeTriangleDim( NaviTriangle *triangle) {
	
	int rWidth = (rand() % widthRange) + triangleMinWidth;
	int rHeight = (rand() % heightRange) + triangleMinHeight;
	triangle->width = rWidth;
	triangle->height = rHeight;
}

/**
 * Initialize triangles array, and compute initial dims
 */
void initializeTriangles(NaviTriangle triangles[trianglesSz]) {
	bool dir = ORIENT_TOP_BOT; // also false, just need to keep flipping this
	int sz = trianglesSz;
	for (int i=0; i<sz; i++) {
		NaviTriangle triangle = { i, NULL, NULL, NULL, dir, NULL,
								NULL, NULL, NULL};
		triangle.draw = i == 0 ? true : false; // only draw the first triangle initially
		randomizeTriangleDim(&triangle);
		triangle.inset = -triangle.width;
		triangles[i] = triangle;
		// flip dir
		dir = !dir;
	}
}

/**
 * Checks and moves triangles across screen, 
 * if they are out of screen: 
 * * switches draw OFF
 * * it recomputes random dims, to keep the game flow going
 */
void trianglesMoveOrRecompute(NaviTriangle triangles[trianglesSz]) {
	int sz = trianglesSz;
	for (int i = 0; i < sz; i++) {
		NaviTriangle triangle = triangles[i];
		if (triangle.draw) {
			if (triangle.inset < screenWidth) {
				triangle.inset += 5;
			} else {
				triangle.draw = false;
				randomizeTriangleDim(&triangle);
				// we want an effect of triangle coming from right of screen instead of spawning, 
				// give inset -width
				triangle.inset = -triangle.width;
			}
			computeRawTriangleCoords(&triangle);
			triangles[i] = triangle;
		}
	}
}

/**
 * Function checks if there is enough space to draw each triangle, 
 * and switches on draw value
 * @mark TODO:
 * * can make the width between triangles less as speed increases
 * * to add a layer of difficulty
 */
void trianglesCheckDrawSpace(NaviTriangle triangles[trianglesSz]) {
	int sz = trianglesSz;
	if (sz == 1) return;
	for (int i = 0; i < sz; i++) {
		int prevInd = (sz + i - 1 ) % sz;
		NaviTriangle trianglePrev = triangles[prevInd];
		NaviTriangle triangleCurr = triangles[i];
		int tPrevHalfDist = trianglePrev.inset + (trianglePrev.width / 2);
		if (!triangleCurr.draw && tPrevHalfDist > 0) {
			// computing the position of the first half of the prev triangle on screen
			int trianglePrevEndHalf = (screenWidth - trianglePrev.inset) - (trianglePrev.width/2);
			int spaceAfter = screenWidth - trianglePrevEndHalf;
			// checking if there is atleast 1/4 of current triangles width on screen
			if (spaceAfter > (triangleCurr.width)/4) { 
				triangleCurr.draw = true;
				triangles[i] = triangleCurr;
			}
		}
	}
}

/**
 * Draw triangles on screen that have space to be drawn
 */
void trianglesDraw(NaviTriangle triangles[trianglesSz]) {
	int sz = trianglesSz;
	for (int i = 0; i < sz; i++) {
		NaviTriangle triangle = triangles[i];
		if (triangle.draw) {
			drawTriangle(&triangle);
		}
	}
}

float computeDistanceBwPoints(Vector2 v1, Vector2 v2) {
	float x_comp = Square((v2.x - v1.x));
	float y_comp = Square((v2.y - v1.y));
	float dist = sqrt(x_comp + y_comp);
	return dist;
}

/**
 * Get triangle closes to player by matching x and y position
 * we compare the centroid of the triangles for easier comparison ( until a better appraoch is found )
 */
NaviTriangle GetTriangleClosest(NaviTriangle triangles[trianglesSz], NaviTriangle playerTriangle) {
	NaviTriangle closestTriangle;
	Vector2 playerCentroid = computeTriangleCentroid(playerTriangle);
	float minDiff = INT_MAX;
	for (int i = 0; i < trianglesSz; i++) {
		NaviTriangle triangle = triangles[i];
		Vector2 triangleCentroid = computeTriangleCentroid(triangle);
		float pointDiff = computeDistanceBwPoints(playerCentroid, triangleCentroid);
		if (pointDiff < minDiff) {
			minDiff = pointDiff;
			closestTriangle = triangle;
		}
	}
	return closestTriangle;
}

/**
 * Compute if triangle is interior
 * @mark TODO: Understand barycentric maths to understand this function
 */
bool pointInTriangleInterior(NaviTriangle triangle, Vector2 playerPoint) {
	// choose an origin point for triangle v0, lets say vertex v1
	Vector2 A = triangle.v1;
	// compute vectors relative to origin point now
	Vector2 p1 = triangle.v2;
	Vector2 B = { p1.x - A.x, p1.y - A.y };
	Vector2 p2 = triangle.v3;
	Vector2 C = { p2.x - A.x, p2.y - A.y };
	Vector2 P = { playerPoint.x - A.x, playerPoint.y - A.y };
	
	float d = Det(B, C);
	Vector2 CB = { B.x - C.x, B.y - C.y };
	float w_a = (Det(P, CB) + Det(B, C)) / d;
	float w_b = Det(P, C) / d;
	float w_c = Det(B, P) / d;

	// point is inside if all 0<= w_n <= 1
	if ((w_a >= 0 && w_a <= 1) && (w_b >= 0 && w_b <= 1)
		&& (w_c >= 0 && w_c <= 1)) {
		return true;
	}
	return false;
}

/**
 * Detect if the player triangles collides with any of the randomly generated game obstacle triangles
 * Using:
 * * Vertex Matching for triangle closest to player position:
 * * * If any point of player falls in the closest triangle
 * * * using triangle interior formula (https://mathworld.wolfram.com/TriangleInterior.html#:~:text=The%20simplest%20way%20to%20determine,it%20lies%20outside%20the%20triangle.),
 * * * We are checking here if the triangle with player point forms a convex hull for the triangle,
 * * * if it exists independant of the player
 * * * * point is outside of the triangle
 */
bool isPlayerInTriangle(NaviTriangle triangles[trianglesSz], NaviTriangle playerTriangle) {
	NaviTriangle closestTriangle = GetTriangleClosest(triangles, playerTriangle);
	// for each player vertex check if it falls in triangle
	// check if it falls on any point that is computed for our random triangle
	Vector2 playerPoint = playerTriangle.v1;
	bool isInside = pointInTriangleInterior(closestTriangle, playerPoint);
	playerPoint = playerTriangle.v2;
	isInside = isInside || pointInTriangleInterior(closestTriangle, playerPoint);
	playerPoint = playerTriangle.v3;
	isInside = isInside || pointInTriangleInterior(closestTriangle, playerPoint);
	
	return isInside;
}

int main()
{
	// Initialization
	//--------------------------------------------------------------------------------------
	InitWindow(screenWidth, screenHeight, "Navi(gate)Maze");
	// TODO: Load resources / Initialize variables at this point
	SetTargetFPS(60);
	//--------------------------------------------------------------------------------------
	/**
	 * Random Triangle Coords Generation
	 * width min: 80
	 * width max: triangleMaxWidth;
	 * height min: 100
	 * height max: triangleMaxHeight;
	 */
	//--------------------------------------------------------------------------------------
	// Main game loop
	int frameCounter = 1;
	srand(time(NULL)); // Seed RAND
	
	// need array of triangles [t, b, t, b, t, b, t, b] => 8*32 = 256 bytes NOICE! very binary
	NaviTriangle triangles[trianglesSz];
	// initializing triangle with null dim and inset, will randomize them
	initializeTriangles(triangles);
	NaviTriangle playerTriangle;
	playerTriangle.draw = true;
	playerTriangle.inset = -1;
	playerTriangle.v1 = { 100, 100 };
	playerTriangle.v2 = { 120, 115 };
	playerTriangle.v3 = { 100, 130 };
	bool gamePlay = true;
	bool isCollision = false;
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		// TODO: Update variables / Implement example logic at this point
		//----------------------------------------------------------------------------------
		if (IsKeyPressed(KEY_SPACE)) {
			// pause/resume game
			gamePlay = !gamePlay;
		}
		if (gamePlay) {
			trianglesMoveOrRecompute(triangles);
			trianglesCheckDrawSpace(triangles);
		}
		isCollision = isPlayerInTriangle(triangles, playerTriangle);
		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();
		ClearBackground(DARKGRAY);
		trianglesDraw(triangles);
		if (isCollision) {
			//gamePlay = false;
			const char *status = "collision";
			DrawText(status, (screenWidth/2), 10, 20, GREEN);

		}
		drawTriangle(&playerTriangle);
		// TODO: Draw everything that requires to be drawn at this point:
		DrawFPS(0, 0);
		EndDrawing();
		//----------------------------------------------------------------------------------
		frameCounter++;
	}
	
	// De-Initialization
	//--------------------------------------------------------------------------------------
	// TODO: Unload all loaded resources at this point
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
	return 0;
}