#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define ENEMY_COUNT 5
#define SCALE 1.4f
#define LANE_COUNT 4
#define MIN_ENEMY_SPACING 250 
#define OVERTAKE_DISTANCE 200.0f 

#define ROAD_LEFT_EDGE 140
#define ROAD_RIGHT_EDGE 670

#define HIGHSCORE_FILE "highscore.txt"

typedef enum { 
    MENU, 
    GAMEPLAY, 
    GAMEOVER 
} GameState;

typedef struct Enemy {
    Vector2 pos;
    Texture2D sprite;
    float speed;
    int currentLane;
} Enemy;

typedef struct Fuel {
    Vector2 pos;
    bool active;
} Fuel;

int LoadHighScore() {
    FILE *file = fopen(HIGHSCORE_FILE, "r");
    int score = 0;
    if (file) {
        if (fscanf(file, "%d", &score) != 1) {
            score = 0;
        }
        fclose(file);
    }
    return score;
}

void SaveHighScore(int score) {
    FILE *file = fopen(HIGHSCORE_FILE, "w");
    if (file) {
        fprintf(file, "%d", score);
        fclose(file);
    }
}

bool DrawButton(Rectangle bounds, const char* text, Color baseColor) {
    Vector2 mousePoint = GetMousePosition();
    bool clicked = false;
    if (CheckCollisionPointRec(mousePoint, bounds)) {
        DrawRectangleRec(bounds, Fade(baseColor, 0.8f));
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) clicked = true;
    } else {
        DrawRectangleRec(bounds, baseColor);
    }
    DrawRectangleLinesEx(bounds, 3, BLACK);
    int textWidth = MeasureText(text, 30);
    DrawText(text, bounds.x + bounds.width/2 - textWidth/2, bounds.y + bounds.height/2 - 15, 30, BLACK);
    return clicked;
}

