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
const int BULLET_SIZE = 5;
const int INITIAL_USER_SPEED = 5;
const int BULLET_SPEED = 10;
bool gameOver = false;

int ballPosX = WINDOW_WIDTH / 2;
int ballPosY = WINDOW_HEIGHT / 2;
int ballVelX = 0, ballVelY = 0;

bool upPressed = false;
bool downPressed = false;
bool leftPressed = false;
bool rightPressed = false;
bool spacePressed = false;

struct Bullet {
    int x, y;
    int velX, velY;
};

std::vector<Bullet> bullets;

struct Asteroid {
    int x, y;
    int velX, velY;
    int size;
};

std::vector<Asteroid> asteroids;

bool checkCollision(int x1, int y1, int size1, int x2, int y2, int size2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    float distance = sqrt(dx * dx + dy * dy);
    return distance < (size1 / 2 + size2 / 2);
}

void restartGame() {
    // Reset player position and velocity
    ballPosX = WINDOW_WIDTH / 2;
    ballPosY = WINDOW_HEIGHT / 2;
    ballVelX = 0;
    ballVelY = 0;

    // Reinitialize asteroids (random positions and velocities)
    srand(static_cast<unsigned int>(time(0))); // Reseed for new positions
    asteroids.clear();
    for (int i = 0; i < 5; ++i) {
        asteroids.push_back({rand() % WINDOW_WIDTH, rand() % WINDOW_HEIGHT, rand() % 3 - 1, rand() % 3 - 1, 30});
    }

    // Reset bullets
    bullets.clear();

    gameOver = false;
    std::cout << "Game Restarted!" << std::endl;
}

void fireBullet() {
    Bullet newBullet;
    newBullet.x = ballPosX + BALL_SIZE / 2 - BULLET_SIZE / 2; // Spawn at the center of the player
    newBullet.y = ballPosY;
    newBullet.velX = 0;
    newBullet.velY = BULLET_SPEED;

    // If moving up or down, fire accordingly
    if (upPressed) {
        newBullet.velY = -BULLET_SPEED;
    } else if (downPressed) {
        newBullet.velY = BULLET_SPEED;
    }

    bullets.push_back(newBullet);
}

void moveAsteroids() {
    for (auto& asteroid : asteroids) {
        asteroid.x += asteroid.velX;
        asteroid.y += asteroid.velY;

        // Wrap asteroids around screen edges
        if (asteroid.x < 0) asteroid.x = WINDOW_WIDTH;
        if (asteroid.x > WINDOW_WIDTH) asteroid.x = 0;
        if (asteroid.y < 0) asteroid.y = WINDOW_HEIGHT;
        if (asteroid.y > WINDOW_HEIGHT) asteroid.y = 0;
    }
}

void checkGameOver() {
    // Check for collision between the player and asteroids
    for (const auto& asteroid : asteroids) {
        if (checkCollision(ballPosX, ballPosY, BALL_SIZE, asteroid.x, asteroid.y, asteroid.size)) {
            gameOver = true;
            std::cout << "Game Over!" << std::endl;
        }
    }
}

