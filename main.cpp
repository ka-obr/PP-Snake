#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "includes/drawString.h"
#include "includes/drawSurface.h"
#include "includes/drawPixel.h"
#include "includes/drawLine.h"
#include "includes/drawRectangle.h"

extern "C" {
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
}

#define UNIT_WIDTH 25

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
#define SNAKE_SPEED 0.15 // Snake speed in sec
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

//Points for blue & red dot dot
#define BLUE_POINTS 2
#define RED_POINTS 5

//Speedup of the game defines
#define GAME_SPEED 20.0 //percentage of game acceleration
#define GAME_SPEED_TIME 10 //time in seconds to increase game speed

//Red dot defines
#define RED_DOT_TIME 5.0 //time in seconds
#define RED_DOT_PERC 40 //chance of red dot appealing
#define RED_DOT_SNAKE_SPEED 0.05 //increasing speed of the snake after eating red dot
#define SHORTENING_SNAKE 5

const enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
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

bool initialize_SDL(SDL_Window*& window, SDL_Renderer*& renderer, SDL_Surface*& screen, SDL_Texture*& scrtex) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return false;
    }

    int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH * UNIT_WIDTH, SCREEN_HEIGHT * UNIT_WIDTH, 0, &window, &renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH * UNIT_WIDTH, SCREEN_HEIGHT * UNIT_WIDTH);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "Karol Obrycki index: 203264");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH * UNIT_WIDTH, SCREEN_HEIGHT * UNIT_WIDTH, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH * UNIT_WIDTH, SCREEN_HEIGHT * UNIT_WIDTH);

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

