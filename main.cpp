#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const int BALL_SIZE = 20;
const int BULLET_SIZE = 10;
const int ASTEROID_SIZE = 30;
float INITIAL_USER_SPEED = 120.0f;
const int NEW_ASTEROID_KILL_THRESHOLD = 10;
float BULLET_COOLDOWN = 0.8f;
const float FIXED_TIMESTEP = 1.0f / 60.0f;
bool isFiring = false;
int asteroidKillCount = 0;
bool gameOver = false;
int lives = 3;
float asteroidSpeedMultiplier = 1.0f;
int counter = 0;
const int HEART_WIDTH = 7;
const int HEART_HEIGHT = 6;
const int HEART_SPACING = 20;

// Input States
bool upPressed = false, downPressed = false, leftPressed = false, rightPressed = false, spacePressed = false;

// Player Propertiesd
float ballPosX = WINDOW_WIDTH / 2;
float ballPosY = WINDOW_HEIGHT / 2;
float ballVelX = 0, ballVelY = 0;

// Bullet and Asteroid Structures
struct Bullet {
    float x, y;
    float speed;
};

struct Asteroid {
    float x, y;
    float velX, velY;
    float size;
};

struct Explosion {
    float x, y;
    int currentFrame;
    float frameTime;  // Time elapsed for the current frame
};

std::vector<Explosion> explosions;
std::vector<Bullet> bullets;
std::vector<Asteroid> asteroids;

// Textures
SDL_Texture* userTexture = nullptr;
SDL_Texture* asteroidTexture = nullptr;
SDL_Texture* bulletTexture = nullptr;
SDL_Texture* heartTexture = nullptr;

// Function Declarations
bool checkCollisionAABB(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
void renderTexture(SDL_Renderer* renderer, SDL_Texture* texture, float x, float y, float width, float height, SDL_Rect srcRect);
void handleInput();
void movePlayer(float deltaTime);
void moveAsteroids(float deltaTime);
void moveBullets(float deltaTime);
void checkBulletCollisions();
void checkGameOver();
void spawnBullet();
void respawnAsteroid(Asteroid& asteroid);
void spawnAsteroids(int count);
void resetGame();

// Functions
bool checkCollisionAABB(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void renderTexture(SDL_Renderer* renderer, SDL_Texture* texture, float x, float y, float width, float height, SDL_Rect srcRect) {
    SDL_Rect dstRect = { static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height) };
    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
}

void spawnExplosion(float x, float y) {
    Explosion explosion = { x, y, 0, 0.0f };
    explosions.push_back(explosion);
}

void updateExplosions(float deltaTime) {
    for (auto it = explosions.begin(); it != explosions.end();) {
        it->frameTime += deltaTime;
        if (it->frameTime >= 0.1f) { // Assume each frame lasts 0.1 seconds
            it->frameTime = 0.0f;
            it->currentFrame++;
        }
        if (it->currentFrame >= 6) { // 6 frames in the explosion
            it = explosions.erase(it);  // Remove finished explosion
        } else {
            ++it;
        }
    }
}


void handleInput() {
    ballVelX = ballVelY = 0;
    if (upPressed) ballVelY -= INITIAL_USER_SPEED;
    if (downPressed) ballVelY += INITIAL_USER_SPEED;
    if (leftPressed) ballVelX -= INITIAL_USER_SPEED;
    if (rightPressed) ballVelX += INITIAL_USER_SPEED;
    static float lastBulletTime = 0;
    if (spacePressed && SDL_GetTicks() - lastBulletTime > BULLET_COOLDOWN * 1000) {
        spawnBullet();
        lastBulletTime = SDL_GetTicks();
        isFiring = true;
    }
    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_R]) {
        resetGame();
    }
}

void movePlayer(float deltaTime) {
    ballPosX += ballVelX * deltaTime;
    ballPosY += ballVelY * deltaTime;
    if (ballPosX < 0) ballPosX = 0;
    if (ballPosX + BALL_SIZE > WINDOW_WIDTH) ballPosX = WINDOW_WIDTH - BALL_SIZE;
    if (ballPosY < 0) ballPosY = 0;
    if (ballPosY + BALL_SIZE > WINDOW_HEIGHT) ballPosY = WINDOW_HEIGHT - BALL_SIZE;
}