void checkBulletCollisions() {
    for (size_t i = 0; i < bullets.size(); ++i) {
        for (size_t j = 0; j < asteroids.size(); ++j) {
            if (checkCollision(bullets[i].x, bullets[i].y, BULLET_SIZE, asteroids[j].x, asteroids[j].y, asteroids[j].size)) {
                // Remove asteroid and bullet if collision occurs
                asteroids.erase(asteroids.begin() + j);
                bullets.erase(bullets.begin() + i);
                i--;  // Adjust index due to bullet removal
                break; // Exit loop after one collision is handled
            }
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Space Ship with Asteroids", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cout << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW!" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    bool quit = false;
    SDL_Event e;

    srand(static_cast<unsigned int>(time(0)));

    // Initialize asteroids
    for (int i = 0; i < 5; ++i) {
        asteroids.push_back({rand() % WINDOW_WIDTH, rand() % WINDOW_HEIGHT, rand() % 3 - 1, rand() % 3 - 1, 30});
    }

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:    upPressed = true; break;
                    case SDLK_DOWN:  downPressed = true; break;
                    case SDLK_LEFT:  leftPressed = true; break;
                    case SDLK_RIGHT: rightPressed = true; break;
                    case SDLK_SPACE: spacePressed = true; break;
                    case SDLK_r:     restartGame(); break; // Restart game
                }
            }
            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:    upPressed = false; break;
                    case SDLK_DOWN:  downPressed = false; break;
                    case SDLK_LEFT:  leftPressed = false; break;
                    case SDLK_RIGHT: rightPressed = false; break;
                    case SDLK_SPACE: spacePressed = false; break;
                }
            }
        }

        // Ball movement logic
        if (gameOver) {
            continue;  // If the game is over, skip the movement logic
        }

        ballVelY = 0;
        ballVelX = 0;
        if (upPressed && downPressed) {
            ballVelY = 0;
        } else if (upPressed) {
            ballVelY = INITIAL_USER_SPEED;
        } else if (downPressed) {
            ballVelY = -INITIAL_USER_SPEED;
        }
        if (leftPressed && rightPressed) {
            ballVelX = 0;
        } else if (leftPressed) {
            ballVelX = -INITIAL_USER_SPEED;
        } else if (rightPressed) {
            ballVelX = INITIAL_USER_SPEED;
        }

        ballPosX += ballVelX;
        ballPosY += ballVelY;

        if (ballPosX < 0) ballPosX = 0;
        if (ballPosX + BALL_SIZE > WINDOW_WIDTH) ballPosX = WINDOW_WIDTH - BALL_SIZE;
        if (ballPosY < 0) ballPosY = 0;
        if (ballPosY + BALL_SIZE > WINDOW_HEIGHT) ballPosY = WINDOW_HEIGHT - BALL_SIZE;

        // Fire bullets if space bar is pressed
        if (spacePressed) {
            fireBullet();
        }

        // Update bullets
        for (size_t i = 0; i < bullets.size(); ++i) {
            bullets[i].y += bullets[i].velY;

            // Remove bullets that go off the screen
            if (bullets[i].y < 0 || bullets[i].y > WINDOW_HEIGHT) {
                bullets.erase(bullets.begin() + i);
                i--;
            }
        }

        // Move asteroids
        moveAsteroids();

        // Check for bullet-asteroid collisions
        checkBulletCollisions();

        // Check if the player has collided with an asteroid
        checkGameOver();

        // Render everything
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Rendering player (ship)
        float normalizedX = (ballPosX / (float)WINDOW_WIDTH) * 2 - 1;
        float normalizedY = (ballPosY / (float)WINDOW_HEIGHT) * 2 - 1;
        float normalizedSize = (BALL_SIZE / (float)WINDOW_WIDTH) * 2;

        glBegin(GL_QUADS);
        glVertex2f(normalizedX - normalizedSize, normalizedY - normalizedSize);
        glVertex2f(normalizedX + normalizedSize, normalizedY - normalizedSize);
        glVertex2f(normalizedX + normalizedSize, normalizedY + normalizedSize);
        glVertex2f(normalizedX - normalizedSize, normalizedY + normalizedSize);
        glEnd();

        // Render bullets
        for (const auto& bullet : bullets) {
            float bulletX = (bullet.x / (float)WINDOW_WIDTH) * 2 - 1;
            float bulletY = (bullet.y / (float)WINDOW_HEIGHT) * 2 - 1;
            float bulletSize = (BULLET_SIZE / (float)WINDOW_WIDTH) * 2;

            glBegin(GL_QUADS);
            glVertex2f(bulletX - bulletSize, bulletY - bulletSize);
            glVertex2f(bulletX + bulletSize, bulletY - bulletSize);
            glVertex2f(bulletX + bulletSize, bulletY + bulletSize);
            glVertex2f(bulletX - bulletSize, bulletY + bulletSize);
            glEnd();
        }

        // Render asteroids
        for (const auto& asteroid : asteroids) {
            float asteroidX = (asteroid.x / (float)WINDOW_WIDTH) * 2 - 1;
            float asteroidY = (asteroid.y / (float)WINDOW_HEIGHT) * 2 - 1;
            float asteroidSize = (asteroid.size / (float)WINDOW_WIDTH) * 2;

            glBegin(GL_QUADS);
            glVertex2f(asteroidX - asteroidSize, asteroidY - asteroidSize);
            glVertex2f(asteroidX + asteroidSize, asteroidY - asteroidSize);
            glVertex2f(asteroidX + asteroidSize, asteroidY + asteroidSize);
            glVertex2f(asteroidX - asteroidSize, asteroidY + asteroidSize);
            glEnd();
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
