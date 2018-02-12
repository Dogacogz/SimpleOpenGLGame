#include <cmath>
#include <iostream>
#include <ctime>
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <time.h>
#include <vector>

#ifdef __UNIX__
#	include <GLUT/glut.h>
#	include <unistd.h>
#endif

#ifdef __APPLE__
#	include <GLUT/glut.h>
#	include <unistd.h>
#else
#	include <GL/glut.h>
#	include <windows.h>
#endif

#define PI 3.14159265
#define THINGY_RADIUS_DEFAULT 0.2
#define STANDARD_BOMB_SIZE 0.2
#define THINGS_ARE_ALIVE true
#define CONTROL_COLLISION_ACCORDING_TO_LEVEL true
#define TOTAL_THINGY_COUNT 30
#define MAX_GENERATABLE_BOMB_COUNT INT_MAX

using namespace std;

struct Thing {
	double positionX = 0;
	double positionY = 0;
	int dxdy[2]; //moving on which axis? 
					//(1,0 dx) - (0,1 dy) - (-1,0 -dx) - (0,-1 -dy)
					//(1,1 dx,dy) - (-1,1 -dx,dy) - (1,-1 dx,-dy) - (-1,-1 -dx,-dy)
	int level = 0;
	double speed = 0.25, disappear_count_elapsed_time = 0;
	bool bombed = false;
	bool destroyed_after_collision = false;
	bool is_disappear_counter_started = false;
	time_t disappear_count_start;
};

struct Bomb {
	double positionX = DBL_MAX;
	double positionY = DBL_MAX;
	int level = 0;
	int level_count = 5;
	double speed = 0.05;
	double carry_distance = 0;
	bool isDrawn = false;
	double relative_size = 0.2;
};

// Globals & constants. I tried to name them as possible as reasonable & self-explanatory.
static GLsizei width, height;

int CURRENT_GAME_POINTS = 0, //will be implemented
	evil_thingy_count = 0,
	active_bomb_count = 0,
	past_bomb_count = 0,
	current_single_step_count = 0,
	single_stepped_n_times = 0,
	debug_current_single_step_count = 0,
	debug_single_stepped_n_times = 0,
	thingy_count_per_level[6];

double angle, Ix, Iy, 
	carry_distance_dx[TOTAL_THINGY_COUNT], 
	carry_distance_dy[TOTAL_THINGY_COUNT],
	mouseX = DBL_MAX, mouseY = DBL_MAX;

struct Thing thingies[TOTAL_THINGY_COUNT];
vector<Bomb> bombs;

bool positive_out_of_the_screen_dx[TOTAL_THINGY_COUNT],
	negative_out_of_the_screen_dx[TOTAL_THINGY_COUNT],
	positive_out_of_the_screen_dy[TOTAL_THINGY_COUNT],
	negative_out_of_the_screen_dy[TOTAL_THINGY_COUNT],
	bomb_negative_out_of_the_screen_dy[TOTAL_THINGY_COUNT],
	left_mouse_button_clicked = false,
	isPaused = false,
	single_stepped_pause = false,
	debug_running_state_pause = false,
	resolution_changes_adjusted_larger_than_600px = false,
	resolution_changes_adjusted_larger_than_1000px = false,
	resolution_changes_adjusted_smaller_than_400px = false;

float LO = -3.6,
	HI = 3.6,
	thingy_radius = THINGY_RADIUS_DEFAULT,
	thingy_radius_changed_for_400px = THINGY_RADIUS_DEFAULT,
	thingy_radius_changed_for_600px = THINGY_RADIUS_DEFAULT,
	thingy_radius_changed_for_1000px = THINGY_RADIUS_DEFAULT;

