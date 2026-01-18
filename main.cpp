#include <SDL.h>
#include <SDL_main.h>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#define MAIN_FUNCTION SDL_main
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#undef MAIN_FUNCTION
#define MAIN_FUNCTION main
#endif

int SCREEN_WIDTH  = 800;
int SCREEN_HEIGHT = 600;

struct Ball {
    float     x, y;
    float     vy;
    float     radius;
    SDL_Color color;
};

struct Pillar {
    float x;
    float width;
    float topHeight;
    float gap;
    bool  passed;
};

void drawCircle(SDL_Renderer* renderer, int x, int y, int r, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int w = 0; w < r * 2; w++) {
        for (int h = 0; h < r * 2; h++) {
            int dx = r - w;
            int dy = r - h;
            if ((dx * dx + dy * dy) <= (r * r)) { SDL_RenderDrawPoint(renderer, x + dx, y + dy); }
        }
    }
}

SDL_Window*   window   = NULL;
SDL_Renderer* renderer = NULL;

Ball  ball         = {100.0f, SCREEN_HEIGHT / 2.0f, 0.0f, 15.0f, {255, 50, 50, 255}};
float gravity      = 0.4f;
float jumpStrength = -7.0f;

std::vector<Pillar> pillars;
float               pillarSpeed      = 4.0f;
int                 pillarSpawnTimer = 0;
int                 score            = 0;

bool      quit = false;
SDL_Event e;
bool      gameOver = false;

// Scale game speed on Web to make gameplay slower (adjustable)
#ifdef __EMSCRIPTEN__
const float WEB_SPEED_FACTOR = 0.7f; // 70% speed on Emscripten builds
#else
const float WEB_SPEED_FACTOR = 1.0f;
#endif

// Helper to draw a single 7-segment digit
auto drawDigit = [](int digit, int x, int y, int size) {
    // bitmask for segments: a, b, c, d, e, f, g
    //      a
    //    f   b
    //      g
    //    e   c
    //      d
    unsigned char segments[] = {
        0x3F, // 0: abcdef
        0x06, // 1: bc
        0x5B, // 2: abdeg
        0x4F, // 3: abcdg
        0x66, // 4: bcfg
        0x6D, // 5: acdfg
        0x7D, // 6: acdefg
        0x07, // 7: abc
        0x7F, // 8: abcdefg
        0x6F  // 9: abcdfg
    };
    unsigned char mask = segments[digit % 10];
    int           t    = size / 5; // thickness

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if (mask & 0x01) {
        SDL_Rect r = {x, y, size, t};
        SDL_RenderFillRect(renderer, &r);
    } // a
    if (mask & 0x02) {
        SDL_Rect r = {x + size - t, y, t, size};
        SDL_RenderFillRect(renderer, &r);
    } // b
    if (mask & 0x04) {
        SDL_Rect r = {x + size - t, y + size, t, size};
        SDL_RenderFillRect(renderer, &r);
    } // c
    if (mask & 0x08) {
        SDL_Rect r = {x, y + 2 * size - t, size, t};
        SDL_RenderFillRect(renderer, &r);
    } // d
    if (mask & 0x10) {
        SDL_Rect r = {x, y + size, t, size};
        SDL_RenderFillRect(renderer, &r);
    } // e
    if (mask & 0x20) {
        SDL_Rect r = {x, y, t, size};
        SDL_RenderFillRect(renderer, &r);
    } // f
    if (mask & 0x40) {
        SDL_Rect r = {x, y + size - t / 2, size, t};
        SDL_RenderFillRect(renderer, &r);
    } // g
};

auto drawNumber = [](int num, int x, int y, int size) {
    std::string s = std::to_string(num);
    for (int i = 0; i < (int)s.length(); ++i) {
        drawDigit(s[i] - '0', x + i * (size + size / 2), y, size);
    }
};

