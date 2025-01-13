#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <mmsystem.h>

#include "includes/drawString.h"
#include "includes/drawSurface.h"
#include "includes/drawPixel.h"
#include "includes/drawLine.h"
#include "includes/drawRectangle.h"

extern "C" {
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
}

#define UNIT 25

//Board defines in units
#define SCREEN_WIDTH 34
#define SCREEN_HEIGHT 25
#define FIELD_WIDTH 26
#define FIELD_HEIGHT 18
#define FIELD_X 4
#define FIELD_Y 3

//Snake defines in units
#define SNAKE_LENGTH 7
#define MAX_SNAKE_LENGTH 100
#define SNAKE_SPEED 3.0 //Snake speed in units per second
#define SNAKE_X 12
#define SNAKE_Y 10

//Additional information board defines
#define INFO_TEXT_WIDTH 842
#define INFO_TEXT_HEIGHT 54
#define INFO_TEXT_X 4
#define INFO_TEXT_Y 4

//Lose information board defines
#define LOSE_INFO_X 275
#define LOSE_INFO_Y 258
#define LOSE_INFO_WIDTH 300
#define LOSE_INFO_HEIGHT 50

//New game text defines
#define NEW_GAME_TIME 1
#define NEW_GAME_X 393
#define NEW_GAME_Y 267

//Progress bar defines
#define PROGRESS_BAR_X 320
#define PROGRESS_BAR_Y 560
#define PROGRESS_BAR_WIDTH 200
#define PROGRESS_BAR_HEIGHT 25

//Points for blue & red dot dot
#define BLUE_POINTS 2
#define RED_POINTS 5

//Speedup of the game defines
#define GAME_SPEED 20.0 //percentage of game acceleration
#define GAME_SPEED_TIME 10 //time in seconds to increase game speed

//Red dot defines
#define RED_DOT_TIME 5.0 //time in seconds
#define RED_DOT_PERC 30 //chance of red dot appealing
#define RED_DOT_SNAKE_SPEED 2.0 //slowing speed of the snake after eating red dot by n units per second
#define SHORTENING_SNAKE 8

const enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

struct GameState {
    int t1, t2, quit, frames, points;
    double delta, worldTime, fpsTimer, fps, newGameTimer, snakeTimer, snakeSpeed, lastSpeedIncreaseTime, redDotTimer;
    bool gameOver;
};

struct GameResources {
    Uint32 czarny, zielony, czerwony, niebieski, bialy, jasny_niebieski, fioletowy;
    SDL_Event event;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* screen;
    SDL_Surface* charset;
    SDL_Surface* blueDotSurface;
    SDL_Surface* redDotSurface;
    SDL_Texture* scrtex;
};

struct Snake {
    //Array of snake segments storing unit x [0] and unit y [1] coordinates
    int segments[MAX_SNAKE_LENGTH][2];
    int length;
    Direction direction;
};

struct blueDot {
    int x;
    int y;
};

struct redDot {
    int x;
    int y;
};


//initialize functions
bool initialize_SDL(GameResources& gameResources) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return false;
    }

    int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH * UNIT, SCREEN_HEIGHT * UNIT, 0, &gameResources.window, &gameResources.renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(gameResources.renderer, SCREEN_WIDTH * UNIT, SCREEN_HEIGHT * UNIT);
    SDL_SetRenderDrawColor(gameResources.renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(gameResources.window, "Karol Obrycki index: 203264");

    gameResources.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH * UNIT, SCREEN_HEIGHT * UNIT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    gameResources.scrtex = SDL_CreateTexture(gameResources.renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH * UNIT, SCREEN_HEIGHT * UNIT);

    return true;
}

void initialize_snake(Snake& snake) {
    snake.length = SNAKE_LENGTH;
    snake.direction = UP;
    for (int i = 0; i < snake.length; i++) {
        snake.segments[i][0] = SNAKE_X;
        snake.segments[i][1] = SNAKE_Y + i;
    }
}

