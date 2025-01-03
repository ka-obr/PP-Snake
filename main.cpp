#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

#include "includes/drawString.h"
#include "includes/drawSurface.h"
#include "includes/drawPixel.h"
#include "includes/drawLine.h"
#include "includes/drawRectangle.h"

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	1000
#define SCREEN_HEIGHT	700
#define FIELD_WIDTH 800
#define FIELD_HEIGHT 600
#define FIELD_X 100
#define FIELD_Y 75
#define SNAKE_LENGTH 5
#define MAX_SNAKE_LENGTH 100

#define INFO_TEXT_WIDTH SCREEN_WIDTH - 8
#define INFO_TEXT_HEIGHT 54
#define NEW_GAME_TIME 1


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

	//SDL_ShowCursor(SDL_DISABLE);

	return true;
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

void create_colors(SDL_Surface* screen, Uint32* czarny, Uint32* zielony, Uint32* czerwony, Uint32* niebieski, Uint32* bialy, Uint32* jasny_niebieski) {
	*czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	*zielony = SDL_MapRGB(screen->format, 23, 199, 64);
	*czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	*niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	*bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
	*jasny_niebieski = SDL_MapRGB(screen->format, 110, 149, 255);
};

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

void display_new_game(SDL_Surface* screen, SDL_Surface* charset, double& newGameTimer, double delta) {
	if (newGameTimer > 0) {
		DrawString(screen, SCREEN_WIDTH / 2 - 4 * 8, SCREEN_HEIGHT / 2 - 8, "New game", charset);
		newGameTimer -= delta;
	}
}

void reset_game(double& worldTime, double& distance, double& etiSpeed, int& frames, double& fpsTimer, double& fps, double& newGameTimer) {
	worldTime = 0;
	distance = 0;
	etiSpeed = 1;
	frames = 0;
	fpsTimer = 0;
	fps = 0;
	newGameTimer = NEW_GAME_TIME;
}

void update_game_state(int& t1, int& t2, double& delta, double& worldTime, double& distance, double etiSpeed, SDL_Surface* screen, Uint32 jasny_niebieski, Uint32 zielony, SDL_Surface* eti, double& fpsTimer, int& frames, double& fps) {
	t2 = SDL_GetTicks();

	// w tym momencie t2-t1 to czas w milisekundach,
	// jaki uplyna³ od ostatniego narysowania ekranu
	// delta to ten sam czas w sekundach
	// here t2-t1 is the time in milliseconds since
	// the last screen was drawn
	// delta is the same time in seconds
	delta = (t2 - t1) * 0.001;
	t1 = t2;

	worldTime += delta;

	distance += etiSpeed * delta;

	SDL_FillRect(screen, NULL, jasny_niebieski);

	draw_game_field(screen, zielony);

	DrawSurface(screen, eti,
		SCREEN_WIDTH / 2 + sin(distance) * SCREEN_HEIGHT / 3,
		SCREEN_HEIGHT / 2 + cos(distance) * SCREEN_HEIGHT / 3);

	fpsTimer += delta;
	if (fpsTimer > 0.5) {
		fps = frames * 2;
		frames = 0;
		fpsTimer -= 0.5;
	}
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc, points = 0;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed, newGameTimer;
	Uint32 czarny, zielony, czerwony, niebieski, bialy, jasny_niebieski;
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Surface *eti;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;

	printf("SKIBIDI\n");

	if (!initialize_SDL(window, renderer, screen, scrtex)) {
		return 1;
	}

	if (!load_charset(charset, screen, scrtex, window, renderer)) {
		return 1;
	}

	eti = SDL_LoadBMP("./eti.bmp");
	if(eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};

	create_colors(screen, &czarny, &zielony, &czerwony, &niebieski, &bialy, &jasny_niebieski);

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	distance = 0;
	etiSpeed = 1;
	newGameTimer = 0;

	while(!quit) {
		update_game_state(t1, t2, delta, worldTime, distance, etiSpeed, screen, jasny_niebieski, zielony, eti, fpsTimer, frames, fps);

		// tekst informacyjny / info text
		display_information(screen, charset, zielony, niebieski, worldTime, fps, points);

		// wyswietlenie napisu "New game"
		display_new_game(screen, charset, newGameTimer, delta);


		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
//		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// obs³uga zdarzeñ (o ile jakieœ zasz³y) / handling of events (if there were any)
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) 
						quit = 1;
					else if(event.key.keysym.sym == SDLK_UP) 
						etiSpeed = 2.0;
					else if(event.key.keysym.sym == SDLK_DOWN) 
						etiSpeed = 0.3;
					else if (event.key.keysym.sym == SDLK_n)
						reset_game(worldTime, distance, etiSpeed, frames, fpsTimer, fps, newGameTimer);
					break;
				case SDL_KEYUP:
					etiSpeed = 1.0;
					break;
				case SDL_QUIT:
					quit = 1;
					break;
				};
			};
		frames++;
		};

	// zwolnienie powierzchni / freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
	};