//Just to print all information required to debug purposes.
//It is called when "s" key pressed on the keyboard.
void output_current_game_state() {
	cout << "################"
		<< " PRINTING DEBUG INFO NUMBER "<<
		debug_current_single_step_count 
		<< " ################";

	int thingy_number_counter = 0;
	for (auto thingy : thingies) {
		cout << "Thingy number " << thingy_number_counter + 1  << " values are being listed below: "
			<< "\nIs the thingy number " << thingy_number_counter + 1  << " bombed?: " << thingy.bombed
			<< "\nIs thingy disappear count started?: " << thingy.is_disappear_counter_started
			<< "\nDestroyed after collision?: " << thingy.destroyed_after_collision 
			<< "\nThingy movement direction X: "<< thingy.dxdy[0] << ", Y: " << thingy.dxdy[1]
			<< "\nThingy level: " << thingy.level
			<< "\nThingy position X: " << thingy.positionX << ", Y: " << thingy.positionY
			<< " Speed: " << thingy.speed 
			<< "\nCarried away from original X: " << carry_distance_dx[thingy_number_counter]
			<< ", Y: " << carry_distance_dy[thingy_number_counter]
			<< "\n\n";
		thingy_number_counter++;
	}

	int bomb_counter = 0;
	for (auto bomb : bombs) {
		cout << "Bomb number " << bomb_counter + 1 << " values are being listed below: "
			<< "\nLevel: " << bomb.level << ", Level count to change current level: " << bomb.level_count
			<< "\nBomb movement direction X: " << bomb.positionX << ", Y: " << bomb.positionY
			<< "\nCarried away from original Y: " << bomb.carry_distance 
			<< " Speed: " << bomb.speed
			<< "\nIs it exploded on the ground?: " << bomb.isDrawn << "\n\n";
		bomb_counter++;
	}
	
	cout << "\nCURRENT GAME POINTS YOU HAVE: " << CURRENT_GAME_POINTS << "\n\n";

	cout << "################"
		<< " END OF DEBUG INFO NUMBER " <<
		debug_current_single_step_count
		<< " ################";
}

//Calculates current game points accordingly.
void calculate_game_points(Thing thingy) {
	int level = thingy.level;

	if (level == 0)
		CURRENT_GAME_POINTS -= 5000;
	else if(level == 1)
		CURRENT_GAME_POINTS -= 4000;
	else if (level == 2)
		CURRENT_GAME_POINTS -= 3000;
	else if (level == 3)
		CURRENT_GAME_POINTS -= 1000;
	else if (level == 4)
		CURRENT_GAME_POINTS += 2000;
	else if (level == 5)
		CURRENT_GAME_POINTS += 3000;
}

bool is_bomb_collides(Bomb bomb,double cIx, double cIy, double R) {
	double pdx1 = abs(bomb.positionX - bomb.relative_size - cIx);
	double pdy1 = abs(bomb.positionY - cIy);

	double pdx2 = abs(bomb.positionX + bomb.relative_size - cIx);
	double pdy2 = abs(bomb.positionY - cIy);

	double pdx3 = abs(bomb.positionX - cIx);
	double pdy3 = abs(bomb.positionY - bomb.relative_size - cIy);
	
	return (pdx1 * pdx1 + pdy1 * pdy1 <= R * R)
		|| (pdx2 * pdx2 + pdy2 * pdy2 <= R * R)
		|| (pdx3 * pdx2 + pdy3 * pdy3 <= R * R);
}

bool control_collision_for_all_bombs(Bomb vbomb, Thing& thingy, int thingy_number) {
	double cIx = thingy.positionX + carry_distance_dx[thingy_number];
	double cIy = thingy.positionY + carry_distance_dy[thingy_number];

	if (is_bomb_collides(vbomb, cIx, cIy, thingy_radius)) {
		if (CONTROL_COLLISION_ACCORDING_TO_LEVEL && vbomb.level == thingy.level)
			thingy.bombed = true;
		else
			thingy.bombed = true;
		return true;
	}

	return false;
}

void randomize_thingy_thingies(Thing& thingy) {
	thingy.positionX = LO + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (HI - LO)));
	thingy.positionY = LO + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (HI - LO)));
	thingy.speed = 0.05 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (0.25)));
	thingy.level = rand() % 6;

	if (rand() % 2 == 1)
		thingy.dxdy[0] = (rand() % 2);
	else if (rand() % 2 == 0)
		thingy.dxdy[0] = -(rand() % 2);

	if (rand() % 2 == 1)
		thingy.dxdy[1] = (rand() % 2);
	else if (rand() % 2 == 0)
		thingy.dxdy[1] = -(rand() % 2);
}