void DrawFuelGauge(int screenWidth, float currentFuel) {
    const int tankWidth = 30;
    const int tankHeight = 150;
    const int tankX = screenWidth - tankWidth - 20;
    const int tankY = 20;

    DrawRectangle(tankX, tankY, tankWidth, tankHeight, GRAY);
    DrawRectangleLinesEx((Rectangle){tankX, tankY, tankWidth, tankHeight}, 3, BLACK);

    float fuelHeight = (currentFuel / 100.0f) * tankHeight;
    float fuelY = tankY + (tankHeight - fuelHeight);

    DrawRectangle(tankX + 2, (int)fuelY + 2, tankWidth - 4, (int)fuelHeight - 4, GREEN);

    DrawText("FUEL", tankX - 5, tankY + tankHeight + 10, 20, BLACK);
}

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Car Collision Game");
    SetTargetFPS(60);

    Texture2D background = LoadTexture("assets/background.png");
    Texture2D mainCar    = LoadTexture("assets/main_car.png");
    Texture2D petrolTex  = LoadTexture("assets/petrol.png");

    Texture2D enemyTex[4] = {
        LoadTexture("assets/enemy1.png"),
        LoadTexture("assets/enemy2.png"),
        LoadTexture("assets/enemy3.png"),
        LoadTexture("assets/enemy4.png")
    };

    GameState currentState = MENU;
    
    int lanes[LANE_COUNT] = { 200, 300, 450, 540 };
    
    float carX = screenWidth / 2.0f - (mainCar.width * SCALE) / 2.0f;
    float carY = screenHeight - mainCar.height * SCALE - 20;
    
    float startSpeed = 5.0f;    
    float currentSpeed = startSpeed; 
    float maxSpeed = 15.0f;        
    
    float fuel = 100.0f;
    float baseFuelDrainRate = 0.1f; 
    int score = 0;
    float scoreTimer = 0;
    
    float scrollingBack = 0.0f;
    float bgScale = (float)screenWidth / background.width; 
    
    Enemy enemies[ENEMY_COUNT];
    Fuel fuelPickup = {0};
    fuelPickup.active = false;
    
    bool crashed = false;
    bool outOfFuel = false;
    
    int highScore = LoadHighScore();
    bool newHighScoreSet = false;

    for (int i = 0; i < ENEMY_COUNT; i++) enemies[i].pos.y = -1000;
    
    Color goldColor = {255, 203, 0, 255}; 

    while (!WindowShouldClose())
    {
        
        
        scrollingBack += currentSpeed; 
        if (scrollingBack >= background.height * bgScale) {
            scrollingBack = 0;
        }

        switch (currentState) 
        {
            case MENU:
                break;

            case GAMEPLAY:
            {
                
                if (currentSpeed < maxSpeed) {
                    currentSpeed += 0.003f; 
                }

                
                if (IsKeyDown(KEY_LEFT))  carX -= 6;
                if (IsKeyDown(KEY_RIGHT)) carX += 6;

                
                if (carX < ROAD_LEFT_EDGE || (carX + mainCar.width * SCALE) > ROAD_RIGHT_EDGE) {
                    crashed = true; 
                    currentState = GAMEOVER;
                }

                
                scoreTimer += GetFrameTime();
                if (scoreTimer >= 0.5f) {
                    score += 5; 
                    scoreTimer = 0;
                }

                
                fuel -= baseFuelDrainRate; 
                if (fuel <= 0) {
                    fuel = 0;
                    outOfFuel = true;
                    currentState = GAMEOVER;
                }

                
                for (int i = 0; i < ENEMY_COUNT; i++)
                {
                    enemies[i].pos.y += (currentSpeed - enemies[i].speed);

                    
                    if (enemies[i].pos.y > screenHeight) {
                        bool safeSpawn = false;
                        while (!safeSpawn) {
                            int lane = GetRandomValue(0, LANE_COUNT - 1);
                            float newY = GetRandomValue(-1000, -200);
                            safeSpawn = true;
                            for (int j = 0; j < ENEMY_COUNT; j++) {
                                if (j == i) continue;
                                if (lanes[lane] == (int)enemies[j].pos.x) {
                                    if (fabs(newY - enemies[j].pos.y) < MIN_ENEMY_SPACING) {
                                        safeSpawn = false;
                                        break;
                                    }
                                }
                            }
                            if (safeSpawn) {
                                enemies[i].currentLane = lane;
                                enemies[i].pos.x = lanes[lane];
                                enemies[i].pos.y = newY;
                                enemies[i].sprite = enemyTex[GetRandomValue(0, 3)];
                                enemies[i].speed = (float)GetRandomValue(2, 5); 
                            }
                        }
                    }
                }

                
                if (!fuelPickup.active) {
                    if (GetRandomValue(0, 50) == 1) {
                        bool safeFuelPos = false;
                        int attempts = 0;
                        while (!safeFuelPos && attempts < 10) {
                            safeFuelPos = true;
                            int lane = GetRandomValue(0, LANE_COUNT - 1);
                            float fuelY = GetRandomValue(-600, -200);
                            Rectangle proposedFuel = { lanes[lane], fuelY, petrolTex.width * SCALE, petrolTex.height * SCALE };
                            for (int i = 0; i < ENEMY_COUNT; i++) {
                                Rectangle enemyRect = { enemies[i].pos.x, enemies[i].pos.y, enemies[i].sprite.width * SCALE, enemies[i].sprite.height * SCALE };
                                if (CheckCollisionRecs(proposedFuel, enemyRect)) {
                                    safeFuelPos = false;
                                    break;
                                }
                            }
                            if (safeFuelPos) {
                                fuelPickup.pos.x = lanes[lane];
                                fuelPickup.pos.y = fuelY;
                                fuelPickup.active = true;
                            }
                            attempts++;
                        }
                    }
                } else {
                    fuelPickup.pos.y += currentSpeed;
                    if (fuelPickup.pos.y > screenHeight) fuelPickup.active = false;
                }

                
                Rectangle carRect = { carX, carY, mainCar.width * SCALE, mainCar.height * SCALE };
                for (int i = 0; i < ENEMY_COUNT; i++) {
                    Rectangle eRect = { enemies[i].pos.x + 5, enemies[i].pos.y + 5, (enemies[i].sprite.width * SCALE) - 10, (enemies[i].sprite.height * SCALE) - 10 };
                    if (CheckCollisionRecs(carRect, eRect)) {
                        crashed = true;
                        currentState = GAMEOVER;
                    }
                }
                if (fuelPickup.active) {
                    Rectangle fRect = { fuelPickup.pos.x, fuelPickup.pos.y, petrolTex.width * SCALE, petrolTex.height * SCALE };
                    if (CheckCollisionRecs(carRect, fRect)) {
                        fuel += 35;
                        if (fuel > 100) fuel = 100;
                        fuelPickup.active = false;
                    }
                }

            } break;

            case GAMEOVER:
            {
                
                if (score > highScore) {
                    highScore = score;
                    SaveHighScore(highScore);
                    newHighScoreSet = true;
                } else {
                    newHighScoreSet = false;
                }
            }
            
            break;
        }

        
        
        BeginDrawing();
        ClearBackground(RAYWHITE);

        
        DrawTextureEx(background, (Vector2){0, scrollingBack}, 0.0f, bgScale, WHITE);
        DrawTextureEx(background, (Vector2){0, scrollingBack - (background.height * bgScale)}, 0.0f, bgScale, WHITE);

        
        if (fuelPickup.active) DrawTextureEx(petrolTex, fuelPickup.pos, 0, SCALE, currentState == GAMEOVER ? Fade(WHITE, 0.5f) : WHITE);
        for (int i = 0; i < ENEMY_COUNT; i++) DrawTextureEx(enemies[i].sprite, enemies[i].pos, 0, SCALE, currentState == GAMEOVER ? Fade(WHITE, 0.5f) : WHITE);
        DrawTextureEx(mainCar, (Vector2){carX, carY}, 0, SCALE, currentState == GAMEOVER ? Fade(RED, 0.8f) : WHITE);


        switch (currentState) 
        {
            case MENU:
            {
                DrawText("CAR TRAFFIC", screenWidth/2 - MeasureText("CAR TRAFFIC", 60)/2 + 4, 104, 60, BLACK);
                DrawText("CAR TRAFFIC", screenWidth/2 - MeasureText("CAR TRAFFIC", 60)/2, 100, 60, ORANGE);
                DrawText("COLLISION GAME", screenWidth/2 - MeasureText("COLLISION GAME", 30)/2, 160, 30, WHITE);
                
                
                DrawText(TextFormat("High Score: %d", highScore), screenWidth/2 - MeasureText(TextFormat("High Score: %d", highScore), 20)/2, 210, 20, BLACK);

                Rectangle playBtn = { screenWidth/2 - 100, 250, 200, 80 };
                
                if (DrawButton(playBtn, "", goldColor)) {
                    
                    score = 0;
                    fuel = 100.0f;
                    currentSpeed = startSpeed; 
                    crashed = false;
                    outOfFuel = false;
                    carX = screenWidth / 2.0f - (mainCar.width * SCALE) / 2.0f;
                    fuelPickup.active = false;
                    newHighScoreSet = false;
                    
                    for (int i = 0; i < ENEMY_COUNT; i++) {
                        bool safePos = false;
                        while (!safePos) {
                            safePos = true;
                            int lane = GetRandomValue(0, LANE_COUNT - 1);
                            float startY = GetRandomValue(-1200, -100); 
                            for (int j = 0; j < i; j++) {
                                if (lanes[lane] == (int)enemies[j].pos.x) {
                                    if (fabs(startY - enemies[j].pos.y) < MIN_ENEMY_SPACING) {
                                        safePos = false;
                                        break;
                                    }
                                }
                            }
                            if (safePos) {
                                enemies[i].currentLane = lane;
                                enemies[i].pos.x = lanes[lane];
                                enemies[i].pos.y = startY;
                                enemies[i].sprite = enemyTex[GetRandomValue(0, 3)];
                                enemies[i].speed = (float)GetRandomValue(2, 4); 
                            }
                        }
                    }
                    currentState = GAMEPLAY;
                }
                
                Vector2 v1 = { screenWidth/2 - 10, 275 };
                Vector2 v2 = { screenWidth/2 - 10, 305 };
                Vector2 v3 = { screenWidth/2 + 20, 290 };
                DrawTriangle(v1, v2, v3, BLACK);
            } break;

            case GAMEPLAY:
            {
                
                DrawText(TextFormat("Score: %d", score), 20, 20, 25, BLACK);
                DrawText(TextFormat("High Score: %d", highScore), 20, 55, 20, DARKBLUE);
                
                
                DrawFuelGauge(screenWidth, fuel);
                
            } break;

            case GAMEOVER:
            {
                
                DrawRectangle(screenWidth/2 - 200, screenHeight/2 - 150, 400, 300, Fade(BLACK, 0.8f));

                if (crashed) {
                    DrawText("CRASHED!", screenWidth/2 - MeasureText("CRASHED!", 40)/2, 120, 40, RED);
                }
                else if (outOfFuel) {
                    DrawText("OUT OF FUEL!", screenWidth/2 - MeasureText("OUT OF FUEL!", 40)/2, 120, 40, ORANGE);
                }
                
                
                DrawText(TextFormat("Final Score: %d", score), screenWidth/2 - MeasureText(TextFormat("Final Score: %d", score), 30)/2, 180, 30, WHITE);
                
                
                if (newHighScoreSet) {
                    DrawText("New High Score!", screenWidth/2 - MeasureText("New High Score!", 25)/2, 220, 25, goldColor);
                }
                
                DrawText(TextFormat("High Score: %d", highScore), screenWidth/2 - MeasureText(TextFormat("High Score: %d", highScore), 20)/2, 250, 20, LIGHTGRAY);


                Rectangle restartBtn = { screenWidth/2 - 80, 290, 160, 60 };
                if (DrawButton(restartBtn, "RESTART", goldColor)) {
                    
                    score = 0;
                    fuel = 100.0f;
                    currentSpeed = startSpeed; 
                    crashed = false;
                    outOfFuel = false;
                    carX = screenWidth / 2.0f - (mainCar.width * SCALE) / 2.0f;
                    fuelPickup.active = false;
                    newHighScoreSet = false;

                    for (int i = 0; i < ENEMY_COUNT; i++) {
                        bool safePos = false;
                        while (!safePos) {
                            safePos = true;
                            int lane = GetRandomValue(0, LANE_COUNT - 1);
                            float startY = GetRandomValue(-1200, -100); 
                            for (int j = 0; j < i; j++) {
                                if (lanes[lane] == (int)enemies[j].pos.x) {
                                    if (fabs(startY - enemies[j].pos.y) < MIN_ENEMY_SPACING) {
                                        safePos = false;
                                        break;
                                    }
                                }
                            }
                            if (safePos) {
                                enemies[i].currentLane = lane;
                                enemies[i].pos.x = lanes[lane];
                                enemies[i].pos.y = startY;
                                enemies[i].sprite = enemyTex[GetRandomValue(0, 3)];
                                enemies[i].speed = (float)GetRandomValue(2, 4); 
                            }
                        }
                    }
                    currentState = GAMEPLAY;
                }
            } break;
        }

        EndDrawing();
    }

    UnloadTexture(background);
    UnloadTexture(mainCar);
    UnloadTexture(petrolTex);
    for(int i=0; i<4; i++) UnloadTexture(enemyTex[i]);

    CloseWindow();
    return 0;
}
