#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define WIDTH 240
#define HEIGHT 240
#define SCALE 3

#ifdef PLATFORM_WEB
	#include <emscripten/emscripten.h>
	// Save/Load StorageValue don't work on web, these are equivalents using browser's localstorage feature
	#define save(i, v) emscripten_run_script(TextFormat("localStorage.setItem(\"%d\", %d);", i, v))
	#define load(i) emscripten_run_script_int(TextFormat("localStorage.getItem(\"%d\");", i))
#else
	#define save SaveStorageValue
	#define load LoadStorageValue
#endif

void mainloop(void);
void init(void);
void update(void);
void draw(void);
void start(bool resetscore);
void worldinit(void);
void update_title(void);
void draw_title(void);
void update_running(void);
void draw_running(void);
void draw_gameover(void);
void update_deathanim(void);
void update_options(void);
void draw_options(void);
void update_starting(void);
void draw_starting(void);

// _____________________________________________________________________________
//
//  Gameplay related variables
// _____________________________________________________________________________
//

//                       easy norm hard
const int speeds[]    = {115, 135, 145};  // speed is how fast WASD moves the player, not the scroll speed (pixels per second)
const int minspaces[] = {80,  77,  74};   // min amount of empty space, below 75 is very hard
const int maxspaces[] = {105, 90,  87};   // max (starting) amount of empty space, decreases every SPACEDEC frames until minspace
const int mindeltas[] = {6,   6,   7};    // minimum (starting) delta (how steep and rough the cave is), increases every DELTAINC frames
const int maxdeltas[] = {8,   8,   8};    // maximum delta, anything above 9 is quite unplayable
#define SPACEDEC 300
#define DELTAINC 3000
#define TITLEDELTA 12
#define TITLESPACE 70

Rectangle player = {0, HEIGHT/2, 16, 14};
int score, space, delta, starttimer, lives;
int speed, minspace, maxspace, mindelta, maxdelta;
#define MAXLIVES (difficulty == DIF_HARD ? 1 : 2)
#define MINDEPTH -40
#define MAXDEPTH 200

enum {
	DIF_EASY,
	DIF_NORMAL,
	DIF_HARD,
	DIF_COUNT
} difficulty = DIF_NORMAL;

enum {
	F_NONE,
	F_SPIKE,   // a random one from these is chosen, multiple spikes to make
	F_SPIKE1,  // them more common than health
	F_SPIKE2,
	F_SPIKE3,
	F_SPIKE4,
	F_SPIKE5,
	F_HEALTH
} falling;

int falldelay;
int falltimer;
Rectangle fallpos = {0, 0, 16, 16};
#define FALLSPEED 300     // how fast spikes and health packs fall (pixels per second)
#define MINFALLDELAY 100  // min/max amount of time between spike/health drops
#define MAXFALLDELAY (600 - 50*difficulty)

// _____________________________________________________________________________
//
//  Assets and graphics related variables
// _____________________________________________________________________________
//

RenderTexture rt;
Texture buttons[3], playertex[DIF_COUNT], title, gameover, spike, health, scanline;
Sound death, splash, selectsound, healthsound, spikedrop, spikefall, countdown;
Font font;
#define FONTSIZE 8

// Window icon, favicon (html) is used on Web and app icon on Android
#ifdef PLATFORM_DESKTOP
	Image icon;
#endif

// App needs to be full screen on Android, easy way to center a 1:1 aspect ratio
// game to a widescreen display is using a render texture
#ifdef PLATFORM_ANDROID
	RenderTexture androidrt;
#endif

const Color wallcolors[DIF_COUNT] = {
	(Color) {192, 176, 144, 255},  // easy
	(Color) {96, 112, 128, 255},   // normal
	(Color) {160, 96, 80, 255}     // hard
};

const Color watercolors[DIF_COUNT] = {
	(Color) {16, 64, 144, 255},
	(Color) {32, 0, 128, 255},
	(Color) {192, 128, 0, 255}
};

bool debug, fancygfx = true;
float waterlevel = 190;
int shake, startanim;
float playerrotation = 0.0f;
unsigned int framecount;

#define GOMSGCOUNT 6
const char *gomsgs[GOMSGCOUNT] = {
	"0",
	"GO!",
	"RUN!",
	"LET'S GO!",
	"GOOD LUCK!",
	"dskfhshf"
};