void handle_thingy_drawing_sequence(Thing& thingy, int thingy_number) {
	if (active_bomb_count > 0) {
		if (thingy.level == 4 && !thingy.bombed) {
			glColor3f(0.75, 0.0, 0.8);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 5 && !thingy.bombed) {
			glColor3f(0.5, 0.0, 1.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 0 && !thingy.bombed) {
			glColor3f(1.0, 1.0, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 1 && !thingy.bombed) {
			glColor3f(0.8, 0.8, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 2 && !thingy.bombed) {
			glColor3f(0.6, 0.6, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 3 && !thingy.bombed) {
			glColor3f(0.4, 0.4, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
	}
	else if(active_bomb_count <= 0){
		if (thingy.level == 4 && !thingy.bombed) {
			glColor3f(0.75, 0.0, 0.8);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 5 && !thingy.bombed) {
			glColor3f(0.5, 0.0, 1.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 0 && !thingy.bombed) {
			glColor3f(1.0, 1.0, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 1 && !thingy.bombed) {
			glColor3f(0.8, 0.8, 0.0 && !thingy.bombed);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 2 && !thingy.bombed) {
			glColor3f(0.6, 0.6, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
		else if (thingy.level == 3 && !thingy.bombed) {
			glColor3f(0.4, 0.4, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < 10; ++i) {
				angle = (2 * PI * i / 10);
				Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
				Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
				glVertex2f(Ix, Iy);
			}
			glEnd();
		}
	}
	if(thingy.bombed && !thingy.destroyed_after_collision) { //implement destruction phase here.
		if (!thingy.is_disappear_counter_started) {
			thingy.disappear_count_start = clock();
			thingy.is_disappear_counter_started = true;
		}
		
		glColor3f(0.0, 0.0, 0.0);
		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < 10; ++i) {
			angle = (2 * PI * i / 10);
			Ix = (cos(angle) * thingy_radius) + thingy.positionX + carry_distance_dx[thingy_number];
			Iy = (sin(angle) * thingy_radius) + thingy.positionY + carry_distance_dy[thingy_number];
			glVertex2f(Ix, Iy);
		}
		glEnd();
		if (thingy.is_disappear_counter_started)
			thingy.disappear_count_elapsed_time = (clock() - thingy.disappear_count_start) / CLOCKS_PER_SEC;
		
		if (thingy.disappear_count_elapsed_time == 1) {
			thingy.destroyed_after_collision = true;
			thingy.disappear_count_elapsed_time = 0;
			thingy.is_disappear_counter_started = 0;
			calculate_game_points(thingy);
		}
		
	}
}

void handy_thingy_movements() {
	for (int i = 0; i < TOTAL_THINGY_COUNT; i++) {
		if (!thingies[i].bombed) {
			if (thingies[i].dxdy[0] == 0 && thingies[i].dxdy[1] == 0)
				randomize_thingy_thingies(thingies[i]);
			else {

				if (thingies[i].dxdy[0] == 1 && thingies[i].dxdy[1] == 0) {	//1.0
					if ((thingies[i].positionX + carry_distance_dx[i]) <= 4.0 && !positive_out_of_the_screen_dx[i])
						carry_distance_dx[i] += thingies[i].speed;
					else if ((thingies[i].positionX + carry_distance_dx[i]) >= 4.0 && !positive_out_of_the_screen_dx[i]) {
						positive_out_of_the_screen_dx[i] = true;
						carry_distance_dx[i] -= thingies[i].speed;
					}
					else if (positive_out_of_the_screen_dx[i] && (carry_distance_dx[i] + thingies[i].positionX) <= 4.0) {
						if ((carry_distance_dx[i] + thingies[i].positionX) <= -4.0) {
							positive_out_of_the_screen_dx[i] = false;
							negative_out_of_the_screen_dx[i] = true;
						}
						else
							carry_distance_dx[i] -= thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == 0 && thingies[i].dxdy[1] == 1) {//0.1

					if ((thingies[i].positionY + carry_distance_dy[i]) <= 4.0 && !positive_out_of_the_screen_dy[i])
						carry_distance_dy[i] += thingies[i].speed;
					else if ((thingies[i].positionY + carry_distance_dy[i]) >= 4.0 && !positive_out_of_the_screen_dy[i]) {
						positive_out_of_the_screen_dy[i] = true;
						carry_distance_dy[i] -= thingies[i].speed;
					}
					else if (positive_out_of_the_screen_dy[i] && (carry_distance_dy[i] + thingies[i].positionY) <= 4.0) {
						if ((carry_distance_dy[i] + thingies[i].positionY) <= -4.0) {
							positive_out_of_the_screen_dy[i] = false;
							negative_out_of_the_screen_dy[i] = true;
						}
						else
							carry_distance_dy[i] -= thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == 1 && thingies[i].dxdy[1] == 1) { //1.1
					if ((thingies[i].positionY + carry_distance_dy[i]) <= 4.0 && !positive_out_of_the_screen_dy[i])
						carry_distance_dy[i] += thingies[i].speed;
					else if ((thingies[i].positionY + carry_distance_dy[i]) >= 4.0 && !positive_out_of_the_screen_dy[i]) {
						positive_out_of_the_screen_dy[i] = true;
						carry_distance_dy[i] -= thingies[i].speed;
					}
					else if (positive_out_of_the_screen_dy[i] && (carry_distance_dy[i] + thingies[i].positionY) <= 4.0) {
						if ((carry_distance_dy[i] + thingies[i].positionY) <= -4.0) {
							positive_out_of_the_screen_dy[i] = false;
							negative_out_of_the_screen_dy[i] = true;
						}
						else
							carry_distance_dy[i] -= thingies[i].speed;
					}
					if ((thingies[i].positionX + carry_distance_dx[i]) <= 4.0 && !positive_out_of_the_screen_dx[i])
						carry_distance_dx[i] += thingies[i].speed;
					else if ((thingies[i].positionX + carry_distance_dx[i]) >= 4.0 && !positive_out_of_the_screen_dx[i]) {
						positive_out_of_the_screen_dx[i] = true;
						carry_distance_dx[i] -= thingies[i].speed;
					}
					else if (positive_out_of_the_screen_dx[i] && (carry_distance_dx[i] + thingies[i].positionX) <= 4.0) {
						if ((carry_distance_dx[i] + thingies[i].positionX) <= -4.0) {
							positive_out_of_the_screen_dx[i] = false;
							negative_out_of_the_screen_dx[i] = true;
						}
						else
							carry_distance_dx[i] -= thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == -1 && thingies[i].dxdy[1] == 0) { //-1.0
					if ((thingies[i].positionX + carry_distance_dx[i]) >= -4.0 && !negative_out_of_the_screen_dx[i])
						carry_distance_dx[i] -= thingies[i].speed;
					else if ((thingies[i].positionX + carry_distance_dx[i]) <= -4.0 && !negative_out_of_the_screen_dx[i]) {
						negative_out_of_the_screen_dx[i] = true;
						carry_distance_dx[i] += thingies[i].speed;
					}
					else if ((thingies[i].positionX + carry_distance_dx[i]) >= -4.0 && negative_out_of_the_screen_dx[i]) {
						if ((carry_distance_dx[i] + thingies[i].positionX) >= 4.0) {
							negative_out_of_the_screen_dx[i] = false;
							positive_out_of_the_screen_dx[i] = true;
						}
						else
							carry_distance_dx[i] += thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == 0 && thingies[i].dxdy[1] == -1) { //0.-1
					if ((thingies[i].positionY + carry_distance_dy[i]) >= -4.0 && !negative_out_of_the_screen_dy[i])
						carry_distance_dy[i] -= thingies[i].speed;
					else if ((thingies[i].positionY + carry_distance_dy[i]) <= -4.0 && !negative_out_of_the_screen_dy[i]) {
						negative_out_of_the_screen_dy[i] = true;
						carry_distance_dy[i] += thingies[i].speed;
					}
					else if ((thingies[i].positionY + carry_distance_dy[i]) >= -4.0 && negative_out_of_the_screen_dy[i]) {
						if ((carry_distance_dy[i] + thingies[i].positionY) >= 4.0) {
							negative_out_of_the_screen_dy[i] = false;
							positive_out_of_the_screen_dy[i] = true;
						}
						else
							carry_distance_dy[i] += thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == -1 && thingies[i].dxdy[1] == -1) { //-1,-1
					if ((thingies[i].positionX + carry_distance_dx[i]) >= -4.0 && !negative_out_of_the_screen_dx[i])
						carry_distance_dx[i] -= thingies[i].speed;
					else if ((thingies[i].positionX + carry_distance_dx[i]) <= -4.0 && !negative_out_of_the_screen_dx[i]) {
						negative_out_of_the_screen_dx[i] = true;
						carry_distance_dx[i] += thingies[i].speed;
					}
					else if ((thingies[i].positionX + carry_distance_dx[i]) >= -4.0 && negative_out_of_the_screen_dx[i]) {
						if ((carry_distance_dx[i] + thingies[i].positionX) >= 4.0) {
							negative_out_of_the_screen_dx[i] = false;
							positive_out_of_the_screen_dx[i] = true;
						}
						else
							carry_distance_dx[i] += thingies[i].speed;
					}

					if ((thingies[i].positionY + carry_distance_dy[i]) >= -4.0 && !negative_out_of_the_screen_dy[i])
						carry_distance_dy[i] -= thingies[i].speed;
					else if ((thingies[i].positionY + carry_distance_dy[i]) <= -4.0 && !negative_out_of_the_screen_dy[i]) {
						negative_out_of_the_screen_dy[i] = true;
						carry_distance_dy[i] += thingies[i].speed;
					}
					else if ((thingies[i].positionY + carry_distance_dy[i]) >= -4.0 && negative_out_of_the_screen_dy[i]) {
						if ((carry_distance_dy[i] + thingies[i].positionY) >= 4.0) {
							negative_out_of_the_screen_dy[i] = false;
							positive_out_of_the_screen_dy[i] = true;
						}
						else
							carry_distance_dy[i] += thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == -1 && thingies[i].dxdy[1] == 1) { //-1,1
					if ((thingies[i].positionX + carry_distance_dx[i]) >= -4.0 && !negative_out_of_the_screen_dx[i])
						carry_distance_dx[i] -= thingies[i].speed;
					else if ((thingies[i].positionX + carry_distance_dx[i]) <= -4.0 && !negative_out_of_the_screen_dx[i]) {
						negative_out_of_the_screen_dx[i] = true;
						carry_distance_dx[i] += thingies[i].speed;
					}
					else if ((thingies[i].positionX + carry_distance_dx[i]) >= -4.0 && negative_out_of_the_screen_dx[i]) {
						if ((carry_distance_dx[i] + thingies[i].positionX) >= 4.0) {
							negative_out_of_the_screen_dx[i] = false;
							positive_out_of_the_screen_dx[i] = true;
						}
						else
							carry_distance_dx[i] += thingies[i].speed;
					}

					if ((thingies[i].positionY + carry_distance_dy[i]) <= 4.0 && !positive_out_of_the_screen_dy[i])
						carry_distance_dy[i] += thingies[i].speed;
					else if ((thingies[i].positionY + carry_distance_dy[i]) >= 4.0 && !positive_out_of_the_screen_dy[i]) {
						positive_out_of_the_screen_dy[i] = true;
						carry_distance_dy[i] -= thingies[i].speed;
					}
					else if (positive_out_of_the_screen_dy[i] && (carry_distance_dy[i] + thingies[i].positionY) <= 4.0) {
						if ((carry_distance_dy[i] + thingies[i].positionY) <= -4.0) {
							positive_out_of_the_screen_dy[i] = false;
							negative_out_of_the_screen_dy[i] = true;
						}
						else
							carry_distance_dy[i] -= thingies[i].speed;
					}
				}
				else if (thingies[i].dxdy[0] == 1 && thingies[i].dxdy[1] == -1) { //1,-1
					if ((thingies[i].positionX + carry_distance_dx[i]) <= 4.0 && !positive_out_of_the_screen_dx[i])
						carry_distance_dx[i] += thingies[i].speed;
					else if ((thingies[i].positionX + carry_distance_dx[i]) >= 4.0 && !positive_out_of_the_screen_dx[i]) {
						positive_out_of_the_screen_dx[i] = true;
						carry_distance_dx[i] -= thingies[i].speed;
					}
					else if (positive_out_of_the_screen_dx[i] && (carry_distance_dx[i] + thingies[i].positionX) <= 4.0) {
						if ((carry_distance_dx[i] + thingies[i].positionX) <= -4.0) {
							positive_out_of_the_screen_dx[i] = false;
							negative_out_of_the_screen_dx[i] = true;
						}
						else
							carry_distance_dx[i] -= thingies[i].speed;
					}

					if ((thingies[i].positionY + carry_distance_dy[i]) >= -4.0 && !negative_out_of_the_screen_dy[i])
						carry_distance_dy[i] -= thingies[i].speed;
					else if ((thingies[i].positionY + carry_distance_dy[i]) <= -4.0 && !negative_out_of_the_screen_dy[i]) {
						negative_out_of_the_screen_dy[i] = true;
						carry_distance_dy[i] += thingies[i].speed;
					}
					else if ((thingies[i].positionY + carry_distance_dy[i]) >= -4.0 && negative_out_of_the_screen_dy[i]) {
						if ((carry_distance_dy[i] + thingies[i].positionY) >= 4.0) {
							negative_out_of_the_screen_dy[i] = false;
							positive_out_of_the_screen_dy[i] = true;
						}
						else
							carry_distance_dy[i] += thingies[i].speed;
					}
				}
			}
		}
	}
}

void handle_bomb_drawing_sequence(Bomb bomby) {
	if (!bomby.isDrawn) {
		if (bomby.level == 0) {
			glColor3f(1.0, 1.0, 0.0);
			glBegin(GL_TRIANGLES);
			glVertex2f(bomby.positionX - bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX + bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX, bomby.positionY - bomby.relative_size + bomby.carry_distance);
			glEnd();
		}
		else if (bomby.level == 1) {
			glColor3f(0.8, 0.8, 0.0);
			glBegin(GL_TRIANGLES);
			glVertex2f(bomby.positionX - bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX + bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX, bomby.positionY - bomby.relative_size + bomby.carry_distance);
			glEnd();
		}
		else if (bomby.level == 2) {
			glColor3f(0.6, 0.6, 0.0);
			glBegin(GL_TRIANGLES);
			glVertex2f(bomby.positionX - bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX + bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX, bomby.positionY - bomby.relative_size + bomby.carry_distance);
			glEnd();
		}
		else if (bomby.level == 3) {
			glColor3f(0.4, 0.4, 0.0);
			glBegin(GL_TRIANGLES);
			glVertex2f(bomby.positionX - bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX + bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX, bomby.positionY - bomby.relative_size + bomby.carry_distance);
			glEnd();
		}
		else if (bomby.level == 4) {
			glColor3f(0.75, 0.0, 0.8);
			glBegin(GL_TRIANGLES);
			glVertex2f(bomby.positionX - bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX + bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX, bomby.positionY - bomby.relative_size + bomby.carry_distance);
			glEnd();
		}
		else if (bomby.level == 5) {
			glColor3f(0.5, 0.0, 1.0);
			glBegin(GL_TRIANGLES);
			glVertex2f(bomby.positionX - bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX + bomby.relative_size, bomby.positionY + bomby.carry_distance);
			glVertex2f(bomby.positionX, bomby.positionY - bomby.relative_size + bomby.carry_distance);
			glEnd();
		}
	}
}

void handle_bomb_movements_on_dy(Bomb &bomby) {
	if (bomby.level == 5 && bomby.level_count == 0) {
		bomby.isDrawn = true;
		active_bomb_count--;
		for (vector<Bomb>::iterator it = bombs.begin(); it != bombs.end();) {
			Bomb bomb = bombs.at(distance(bombs.begin(), it));
			if (bomb.positionX == bomby.positionX &&
				bomb.positionY == bomby.positionY &&
				bomb.level == bomby.level &&
				bomb.level_count == bomby.level_count &&
				bomb.isDrawn) {
				
				it = bombs.erase(it);
			} else if(bomb.positionX == -100)
				it = bombs.erase(it);
			else
				++it;
		}
	}

	if (bomby.carry_distance + bomby.positionY <= -4.0) {
		bomby.isDrawn = true;
		active_bomb_count--;
		for (vector<Bomb>::iterator it = bombs.begin(); it != bombs.end();) {
			Bomb bomb = bombs.at(distance(bombs.begin(), it));
			if (bomb.positionX == bomby.positionX &&
				bomb.positionY == bomby.positionY &&
				bomb.level == bomby.level &&
				bomb.level_count == bomby.level_count &&
				bomb.isDrawn) {

				it = bombs.erase(it);
			}
			else
				++it;
		}
	}
	
	if (bomby.level != 5 && bomby.level_count != 0) {
		bomby.relative_size = bomby.relative_size - 0.005;
		for (int thingy_number = 0; thingy_number < TOTAL_THINGY_COUNT; thingy_number++)
			control_collision_for_all_bombs(bomby, thingies[thingy_number], thingy_number);
	}

	if (bomby.level_count != 0)
		bomby.level_count--;
	else if(bomby.level_count == 0 && bomby.level != 5) {
		bomby.level_count = 5;
		bomby.level++;
	}
}

void drawScene(void) {
	glClear(GL_COLOR_BUFFER_BIT);

	for (vector<Bomb>::iterator it = bombs.begin(); it != bombs.end();) {
		Bomb bomb = bombs.at(distance(bombs.begin(), it));
		if (bombs.at(distance(bombs.begin(), it)).isDrawn)
			it = bombs.erase(it);
		else if (bomb.positionX == -100)
			it = bombs.erase(it);
		else
			++it;
	}

	if (left_mouse_button_clicked &&
			active_bomb_count != past_bomb_count &&
			active_bomb_count > 0) {
		
		Bomb bomby;
		bomby.positionX = mouseX;
		bomby.positionY = mouseY;
		bombs.push_back(bomby);
		mouseX = -100; mouseY = -100;

		for (Bomb bomb : bombs)
			handle_bomb_drawing_sequence(bomb);

		past_bomb_count = active_bomb_count;
	}else if (left_mouse_button_clicked &&
			active_bomb_count == past_bomb_count &&
			active_bomb_count > 0) {

		for (Bomb bomb : bombs)
			if(!bomb.isDrawn)
				handle_bomb_drawing_sequence(bomb);
	}

	if (evil_thingy_count > 0)
		for (int i = 0; i < TOTAL_THINGY_COUNT; i++)
			handle_thingy_drawing_sequence(thingies[i], i);
	else
		exit(0);

	glutPostRedisplay();
	glFlush();
}

void setup(void){
	for (int i = 0; i < 6; i++)
		thingy_count_per_level[i] = 0;

	srand(static_cast <unsigned> (time(NULL)));
	for (int i = 0; i < TOTAL_THINGY_COUNT; i++) {
		randomize_thingy_thingies(thingies[i]);

		while (thingies[i].dxdy[0] == 0 && thingies[i].dxdy[1] == 0)
			randomize_thingy_thingies(thingies[i]);

		carry_distance_dx[i] = 0;
		carry_distance_dy[i] = 0;
		positive_out_of_the_screen_dx[i] = false;
		negative_out_of_the_screen_dx[i] = false;
		positive_out_of_the_screen_dy[i] = false;
		negative_out_of_the_screen_dy[i] = false;

		if (thingies[i].level >= 4)
			evil_thingy_count++;
		thingy_count_per_level[thingies[i].level]++;
	}
	
	glClearColor(0.75, 0.75, 0.75, 0.0);
	glutPostRedisplay();
	glFlush();
}

void set_bomb_relative_size_for_all_bombs(double new_size, int op) {
	if(op == 0)
		for (vector<Bomb>::iterator it = bombs.begin(); it != bombs.end();) {
			Bomb &bomb = bombs.at(distance(bombs.begin(), it));
			bomb.relative_size = new_size;
			++it;
		}
	else if(op == 1)
		for (vector<Bomb>::iterator it = bombs.begin(); it != bombs.end();) {
			Bomb &bomb = bombs.at(distance(bombs.begin(), it));
			bomb.relative_size = bomb.relative_size * 1.5;
			++it;
		}
	for (vector<Bomb>::iterator it = bombs.begin(); it != bombs.end();) {
		Bomb &bomb = bombs.at(distance(bombs.begin(), it));
		++it;
	}
}

void reshape(int w, int h){
	width = glutGet(GLUT_WINDOW_WIDTH);
	height = glutGet(GLUT_WINDOW_HEIGHT);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-4.0, 4.0, -4.0, 4.0);
	glMatrixMode(GL_MODELVIEW);

	if (height < 1000 && width < 1000) {
		resolution_changes_adjusted_larger_than_1000px = false;
		thingy_radius = thingy_radius_changed_for_1000px;
	}
	if (height < 600 && width < 600) {
		resolution_changes_adjusted_larger_than_600px = false;
		thingy_radius = thingy_radius_changed_for_600px;
	}

	if (height < 600 && height > 300 && height < 600 && height > 300) {
		thingy_radius = THINGY_RADIUS_DEFAULT;
		set_bomb_relative_size_for_all_bombs(STANDARD_BOMB_SIZE, 0);
	}

	if (width > 400.0 || height > 400.0) {
		if (!resolution_changes_adjusted_larger_than_600px && (height > 600.0 || width > 600.0)) {
			thingy_radius_changed_for_600px = thingy_radius;
			thingy_radius = thingy_radius * 0.7;
			set_bomb_relative_size_for_all_bombs(STANDARD_BOMB_SIZE, 0);
			resolution_changes_adjusted_larger_than_600px = true;
		}
		else if (!resolution_changes_adjusted_larger_than_1000px && (height > 1000.0 || width > 1000.0)) {
			thingy_radius_changed_for_1000px = thingy_radius;
			thingy_radius = thingy_radius * 0.7;
			set_bomb_relative_size_for_all_bombs(STANDARD_BOMB_SIZE, 0);
			resolution_changes_adjusted_larger_than_1000px = true;
		}
		glutPostRedisplay();
		glFlush();
	}
	else if (!resolution_changes_adjusted_smaller_than_400px && (width < 300.0 || height < 300.0)) {
		thingy_radius_changed_for_400px = thingy_radius;
		thingy_radius = thingy_radius * 1.5;
		set_bomb_relative_size_for_all_bombs(1.5, 1);
		glutPostRedisplay();
		glFlush();
	}
}

void keyInput(unsigned char key, int x, int y){
	switch (key) {
	case 113: //q
		exit(0);
		break;
	case 112: //p
		if (!isPaused && debug_running_state_pause)
			debug_running_state_pause = false;
		else if (isPaused)
			isPaused = false;
		else if (!isPaused)
			isPaused = true;
		
		break;
	case 115: //s
		debug_running_state_pause = true;
		debug_current_single_step_count ++;
		output_current_game_state();
		break;
	}
}

void keyInputSpecial(int key, int x, int y) {
	
}

void myMouse(int button, int state, int mx, int my) {
	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {
			double mYY = (float((glutGet(GLUT_WINDOW_HEIGHT)) - float(my)) - (glutGet(GLUT_WINDOW_HEIGHT) / 2)) / (glutGet(GLUT_WINDOW_HEIGHT) / 8);
			double mXX = (float(mx) - (glutGet(GLUT_WINDOW_WIDTH) / 2)) / (glutGet(GLUT_WINDOW_WIDTH) / 8);
			mouseY = mYY; mouseX = mXX;
			left_mouse_button_clicked = true;
			active_bomb_count++;
		}
		break;
	case GLUT_MIDDLE_BUTTON:
		if (state == GLUT_DOWN) {
			if (single_stepped_pause)
				single_stepped_pause = false;
			else if (!single_stepped_pause)
				single_stepped_pause = true;
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
			current_single_step_count++;
		break;
	}
}

void myTimeOut(int id) {
	if (!isPaused && !single_stepped_pause && !debug_running_state_pause) {
		if (THINGS_ARE_ALIVE)
			handy_thingy_movements();
		
		if (active_bomb_count > 0)
			for (Bomb &bomb : bombs)
				handle_bomb_movements_on_dy(bomb);

		glutPostRedisplay();
		glutTimerFunc(100, myTimeOut, 0);
	}
	else if(isPaused) {
		glutPostRedisplay();
		glutTimerFunc(150, myTimeOut, 0);
	}
	else if (single_stepped_pause) {
		if (current_single_step_count != single_stepped_n_times) {
			single_stepped_n_times = current_single_step_count;
			
			handy_thingy_movements();
			if (active_bomb_count > 0)
				for (Bomb &bomb : bombs)
					handle_bomb_movements_on_dy(bomb);
		}
		glutPostRedisplay();
		glutTimerFunc(150, myTimeOut, 0);
	}
	else if (debug_running_state_pause) {
		if (debug_current_single_step_count != debug_single_stepped_n_times) {
			debug_single_stepped_n_times = debug_current_single_step_count;

			handy_thingy_movements();
			if (active_bomb_count > 0)
				for (Bomb &bomb : bombs)
					handle_bomb_movements_on_dy(bomb);
		}
		glutPostRedisplay();
		glutTimerFunc(150, myTimeOut, 0);
	}
}

int main(int argc, char **argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(400, 400);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("assignment2.cpp");
	setup();
	glutDisplayFunc(drawScene);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyInput);
	glutSpecialFunc(keyInputSpecial);
	glutMouseFunc(myMouse);
	glutTimerFunc(150, myTimeOut, 0);
	glutMainLoop();

	return 0;
}