void initialize_game_state(GameState& gameState) {
    gameState.t1 = SDL_GetTicks();
    gameState.quit = 0;
    gameState.frames = 0;
    gameState.points = 0;
    gameState.delta = 0;
    gameState.worldTime = 0;
    gameState.fpsTimer = 0;
    gameState.fps = 0;
    gameState.newGameTimer = 0;
    gameState.snakeTimer = 0;
    gameState.snakeSpeed = SNAKE_SPEED;
    gameState.lastSpeedIncreaseTime = 0;
    gameState.redDotTimer = 0;
    gameState.gameOver = false;
}

void initialize_blue_dot(int& x, int& y, Snake& snake) {
    bool validPosition;
    do {
        validPosition = true;
		//Random position of the blue dot on the game field
        x = rand() % FIELD_WIDTH;
        y = rand() % FIELD_HEIGHT;
        for (int i = 0; i < snake.length; i++) {
			//is there a snake segment on the blue dot position
            if (x == snake.segments[i][0] && y == snake.segments[i][1]) {
                validPosition = false;
                break;
            }
        }
    } while (!validPosition);
}

void initialize_red_dot(int& x, int& y, Snake& snake, blueDot& blueDot) {
    bool validPosition;
    do {
        validPosition = true;
		//Random position of the red dot on the game field
        x = rand() % FIELD_WIDTH;
        y = rand() % FIELD_HEIGHT;
        for (int i = 0; i < snake.length; i++) {
			//is there a snake segment on the red dot position
            if (x == snake.segments[i][0] && y == snake.segments[i][1]) {
                validPosition = false;
                break;
            }
        }
        if (x == blueDot.x && y == blueDot.y) {
            validPosition = false;
        }
    } while (!validPosition);
}

void initialize_dots(blueDot& blueDot, redDot& redDot, Snake& snake, double& redDotTimer) {
    initialize_blue_dot(blueDot.x, blueDot.y, snake);
	if (rand() % 100 < RED_DOT_PERC) { //initializing red dot
        initialize_red_dot(redDot.x, redDot.y, snake, blueDot);
        redDotTimer = 0;
    }
	else { //initalizing red dot out of the game field
        redDot.x = -1;
        redDot.y = -1;
    }
}

bool load_bmp(GameResources& gameResources) {
    gameResources.charset = SDL_LoadBMP("./cs8x8.bmp");
    gameResources.redDotSurface = SDL_LoadBMP("./red_dot.bmp");
    gameResources.blueDotSurface = SDL_LoadBMP("./blue_dot.bmp");

    if (gameResources.charset == NULL || gameResources.blueDotSurface == NULL || gameResources.redDotSurface == NULL) {
        printf("SDL_LoadBMP error: %s\n", SDL_GetError());
        SDL_FreeSurface(gameResources.screen);
        SDL_DestroyTexture(gameResources.scrtex);
        SDL_DestroyWindow(gameResources.window);
        SDL_DestroyRenderer(gameResources.renderer);
        SDL_Quit();
        return false;
    }

    SDL_SetColorKey(gameResources.charset, true, 0x000000);
    SDL_SetColorKey(gameResources.redDotSurface, true, 0x000000);
    SDL_SetColorKey(gameResources.blueDotSurface, true, 0x000000);
    return true;
}

void create_colors(GameResources& gameResources) {
    gameResources.czarny = SDL_MapRGB(gameResources.screen->format, 0x00, 0x00, 0x00);
    gameResources.zielony = SDL_MapRGB(gameResources.screen->format, 23, 199, 64);
    gameResources.czerwony = SDL_MapRGB(gameResources.screen->format, 198, 0x00, 0x00);
    gameResources.niebieski = SDL_MapRGB(gameResources.screen->format, 0x11, 0x11, 0xCC);
    gameResources.bialy = SDL_MapRGB(gameResources.screen->format, 0xFF, 0xFF, 0xFF);
    gameResources.jasny_niebieski = SDL_MapRGB(gameResources.screen->format, 110, 149, 255);
    gameResources.fioletowy = SDL_MapRGB(gameResources.screen->format, 139, 44, 168);
}


