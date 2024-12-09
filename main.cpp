#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glew.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const int BALL_SIZE = 20;
const int BULLET_SIZE = 10;
const int ASTEROID_SIZE = 30;
const float INITIAL_USER_SPEED = 5.0f;
int asteroidKillCount = 0;
int targetAsteroidCount = 5;
const int NEW_ASTEROID_KILL_THRESHOLD = 10;
const float MAX_ASTEROID_SPEED = 10.0f; // Maximum speed for asteroids
const float SPEED_INCREMENT = 0.5f;    // Speed increment on collision

struct Bullet {
    float x, y;
    float speed;
};

struct Asteroid {
    float x, y;
    float velX, velY;
    float size;
};

bool upPressed = false, downPressed = false, leftPressed = false, rightPressed = false, spacePressed = false;

float ballPosX = WINDOW_WIDTH / 2;
float ballPosY = WINDOW_HEIGHT / 2;
float ballVelX = 0, ballVelY = 0;

std::vector<Bullet> bullets;
std::vector<Asteroid> asteroids;

bool gameOver = false;

// Axis-Aligned Bounding Box Collision Detection
bool checkCollisionAABB(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void handleInput() {
    ballVelX = ballVelY = 0;

    if (upPressed) ballVelY = INITIAL_USER_SPEED;
    if (downPressed) ballVelY = -INITIAL_USER_SPEED;
    if (leftPressed) ballVelX = -INITIAL_USER_SPEED;
    if (rightPressed) ballVelX = INITIAL_USER_SPEED;
}

void movePlayer() {
    ballPosX += ballVelX;
    ballPosY += ballVelY;
    if (ballPosX < 0) ballPosX = 0;
    if (ballPosX + BALL_SIZE > WINDOW_WIDTH) ballPosX = WINDOW_WIDTH - BALL_SIZE;
    if (ballPosY < 0) ballPosY = 0;
    if (ballPosY + BALL_SIZE > WINDOW_HEIGHT) ballPosY = WINDOW_HEIGHT - BALL_SIZE;
}

void moveAsteroids() {
    for (auto& asteroid : asteroids) {
        asteroid.x += asteroid.velX;
        asteroid.y += asteroid.velY;
        if (asteroid.x < 0 || asteroid.x + asteroid.size > WINDOW_WIDTH) asteroid.velX = -asteroid.velX;
        if (asteroid.y < 0 || asteroid.y + asteroid.size > WINDOW_HEIGHT) asteroid.velY = -asteroid.velY;
    }
}

void moveBullets() {
    for (size_t i = 0; i < bullets.size(); ++i) {
        bullets[i].y += bullets[i].speed;
        if (bullets[i].y + BULLET_SIZE < 0) {
            bullets.erase(bullets.begin() + i);
            i--;
        }
    }
}

void checkGameOver() {
    for (const auto& asteroid : asteroids) {
        if (checkCollisionAABB(ballPosX, ballPosY, BALL_SIZE, BALL_SIZE, asteroid.x, asteroid.y, asteroid.size, asteroid.size)) {
            gameOver = true;
            std::cout << "Game Over!" << std::endl;
            return;
        }
    }
}

void respawnAsteroid(Asteroid& asteroid) {
    // Spawn on a random edge of the window
    int edge = rand() % 4; // 0: Top, 1: Bottom, 2: Left, 3: Right
    float x, y;

    switch (edge) {
        case 0: // Top edge
            x = rand() % (WINDOW_WIDTH - ASTEROID_SIZE);
            y = 0;
            break;
        case 1: // Bottom edge
            x = rand() % (WINDOW_WIDTH - ASTEROID_SIZE);
            y = WINDOW_HEIGHT - ASTEROID_SIZE;
            break;
        case 2: // Left edge
            x = 0;
            y = rand() % (WINDOW_HEIGHT - ASTEROID_SIZE);
            break;
        case 3: // Right edge
            x = WINDOW_WIDTH - ASTEROID_SIZE;
            y = rand() % (WINDOW_HEIGHT - ASTEROID_SIZE);
            break;
    }

    asteroid.x = x;
    asteroid.y = y;

    // Calculate velocity towards the center of the screen
    float centerX = WINDOW_WIDTH / 2.0f;
    float centerY = WINDOW_HEIGHT / 2.0f;
    float dx = centerX - (asteroid.x + ASTEROID_SIZE / 2);
    float dy = centerY - (asteroid.y + ASTEROID_SIZE / 2);
    float magnitude = sqrt(dx * dx + dy * dy);

    asteroid.velX = (dx / magnitude) * (rand() % 3 + 1); // Scale speed randomly
    asteroid.velY = (dy / magnitude) * (rand() % 3 + 1);
    asteroid.size = ASTEROID_SIZE;
}

void addAsteroid(float x, float y, float velX, float velY, float size) {
    Asteroid newAsteroid;
    newAsteroid.x = x;
    newAsteroid.y = y;
    newAsteroid.velX = velX;
    newAsteroid.velY = velY;
    newAsteroid.size = size;
    asteroids.push_back(newAsteroid);
}

void handleAsteroidCollisions() {
    const float SPEED_THRESHOLD = 5.0f; // Minimum speed difference to trigger splitting
    const float MIN_SIZE = 10.0f;      // Minimum size for split asteroids

    for (size_t i = 0; i < asteroids.size(); ++i) {
        for (size_t j = i + 1; j < asteroids.size(); ++j) {
            Asteroid& asteroid1 = asteroids[i];
            Asteroid& asteroid2 = asteroids[j];

            // Check for collision
            if (checkCollisionAABB(asteroid1.x, asteroid1.y, asteroid1.size, asteroid1.size,
                                   asteroid2.x, asteroid2.y, asteroid2.size, asteroid2.size)) {
                float speed1 = std::sqrt(asteroid1.velX * asteroid1.velX + asteroid1.velY * asteroid1.velY);
                float speed2 = std::sqrt(asteroid2.velX * asteroid2.velX + asteroid2.velY * asteroid2.velY);

                // Determine which asteroid is faster
                Asteroid& faster = (speed1 > speed2) ? asteroid1 : asteroid2;
                Asteroid& slower = (speed1 > speed2) ? asteroid2 : asteroid1;

                // Split slower asteroid if size is large enough
                if (slower.size > MIN_SIZE && std::abs(speed1 - speed2) > SPEED_THRESHOLD) {
                    float splitSize = slower.size / 2.0f;
                    float splitSpeedIncrease = 2.0f; // Increase speed for split asteroids

                    // Remove slower asteroid
                    asteroids.erase(asteroids.begin() + (speed1 > speed2 ? j : i));
                    if (speed1 > speed2) j--; // Adjust index after erasing

                    // Add two smaller asteroids
                    addAsteroid(slower.x, slower.y, -slower.velX * splitSpeedIncrease, -slower.velY * splitSpeedIncrease, splitSize);
                    addAsteroid(slower.x, slower.y, slower.velX * splitSpeedIncrease, slower.velY * splitSpeedIncrease, splitSize);
                    continue;
                }

                // Regular bounce: Swap velocities for non-splitting collision
                std::swap(asteroid1.velX, asteroid2.velX);
                std::swap(asteroid1.velY, asteroid2.velY);
            }
        }
    }
}

void checkBulletCollisions() {
    for (size_t i = 0; i < bullets.size(); ++i) {
        for (size_t j = 0; j < asteroids.size(); ++j) {
            if (checkCollisionAABB(bullets[i].x, bullets[i].y, BULLET_SIZE, BULLET_SIZE, asteroids[j].x, asteroids[j].y, asteroids[j].size, asteroids[j].size)) {
                // Remove the bullet
                bullets.erase(bullets.begin() + i);
                i--;  // Adjust index due to bullet removal

                // Respawn the asteroid
                respawnAsteroid(asteroids[j]);

                // Increment kill count
                asteroidKillCount++;
                std::cout << "Asteroid destroyed! Kill count: " << asteroidKillCount << std::endl;

                // Spawn a new asteroid every 10 kills
                if (asteroidKillCount % NEW_ASTEROID_KILL_THRESHOLD == 0) {
                    Asteroid newAsteroid;
                    respawnAsteroid(newAsteroid);
                    asteroids.push_back(newAsteroid);
                    std::cout << "New asteroid spawned! Total: " << asteroids.size() << std::endl;
                }

                break;  // Exit inner loop after handling collision
            }
        }
    }
}

void spawnBullet() {
    Bullet bullet;
    bullet.x = ballPosX + BALL_SIZE / 2 - BULLET_SIZE / 2;
    bullet.y = ballPosY;
    bullet.speed = 10.0f;
    bullets.push_back(bullet);
}

void spawnAsteroids(int count) {
    srand(static_cast<unsigned>(time(0)));
    for (int i = 0; i < count; ++i) {
        Asteroid asteroid;
        asteroid.x = rand() % WINDOW_WIDTH;
        asteroid.y = rand() % WINDOW_HEIGHT;
        asteroid.velX = (rand() % 2 == 0 ? 1 : -1) * (rand() % 3 + 1);
        asteroid.velY = (rand() % 2 == 0 ? 1 : -1) * (rand() % 3 + 1);
        asteroid.size = ASTEROID_SIZE;
        asteroids.push_back(asteroid);
    }
}


void renderRectangle(float x, float y, float width, float height, float r, float g, float b) {
    float normalizedX = (x / WINDOW_WIDTH) * 2 - 1;
    float normalizedY = (y / WINDOW_HEIGHT) * 2 - 1;
    float normalizedWidth = (width / WINDOW_WIDTH) * 2;
    float normalizedHeight = (height / WINDOW_HEIGHT) * 2;

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(normalizedX, normalizedY);
    glVertex2f(normalizedX + normalizedWidth, normalizedY);
    glVertex2f(normalizedX + normalizedWidth, normalizedY + normalizedHeight);
    glVertex2f(normalizedX, normalizedY + normalizedHeight);
    glEnd();
}

void resetGame() {
    // Reset player position
    ballPosX = WINDOW_WIDTH / 2;
    ballPosY = WINDOW_HEIGHT / 2;
    ballVelX = 0;
    ballVelY = 0;

    // Clear bullets and asteroids
    bullets.clear();
    asteroids.clear();

    // Reset the game state
    gameOver = false;

    // Reset the kill count
    asteroidKillCount = 0;
    targetAsteroidCount = 5; // Reset the target asteroid count to initial value

    // Respawn asteroids
    spawnAsteroids(targetAsteroidCount);

    std::cout << "Game reset!" << std::endl;
}


int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Space Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed!" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Initialize the game
    resetGame();

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: upPressed = true; break;
                    case SDLK_DOWN: downPressed = true; break;
                    case SDLK_LEFT: leftPressed = true; break;
                    case SDLK_RIGHT: rightPressed = true; break;
                    case SDLK_SPACE: if (!spacePressed) {
                        spawnBullet(); 
                        spacePressed = true;
                        }
                        break;
                    case SDLK_r: resetGame(); break; // Reset game on 'R'
                }
            }

            if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: upPressed = false; break;
                    case SDLK_DOWN: downPressed = false; break;
                    case SDLK_LEFT: leftPressed = false; break;
                    case SDLK_RIGHT: rightPressed = false; break;
                    case SDLK_SPACE: spacePressed = false; break;
                }
            }
        }

        if (!gameOver) {
            handleInput();
            movePlayer();
            moveAsteroids();
            moveBullets();
            checkGameOver();
            checkBulletCollisions();
            handleAsteroidCollisions();
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderRectangle(ballPosX, ballPosY, BALL_SIZE, BALL_SIZE, 1.0f, 0.0f, 0.0f); // Player
        for (const auto& asteroid : asteroids)
            renderRectangle(asteroid.x, asteroid.y, asteroid.size, asteroid.size, 0.0f, 1.0f, 0.0f); // Asteroids
        for (const auto& bullet : bullets)
            renderRectangle(bullet.x, bullet.y, BULLET_SIZE, BULLET_SIZE, 1.0f, 1.0f, 0.0f); // Bullets

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