void gameLoop()
{

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) { quit = true; }

        bool inputTriggered = false;

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_SPACE) { inputTriggered = true; }
        }

        if (e.type == SDL_FINGERDOWN) { inputTriggered = true; }

        if (inputTriggered) {
            if (gameOver) {
                // Reset game
                ball.y  = SCREEN_HEIGHT / 2.0f;
                ball.vy = 0;
                pillars.clear();
                score    = 0;
                gameOver = false;
            } else {
                ball.vy = jumpStrength * WEB_SPEED_FACTOR;
            }
        }
    }

    if (!gameOver) {
        // Update ball (scaled for Web builds)
        ball.vy += gravity * WEB_SPEED_FACTOR;
        ball.y += ball.vy * WEB_SPEED_FACTOR;

        // Spawn pillars
        if (pillarSpawnTimer-- <= 0) {
#ifdef __ANDROID__
            float gap = 200.0f + (rand() % 50);
#else
            float gap = 150.0f + (rand() % 50);
#endif
            float minTop = 50.0f;
            float maxTop = SCREEN_HEIGHT - gap - 50.0f;
            float topH   = minTop + (rand() % (int)(maxTop - minTop));
            pillars.push_back({(float)SCREEN_WIDTH, 60.0f, topH, gap, false});
#ifdef __ANDROID__
            pillarSpawnTimer = 150;
#else
            pillarSpawnTimer = 100;
#endif
        }

        // Update pillars
        for (auto& p : pillars) {
            p.x -= pillarSpeed * WEB_SPEED_FACTOR;
            if (!p.passed && p.x + p.width < ball.x) {
                p.passed = true;
                score++;
            }
        }

        // Remove off-screen pillars
        pillars.erase(std::remove_if(pillars.begin(), pillars.end(), [](Pillar const& p) { return p.x + p.width < 0; }), pillars.end());

        // Collision detection
        if (ball.y - ball.radius < 0 || ball.y + ball.radius > SCREEN_HEIGHT) { gameOver = true; }

        for (auto const& p : pillars) {
            // Simplified AABB for ball vs pillar rectangles
            bool hitTop    = (ball.x + ball.radius > p.x && ball.x - ball.radius < p.x + p.width && ball.y - ball.radius < p.topHeight);
            bool hitBottom = (ball.x + ball.radius > p.x && ball.x - ball.radius < p.x + p.width && ball.y + ball.radius > p.topHeight + p.gap);
            if (hitTop || hitBottom) { gameOver = true; }
        }
    }

    // Rendering
    SDL_SetRenderDrawColor(renderer, 24, 24, 24, 255);
    SDL_RenderClear(renderer);

    // Draw pillars
    SDL_SetRenderDrawColor(renderer, 50, 200, 50, 255);
    for (auto const& p : pillars) {
        SDL_Rect topRect    = {(int)p.x, 0, (int)p.width, (int)p.topHeight};
        SDL_Rect bottomRect = {(int)p.x, (int)(p.topHeight + p.gap), (int)p.width, (int)(SCREEN_HEIGHT - (p.topHeight + p.gap))};
        SDL_RenderFillRect(renderer, &topRect);
        SDL_RenderFillRect(renderer, &bottomRect);
    }

    // Draw ball
    drawCircle(renderer, (int)ball.x, (int)ball.y, (int)ball.radius, ball.color);

    // Draw Score
    drawNumber(score, SCREEN_WIDTH / 2 - 20, 30, 20);

    if (gameOver) {
        // Visual feedback for game over (just a color filter or something simple
        // for now)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);
        SDL_Rect fullScreen = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &fullScreen);
    }

    SDL_RenderPresent(renderer);
}

int MAIN_FUNCTION(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    srand(static_cast<unsigned int>(time(NULL)));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
#ifdef __ANDROID__
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0) { // Index 0 for the first display
        SDL_Log("Could not get display mode! SDL_Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SCREEN_WIDTH  = displayMode.w;
    SCREEN_HEIGHT = displayMode.h;

    if (SCREEN_WIDTH < SCREEN_HEIGHT) { std::swap(SCREEN_WIDTH, SCREEN_HEIGHT); }

    SDL_Log("Device resolution: %d x %d", SCREEN_WIDTH, SCREEN_HEIGHT);

    flags |= SDL_WINDOW_FULLSCREEN;
#endif

    window = SDL_CreateWindow("Flappy Ball", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, flags);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

#ifndef TARGET_FPS
#define TARGET_FPS 60
#endif

const int FRAME_DELAY_MS = 1000 / TARGET_FPS;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(gameLoop, TARGET_FPS, 1);
#else
    while (!quit) {
        Uint32 frameStart = SDL_GetTicks();
        gameLoop();
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < (Uint32)FRAME_DELAY_MS) { SDL_Delay(FRAME_DELAY_MS - frameTime); }
    }
#endif

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
