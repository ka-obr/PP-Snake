#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "includes/drawString.h"
#include "includes/drawSurface.h"
#include "includes/drawPixel.h"
#include "includes/drawLine.h"
#include "includes/drawRectangle.h"

extern "C" {
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH 850
#define SCREEN_HEIGHT 550
#define FIELD_WIDTH 650
#define FIELD_HEIGHT 450
#define FIELD_X 100
#define FIELD_Y 75
#define SNAKE_LENGTH 10
#define MAX_SNAKE_LENGTH 100

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define INFO_TEXT_WIDTH SCREEN_WIDTH - 8
#define INFO_TEXT_HEIGHT 54
#define NEW_GAME_TIME 1
#define SNAKE_SPEED 0.1 // Snake speed in sec

struct Snake {
	SDL_Rect segments[MAX_SNAKE_LENGTH];
    int length;
    int direction;
};

bool initialize_SDL(SDL_Window*& window, SDL_Renderer*& renderer, SDL_Surface*& screen, SDL_Texture*& scrtex) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return false;
    }

    int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "Karol Obrycki index: 203264");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    return true;
}

void initialize_snake(Snake& snake) {
    snake.length = SNAKE_LENGTH;
    snake.direction = UP;
    for (int i = 0; i < snake.length; i++) {
        snake.segments[i].x = SCREEN_WIDTH / 2;
        snake.segments[i].y = SCREEN_HEIGHT / 2 + 15 + i * 12;
        snake.segments[i].w = 12;
        snake.segments[i].h = 12;
    }
}

bool load_charset(SDL_Surface*& charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer) {
    charset = SDL_LoadBMP("./cs8x8.bmp");
    if (charset == NULL) {
        printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return false;
    }
    SDL_SetColorKey(charset, true, 0x000000);
    return true;
}

void create_colors(SDL_Surface* screen, Uint32& czarny, Uint32& zielony, Uint32& czerwony, Uint32& niebieski, Uint32& bialy, Uint32& jasny_niebieski, Uint32& fioletowy) {
    czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    zielony = SDL_MapRGB(screen->format, 23, 199, 64);
    czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
    bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
    jasny_niebieski = SDL_MapRGB(screen->format, 110, 149, 255);
    fioletowy = SDL_MapRGB(screen->format, 217, 72, 239);
}

void draw_game_field(SDL_Surface* screen, Uint32 color) {
    SDL_Rect fieldRect = { FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT };
    SDL_FillRect(screen, &fieldRect, color);
}

void display_information(SDL_Surface* screen, SDL_Surface* charset, Uint32 zielony, Uint32 niebieski, double worldTime, double fps, int points) {
    char text[128];
    DrawRectangle(screen, 4, 4, INFO_TEXT_WIDTH, INFO_TEXT_HEIGHT, zielony, niebieski);
    sprintf(text, "Liczba punktow = %d  czas trwania = %.1lf s  %.0lf klatek / s", points, worldTime, fps);
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
    sprintf(text, "Esc - wyjscie, 'n' - nowa gra");
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
    sprintf(text, "Zrealizowane podpunkty: 1.  2.  3.  4.   A  B  C  D");
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 42, text, charset);
}

void lose_information(SDL_Surface* screen, SDL_Surface* charset, Uint32 zielony, Uint32 niebieski) {
    int rectWidth = 300;
    int rectHeight = 50;
    int rectX = SCREEN_WIDTH / 2 - rectWidth / 2;
    int rectY = SCREEN_HEIGHT / 2 - rectHeight / 2;

    DrawRectangle(screen, rectX, rectY, rectWidth, rectHeight, zielony, niebieski);

    const char* gameOverText = "GAME OVER";
    const char* infoText = "Wyjscie - Esc, Nowa gra - 'n'";

    int gameOverTextWidth = strlen(gameOverText) * 8;
    int infoTextWidth = strlen(infoText) * 8;

    DrawString(screen, rectX + (rectWidth - gameOverTextWidth) / 2, rectY + 10, gameOverText, charset);
    DrawString(screen, rectX + (rectWidth - infoTextWidth) / 2, rectY + 30, infoText, charset);
}