const char *gomsg;
RenderTexture gomsgrt;
int gomsgtimer;

#define LINKCOLOR ((Color) {80, 64, 255, 255})
#define GXYELLOW ((Color) {255, 192, 0, 255})
#define GXCYAN ((Color) {0, 255, 255, 255})
#define GAMEOVERBUTTON ((Color) {255, 160, 160, 255})

// _____________________________________________________________________________
//
//  Other variables and defines
// _____________________________________________________________________________
//

#ifdef PLATFORM_ANDROID
	bool touchmode = true;
#else
	bool touchmode = false;
#endif

bool shouldclose = false;  // Should the game close next frame? (if back button pressed)

int hiscores[DIF_COUNT];
bool gothiscore;     // Was a high score achieved last game? Used in gameover screen
bool hiscoreloaded;  // High scores are loaded only once from the savefile,
                     // this is a flag to prevent loading save file every frame

int world[WIDTH/3][2]; // [0] is height, [1] is color tint (higher = darker)
                       // we use width/3 instead of width/blocksize because
                       // blocksize depends on control mode

#define BLOCKSIZE (3 + touchmode)    // The map scrolls one block per frame, so higher blocksize is faster
#define WORLDSIZE (WIDTH/BLOCKSIZE)

enum {
	ST_TITLE,
	ST_RUNNING,
	ST_GAMEOVER,
	ST_DEATHANIM,  // death animation (player falling down)
	ST_OPTIONS,    // set difficulty screen
	ST_STARTING    // 3-2-1 countdown before game starts
} state = ST_TITLE;

// Draw text with custom font and shadow
#define drawtext(text, x, y, color) {\
	DrawTextEx(font, text, (Vector2) {x + 1, y}, FONTSIZE, 0, BLACK); \
	DrawTextEx(font, text, (Vector2) {x, y + 1}, FONTSIZE, 0, BLACK); \
	DrawTextEx(font, text, (Vector2) {x + 1, y + 1}, FONTSIZE, 0, BLACK); \
	DrawTextEx(font, text, (Vector2) {x, y}, FONTSIZE, 0, color); \
}

#ifdef PLATFORM_ANDROID
	// GetTouchPosition() returns values from 0.0 (left/top) to 1.0 (right/bottom)
	// Also we need to offset the X correctly because the game screen is centered
	#define INPUTX ((int) (GetTouchPosition(0).x*GetScreenWidth() - GetScreenWidth()/2 + WIDTH*SCALE/2)/SCALE)
	#define INPUTY ((int) (GetTouchPosition(0).y*HEIGHT))
#else
	#define INPUTX (GetTouchX()/SCALE)  // GetTouchX also works for mouse, but doesn't work with SetMouseScale
	#define INPUTY (GetTouchY()/SCALE)
#endif

// _____________________________________________________________________________
//
//  Loading assets
// _____________________________________________________________________________
//

void init(void) {
	// Android uses assets/ as the base directory for loading assets
	// On other platforms it's the working directory, usually the folder where
	// the game executable is located
	#ifndef PLATFORM_ANDROID
		ChangeDirectory("assets");
	#endif

	buttons[0] = LoadTexture("button0.png");
	buttons[1] = LoadTexture("button1.png");
	buttons[2] = LoadTexture("button2.png");
	playertex[DIF_EASY] = LoadTexture("playereasy.png");
	playertex[DIF_NORMAL] = LoadTexture("player.png");
	playertex[DIF_HARD] = LoadTexture("playerhard.png");

	title = LoadTexture("title.png");
	gameover = LoadTexture("gameover.png");
	spike = LoadTexture("spike.png");
	health = LoadTexture("health.png");
	scanline = LoadTexture("scanline.png");

	death = LoadSound("death.wav");
	selectsound = LoadSound("select.wav");
	healthsound = LoadSound("health.wav");
	spikedrop = LoadSound("spikedrop.wav");
	spikefall = LoadSound("spikefall.wav");
	splash = LoadSound("splash.wav");
	countdown = LoadSound("countdown.wav");

	font = LoadFontEx("font.ttf", 8, NULL, 0);

	#ifndef PLATFORM_ANDROID
		ChangeDirectory("..");
	#endif

	space = TITLESPACE;
	delta = TITLEDELTA;
	worldinit();

	hiscores[DIF_EASY] = load(DIF_EASY);
	hiscores[DIF_NORMAL] = load(DIF_NORMAL);
	hiscores[DIF_HARD] = load(DIF_HARD);
}