//draw functions
void draw_progress_bar(SDL_Surface* screen, Uint32 color1, Uint32 color2, double redDotTimer) {
    double progress = redDotTimer / RED_DOT_TIME;
    DrawRectangle(screen, PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, color1, color1);
    DrawRectangle(screen, PROGRESS_BAR_X + 2, PROGRESS_BAR_Y + 2, (PROGRESS_BAR_WIDTH - 4) * (1.0 - progress), PROGRESS_BAR_HEIGHT - 4, color2, color2);
}

void draw_dot(SDL_Surface* screen, int x, int y, SDL_Surface* dot) {
    SDL_Rect dotRect;
    dotRect.w = UNIT;
    dotRect.h = UNIT;
    dotRect.x = (x + FIELD_X) * UNIT;
    dotRect.y = (y + FIELD_Y) * UNIT;
    SDL_BlitSurface(dot, NULL, screen, &dotRect);
}

void draw_dots(GameResources& gameResources, blueDot& blueDot, redDot& redDot, double redDotTimer) {
    draw_dot(gameResources.screen, blueDot.x, blueDot.y, gameResources.blueDotSurface);
	if (redDot.x != -1) { //if red dot is in the game field
        draw_dot(gameResources.screen, redDot.x, redDot.y, gameResources.redDotSurface);
        draw_progress_bar(gameResources.screen, gameResources.zielony, gameResources.czerwony, redDotTimer);
    }
}

void draw_snake(Snake& snake, GameResources& gameResources) {
    for (int i = 0; i < snake.length; i++) {
        DrawRectangle(gameResources.screen, snake.segments[i][0] * UNIT + FIELD_X * UNIT, snake.segments[i][1] * UNIT + FIELD_Y * UNIT, UNIT, UNIT, gameResources.fioletowy, gameResources.fioletowy);
    }
}

void draw_game_field(SDL_Surface* screen, Uint32 color) {
    DrawRectangle(screen, FIELD_X * UNIT, FIELD_Y * UNIT, FIELD_WIDTH * UNIT, FIELD_HEIGHT * UNIT, color, color);
}


//boundary check functions
void check_dot_collision(Snake& snake, blueDot& blueDot, redDot& redDot, int& points, double& snakeSpeed, double& redDotTimer) {
	//lengthening the snake after eating the blue dot
    if (snake.segments[0][0] == blueDot.x && snake.segments[0][1] == blueDot.y) {
        if (snake.length < MAX_SNAKE_LENGTH) {
            snake.segments[snake.length][0] = snake.segments[snake.length - 1][0];
            snake.segments[snake.length][1] = snake.segments[snake.length - 1][1];
            snake.length++;
        }

        points += BLUE_POINTS;
        initialize_blue_dot(blueDot.x, blueDot.y, snake);
        PlaySound(TEXT("./sound_blue.wav"), NULL, SND_FILENAME | SND_ASYNC);
    }
    else if (snake.segments[0][0] == redDot.x && snake.segments[0][1] == redDot.y) {
		//disappearing the red dot after eating it
        points += RED_POINTS;
        redDot.x = -1;
        redDot.y = -1;

		//generating the effect of the red dot
        if (rand() % 2 == 0) {
            snakeSpeed -= RED_DOT_SNAKE_SPEED;
        }
        else {
            if (snake.length > SHORTENING_SNAKE + 1) {
                snake.length -= SHORTENING_SNAKE;
            }
            else {
                snakeSpeed -= RED_DOT_SNAKE_SPEED;
            }
        }
        PlaySound(TEXT("./sound_red.wav"), NULL, SND_FILENAME | SND_ASYNC);
    }
}

