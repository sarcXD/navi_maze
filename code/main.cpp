
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define Abs(x) (x < 0 ? x*-1: x)
#define Square(x) (x*x)
#define Det(u, v) ((u.x*v.y) - (u.y*v.x))
#define Pi 3.141592653589793
#define RadToDeg(rad) (rad*180/Pi)
#define DegToRad(deg) (deg*Pi/180)

#define ORIENT_TOP_BOT 0
#define ORIENT_BOT_TOP 1
#define TRANS_OP_SUB -1.0
#define TRANS_OP_ADD 1.0

/**
	* DONE:
	* Traingle generation
	* Verifying if point in triangle
	* Triangle rotation
	* 
	* Current:
	* Implement scoring system
	* 
	* TODO(talha)
	* Understanding how texture rendering works:
	* * Look into glfw since raylib uses glfw for any subsequent
	* * drawing as a means of using opengl
	* Understanding triangle coloring
	* Implement increase in triangle speed with time
	*/
const int screenWidth = 800;
const int screenHeight = 450;
// random triangle bounds
int triangleMinWidth = (screenWidth / 4) - 50;
int triangleMaxWidth = (screenWidth / 4) + 50;
int triangleMinHeight = (screenHeight / 2) - 50;
int triangleMaxHeight = (screenHeight / 2);

int heightRange = triangleMaxHeight - triangleMinHeight;
int widthRange = triangleMaxWidth - triangleMinWidth;
const int trianglesSz = 8;
float minAngle = DegToRad(-120);
float maxAngle = DegToRad(120);
float rotationRate = .07;
float insetStep = 5;

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
	Vector2 v3;
	float movementRate; // how quickly object should move on screen
	
	// things only for rotation
	int rotationDir = -1; // which direction to rotate object
	bool hitMin = false; // has object reached min angle in rotation
	bool hitMax = false; // has object reached max angle in rotation
	float currentAngle = DegToRad(0);

	// things only for movement
	float nextStep; // how much triangle should move vertically Depends on currentAngle
}; // 54 bytes

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
	float steps = Abs(dy) > Abs(dx) ? Abs(dy) : Abs(dx);
	float incX = dx / steps;
	float incY = dy / steps;
	float step = 0;
	do {
		y_k = y + incY;
		x_k = x + incX;
		y = y_k;
		x = x_k;
		DrawPixel(x, y, BLACK);
		step++;
	} while (step < steps);
}

/**
	* Computes centroid from 3 triangle points
	*/
Vector2 ComputeTriangleCentroid(NaviTriangle triangle) {
	float x = (triangle.v1.x + triangle.v2.x + triangle.v3.x)/3;
	float y = (triangle.v1.y + triangle.v2.y + triangle.v3.y)/3;
	Vector2 centroid = {x, y};
	return centroid;
}

