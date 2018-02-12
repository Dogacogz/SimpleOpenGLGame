# SimpleOpenGLGame
A simple OpenGL game in which you kill things as you drop bombs on them. The bomb and the thingy must be at the same color/level for bombs to kill things.

Note that you have to click TWICE to start dropping bombs, after the first party of the bombs are exploded.

# Game Description:

* Each thing foats on a certain level, which can be thought of as its z-coordinate. There are 5 different levels for z = 0; 1; 2; 3; 4. Each level contains 4 things initially. There is also one additional level z = 5 called the ground level. On levels 0, 1, 2, live the good things (drawn in yellow) and on levels 3 and 4 live the evil things (drawn in purple).The objective of the game is to destroy all the evil things (for which you gain points) while destroying as few good things as possible (for which you lose points). To tell differences in level, the darker colors are for lower levels. Things that are at the same level have the same color.

* You destroy things by dropping bombs on them. This is done by clicking the left mouse button at the position that you want the bomb to start. A bomb starts at level 0, and slowly falls to each of the other levels until it hits the ground level, where it disappears. A bomb is drawn as a smaller
square (or any design that you may have). To indicate the present level of the bomb, its color changes (to match the color of the level) as it falls. When a bomb at level i hits a thing that lives at level i (meaning their squares overlap) the the thing is hit and changes color to black to indicate
this. About a second later, the thing disappears (meaning that it is destroyed). After a bomb hits a thing, the bomb continues to fall. Score and other status information should be written onto the screen.

* The game is terminated either when the user press the 'q' key, or when all the evil things are destroyed. The game can be paused by pressing the middle mouse button, and the game can be single-stepped by pressing the right mouse button.

# Input Events:

* When the 'q' key is pressed, the game is terminated.
	
* Left button down: This causes a bomb to be dropped centered at the current mouse coordinates. There is no limit on the number of bombs that the user is allowed to have active at any time.
	
* 'p': When the 'p' key is pressed, the game pauses. When 'p' is pressed a second time the game resumes.
	
* 's': When the 's' key is pressed, the game pauses if it is in a running state. (If it is already in a paused state or if the game is over, it stays in its current state.) The state of the game is advanced by one step, and then the program outputs all the game's current state to the standard output (for debugging purposes). This should include the locations of the things and the bombs and any other information that you and important. By repeatedly hitting 's' key, the program will perform consecutive single steps.