void turn_snake_right(Snake& snake) {
    switch (snake.direction) {
    case UP:
        snake.direction = RIGHT;
        break;
    case RIGHT:
        snake.direction = DOWN;
        break;
    case DOWN:
        snake.direction = LEFT;
        break;
    case LEFT:
        snake.direction = UP;
        break;
    }
}

void turn_snake_left(Snake& snake) {
    switch (snake.direction) {
    case UP:
        snake.direction = LEFT;
        break;
    case LEFT:
        snake.direction = DOWN;
        break;
    case DOWN:
        snake.direction = RIGHT;
        break;
    case RIGHT:
        snake.direction = UP;
        break;
    }
}

bool will_hit_wall(Snake& snake) {
    switch (snake.direction) {
    case UP:
        return snake.segments[0][1] - 1 < 0;
    case DOWN:
        return snake.segments[0][1] + 1 >= FIELD_HEIGHT;
    case LEFT:
        return snake.segments[0][0] - 1 < 0;
    case RIGHT:
        return snake.segments[0][0] + 1 >= FIELD_WIDTH;
    }
    return false;
}

void check_snake_bounds(Snake& snake) {
    if (will_hit_wall(snake)) {
        turn_snake_right(snake);
        if (will_hit_wall(snake)) {
            turn_snake_left(snake);
            turn_snake_left(snake);
        }
    }
}


//text functions
void display_information(GameState& gameState, GameResources& gameResources) {
    char text[128];
    DrawRectangle(gameResources.screen, INFO_TEXT_X, INFO_TEXT_Y, INFO_TEXT_WIDTH, INFO_TEXT_HEIGHT, gameResources.zielony, gameResources.niebieski);
    sprintf(text, "Liczba punktow = %d  czas trwania = %.1lf s  %.0lf klatek / s", gameState.points, gameState.worldTime, gameState.fps);
    DrawString(gameResources.screen, gameResources.screen->w / 2 - strlen(text) * 8 / 2, 10, text, gameResources.charset);
    sprintf(text, "Esc - wyjscie, 'n' - nowa gra");
    DrawString(gameResources.screen, gameResources.screen->w / 2 - strlen(text) * 8 / 2, 26, text, gameResources.charset);
    sprintf(text, "Zrealizowane podpunkty: 1.  2.  3.  4.   A  B  C  D");
    DrawString(gameResources.screen, gameResources.screen->w / 2 - strlen(text) * 8 / 2, 42, text, gameResources.charset);
}

void lose_information(GameResources& gameResources) {
    DrawRectangle(gameResources.screen, LOSE_INFO_X, LOSE_INFO_Y - 10, LOSE_INFO_WIDTH, LOSE_INFO_HEIGHT, gameResources.zielony, gameResources.niebieski);

    const char* gameOverText = "GAME OVER";
    const char* infoText = "Wyjscie - Esc, Nowa gra - 'n'";

	//8 pixels per character
    int gameOverTextWidth = strlen(gameOverText) * 8;
    int infoTextWidth = strlen(infoText) * 8;

    DrawString(gameResources.screen, LOSE_INFO_X + (LOSE_INFO_WIDTH - gameOverTextWidth) / 2, LOSE_INFO_Y, gameOverText, gameResources.charset);
    DrawString(gameResources.screen, LOSE_INFO_X + (LOSE_INFO_WIDTH - infoTextWidth) / 2, LOSE_INFO_Y + 20, infoText, gameResources.charset);
}

void display_new_game(GameResources& gameResources, GameState& gameState) {
	if (gameState.newGameTimer > 0) { //time for displaying "New game"
        DrawString(gameResources.screen, NEW_GAME_X, NEW_GAME_Y, "New game", gameResources.charset);
        gameState.newGameTimer -= gameState.delta;
    }
}