void ComputeRawTriangleCoords(NaviTriangle *triangle) {
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

void DrawTriangle(NaviTriangle *triangle) {
	// counter clock-wise line-drawing
	// a->b, b->c, c->a
	DDALineDrawing(triangle->v1, triangle->v2);
	DDALineDrawing(triangle->v2, triangle->v3);
	DDALineDrawing(triangle->v3, triangle->v1);
}

/**
	* Draw triangles on screen that have space to be drawn
	*/
void TrianglesDraw(NaviTriangle triangles[trianglesSz]) {
	int sz = trianglesSz;
	for (int i = 0; i < sz; i++) {
		NaviTriangle triangle = triangles[i];
		if (triangle.draw) {
			DrawTriangle(&triangle);
		}
	}
}

void RandomizeTriangleDim( NaviTriangle *triangle) {
	float rWidth = (rand() % widthRange) + triangleMinWidth;
	float rHeight = (rand() % heightRange) + triangleMinHeight;
	triangle->width = rWidth;
	triangle->height = rHeight;
}

/**
 * Initialize triangles array, and compute initial dims
 */
void InitializeTriangles(NaviTriangle triangles[trianglesSz]) {
	bool dir = ORIENT_TOP_BOT; // also false, just need to keep flipping this
	int sz = trianglesSz;
	for (int i=0; i<sz; i++) {
		NaviTriangle triangle = { i, NULL, NULL, NULL, dir, NULL,
								NULL, NULL, NULL};
		triangle.draw = i == 0 ? true : false; // only draw the first triangle initially
		RandomizeTriangleDim(&triangle);
		triangle.inset = -triangle.width;
		triangle.movementRate = 2;
		triangles[i] = triangle;
		// flip dir
		dir = !dir;
	}
}

/**
 * Initialize player at default location
 */
NaviTriangle InitializePlayerTriangle() {
	NaviTriangle playerTriangle;
	playerTriangle.draw = true;
	playerTriangle.inset = -1;
	playerTriangle.movementRate = .1;
	playerTriangle.v1 = { 100, 100 };
	playerTriangle.v2 = { 120, 115 };
	playerTriangle.v3 = { 100, 130 };
	return playerTriangle;
}

/**
 * Checks and moves triangles across screen, 
 * if they are out of screen: 
 * * switches draw OFF
 * * it recomputes random dims, to keep the game flow going
 */
void TrianglesMoveOrRecompute(NaviTriangle triangles[trianglesSz]) {
	int sz = trianglesSz;
	for (int i = 0; i < sz; i++) {
		NaviTriangle triangle = triangles[i];
		if (triangle.draw) {
			if (triangle.inset < screenWidth) {
				triangle.inset += (triangle.movementRate + insetStep);
			} else {
				triangle.draw = false;
				RandomizeTriangleDim(&triangle);
				// we want an effect of triangle coming from right of screen instead of spawning, 
				// give inset -width
				triangle.inset = -triangle.width;
			}
			ComputeRawTriangleCoords(&triangle);
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
void TrianglesCheckDrawSpace(NaviTriangle triangles[trianglesSz]) {
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

float ComputeDistanceBwPoints(Vector2 v1, Vector2 v2) {
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
	Vector2 playerCentroid = ComputeTriangleCentroid(playerTriangle);
	float minDiff = INT_MAX;
	for (int i = 0; i < trianglesSz; i++) {
		NaviTriangle triangle = triangles[i];
		Vector2 triangleCentroid = ComputeTriangleCentroid(triangle);
		float pointDiff = ComputeDistanceBwPoints(playerCentroid, triangleCentroid);
		if (pointDiff < minDiff) {
			minDiff = pointDiff;
			closestTriangle = triangle;
		}
	}
	return closestTriangle;
}

/**
 * Compute if triangle is interior
 * Uses barycentric coordinates, to interpret if the point falls inside or 
 * on triangle edges and vertices
 */
bool PointInTriangleInterior(NaviTriangle triangle, Vector2 P) {
	// choose an origin point for triangle v0, lets say vertex v1
	Vector2 A = triangle.v1;
	// compute vectors relative to origin point now
	Vector2 p1 = triangle.v2;
	Vector2 B = { p1.x - A.x, p1.y - A.y };
	Vector2 p2 = triangle.v3;
	Vector2 C = { p2.x - A.x, p2.y - A.y };
	/**
	 * Self understood formula
	 */
	float beta = (Det(C, P) + Det(A, C)) / Det(C, B);
	float gamma = (Det(B, P) + Det(A, B)) / Det(B, C);
	/** beta >=0 and gamma >= 0 -- points in triangle
	 * beta + gamma <= 1 -- satisfies condition that 'alpha' the weight we substituted out here
	 * * is also >= 0
	 * * alpha + beta + gamma = 1 - condition of the plane and barycentric coordinates
	 */
	if (beta >= 0 && gamma >= 0 && (beta + gamma) <= 1) {
		return true;
	}
	return false;
}

/**
 * Detect if the player triangles collides with any of the randomly generated game obstacle triangles
 * Using:
 * * Vertex Matching for triangle closest to player position:
 * * * If any point of player falls in the closest triangle
 * * * using triangle interior formula
 * * * We are checking here if the triangle with player point forms a convex hull for the triangle,
 * * * if it exists independant of the player
 * * * * point is outside of the triangle
 */
bool IsPlayerInTriangle(NaviTriangle triangles[trianglesSz], NaviTriangle playerTriangle) {
	NaviTriangle closestTriangle = GetTriangleClosest(triangles, playerTriangle);
	// for each player vertex check if it falls in triangle
	// check if it falls on any point that is computed for our random triangle
	Vector2 playerPoint = playerTriangle.v1;
	bool isInside = PointInTriangleInterior(closestTriangle, playerPoint);
	playerPoint = playerTriangle.v2;
	isInside = isInside || PointInTriangleInterior(closestTriangle, playerPoint);
	playerPoint = playerTriangle.v3;
	isInside = isInside || PointInTriangleInterior(closestTriangle, playerPoint);
	
	return isInside;
}

Vector2 RotationMult(Vector2 v, float phi) {
	Vector2 rotVec;
	rotVec.x = (cos(phi)*v.x) - (v.y*sin(phi));
	rotVec.y = (sin(phi)*v.x) + (cos(phi)*v.y);
	return rotVec;
}

/**
 * A self implementation of arctan2 specific to my game rotation
 * fun note: reached to this myself through experimentation of quad values
 * and fixing them for arctan results
 */
float ArcTan2(float y, float x) {
	float theta = atan(y / x);
	if (x >= 0 && y >= 0) {
		return theta;
	}
	if (x < 0 && y >= 0) {
		return Pi + theta;
	}
	if (x >= 0 && y < 0) {
		return theta;
	}
	if (x < 0 && y < 0) {
		return theta - Pi;
	}
}

/**
 * Computes the angle (in radians) of the main vector from origin
 * Doing using atan2
 */
float ComputeAngle(NaviTriangle triangle) {
	Vector2 p = triangle.v2;
	float theta = ArcTan2(p.y,p.x);
	return theta;
}

float ComputeNextStep(NaviTriangle triangle) {
	Vector2 p = triangle.v2;
	float speed = 5;
	float angle = triangle.currentAngle;
	if (triangle.currentAngle > 90 && triangle.currentAngle < -90) {
		angle = angle > 90 ? 180 - angle : -180 - angle;
	}
	float step = speed*sin(angle);
	return step;
}

void ComputeInsetStep(NaviTriangle triangle) {
	Vector2 p = triangle.v2;
	float speed = 5;
	float angle = triangle.currentAngle;
	if (triangle.currentAngle > 90 && triangle.currentAngle < -90) {
		angle = angle > 90 ? 180 - angle : -180 - angle;
	}
	insetStep = speed*cos(angle);
}

NaviTriangle RotateTriangleAboutOrigin(NaviTriangle triangle) {
	if (triangle.hitMin && triangle.hitMax) return triangle;
	Vector2 v1 = triangle.v1;
	Vector2 v2 = triangle.v2;
	Vector2 v3 = triangle.v3;

	float phi = ((float)triangle.rotationDir)*rotationRate;
	float theta1 = triangle.currentAngle + phi;
	if (theta1 > maxAngle) {
		triangle.hitMax = 1;
		triangle.rotationDir = -1;
		return triangle;
	}
	if (theta1 < minAngle) {
		triangle.hitMin = 1;
		triangle.rotationDir = 1;
		return triangle;
	}
	// after here, all is fine, multiply with rotation matrix
	triangle.v1 = RotationMult(v1, phi);
	triangle.v2 = RotationMult(v2, phi);
	triangle.v3 = RotationMult(v3, phi);
	triangle.currentAngle = ComputeAngle(triangle);
	// next Step would be the same as v2 (the main arrow/point of player triangle) y value, 
	// when its translated to origin respect to its centroid
	triangle.nextStep = ComputeNextStep(triangle);
	ComputeInsetStep(triangle);
	return triangle;
}

/**
 * Translate a trianle use a translation vector and a sign to represent operation type
 * can be positive and negative
 */
NaviTriangle TranslateTriangleRelative(NaviTriangle triangle, Vector2 Vt, float opSign) {
	Vector2 v1 = triangle.v1;
	Vector2 v2 = triangle.v2;
	Vector2 v3 = triangle.v3;
	Vt = { opSign*Vt.x, opSign*Vt.y };
	triangle.v1 = { v1.x + Vt.x, v1.y + Vt.y };
	triangle.v2 = { v2.x + Vt.x, v2.y + Vt.y };
	triangle.v3 = { v3.x + Vt.x, v3.y + Vt.y };
	return triangle;
}

/**
 * Function to handle Player Rotation about center
 * Steps:
 * Move triangle center to origin
 * Rotate about center
 * Move triangle back
 */
NaviTriangle HandlePlayerRotation(NaviTriangle player) {
 	Vector2 playerCentroid = ComputeTriangleCentroid(player);
	NaviTriangle originPlayer = TranslateTriangleRelative(player, playerCentroid, TRANS_OP_SUB);
	NaviTriangle rotatedPlayer = RotateTriangleAboutOrigin(originPlayer);
	player = TranslateTriangleRelative(rotatedPlayer, playerCentroid, TRANS_OP_ADD);
	return player;
}

/**
 * Simple function to move player up and down
 */
void HandlePlayerMovement(NaviTriangle *player) {
	Vector2 pinPoint = player->v2;
	float nextPos = (pinPoint.y + player->nextStep);
	if (nextPos <= screenHeight && nextPos >= 0) {
		// move player down
		(player->v1).y += player->nextStep;
		(player->v2).y += player->nextStep;
		(player->v3).y += player->nextStep;
	}
}

void PlayerInputHandler(NaviTriangle *playerTriangle, bool *gamePlay) {
	// handle long precise rotation
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		// pause/resume game
		*gamePlay = !*gamePlay;
		// reset player rotation state
		playerTriangle->hitMin = 0;
		playerTriangle->hitMax = 0;
		playerTriangle->rotationDir = -1;
	}
	if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
		// pause/resume game
		*gamePlay = !*gamePlay;
		// reset player rotation state
		playerTriangle->hitMin = 0;
		playerTriangle->hitMax = 0;
		playerTriangle->rotationDir = 1;
	}
}

/**
 * Resets obstacles and player state
 */
void ResetGameState(NaviTriangle *playerTriangle, NaviTriangle triangles[trianglesSz]) {
	InitializeTriangles(triangles);
	*playerTriangle = InitializePlayerTriangle();
}
int main()
{
	// Initialization
	//--------------------------------------------------------------------------------------
	InitWindow(screenWidth, screenHeight, "Navi(gate)Maze");
	// TODO: Load resources / Initialize variables at this point
	SetTargetFPS(60);
	// Main game loop
	int frameCounter = 1;
	srand(time(NULL)); // Seed RAND
	
	// need array of triangles [t, b, t, b, t, b, t, b] => 8*32 = 256 bytes NOICE! very binary
	NaviTriangle triangles[trianglesSz];
	// initializing triangle with null dim and inset, will randomize them
	InitializeTriangles(triangles);
	NaviTriangle playerTriangle = InitializePlayerTriangle();
	bool gamePlay = true;
	bool isCollision = false;
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		// TODO: Update variables / Implement example logic at this point
		//----------------------------------------------------------------------------------
		isCollision = IsPlayerInTriangle(triangles, playerTriangle);
		PlayerInputHandler(&playerTriangle, &gamePlay);
		if (gamePlay) {
			// obstacles
			TrianglesMoveOrRecompute(triangles);
			TrianglesCheckDrawSpace(triangles);

			// player
			HandlePlayerMovement(&playerTriangle);
		}
		if (!gamePlay) {
			if (playerTriangle.hitMin && playerTriangle.hitMax) {
				gamePlay = true;
				playerTriangle.hitMin = 0;
				playerTriangle.hitMax = 0;
			}
			playerTriangle = HandlePlayerRotation(playerTriangle);
		}
		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();
		ClearBackground(DARKGRAY);
		TrianglesDraw(triangles);
		if (isCollision) {
			const char *status = "collision";
			DrawText(status, (screenWidth/2), 10, 20, GREEN);
			ResetGameState(&playerTriangle, triangles);
		}
		DrawTriangle(&playerTriangle);
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