// _____________________________________________________________________________
//
//  Startup
// _____________________________________________________________________________
//

int main() {
	speed = speeds[difficulty];
	minspace = minspaces[difficulty];
	maxspace = maxspaces[difficulty];
	mindelta = mindeltas[difficulty];
	maxdelta = maxdeltas[difficulty];

	// Fill entire screen on Android, otherwise there will be flashing borders
	// Also window title is ignored on Android, the app name is used instead
	#ifdef PLATFORM_ANDROID
		InitWindow(0, 0, "");
		androidrt = LoadRenderTexture(WIDTH*SCALE, HEIGHT*SCALE);
	#else
		InitWindow(WIDTH*SCALE, HEIGHT*SCALE, "CaveScroller");
	#endif

	#ifdef PLATFORM_DESKTOP
		icon = LoadImage("assets/icon.png");
		ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		SetWindowIcon(icon);
		UnloadImage(icon);

		// high score storage must be initialized to 0, otherwise there will be garbage values
		// this doesn't happen on web because localstorage is used instead
		if (!FileExists("storage.data")) {
			for (int i = 0; i < DIF_COUNT; i++) save(i, 0);
		}
	#endif

	// ESC is used to go to the main menu, so unbind it from exiting the game
	SetExitKey(0);
	
	rt = LoadRenderTexture(WIDTH, HEIGHT);
	gomsgrt = LoadRenderTexture(WIDTH, HEIGHT/2);

	InitAudioDevice();
	init();

	SetTargetFPS(60);
	#ifdef PLATFORM_WEB
		emscripten_set_main_loop(mainloop, 60, 1);  // setting fps (2nd arg) to 60 here still gives me 62fps
	#else
		while (!WindowShouldClose() && !shouldclose) mainloop();
	#endif

	CloseAudioDevice();
	CloseWindow();
	return 0;
}

// _____________________________________________________________________________
//
//  Main loop
// _____________________________________________________________________________
//

void mainloop(void) {
	update();

	BeginTextureMode(rt);
	draw();
	EndTextureMode();

	#ifdef PLATFORM_ANDROID
		BeginTextureMode(androidrt);
	#else
		BeginDrawing();
	#endif

	// note: this call would have to be rewritten if changing screen size (not scale) or startanim length
	DrawTexturePro(
		rt.texture,
		(Rectangle) {0, 0, WIDTH, -HEIGHT},
		(Rectangle) {(60 - startanim/2)*SCALE, (120 - startanim)*SCALE, (120 + startanim)*SCALE, startanim*2*SCALE},
		(Vector2) {GetRandomValue(-shake, shake), GetRandomValue(-shake, shake)}, 0.0f, WHITE
	);

	if (shake) shake--;
	if (startanim < 120) startanim++;
	
	if (fancygfx) {
		// simulate CRT flicker and scanlines
		DrawRectangle(
			0, 0, WIDTH*SCALE, HEIGHT*SCALE,
			(Color) {
				difficulty == DIF_HARD ? 32 : 0, 0, 0,
				GetRandomValue(difficulty == DIF_HARD ? 20 : 0, difficulty == DIF_HARD ? 40 : 5)
			}
		);
		DrawTexture(scanline, 0, 0, WHITE);

		if (GetRandomValue(0, 30) == 0 && startanim > 119) {
			DrawTexturePro(
				rt.texture,
				(Rectangle) {0, 0, WIDTH, -HEIGHT},
				(Rectangle) {GetRandomValue(-16, 16), GetRandomValue(-8, 8), WIDTH*SCALE, HEIGHT*SCALE},
				(Vector2) {GetRandomValue(-shake, shake), GetRandomValue(-shake, shake)}, 0.0f,
				(Color) {GetRandomValue(1, 2)*127, GetRandomValue(1, 2)*127, GetRandomValue(1, 2)*127, 24}
			);
		}
	}

	if (debug) {
		DrawText(
			TextFormat(
				"fps %d\ndelta %d\nspace %d\nworld[0] %d\ntouch %d,%d\ntouched %d\nwater %f",
				GetFPS(), delta, space, world[0][0], INPUTX, INPUTY,
				IsMouseButtonDown(MOUSE_BUTTON_LEFT), waterlevel
			),
			180*SCALE, 0, 10, YELLOW
		);

		DrawLine(0, waterlevel*SCALE, WIDTH*SCALE, waterlevel*SCALE, GXCYAN);
	}

	if (state != ST_DEATHANIM) {
		if (playerrotation > 0) playerrotation--;
		else if (playerrotation < 0) playerrotation++;
	}

	#ifdef PLATFORM_ANDROID
		EndTextureMode();

		BeginDrawing();
		ClearBackground(BLACK);
		DrawTexturePro(
			androidrt.texture,
			(Rectangle) {0, 0, WIDTH*SCALE, -HEIGHT*SCALE},
			(Rectangle) {GetScreenWidth()/2 - WIDTH*SCALE/2, 0, GetScreenHeight(), GetScreenHeight()},
			(Vector2) {0, 0}, 0.0f, WHITE
		);
	#endif

	EndDrawing();
	framecount++;
}