//game functions
void reset_game(GameState& gameState, Snake& snake, blueDot& blueDot, redDot& redDot) {
    gameState.worldTime = 0;
    gameState.frames = 0;
    gameState.fpsTimer = 0;
    gameState.fps = 0;
    gameState.points = 0;
    gameState.lastSpeedIncreaseTime = 0;
    gameState.newGameTimer = NEW_GAME_TIME;
    gameState.snakeSpeed = SNAKE_SPEED;
    gameState.redDotTimer = 0;
    initialize_snake(snake);
    initialize_blue_dot(blueDot.x, blueDot.y, snake);
    if (rand() % 100 < RED_DOT_PERC) {
        initialize_red_dot(redDot.x, redDot.y, snake, blueDot);
        gameState.redDotTimer = 0;
    }
    else {
        redDot.x = -1;
        redDot.y = -1;
    }
}

void handle_events(GameState& gameState, GameResources& gameResources, Snake& snake, blueDot& blueDot, redDot& redDot) {
    while (SDL_PollEvent(&gameResources.event)) {
        switch (gameResources.event.type) {
        case SDL_KEYDOWN:
            if (gameResources.event.key.keysym.sym == SDLK_ESCAPE)
                gameState.quit = 1;
            else if (gameResources.event.key.keysym.sym == SDLK_UP && snake.direction != DOWN && snake.segments[0][1] - 1 >= 0) {
                snake.direction = UP;
            }
            else if (gameResources.event.key.keysym.sym == SDLK_DOWN && snake.direction != UP && snake.segments[0][1] + 1 < FIELD_HEIGHT) {
                snake.direction = DOWN;
            }
            else if (gameResources.event.key.keysym.sym == SDLK_LEFT && snake.direction != RIGHT && snake.segments[0][0] - 1 >= 0) {
                snake.direction = LEFT;
            }
            else if (gameResources.event.key.keysym.sym == SDLK_RIGHT && snake.direction != LEFT && snake.segments[0][0] + 1 < FIELD_WIDTH) {
                snake.direction = RIGHT;
            }
            else if (gameResources.event.key.keysym.sym == SDLK_n) {
                gameState.gameOver = false;
                reset_game(gameState, snake, blueDot, redDot);
            }
            break;
        case SDL_QUIT:
            gameState.quit = 1;
            break;
        }
    }
}

void update_snake_and_dot(GameState& gameState, redDot& redDot, blueDot& blueDot, Snake& snake) {
    //Update the snake and red dot timers based on delta time and snake speed
    gameState.snakeTimer += gameState.delta * gameState.snakeSpeed;
    gameState.redDotTimer += gameState.delta;

    //Check if it's time to increase the snake's speed
    if (gameState.worldTime - gameState.lastSpeedIncreaseTime >= GAME_SPEED_TIME) {
        gameState.snakeSpeed *= (1.0 + GAME_SPEED / 100.0);
        gameState.lastSpeedIncreaseTime = gameState.worldTime;
    }

    //Check if it's time for the red dot to appear
    if (gameState.redDotTimer >= RED_DOT_TIME) {
        gameState.redDotTimer = 0;
        if (rand() % 100 < RED_DOT_PERC) {
            initialize_red_dot(redDot.x, redDot.y, snake, blueDot);
        }
        else { //initalizing red dot out of the game field
            redDot.x = -1;
            redDot.y = -1;
        }
    }
}

void move_snake(Snake& snake, GameState& gameState, blueDot& blueDot, redDot& redDot) {
    if (gameState.snakeTimer >= 1.0) {
        gameState.snakeTimer = 0;

        //Move snake segments
        for (int i = snake.length - 1; i > 0; i--) {
            snake.segments[i][0] = snake.segments[i - 1][0];
            snake.segments[i][1] = snake.segments[i - 1][1];
        }

        //Checking borders for snake
        check_snake_bounds(snake);

        //Uploading snake head position
        switch (snake.direction) {
        case UP:
            snake.segments[0][1]--;
            break;
        case DOWN:
            snake.segments[0][1]++;
            break;
        case LEFT:
            snake.segments[0][0]--;
            break;
        case RIGHT:
            snake.segments[0][0]++;
            break;
        }

        check_dot_collision(snake, blueDot, redDot, gameState.points, gameState.snakeSpeed, gameState.redDotTimer);
    }
}