void display_new_game(SDL_Surface* screen, SDL_Surface* charset, double& newGameTimer, double delta) {
    if (newGameTimer > 0) {
        DrawString(screen, SCREEN_WIDTH / 2 - 4 * 8, SCREEN_HEIGHT / 2 - 8, "New game", charset);
        newGameTimer -= delta;
    }
}

void reset_game(double& worldTime, int& frames, double& fpsTimer, double& fps, double& newGameTimer, Snake& snake) {
    worldTime = 0;
    frames = 0;
    fpsTimer = 0;
    fps = 0;
    newGameTimer = NEW_GAME_TIME;
    snake.length = SNAKE_LENGTH;
    snake.direction = UP;
    for (int i = 0; i < snake.length; i++) {
        snake.segments[i].x = SCREEN_WIDTH / 2;
        snake.segments[i].y = SCREEN_HEIGHT / 2 + 15 + i * 12;
        snake.segments[i].w = 12;
        snake.segments[i].h = 12;
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

void update_and_draw_snake(Snake& snake, double& snakeTimer, double delta, SDL_Surface* screen, Uint32 color) {
    snakeTimer += delta;
    if (snakeTimer >= SNAKE_SPEED) {
        snakeTimer = 0;

        // Przesuniêcie segmentów cia³a
        for (int i = snake.length - 1; i > 0; i--) {
            snake.segments[i] = snake.segments[i - 1];
        }

        // Aktualizacja pozycji g³owy wê¿a
        switch (snake.direction) {
        case UP:
            snake.segments[0].y -= 12;
            break;
        case DOWN:
            snake.segments[0].y += 12;
            break;
        case LEFT:
            snake.segments[0].x -= 12;
            break;
        case RIGHT:
            snake.segments[0].x += 12;
            break;
        }
    }

    // Rysowanie segmentów cia³a wê¿a na powierzchni
    for (int i = 0; i < snake.length; i++) {
        SDL_FillRect(screen, &snake.segments[i], color);
    }
}

void snake_collision(Snake& snake, bool& gameOver) {
    //kolizja z cia³em
    for (int i = 1; i < snake.length; i++) {
        if (snake.segments[0].x == snake.segments[i].x && snake.segments[0].y == snake.segments[i].y) {
            gameOver = true;
        }
    }
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
    int t1, t2, quit, frames, points = 0;
    double delta, worldTime, fpsTimer, fps, newGameTimer, snakeTimer = 0;
    Uint32 czarny, zielony, czerwony, niebieski, bialy, jasny_niebieski, fioletowy;
    SDL_Event event;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* screen;
    SDL_Surface* charset;
    SDL_Texture* scrtex;
    Snake snake;
    bool gameOver = false;

    printf("SKIBIDI\n");

    if (!initialize_SDL(window, renderer, screen, scrtex)) {
        return 1;
    }

    if (!load_charset(charset, screen, scrtex, window, renderer)) {
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

	initialize_snake(snake);

    while (!quit) {
        update_game_state(t1, t2, delta, worldTime, screen, jasny_niebieski, zielony, fpsTimer, frames, fps, gameOver);

        if (!gameOver) {
            update_and_draw_snake(snake, snakeTimer, delta, screen, fioletowy);
            snake_collision(snake, gameOver);
        }
        else {
            lose_information(screen, charset, zielony, niebieski);
        };

        // Tekst informacyjny
        display_information(screen, charset, zielony, niebieski, worldTime, fps, points);

        // Wyswietlenie napisu "New game"
        display_new_game(screen, charset, newGameTimer, delta);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Obs³uga zdarzeñ
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    quit = 1;
                else if (event.key.keysym.sym == SDLK_UP && snake.direction != DOWN) {
                    snake.direction = UP;
                }
                else if (event.key.keysym.sym == SDLK_DOWN && snake.direction != UP) {
                    snake.direction = DOWN;
                }
                else if (event.key.keysym.sym == SDLK_LEFT && snake.direction != RIGHT) {
                    snake.direction = LEFT;
                }
                else if (event.key.keysym.sym == SDLK_RIGHT && snake.direction != LEFT) {
                    snake.direction = RIGHT;
                }
                else if (event.key.keysym.sym == SDLK_n) {
					gameOver = false;
                    reset_game(worldTime, frames, fpsTimer, fps, newGameTimer, snake);
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