// _____________________________________________________________________________
//
//  Button
// _____________________________________________________________________________
//

#define BUTTONX (WIDTH/2 - buttons[0].width/2)  // button X position is always the same, middle of the screen

bool button(int y, const char *text, Color tint) {
	enum {
		NONE, HOVER, DOWN, PRESSED
	} buttonstate = NONE;

	if (INPUTX >= BUTTONX && INPUTX <= BUTTONX + buttons[0].width) {
		if (INPUTY >= y && INPUTY <= y + buttons[0].height) {
			buttonstate = HOVER;
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) buttonstate = DOWN;
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) buttonstate = PRESSED;
		}
	}

	bool result = false;
	switch (buttonstate) {
		case NONE: DrawTexture(buttons[0], BUTTONX, y, tint); break;
		case HOVER: DrawTexture(buttons[1], BUTTONX, y, tint); break;
		case DOWN: DrawTexture(buttons[2], BUTTONX, y, tint); break;
		case PRESSED: DrawTexture(buttons[2], BUTTONX, y, tint); result = true; break;
	}

	Vector2 measure = MeasureTextEx(font, text, FONTSIZE, 0);
	drawtext(
		text,
		BUTTONX + buttons[0].width/2 - measure.x/2,
		y + buttons[0].height/2 - measure.y/2, WHITE
	);
	return result;
}

bool link(int x, int y, const char *text, Color tint) {
	bool pressed = false;
	Vector2 measure = MeasureTextEx(font, text, FONTSIZE, 0);

	if (INPUTX >= x && INPUTX <= x + measure.x) {
		if (INPUTY >= y && INPUTY <= y + measure.y) {
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) pressed = true;
		}
	}

	drawtext(text, x, y, tint);
	return pressed;
}

// _____________________________________________________________________________
//
//  Update and draw loops (these branch off depending on game state)
// _____________________________________________________________________________
//

void update(void) {
	switch (state) {
		case ST_GAMEOVER: break;
		case ST_TITLE: update_title(); break;
		case ST_RUNNING: update_running(); break;
		case ST_DEATHANIM: update_deathanim(); break;
		case ST_OPTIONS: update_options(); break;
		case ST_STARTING: update_starting(); break;
	}

	if (IsKeyPressed(KEY_G)) fancygfx = !fancygfx;
	if (IsKeyPressed(KEY_R)) start(true);
	if (IsKeyPressed(KEY_ESCAPE) && state != ST_TITLE) {
		state = ST_TITLE;
		space = TITLESPACE;
		delta = TITLEDELTA;
		worldinit();
	}
	if (IsKeyPressed(KEY_B)) debug = !debug;

	#ifdef PLATFORM_ANDROID
		if (IsKeyPressed(KEY_BACK)) {
			if (state == ST_TITLE) shouldclose = true;
			else state = ST_TITLE;
		}
	#endif
}

void draw(void) {
	ClearBackground(BLACK);
	switch (state) {
		case ST_TITLE: draw_title(); break;
		case ST_DEATHANIM:
		case ST_RUNNING: draw_running(); break;
		case ST_GAMEOVER: draw_gameover(); break;
		case ST_OPTIONS: draw_options(); break;
		case ST_STARTING: draw_starting(); break;
	}
}

// _____________________________________________________________________________
//
//  Utilities
// _____________________________________________________________________________
//

