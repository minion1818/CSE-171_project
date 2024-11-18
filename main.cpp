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

int ballPosX = WINDOW_WIDTH / 2;
int ballPosY = WINDOW_HEIGHT / 2;
int ballVelX = 0, ballVelY = 0;
const int BALL_SIZE = 20;
const int INITIAL_USER_SPEED = 5;

struct Circle {
    int posX;
    int posY;
    int velX;
    int velY;
    const int size = 30;
    float speed;
};

bool upPressed = false;
bool downPressed = false;
bool leftPressed = false;
bool rightPressed = false;

bool checkCollision(int x1, int y1, int size1, int x2, int y2, int size2) {
    int radius1 = size1 / 2;
    int radius2 = size2 / 2;
    int dx = x1 - x2;
    int dy = y1 - y2;
    int distanceSquared = dx * dx + dy * dy;
    return distanceSquared < (radius1 + radius2) * (radius1 + radius2);
}

void handleCircleCollision(Circle& c1, Circle& c2) {
    int dx = c1.posX - c2.posX;
    int dy = c1.posY - c2.posY;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance < c1.size / 2 + c2.size / 2) {
        c1.velX = -c1.velX;
        c1.velY = -c1.velY;
        c2.velX = -c2.velX;
        c2.velY = -c2.velY;

        float overlap = (c1.size / 2 + c2.size / 2) - distance;
        c1.posX += (dx / distance) * overlap / 2;
        c1.posY += (dy / distance) * overlap / 2;
        c2.posX -= (dx / distance) * overlap / 2;
        c2.posY -= (dy / distance) * overlap / 2;
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

    std::vector<Circle> circles;

    for (int i = 0; i < 5; ++i) {
        Circle circle;
        circle.posX = rand() % WINDOW_WIDTH;
        circle.posY = rand() % WINDOW_HEIGHT;
        circle.velX = (rand() % 2 == 0 ? 1 : -1) * (rand() % 2 + 1);
        circle.velY = (rand() % 2 == 0 ? 1 : -1) * (rand() % 2 + 1);
        circle.speed = 0.5f;
        circles.push_back(circle);
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
                }
            }
            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:    upPressed = false; break;
                    case SDLK_DOWN:  downPressed = false; break;
                    case SDLK_LEFT:  leftPressed = false; break;
                    case SDLK_RIGHT: rightPressed = false; break;
                }
            }
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

        for (auto& circle : circles) {
            circle.speed += 0.005f;
            circle.posX += circle.velX * circle.speed;
            circle.posY += circle.velY * circle.speed;
            if (circle.posX < 0 || circle.posX + circle.size > WINDOW_WIDTH) {
                circle.velX = -circle.velX;
            }
            if (circle.posY < 0 || circle.posY + circle.size > WINDOW_HEIGHT) {
                circle.velY = -circle.velY;
            }

            if (checkCollision(ballPosX, ballPosY, BALL_SIZE, circle.posX, circle.posY, circle.size)) {
                std::cout << "Game Over! You were hit by an asteroid!" << std::endl;
                quit = true;
            }

            for (auto& otherCircle : circles) {
                if (&circle != &otherCircle) {
                    handleCircleCollision(circle, otherCircle);
                }
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f((ballPosX / (float)WINDOW_WIDTH) * 2 - 1, (ballPosY / (float)WINDOW_HEIGHT) * 2 - 1);
        glVertex2f(((ballPosX + BALL_SIZE) / (float)WINDOW_WIDTH) * 2 - 1, (ballPosY / (float)WINDOW_HEIGHT) * 2 - 1);
        glVertex2f(((ballPosX + BALL_SIZE) / (float)WINDOW_WIDTH) * 2 - 1, ((ballPosY + BALL_SIZE) / (float)WINDOW_HEIGHT) * 2 - 1);
        glVertex2f((ballPosX / (float)WINDOW_WIDTH) * 2 - 1, ((ballPosY + BALL_SIZE) / (float)WINDOW_HEIGHT) * 2 - 1);
        glEnd();

        glColor3f(0.0f, 1.0f, 0.0f);
        for (const auto& circle : circles) {
            glBegin(GL_QUADS);
            glVertex2f((circle.posX / (float)WINDOW_WIDTH) * 2 - 1, (circle.posY / (float)WINDOW_HEIGHT) * 2 - 1);
            glVertex2f(((circle.posX + circle.size) / (float)WINDOW_WIDTH) * 2 - 1, (circle.posY / (float)WINDOW_HEIGHT) * 2 - 1);
            glVertex2f(((circle.posX + circle.size) / (float)WINDOW_WIDTH) * 2 - 1, ((circle.posY + circle.size) / (float)WINDOW_HEIGHT) * 2 - 1);
            glVertex2f((circle.posX / (float)WINDOW_WIDTH) * 2 - 1, ((circle.posY + circle.size) / (float)WINDOW_HEIGHT) * 2 - 1);
            glEnd();
        }

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