void moveAsteroids(float deltaTime) {
    for (auto& asteroid : asteroids) {
        asteroid.x += asteroid.velX * asteroidSpeedMultiplier * deltaTime;
        asteroid.y += asteroid.velY * asteroidSpeedMultiplier * deltaTime;
        if (asteroid.x < 0 || asteroid.x + asteroid.size > WINDOW_WIDTH) asteroid.velX = -asteroid.velX;
        if (asteroid.y < 0 || asteroid.y + asteroid.size > WINDOW_HEIGHT) asteroid.velY = -asteroid.velY;
    }
}

void moveBullets(float deltaTime) {
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& bullet) {
            bullet.y += bullet.speed * deltaTime;
            return (bullet.y + BULLET_SIZE < 0); // Remove if off-screen
        }),
        bullets.end()
    );
}

void checkBulletCollisions() {
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& bullet) {
            for (auto& asteroid : asteroids) {
                if (checkCollisionAABB(bullet.x, bullet.y, BULLET_SIZE, BULLET_SIZE, asteroid.x, asteroid.y, asteroid.size, asteroid.size)) {
                    spawnExplosion(asteroid.x, asteroid.y);
                    respawnAsteroid(asteroid);
                    asteroidKillCount++;
                    if(asteroidKillCount == 100){
                        gameOver=true;
                    }
                    std::cout << "Asteroid destroyed! Kill count: " << asteroidKillCount << std::endl;

                    // Increase asteroid speed every 10 kills
                    if (asteroidKillCount % 10 == 0) {
                        asteroidSpeedMultiplier += 0.1f;
                        BULLET_COOLDOWN -= .05f;
                        INITIAL_USER_SPEED += 10.0f;

                        std::cout << "Asteroid speed increased! Multiplier: " << asteroidSpeedMultiplier << std::endl;
                    }

                    // Spawn a new asteroid every NEW_ASTEROID_KILL_THRESHOLD kills
                    if (asteroidKillCount % NEW_ASTEROID_KILL_THRESHOLD == 0) {
                        Asteroid newAsteroid;
                        respawnAsteroid(newAsteroid);
                        asteroids.push_back(newAsteroid);
                    }
                    return true; // Remove bullet
                }
            }
            return false; // Keep bullet
        }),
        bullets.end()
    );
}

void checkGameOver() {
    for (auto it = asteroids.begin(); it != asteroids.end(); ++it) {
        if (checkCollisionAABB(ballPosX, ballPosY, BALL_SIZE, BALL_SIZE, it->x, it->y, it->size, it->size)) {
            lives--;
            std::cout << "Collision detected! Lives left: " << lives << std::endl;

            spawnExplosion(it->x, it->y);

            // Remove the asteroid that hit the player
            it = asteroids.erase(it);
            Asteroid newAsteroid;
            respawnAsteroid(newAsteroid);
            asteroids.push_back(newAsteroid);

            if (lives <= 0) {
                gameOver = true;
                std::cout << "Game Over! Press 'R' to restart." << std::endl;
            }
            return;
        }
    }
}

void spawnBullet() {
    Bullet bullet;
    bullet.x = ballPosX + BALL_SIZE / 2 - BULLET_SIZE / 2;
    bullet.y = ballPosY;
    bullet.speed = -300.0f;
    bullets.push_back(bullet);
}

void respawnAsteroid(Asteroid& asteroid) {
    asteroid.x = rand() % (WINDOW_WIDTH - ASTEROID_SIZE);
    asteroid.y = rand() % (WINDOW_HEIGHT - ASTEROID_SIZE);
    asteroid.velX = rand() % 200 - 100;
    asteroid.velY = rand() % 200 - 100;
    while (asteroid.velX == 0 && asteroid.velY == 0) {
        asteroid.velX = rand() % 200 - 100;
        asteroid.velY = rand() % 200 - 100;
    }
    asteroid.size = ASTEROID_SIZE;
}

void spawnAsteroids(int count) {
    srand(static_cast<unsigned>(time(0)));
    for (int i = 0; i < count; ++i) {
        Asteroid asteroid;
        respawnAsteroid(asteroid);
        asteroids.push_back(asteroid);
    }
}

void resetGame() {
    ballPosX = WINDOW_WIDTH / 2;
    ballPosY = WINDOW_HEIGHT / 2;
    ballVelX = ballVelY = 0;
    bullets.clear();
    asteroids.clear();
    asteroidKillCount = 0;
    asteroidSpeedMultiplier = 1.0f;
    lives = 3;
    gameOver = false;
    spawnAsteroids(5);
}

SDL_Rect getShipSourceRect() {
    return SDL_Rect{ 7, 7, 18, 18 };  // Example for the ship's texture
}