void worldshift(void) {
	// Shift previous blocks left by 1
	for (int i = 1; i < WORLDSIZE; i++) {
		world[i - 1][0] = world[i][0];
		world[i - 1][1] = world[i][1];
	}

	// Next block = last block + random delta
	world[WORLDSIZE - 1][0] = world[WORLDSIZE - 2][0] + GetRandomValue(-delta, delta);

	// Apply a random tint to the next block
	world[WORLDSIZE - 1][1] = world[WORLDSIZE - 2][1] + GetRandomValue(-1, 1)*2;
	if (world[WORLDSIZE - 1][1] > 30) world[WORLDSIZE - 1][1] = 30;
	if (world[WORLDSIZE - 1][1] < -30) world[WORLDSIZE - 1][1] = -30;
	
	// The player needs to have some room to move around in, so prevent the cave
	// from going too far up/down
	if (world[WORLDSIZE - 1][0] < MINDEPTH)
		world[WORLDSIZE - 1][0] = MINDEPTH + delta*3;

	else if (world[WORLDSIZE - 1][0] > MAXDEPTH)
		world[WORLDSIZE - 1][0] = MAXDEPTH - delta*3;
}

void worldinit(void) {
	// world always begins in the middle of the screen
	world[0][0] = HEIGHT/2 - space/2;
	world[0][1] = GetRandomValue(0, 10);

	// do basically the same as worldshift() for every next block
	for (int i = 1; i < WORLDSIZE; i++) {
		world[i][0] = world[i - 1][0] + GetRandomValue(-delta, delta);
		world[i][1] = world[i - 1][1] + GetRandomValue(-1, 1)*2;
		if (world[i][1] > 30) world[i][1] = 30;
		if (world[i][1] < -30) world[i][1] = -30;

		if (world[i][0] < MINDEPTH) world[i][0] = MINDEPTH + delta*3;
		else if (world[i][0] > MAXDEPTH) world[i][0] = MAXDEPTH - delta*3;
	}
}

void die(bool water) {
	if (player.y > waterlevel) water = true;
	state = ST_DEATHANIM;
	PlaySound(water ? splash : death);
	shake = 20;

	lives--;
	if (lives < 1) {
		if (score + 1 > load(difficulty)) {
			hiscores[difficulty] = score + 1;
			save(difficulty, score + 1);
			gothiscore = true;
		} else gothiscore = false;
	}
}

void start(bool resetscore) {
	if (resetscore) {
		score = 0;
		lives = 1;
		space = maxspace;
		delta = mindelta;
	}

	starttimer = 90;
	state = ST_STARTING;
	worldinit();
	player.x = 0;
	player.y = world[0][0] + space/2;
	playerrotation = 0.0f;
	falling = F_NONE;
	falldelay = GetRandomValue(MINFALLDELAY, MAXFALLDELAY);
	PlaySound(countdown);
}

void watershift(void) {
	if (fancygfx) waterlevel += sin(framecount / 8) / 2;
}

void drawworld(void) {
	ClearBackground((Color) {GetRandomValue(0, 5), 0, GetRandomValue(0, 10), 255});

	DrawRectangle(0, waterlevel, WIDTH, HEIGHT, watercolors[difficulty]);

	for (int i = 0; i < WORLDSIZE; i++) {
		Color wallcolor = wallcolors[difficulty];

		if (fancygfx) {
			wallcolor.r -= world[i][1];
			wallcolor.g -= world[i][1];
			wallcolor.b -= world[i][1];
		}

		DrawRectangle(i*BLOCKSIZE, 0, BLOCKSIZE, world[i][0], wallcolor);
		DrawRectangle(i*BLOCKSIZE, world[i][0] + space, BLOCKSIZE, HEIGHT, wallcolor);

		if (fancygfx) {
			DrawRectangle(i*BLOCKSIZE, 0, BLOCKSIZE, world[i][0] - 50, (Color) {0, 0, 0, 27});
			DrawRectangle(i*BLOCKSIZE, 0, BLOCKSIZE, world[i][0] - 120, (Color) {0, 0, 0, 35});
			DrawRectangle(i*BLOCKSIZE, 0, BLOCKSIZE, world[i][0] - 140, (Color) {0, 0, 0, 56});

			DrawRectangle(i*BLOCKSIZE, world[i][0] + space + 30, BLOCKSIZE, HEIGHT, (Color) {0, 0, 0, 32});
			DrawRectangle(i*BLOCKSIZE, world[i][0] + space + 80, BLOCKSIZE, HEIGHT, (Color) {0, 0, 0, 40});
			DrawRectangle(i*BLOCKSIZE, world[i][0] + space + 100, BLOCKSIZE, HEIGHT, (Color) {0, 0, 0, 64});
		}
	}
}