void initialize_blue_dot(int& x, int& y, Snake& snake) {
    bool validPosition;
    do {
        validPosition = true;
        x = rand() % FIELD_WIDTH;
        y = rand() % FIELD_HEIGHT;
        for (int i = 0; i < snake.length; i++) {
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
        x = rand() % FIELD_WIDTH;
        y = rand() % FIELD_HEIGHT;
        for (int i = 0; i < snake.length; i++) {
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
    if (rand() % 100 < RED_DOT_PERC) {
        initialize_red_dot(redDot.x, redDot.y, snake, blueDot);
        redDotTimer = 0;
    }
    else {
        redDot.x = -1;
        redDot.y = -1;
    }
}

void draw_dot(SDL_Surface* screen, int x, int y, SDL_Surface* dot) {
	SDL_Rect dotRect;
    dotRect.w = UNIT_WIDTH;
    dotRect.h = UNIT_WIDTH;
    dotRect.x = (x + FIELD_X) * UNIT_WIDTH;
    dotRect.y = (y + FIELD_Y) * UNIT_WIDTH;
    SDL_BlitSurface(dot, NULL, screen, &dotRect);
}

void draw_dots(SDL_Surface* screen, blueDot& blueDot, redDot& redDot, SDL_Surface* blueDotSurface, SDL_Surface* redDotSurface) {
    draw_dot(screen, blueDot.x, blueDot.y, blueDotSurface);
	if (redDot.x != -1) {
		draw_dot(screen, redDot.x, redDot.y, redDotSurface);
	}
}

void check_dot_collision(Snake& snake, blueDot& blueDot, redDot& redDot, int& points, double& snakeSpeed, double& redDotTimer) {
    if (snake.segments[0][0] == blueDot.x && snake.segments[0][1] == blueDot.y) {
        if (snake.length < MAX_SNAKE_LENGTH) {
            snake.segments[snake.length][0] = snake.segments[snake.length - 1][0];
            snake.segments[snake.length][1] = snake.segments[snake.length - 1][1];
            snake.length++;
        }

        points += BLUE_POINTS;
        initialize_blue_dot(blueDot.x, blueDot.y, snake);
        printf("Snake length after eating blue dot: %d\n", snake.length);
    }
    else if (snake.segments[0][0] == redDot.x && snake.segments[0][1] == redDot.y) {
        points += RED_POINTS;
        redDot.x = -1;
        redDot.y = -1;
        redDotTimer = 0;

        if (rand() % 2 == 0) {
            snakeSpeed += RED_DOT_SNAKE_SPEED;
        }
        else {
            if (snake.length > SHORTENING_SNAKE + 1) {
                snake.length -= SHORTENING_SNAKE;
            }
            else {
                snakeSpeed += RED_DOT_SNAKE_SPEED;
            }
        }
        printf("Snake length after eating red dot: %d\n", snake.length);
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

bool load_bmp(SDL_Surface*& charset, SDL_Surface*& redDot, SDL_Surface*& blueDot, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer) {
    charset = SDL_LoadBMP("./cs8x8.bmp");
	redDot = SDL_LoadBMP("./red_dot.bmp");
	blueDot = SDL_LoadBMP("./blue_dot.bmp");

    if (charset == NULL || blueDot == NULL || redDot == NULL) {
        printf("SDL_LoadBMP error: %s\n", SDL_GetError());
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return false;
    }

    SDL_SetColorKey(charset, true, 0x000000);
    SDL_SetColorKey(redDot, true, 0x000000);
    SDL_SetColorKey(blueDot, true, 0x000000);
    return true;
}

void create_colors(SDL_Surface* screen, Uint32& czarny, Uint32& zielony, Uint32& czerwony, Uint32& niebieski, Uint32& bialy, Uint32& jasny_niebieski, Uint32& fioletowy) {
    czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    zielony = SDL_MapRGB(screen->format, 23, 199, 64);
    czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
    bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
    jasny_niebieski = SDL_MapRGB(screen->format, 110, 149, 255);
    fioletowy = SDL_MapRGB(screen->format, 139, 44, 168);
}

void draw_game_field(SDL_Surface* screen, Uint32 color) {
    DrawRectangle(screen, FIELD_X * UNIT_WIDTH, FIELD_Y * UNIT_WIDTH, FIELD_WIDTH * UNIT_WIDTH, FIELD_HEIGHT * UNIT_WIDTH, color, color);
}

void display_information(SDL_Surface* screen, SDL_Surface* charset, Uint32 zielony, Uint32 niebieski, double worldTime, double fps, int points) {
    char text[128];
    DrawRectangle(screen, INFO_TEXT_X, INFO_TEXT_Y, INFO_TEXT_WIDTH, INFO_TEXT_HEIGHT, zielony, niebieski);
    sprintf(text, "Liczba punktow = %d  czas trwania = %.1lf s  %.0lf klatek / s", points, worldTime, fps);
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
    sprintf(text, "Esc - wyjscie, 'n' - nowa gra");
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
    sprintf(text, "Zrealizowane podpunkty: 1.  2.  3.  4.   A  B  C  D");
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 42, text, charset);
}

void lose_information(SDL_Surface* screen, SDL_Surface* charset, Uint32 zielony, Uint32 niebieski) {

    DrawRectangle(screen, LOSE_INFO_X, LOSE_INFO_Y - 10, LOSE_INFO_WIDTH, LOSE_INFO_HEIGHT, zielony, niebieski);

    const char* gameOverText = "GAME OVER";
    const char* infoText = "Wyjscie - Esc, Nowa gra - 'n'";

    int gameOverTextWidth = strlen(gameOverText) * 8;
    int infoTextWidth = strlen(infoText) * 8;

    DrawString(screen, LOSE_INFO_X + (LOSE_INFO_WIDTH - gameOverTextWidth) / 2, LOSE_INFO_Y, gameOverText, charset);
    DrawString(screen, LOSE_INFO_X + (LOSE_INFO_WIDTH - infoTextWidth) / 2, LOSE_INFO_Y + 20, infoText, charset);
}

void display_new_game(SDL_Surface* screen, SDL_Surface* charset, double& newGameTimer, double delta) {
    if (newGameTimer > 0) {
        DrawString(screen, NEW_GAME_X, NEW_GAME_Y, "New game", charset);
        newGameTimer -= delta;
    }
}

void reset_game(double& worldTime, int& frames, double& fpsTimer, double& fps, double& newGameTimer, Snake& snake, blueDot& blueDot, redDot& redDot, int& points, double& lastSpeedIncreaseTime, double& snakeSpeed, double& redDotTimer) {
    worldTime = 0;
    frames = 0;
    fpsTimer = 0;
    fps = 0;
    points = 0;
    lastSpeedIncreaseTime = 0;
    newGameTimer = NEW_GAME_TIME;
    snakeSpeed = SNAKE_SPEED;
    redDotTimer = 0;
    initialize_snake(snake);
    initialize_blue_dot(blueDot.x, blueDot.y, snake);
    if (rand() % 100 < RED_DOT_PERC) {
        initialize_red_dot(redDot.x, redDot.y, snake, blueDot);
        redDotTimer = 0;
    }
    else {
        redDot.x = -1;
        redDot.y = -1;
    }
}

void update_game_state(int& t1, int& t2, double& delta, double& worldTime, SDL_Surface* screen, Uint32 jasny_niebieski, Uint32 zielony, double& fpsTimer, int& frames, double& fps, bool& gameOver) {
    t2 = SDL_GetTicks();

    delta = (t2 - t1) * 0.001;
    t1 = t2;

    if (!gameOver) {
        worldTime += delta;
        fpsTimer += delta;
    }

    SDL_FillRect(screen, NULL, jasny_niebieski);

    draw_game_field(screen, zielony);

    if (fpsTimer > 0.5) {
        fps = frames * 2;
        frames = 0;
        fpsTimer -= 0.5;
    }
}

void update_and_draw_snake(Snake& snake, double& snakeTimer, double delta, SDL_Surface* screen, Uint32 color, blueDot& blueDot, redDot& redDot, int& points, double& snakeSpeed, double& worldTime, double& lastSpeedIncreaseTime, double& redDotTimer) {
    snakeTimer += delta;
    redDotTimer += delta;

    if (worldTime - lastSpeedIncreaseTime >= GAME_SPEED_TIME) {
        snakeSpeed *= (100.0 - GAME_SPEED) / 100.0;
        lastSpeedIncreaseTime = worldTime;
    }

    if (redDotTimer >= RED_DOT_TIME) {
        redDotTimer = 0;
        if (rand() % 100 < RED_DOT_PERC) {
            initialize_red_dot(redDot.x, redDot.y, snake, blueDot);
        }
        else {
            redDot.x = -1;
            redDot.y = -1;
        }
    }

    if (snakeTimer >= snakeSpeed) {
        snakeTimer = 0;

        // Przesuniêcie segmentów cia³a
        for (int i = snake.length - 1; i > 0; i--) {
            snake.segments[i][0] = snake.segments[i - 1][0];
            snake.segments[i][1] = snake.segments[i - 1][1];
        }

        // Sprawdzenie, czy w¹¿ nie wyjedzie poza granice pola
        check_snake_bounds(snake);

        // Aktualizacja pozycji g³owy wê¿a
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

        check_dot_collision(snake, blueDot, redDot, points, snakeSpeed, redDotTimer);
    }

    // Rysowanie segmentów cia³a wê¿a na powierzchni
    for (int i = 0; i < snake.length; i++) {
        DrawRectangle(screen, snake.segments[i][0] * UNIT_WIDTH + FIELD_X * UNIT_WIDTH, snake.segments[i][1] * UNIT_WIDTH + FIELD_Y * UNIT_WIDTH, UNIT_WIDTH, UNIT_WIDTH, color, color);
    }
}

void snake_collision(Snake& snake, bool& gameOver) {
    //collision with body
    for (int i = 1; i < snake.length; i++) {
        if (snake.segments[0][0] == snake.segments[i][0] && snake.segments[0][1] == snake.segments[i][1]) {
            gameOver = true;
        }
    }
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
    srand(time(NULL));
    int t1, t2, quit, frames, points = 0;
    double delta, worldTime, fpsTimer, fps, newGameTimer, snakeTimer = 0, snakeSpeed = SNAKE_SPEED, lastSpeedIncreaseTime = 0, redDotTimer = 0;
    Uint32 czarny, zielony, czerwony, niebieski, bialy, jasny_niebieski, fioletowy;
    SDL_Event event;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* screen;
    SDL_Surface* charset;
	SDL_Surface* blueDotSurface;
	SDL_Surface* redDotSurface;
    SDL_Texture* scrtex;
    Snake snake;
	blueDot blueDot;
    redDot redDot;
    bool gameOver = false;

    printf("SKIBIDI\n");

    if (!initialize_SDL(window, renderer, screen, scrtex)) {
        return 1;
    }

    if (!load_bmp(charset, redDotSurface, blueDotSurface, screen, scrtex, window, renderer)) {
        return 1;
    }

    create_colors(screen, czarny, zielony, czerwony, niebieski, bialy, jasny_niebieski, fioletowy);

    t1 = SDL_GetTicks();

    frames = 0;
    fpsTimer = 0;
    fps = 0;
    quit = 0;
    worldTime = 0;
    newGameTimer = 0;
    points = 0;

    initialize_snake(snake);
    initialize_dots(blueDot, redDot, snake, redDotTimer);

    while (!quit) {
        update_game_state(t1, t2, delta, worldTime, screen, jasny_niebieski, zielony, fpsTimer, frames, fps, gameOver);

        if (!gameOver) {
            update_and_draw_snake(snake, snakeTimer, delta, screen, fioletowy, blueDot, redDot, points, snakeSpeed, worldTime, lastSpeedIncreaseTime, redDotTimer);
            draw_dots(screen, blueDot, redDot, blueDotSurface, redDotSurface);
            snake_collision(snake, gameOver);
        }
        else {
            lose_information(screen, charset, zielony, niebieski);
        }

        display_new_game(screen, charset, newGameTimer, delta);

        // Tekst informacyjny
        display_information(screen, charset, zielony, niebieski, worldTime, fps, points);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Obs³uga zdarzeñ
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    quit = 1;
                else if (event.key.keysym.sym == SDLK_UP && snake.direction != DOWN && snake.segments[0][1] - 1 >= 0) {
                        snake.direction = UP;
                }
                else if (event.key.keysym.sym == SDLK_DOWN && snake.direction != UP && snake.segments[0][1] + 1 < FIELD_HEIGHT) {
                        snake.direction = DOWN;
                }
                else if (event.key.keysym.sym == SDLK_LEFT && snake.direction != RIGHT && snake.segments[0][0] - 1 >= 0) {
                        snake.direction = LEFT;
                }
                else if (event.key.keysym.sym == SDLK_RIGHT && snake.direction != LEFT && snake.segments[0][0] + 1 < FIELD_WIDTH) {
                        snake.direction = RIGHT;
                }
                else if (event.key.keysym.sym == SDLK_n) {
                    gameOver = false;
                    reset_game(worldTime, frames, fpsTimer, fps, newGameTimer, snake, blueDot, redDot, points, lastSpeedIncreaseTime, snakeSpeed, redDotTimer);
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            }
        }

        frames++;
    }

    // zwolnienie powierzchni / freeing all surfaces
    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}