void update_game_state(GameState& gameState, GameResources& gameResources) {
    //Get the current time in milliseconds
    gameState.t2 = SDL_GetTicks();

    gameState.delta = (gameState.t2 - gameState.t1) * 0.001; //delta (time between frames) in seconds
    gameState.t1 = gameState.t2;

    //If the game is not over, update the world time and FPS timer
    if (!gameState.gameOver) {
        gameState.worldTime += gameState.delta;
        gameState.fpsTimer += gameState.delta;
    }

    SDL_FillRect(gameResources.screen, NULL, gameResources.jasny_niebieski);

    draw_game_field(gameResources.screen, gameResources.zielony);

    //Update the FPS every 0.5 seconds
    if (gameState.fpsTimer > 0.5) {
        gameState.fps = gameState.frames * 2;
        gameState.frames = 0;
        gameState.fpsTimer -= 0.5;
    }
}

void update_and_draw_snake(Snake& snake, GameResources& gameResources, blueDot& blueDot, redDot& redDot, GameState& gameState) {
    update_snake_and_dot(gameState, redDot, blueDot, snake);
    move_snake(snake, gameState, blueDot, redDot);
    draw_snake(snake, gameResources);
}

void snake_collision(Snake& snake, bool& gameOver) {
    //collision with body
    for (int i = 1; i < snake.length; i++) {
        if (snake.segments[0][0] == snake.segments[i][0] && snake.segments[0][1] == snake.segments[i][1]) {
            gameOver = true;
        }
    }
}

void gameplay(Snake& snake, GameResources& gameResources, blueDot& blueDot, redDot& redDot, GameState& gameState) {
    update_and_draw_snake(snake, gameResources, blueDot, redDot, gameState);
    draw_dots(gameResources, blueDot, redDot, gameState.redDotTimer);
    snake_collision(snake, gameState.gameOver);
}


#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
    srand(time(NULL));
    GameState gameState;
    GameResources gameResources;
    Snake snake;
    blueDot blueDot;
    redDot redDot;

    printf("SKIBIDI\n");

    if (!initialize_SDL(gameResources)) {
        return 1;
    }

    if (!load_bmp(gameResources)) {
        return 1;
    }

    create_colors(gameResources);

    initialize_game_state(gameState);

    initialize_snake(snake);
    initialize_dots(blueDot, redDot, snake, gameState.redDotTimer);

    while (!gameState.quit) {
        update_game_state(gameState, gameResources);

        if (!gameState.gameOver) {
			gameplay(snake, gameResources, blueDot, redDot, gameState);
        }
        else {
            lose_information(gameResources);
        }

        display_new_game(gameResources, gameState);

        display_information(gameState, gameResources);

        SDL_UpdateTexture(gameResources.scrtex, NULL, gameResources.screen->pixels, gameResources.screen->pitch);
        SDL_RenderCopy(gameResources.renderer, gameResources.scrtex, NULL, NULL);
        SDL_RenderPresent(gameResources.renderer);

        handle_events(gameState, gameResources, snake, blueDot, redDot);

        gameState.frames++;
    }

    // zwolnienie powierzchni / freeing all surfaces
    SDL_FreeSurface(gameResources.charset);
    SDL_FreeSurface(gameResources.screen);
    SDL_DestroyTexture(gameResources.scrtex);
    SDL_DestroyRenderer(gameResources.renderer);
    SDL_DestroyWindow(gameResources.window);

    SDL_Quit();
    return 0;
}