// gomsg is drawn at the end of draw_running
void setgomsg(const char *msg) {
	gomsg = msg;
	gomsgtimer = 0;
	BeginTextureMode(gomsgrt);
	ClearBackground(BLANK);
	drawtext(gomsg, WIDTH/2 - strlen(gomsg)*FONTSIZE/2, HEIGHT/4 - FONTSIZE/2, GXCYAN);
	EndTextureMode();
}

// _____________________________________________________________________________
//
//  TITLE state
// _____________________________________________________________________________
//

void update_title(void) {
	worldshift();
	watershift();
}

void draw_title(void) {
	drawworld();

	DrawTexture(title, 0, 0, WHITE);

	if (button(128, "Play", WHITE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
		start(true);
		// don't play select sound because then it will overlap with the countdown sound
	}

	if (button(170, "Options", WHITE) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
		hiscoreloaded = false;
		state = ST_OPTIONS;
		PlaySound(selectsound);
	}
}

// _____________________________________________________________________________
//
//  RUNNING state (main game loop)
// _____________________________________________________________________________
//

void update_running(void) {
	worldshift();
	watershift();

	if (falling) {
		if (falltimer < 0) {
			fallpos.y += (FALLSPEED + 25*difficulty)*GetFrameTime();
			if (fallpos.y > HEIGHT) {
				bool underwater = world[(int) (fallpos.x + 8)/BLOCKSIZE][0] + space > waterlevel;

				if (falling != F_HEALTH) {
					PlaySound(underwater ? splash : spikefall);
					shake = 10;
				} else {
					if (underwater) PlaySound(splash);
					shake = 5;
				}

				falling = F_NONE;
				falldelay = GetRandomValue(MINFALLDELAY, MAXFALLDELAY);
			}

			if (CheckCollisionRecs(player, fallpos)) {
				if (falling != F_HEALTH) {
					die(false);
				} else if (lives < MAXLIVES) {
					lives++;
					PlaySound(healthsound);
				}
				falling = F_NONE;
				falldelay = GetRandomValue(MINFALLDELAY, MAXFALLDELAY);
			}
		}
		else falltimer--;
	} else {
		falldelay--;
		if (falldelay < 1) {
			// 1 in 7 chance to be health
			falling = GetRandomValue(F_SPIKE, F_HEALTH);

			// don't drop health if we don't need it
			// note: maxlives is 1 in hard mode so health never appears
			if (lives == MAXLIVES) falling = F_SPIKE;

			// play a crumbling sound for spikes (not in hard mode so they're a bit harder to spot)
			if (falling != F_HEALTH && difficulty != DIF_HARD) PlaySound(spikedrop);

			fallpos.x = GetRandomValue(14, 160);

			// Make sure the entire spike/health is on screen even when it's shaking
			fallpos.y = 4 - difficulty;

			// Spikes will usually fall close to the player (more likely on harder difficulties)
			if (falling != F_HEALTH && GetRandomValue(1, 4 + difficulty) != 1)
				fallpos.x = player.x + GetRandomValue(-15, 15);

			// Falling object must be on screen though, you don't want to die to
			// a spike that had only 1 pixel visible
			if (fallpos.x < 0) fallpos.x = 0;

			// Shake for 45/35/25 frames before dropping
			falltimer = 45 - 10*difficulty;
		}
	}

	// check collision with blocks
	for (int i = player.x/BLOCKSIZE; i < (player.x + player.width)/BLOCKSIZE; i++) {
		if (player.y < world[i][0]) die(false);
		if (player.y + player.height > world[i][0] + space) die(false);
	}

	if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
		player.y -= speed*GetFrameTime();
		if (player.y < 0) player.y = 6;
		else playerrotation = -3;
	}

	if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
		player.y += speed*GetFrameTime();
		if (player.y + player.height >= HEIGHT) die(true);
		else playerrotation = 3;
	}

	if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
		player.x -= speed*GetFrameTime();
		if (player.x < 0) player.x = 6;
		else playerrotation = -6;
	}

	if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
		player.x += speed*GetFrameTime();
		if (player.x > WIDTH - 16) player.x = WIDTH - 22;
		else playerrotation = 6;
	}

	if (touchmode) {
		player.x = INPUTX - player.width/2;
		player.y = INPUTY - player.height/2;
		if (player.x < 0) player.x = 6;
		if (player.x > WIDTH - 16) player.x = WIDTH - 22;
		if (player.y < 0) die(false);
		if (player.y + player.height >= HEIGHT) die(true);
	}

	if (IsKeyPressed(KEY_T)) setgomsg("trolled!!11");  // yes

	score++;
	if (score == hiscores[difficulty] + 1 && score > 1) setgomsg("NEW BEST");

	if (score % SPACEDEC == 0 && space > minspace) space--;
	if (score % DELTAINC == 0 && delta < maxdelta) delta++;
}

