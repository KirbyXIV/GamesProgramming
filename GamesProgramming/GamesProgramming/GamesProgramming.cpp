#include <SDL.h>
#include <deque>
#include <iostream>
#include <algorithm>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int PLAYER_SIZE = 32;
const int SPEED = 4;
const int SHADOW_DELAY_MS = 3000;

struct Entity {
    float x, y;
    SDL_Rect rect() const {
        return { static_cast<int>(x), static_cast<int>(y), PLAYER_SIZE, PLAYER_SIZE };
    }
};

struct PlayerState {
    float x, y;
    Uint32 timestamp;
};

std::deque<PlayerState> movementHistory;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_Window* window = SDL_CreateWindow("Shadow Echo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Entity player{ 100, 100 };
    Entity shadow{ -100, -100 };

    Uint32 gameStartTime = SDL_GetTicks();
    Uint32 shadowStartTime = 0;
    size_t shadowIndex = 0;
    bool shadowActive = false;

    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsed = currentTime - gameStartTime;

        // Input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        int dx = 0, dy = 0;
        if (keys[SDL_SCANCODE_W]) dy -= SPEED;
        if (keys[SDL_SCANCODE_S]) dy += SPEED;
        if (keys[SDL_SCANCODE_A]) dx -= SPEED;
        if (keys[SDL_SCANCODE_D]) dx += SPEED;

        player.x += dx;
        player.y += dy;

        // Manually clamp the player position within bounds
        if (player.x < 0) player.x = 0;
        if (player.y < 0) player.y = 0;
        if (player.x > SCREEN_WIDTH - PLAYER_SIZE) player.x = SCREEN_WIDTH - PLAYER_SIZE;
        if (player.y > SCREEN_HEIGHT - PLAYER_SIZE) player.y = SCREEN_HEIGHT - PLAYER_SIZE;

        // Record player state
        movementHistory.push_back({ player.x, player.y, elapsed });

        // Start shadow after delay
        if (!shadowActive && elapsed >= SHADOW_DELAY_MS) {
            shadowActive = true;
            shadowStartTime = elapsed - SHADOW_DELAY_MS;
        }

        // Update shadow movement
        if (shadowActive && shadowIndex < movementHistory.size()) {
            Uint32 shadowTime = elapsed - SHADOW_DELAY_MS;

            // Move through recorded steps as time progresses
            while (shadowIndex + 1 < movementHistory.size() &&
                movementHistory[shadowIndex + 1].timestamp <= shadowTime) {
                ++shadowIndex;
            }

            if (shadowIndex < movementHistory.size()) {
                shadow.x = movementHistory[shadowIndex].x;
                shadow.y = movementHistory[shadowIndex].y;
            }
        }

        // Collision check
        SDL_Rect playerRect = player.rect();
        SDL_Rect shadowRect = shadow.rect();

        if (shadowActive && SDL_HasIntersection(&playerRect, &shadowRect)) {
            std::cout << "Caught by your echo! Game over.\n";
            running = false;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
        SDL_RenderFillRect(renderer, &playerRect);

        if (shadowActive) {
            SDL_SetRenderDrawColor(renderer, 200, 0, 100, 255);
            SDL_RenderFillRect(renderer, &shadowRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}