SDL_Rect getBulletSourceRect() {
    return SDL_Rect{ 15, 43, 3, 6 };  // Example for the bullet's texture
}

SDL_Rect getMeteorSourceRect() {
    return SDL_Rect{ 41, 10, 13, 12 };  // Example for the meteor's texture
}

SDL_Rect getFiringSpriteSourceRect() {
    return SDL_Rect{ 39, 7, 18, 18 };  // Example firing sprite rectangle
}



SDL_Rect getExplosionFrame(int frame) {
    // Define frame-specific properties
    const std::vector<SDL_Rect> frames = {
        {73, 10, 13, 12},  // Frame 0
        {9, 42, 13, 12},   // Frame 1
        {41, 41, 15, 14},  // Frame 2
        {70, 40, 18, 18},  // Frame 3
        {8, 76, 13, 11},   // Frame 4
        {42, 77, 11, 9}    // Frame 5
    };

    // Ensure the frame is within range
    if (frame < 0 || frame >= static_cast<int>(frames.size())) {
        frame = 0; // Default to the first frame if out of range
    }
    return frames[frame];
}

void renderExplosions(SDL_Renderer* renderer, SDL_Texture* texture) {
    for (const auto& explosion : explosions) {
        SDL_Rect srcRect = getExplosionFrame(explosion.currentFrame);
        renderTexture(renderer, texture, explosion.x, explosion.y, ASTEROID_SIZE, ASTEROID_SIZE, srcRect);
    }
}

void renderLives(SDL_Renderer* renderer, SDL_Texture* heartTexture, int lives) {
    for (int i = 0; i < lives; i++) {
        int x = 10 + (i * (HEART_WIDTH + HEART_SPACING)); // Position hearts with spacing
        int y = WINDOW_HEIGHT - HEART_HEIGHT - 25; // Align hearts to the bottom-left corner
        SDL_Rect srcRect = { 12, 13, HEART_WIDTH, HEART_HEIGHT }; // Source rectangle for the heart sprite
        SDL_Rect dstRect = { x, y, HEART_WIDTH * 3, HEART_HEIGHT * 3 }; // Destination rectangle on the screen
        SDL_RenderCopy(renderer, heartTexture, &srcRect, &dstRect);
    }
}



int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Failed to initialize SDL_image: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Asteroid Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    userTexture = loadTexture(renderer, "ship.png");
    asteroidTexture = loadTexture(renderer, "meteor.png");
    bulletTexture = loadTexture(renderer, "ship.png");
    heartTexture = loadTexture(renderer, "hearts.png");

    spawnAsteroids(5);

    Uint32 lastTime = SDL_GetTicks();
    while (!gameOver) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                gameOver = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) upPressed = true;
                if (event.key.keysym.sym == SDLK_DOWN) downPressed = true;
                if (event.key.keysym.sym == SDLK_LEFT) leftPressed = true;
                if (event.key.keysym.sym == SDLK_RIGHT) rightPressed = true;
                if (event.key.keysym.sym == SDLK_SPACE) spacePressed = true;
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_UP) upPressed = false;
                if (event.key.keysym.sym == SDLK_DOWN) downPressed = false;
                if (event.key.keysym.sym == SDLK_LEFT) leftPressed = false;
                if (event.key.keysym.sym == SDLK_RIGHT) rightPressed = false;
                if (event.key.keysym.sym == SDLK_SPACE) spacePressed = false;
            }
        }

        handleInput();
        movePlayer(deltaTime);
        moveAsteroids(deltaTime);
        moveBullets(deltaTime);
        checkBulletCollisions();
        checkGameOver();

        SDL_RenderClear(renderer);

        updateExplosions(deltaTime);

        for (auto& asteroid : asteroids) {
            renderTexture(renderer, asteroidTexture, asteroid.x, asteroid.y, asteroid.size, asteroid.size, getMeteorSourceRect());
        }

        renderExplosions(renderer, asteroidTexture);

        for (auto& bullet : bullets) {
            renderTexture(renderer, bulletTexture, bullet.x, bullet.y, BULLET_SIZE, BULLET_SIZE, getBulletSourceRect());
        }

        SDL_Rect shipSrcRect = isFiring ? getFiringSpriteSourceRect() : getShipSourceRect();
        renderTexture(renderer, userTexture, ballPosX, ballPosY, BALL_SIZE, BALL_SIZE, shipSrcRect);
        isFiring = false;

        renderLives(renderer, heartTexture, lives);


        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
