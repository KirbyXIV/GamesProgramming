#include <SDL.h>
#include <SDL_ttf.h>
#include <deque>
#include <iostream>
#include <algorithm>
#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int PLAYER_SIZE = 32;
const int NORMAL_SPEED = 4;      // Normal movement speed
const int SPRINT_SPEED = 8;      // Sprint speed
const int SHADOW_DELAY_MS = 3000;
const int GRAVITY = 1;           // Gravity strength
const int JUMP_STRENGTH = 15;    // Jump force
const int DASH_MAX_SPEED = 12;   // Maximum dash speed
const int DASH_ACCELERATION = 2; // Acceleration during dash
const int TIMER_LIMIT = 30000;   // 30-second timer limit (in milliseconds)

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
    TTF_Init();  // Initialize SDL_ttf for text rendering

    SDL_Window* window = SDL_CreateWindow("Shadow Echo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load a font
    TTF_Font* font = TTF_OpenFont("assets/fonts/PixelifySans-VariableFont_wght.ttf", 24); // Make sure to have the Arial font or change to a valid path
    if (!font) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return -1;
    }

    Entity player{ 100, 100 };
    Entity shadow{ -100, -100 };

    Uint32 gameStartTime = SDL_GetTicks();
    Uint32 shadowStartTime = 0;
    size_t shadowIndex = 0;
    bool shadowActive = false;

    bool running = true;
    SDL_Event event;

    bool isJumping = false;      // To track if player is in the air
    float velocityY = 0;         // Vertical speed, influenced by gravity
    float groundLevel = SCREEN_HEIGHT - PLAYER_SIZE; // Y position of the ground
    bool canDash = true;         // To check if player can dash again
    bool isDashing = false;      // To track whether player is currently dashing
    float dashTime = 0;          // Timer to reset dash
    float dashSpeed = 0;         // Current dash speed (starts at 0 and increases)

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsed = currentTime - gameStartTime;

        // Timer check (30 seconds)
        if (elapsed >= TIMER_LIMIT) {
            std::cout << "Time's up! Game over.\n";
            running = false;
        }

        // Input handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        int dx = 0, dy = 0;

        // Sprint: Increase speed when Left Shift is held down on the ground
        int speed = NORMAL_SPEED;   // Default to normal speed
        if (keys[SDL_SCANCODE_LSHIFT] && !isJumping) {
            speed = SPRINT_SPEED;    // Sprint speed
        }

        // Regular movement (left-right)
        if (keys[SDL_SCANCODE_A]) dx -= speed;
        if (keys[SDL_SCANCODE_D]) dx += speed;

        // Jumping
        if (keys[SDL_SCANCODE_SPACE] && !isJumping) { // Jump
            velocityY = -JUMP_STRENGTH;
            isJumping = true;
        }

        // Dash (only in the air)
        if (keys[SDL_SCANCODE_LSHIFT] && isJumping && canDash) { // Dash in the air
            isDashing = true;  // Enable dashing
            canDash = false;   // Prevent further dashes until reset
            dashTime = currentTime;  // Record the time of dash start
        }

        // Apply dash acceleration (smooth dash)
        if (isDashing) {
            // Increase dash speed gradually (acceleration)
            if (dashSpeed < DASH_MAX_SPEED) {
                dashSpeed += DASH_ACCELERATION;
            }

            // Apply dash speed in the direction the player is moving
            if (keys[SDL_SCANCODE_A]) {
                dx -= static_cast<int>(dashSpeed);  // Dash left with gradual acceleration
            }
            if (keys[SDL_SCANCODE_D]) {
                dx += static_cast<int>(dashSpeed);  // Dash right with gradual acceleration
            }
        }

        // Horizontal movement
        player.x += dx;

        // Gravity effect
        velocityY += GRAVITY; // Pull down the player over time
        player.y += velocityY;

        // Collision with the ground (reset jump)
        if (player.y >= groundLevel) {
            player.y = groundLevel;  // Keep player on the ground
            velocityY = 0;            // Stop falling
            isJumping = false;        // Allow jumping again
            canDash = true;           // Reset dash availability when touching the ground
            isDashing = false;        // Reset dash state when on the ground
            dashSpeed = 0;            // Reset dash speed when grounded
        }

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

        // Display Timer
        int remainingTime = (TIMER_LIMIT - elapsed) / 1000; // Convert to seconds
        std::string timerText = "Time Left: " + std::to_string(remainingTime) + "s";
        SDL_Color textColor = { 255, 255, 255 }; // White text

        // Render the timer text
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, timerText.c_str(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);

        // Get the dimensions of the text texture
        int textWidth = 0, textHeight = 0;
        SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

        // Render the text at the top-left corner of the screen
        SDL_Rect textRect = { 10, 10, textWidth, textHeight };
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS

        // Dash reset: Check if dash has finished (timer)
        if (isDashing && currentTime - dashTime > 200) {
            isDashing = false;  // End the dash after 200ms
        }
    }

    // Clean up
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit(); // Clean up SDL_ttf
    return 0;
}