void draw_running(void) {
	drawworld();

	if (falling) {
		if (falltimer > 0) {
			DrawTexture(
				falling == F_HEALTH ? health : spike,
				fallpos.x + GetRandomValue(-6 + difficulty*2, 6 - difficulty*2),
				fallpos.y + GetRandomValue(-3 + difficulty, 3 - difficulty), WHITE
			);
		} else {
			DrawTexture(
				falling == F_HEALTH ? health : spike,
				fallpos.x, fallpos.y, WHITE
			);
		}
	}

	if (fancygfx)
		DrawTextureEx(playertex[difficulty], (Vector2) {player.x, player.y}, playerrotation, 1.0f, WHITE);
	else DrawTexture(playertex[difficulty], player.x, player.y, WHITE);

	drawtext(TextFormat("SCORE:%.5d   LIVES:%d", score, lives), 36, HEIGHT - 16, (GXYELLOW));

	// draw gomsg ("GO!" text when the game starts) in the middle of the screen,
	// for 80 frames, with increasing size and decreasing opacity
	if (gomsgtimer < 80) {
		gomsgtimer++;

		// note: gomsg render texture is drawn at 2× vertical scale (8×16 character size)
		// the render texture is WIDTH × HEIGHT/2 pixels and drawn at WIDTH × HEIGHT
		DrawTexturePro(
			gomsgrt.texture,
			(Rectangle) {0, 0, WIDTH, -HEIGHT/2},
			(Rectangle) {
				-gomsgtimer*6,
				-gomsgtimer*6,
				WIDTH + gomsgtimer*12,
				HEIGHT + gomsgtimer*12
			},
			(Vector2) {0, 0}, 0.0f, (Color) {255, 255, 255, 255 - gomsgtimer*3}
		);
	}
}

// _____________________________________________________________________________
//
//  GAMEOVER/DEATHANIM state
// _____________________________________________________________________________
//

void update_deathanim(void) {
	if (lives < 0) lives = 0;
	player.y += 3;
	if ((int) player.y % 2 == 0) player.x++;
	playerrotation += 3;
	if (player.y > HEIGHT) {
		if (lives > 0) start(false);
		else state = ST_GAMEOVER;
	}
}

void draw_gameover(void) {
	ClearBackground((Color) {160, 0, 32, 255});
	DrawTexture(gameover, 0, 0, WHITE);

	drawtext(TextFormat("Your score: %.5d", score), 52, 112, WHITE);
	drawtext(TextFormat("High score: %.5d", hiscores[difficulty]), 52, 120, WHITE);
	if (gothiscore) {
		drawtext("NEW", 196, 120, GXYELLOW);
	}

	if (button(144, "Retry", GAMEOVERBUTTON) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
		start(true);
		// don't play select sound because then it will overlap with the countdown sound
	}

	if (button(186, "Title", GAMEOVERBUTTON) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
		state = ST_TITLE;
		space = TITLESPACE;
		delta = TITLEDELTA;
		worldinit();
		PlaySound(selectsound);
	}
}

// _____________________________________________________________________________
//
//  OPTIONS state
// _____________________________________________________________________________
//

void update_options(void) {
	if (!hiscoreloaded) {
		hiscoreloaded = true;
		hiscores[DIF_EASY] = load(DIF_EASY);
		hiscores[DIF_NORMAL] = load(DIF_NORMAL);
		hiscores[DIF_HARD] = load(DIF_HARD);
	}
}

void draw_options(void) {
	ClearBackground((Color) {32, 0, 255, 255});
	drawtext("Difficulty affects the", WIDTH/2 - 88, 0, WHITE);
	drawtext("steepness and amount of open", WIDTH/2 - 112, 8, WHITE);
	drawtext("air in the cave, as well as", WIDTH/2 - 108, 16, WHITE);
	drawtext("the player speed", WIDTH/2 - 64, 24, WHITE);

	drawtext("HIGH SCORES", WIDTH/2 - 44, 40, GXYELLOW);
	
	#ifdef PLATFORM_ANDROID
		drawtext("Not supported yet", WIDTH/2 - 68, 48, WHITE);
	#else
		drawtext(TextFormat("EASY:   %.5d", hiscores[DIF_EASY]), WIDTH/2 - 52, 48, ((Color) {0, 255, 64, 255}));
		drawtext(TextFormat("NORMAL: %.5d", hiscores[DIF_NORMAL]), WIDTH/2 - 52, 56, GXYELLOW);
		drawtext(TextFormat("HARD:   %.5d", hiscores[DIF_HARD]), WIDTH/2 - 52, 64, ((Color) {255, 0, 48, 255}));
	#endif

	const char *difstrings[] = {"Difficulty: EASY", "Difficulty: NORMAL", "Difficulty: HARD"};

	if (button(80, difstrings[difficulty], WHITE)) {
		PlaySound(selectsound);
		difficulty++;
		if (difficulty > DIF_HARD) difficulty = DIF_EASY;

		speed = speeds[difficulty];      
		minspace = minspaces[difficulty];
		maxspace = maxspaces[difficulty];
		mindelta = mindeltas[difficulty];
		maxdelta = maxdeltas[difficulty];
	}

	if (button(112, touchmode ? "Touch mode: ON" : "Touch mode: OFF", WHITE)) {
		touchmode = !touchmode;
		worldinit();
		PlaySound(selectsound);
	}

	if (button(144, fancygfx ? "Graphics: HIGH" : "Graphics: LOW", WHITE)) {
		fancygfx = !fancygfx;
		PlaySound(selectsound);
	}

	if (button(176, "Back", WHITE)) {
		state = ST_TITLE;
		space = TITLESPACE;
		delta = TITLEDELTA;
		worldinit();
		PlaySound(selectsound);
	}

	#ifndef PLATFORM_ANDROID
		if (link(0, 224, "Copy scores", LINKCOLOR)) {
			const char *str = TextFormat(
				"My CaveScroller high scores\nEASY: %.5d\nNORMAL: %.5d\nHARD: %.5d",
				hiscores[DIF_EASY], hiscores[DIF_NORMAL], hiscores[DIF_HARD]
			);

			#ifdef PLATFORM_WEB
				emscripten_run_script(TextFormat("navigator.clipboard.writeText(`%s`); alert('Copied!');", str));
			#else
				SetClipboardText(str); printf("Copied scores to clipboard!\n");
			#endif
		}
		drawtext("___________", 0, 226, LINKCOLOR);

		if (link(96, 224, "Clear scores", LINKCOLOR)) {
			save(DIF_EASY, 0); hiscores[DIF_EASY] = 0;
			save(DIF_NORMAL, 0); hiscores[DIF_NORMAL] = 0;
			save(DIF_HARD, 0); hiscores[DIF_HARD] = 0;
		}
		drawtext("____________", 96, 226, LINKCOLOR);
	#endif

	if (touchmode) {
		if (link(200, 224, "Debug", LINKCOLOR)) debug = !debug;
		drawtext("_____", 200, 226, LINKCOLOR);
	}
}

// _____________________________________________________________________________
//
//  STARTING state
// _____________________________________________________________________________
//

void update_starting(void) {
	starttimer--;
	if (starttimer < 1) {
		setgomsg(gomsgs[GetRandomValue(0, GOMSGCOUNT - 1)]);
		state = ST_RUNNING;
	}
}

void draw_starting(void) {
	drawworld();
	DrawTexture(playertex[difficulty], player.x, player.y, WHITE);
	drawtext(
		TextFormat("%d", (int) starttimer/30 + 1),
		WIDTH/2 - 5, HEIGHT/2 - 55, GXCYAN
	);
}