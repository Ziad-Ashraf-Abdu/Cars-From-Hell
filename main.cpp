#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h> // Corrected header
#include <math.h>

#define WIDTH 1600
#define HEIGHT 800
#define PI 3.14159265
#define MAX_OBJECTS 150
#define MAX_PARTICLES 500
#define MAX_POWERUPS 15
#define FRICTION 0.94f
#define ACCELERATION 0.042f
#define MAX_SPEED 0.68f
#define BRAKE_POWER 0.88f
#define TURN_SPEED 3.8f
#define MAX_LEVEL 16 // NEW: Max level is 16 (secret)

// NEW: Struct for Level 15 (Dino)
typedef struct {
    float x, y, z, vx, vz, angle, size;
    float anim_timer;
    bool active;
    int state; // 0 = idle, 1 = chasing
    float health;
} Dinosaur;

// NEW: 100 DINOS
#define MAX_DINOS 100
Dinosaur dinos[MAX_DINOS];

// NEW: Struct for Flying Dinos
typedef struct {
    float x, y, z, vx, vz, angle, size;
    float anim_timer;
    bool active;
    int state; // 0 = hovering, 1 = diving
    float health;
} FlyingDinosaur;

// NEW: 25 FLYING DINOS
#define MAX_FLYING_DINOS 25
FlyingDinosaur flyingDinos[MAX_FLYING_DINOS];

// NEW: Struct for Level 16 (Alien Ships)
typedef struct {
    float x, y, z, vx, vz, angle, size;
    float anim_timer;
    bool active;
    int state; // 0 = hunting, 1 = attacking
    float health;
    float attack_timer;
    int type; // 0 = laser, 1 = bomber
} AlienShip;

// NEW: 12 ALIEN SHIPS
#define MAX_ALIEN_SHIPS 12
AlienShip alienShips[MAX_ALIEN_SHIPS];


typedef struct {
    float x, y, z, vx, vy, vz, life, r, g, b, size;
    int type;
} Particle;

typedef struct {
    float x, y, z, angle, radius, vx, vy, vz, rotY, timer, targetX, targetZ, health;
    int type;
    bool active, revealed, isAngry;
    // NEW: Teleport ability
    int teleportCharges;
    float teleportCooldown; // NEW: Cooldown
} GameObject;

// NEW: Level 15 targets
GameObject targets[3];
int targetsRemaining = 0;

typedef struct {
    float x, y, z, type, timer;
    bool active;
} PowerUp;

typedef struct {
    float x, y, z, angle, vx, vz, speed, health, damage_flash;
    float drift_angle, boost_timer, shield_timer, invuln_timer, combo, lastHitTime;
    bool drifting, superMode, invincible;
    int kills, perfectDodges;
    // Buff/Debuff Timers
    float reverseGravityTimer;
    float confusionTimer;
    float ghostTimer;
    float timeRiftTimer;
    float empTimer;
    float oilSlickTimer;
    // Ability Cooldowns
    float shield_cooldown;
    float shockwave_cooldown;
    float fly_timer;
    float fly_cooldown;

    // NEW: Upgrade Stats
    float healthBonus;
    float accelerationBonus;
    float turnSpeedBonus;
    float shieldCooldownReduction;
    float flyCooldownReduction;
} Car;

Car car;
GameObject target, objects[MAX_OBJECTS];
Particle particles[MAX_PARTICLES];
PowerUp powerups[MAX_POWERUPS];
int numObjects = 80, particleCount = 0, messageTimer = 0, score = 0, hits = 0;
int currentLevel = 1;
int difficulty;
// NEW: Added UPGRADE_SHOP state
enum { PLAYING, WIN, LOSE, FINAL_WIN, SECRET_WIN, UPGRADE_SHOP } gameState = PLAYING;
float gameTime = 0, anim = 0, cameraShake = 0, screenFlash = 0, lightningTimer = 0;
float arenaRotation = 0, timeWarpEffect = 1.0f, weatherIntensity = 0;
float arenaSize = 80.0f;
float penaltyTimer = 45.0f;
bool keyStates[256] = {false};
bool nightMode = false, raining = false, earthquake = false;
int comboCounter = 0, maxCombo = 0;
float meteorTimer = 0.0f;

// NEW: Level 16 (Secret) variables
float survivalTimer = 90.0f; // 90 seconds
float alienSpawnTimer = 0.0f;

// NEW: Upgrade costs
int costHealth = 2000;
int costAccel = 3000;
int costTurn = 3000;
int costShield = 5000;
int costFly = 5000;


float distance(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2, dz = z1 - z2;
    return sqrt(dx*dx + dz*dz);
}

void addParticle(float x, float y, float z, float vx, float vy, float vz,
                 float r, float g, float b, float size, float life, int type) {
    if (particleCount >= MAX_PARTICLES) particleCount = 0;
    particles[particleCount++] = (Particle){x, y, z, vx, vy, vz, life, r, g, b, size, type};
}

void createExplosion(float x, float y, float z, float intensity) {
    for (int i = 0; i < (int)(40 * intensity); i++) {
        float angle = (rand() % 360) * PI / 180.0f;
        float elevation = ((rand() % 180) - 90) * PI / 180.0f;
        float speed = 0.18f + (rand() % 100) / 120.0f * intensity;
        addParticle(x, y, z,
                    cos(angle) * cos(elevation) * speed,
                    sin(elevation) * speed * 1.2f,
                    sin(angle) * cos(elevation) * speed,
                    0.9f + (rand() % 10) / 100.0f, 0.4f + (rand() % 40) / 100.0f, 0.1f,
                    0.28f + (rand() % 30) / 100.0f, 1.0f + (rand() % 80) / 100.0f, 0);
    }
    for (int i = 0; i < 20 * intensity; i++) {
        float angle = (i * 18) * PI / 180.0f;
        addParticle(x, y + 0.1f, z, cos(angle) * 0.25f * (1.0f + intensity * 0.2f), 0, sin(angle) * 0.25f * (1.0f + intensity * 0.2f),
                    0.9f, 0.6f, 0.2f, 0.4f, 0.6f + intensity * 0.1f, 1);
    }
    cameraShake = intensity * 0.5f;
    screenFlash = intensity * 0.3f;
}

// NEW: Meteor explosion change (less bright)
void createMeteorImpact(float x, float y, float z) {
    // Ground shockwave
    for (int i = 0; i < 10; i++) { // Reduced particle count
        float angle = (i * 36) * PI / 180.0f;
        addParticle(x, y + 0.1f, z, cos(angle) * 0.5f, 0, sin(angle) * 0.5f,
                    0.5f, 0.2f, 0.05f, 0.5f, 0.8f, 1); // Much darker orange
    }
    // Debris
    for (int i = 0; i < 20; i++) { // Reduced particle count
        float angle = (rand() % 360) * PI / 180.0f;
        float elevation = ((rand() % 90)) * PI / 180.0f; // Upwards
        float speed = 0.1f + (rand() % 100) / 150.0f;
        addParticle(x, y, z,
                    cos(angle) * cos(elevation) * speed,
                    sin(elevation) * speed * 1.5f,
                    sin(angle) * cos(elevation) * speed,
                    0.1f, 0.1f, 0.1f, 0.2f, 1.5f, 0); // Darker smoke/debris
    }
    cameraShake = 0.5f; // Reduced shake
    screenFlash = 0.0f; // NO SCREEN FLASH
}


void createShockwave(float x, float z) {
    for(int i = 0; i < 100; i++) {
        float angle = (i * 3.6f) * PI / 180.0f;
        addParticle(x, 0.5f, z, cos(angle) * 0.8f, 0.1f, sin(angle) * 0.8f,
                    1.0f, 0.8f, 0.2f, 0.8f, 1.2f, 1);
    }
    cameraShake = 3.0f;

    // Push/damage objects
    for(int i = 0; i < numObjects; i++) {
        if(!objects[i].active) continue;
        float dist = distance(x, z, objects[i].x, objects[i].z);
        if(dist < 15.0f) {
            float dx = objects[i].x - x;
            float dz = objects[i].z - z;
            objects[i].vx += (dx / dist) * (15.0f - dist) * 0.2f;
            objects[i].vz += (dz / dist) * (15.0f - dist) * 0.2f;

            if(objects[i].type == 5) { // Pursuers
                objects[i].health -= 80;
                if(objects[i].health <= 0) {
                     objects[i].active = false;
                     createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.5f);
                     car.kills++;
                     score += 50;
                }
            } else if (objects[i].type == 1 || objects[i].type == 0 || objects[i].type == 4 || objects[i].type == 3) {
                objects[i].active = false;
                createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.2f);
                car.kills++;
                score += 10;
            }
        }
    }

    // NEW: Damage Dinos
    for(int i = 0; i < MAX_DINOS; i++) {
        if(!dinos[i].active) continue;
        float dist = distance(x, z, dinos[i].x, dinos[i].z);
        if(dist < 15.0f) {
            dinos[i].health -= 50;
            if(dinos[i].health <= 0) {
                 dinos[i].active = false;
                 createExplosion(dinos[i].x, dinos[i].y, dinos[i].z, 2.0f);
                 car.kills++;
                 score += 100;
            }
        }
    }
    // NEW: Damage Flying Dinos
    for(int i = 0; i < MAX_FLYING_DINOS; i++) {
        if(!flyingDinos[i].active) continue;
        float dist = distance(x, z, flyingDinos[i].x, flyingDinos[i].z);
        if(dist < 15.0f) {
            flyingDinos[i].health -= 50;
            if(flyingDinos[i].health <= 0) {
                 flyingDinos[i].active = false;
                 createExplosion(flyingDinos[i].x, flyingDinos[i].y, flyingDinos[i].z, 1.5f);
                 car.kills++;
                 score += 100;
            }
        }
    }
    // NEW: Damage Alien Ships
    for(int i = 0; i < MAX_ALIEN_SHIPS; i++) {
        if(!alienShips[i].active) continue;
        float dist = distance(x, z, alienShips[i].x, alienShips[i].z);
        if(dist < 15.0f) {
            alienShips[i].health -= 50;
            if(alienShips[i].health <= 0) {
                 alienShips[i].active = false;
                 createExplosion(alienShips[i].x, alienShips[i].y, alienShips[i].z, 2.0f);
                 car.kills++;
                 score += 250;
            }
        }
    }
}

void applyPenaltyTeleport() {
    createExplosion(car.x, car.y, car.z, 1.5f);
    float spawnRange = arenaSize - 10.0f;
    car.x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
    car.z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;

    // Ensure not spawning on the target
    if(target.active) {
        while (distance(car.x, car.z, target.x, target.z) < 20.0f) {
            car.x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
            car.z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
        }
    }

    createExplosion(car.x, car.y, car.z, 1.5f);
    car.health -= 10;
    car.damage_flash = 1.0f;
    penaltyTimer = fmax(15.0f, 50.0f - currentLevel * 2.0f);
}


void spawnPowerUp(float x, float z) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) {
            powerups[i] = (PowerUp){x, 1.0f, z, (float)(rand() % 7), 15.0f, true};
            break;
        }
    }
}

// NEW: Reset Car stats, preserving upgrades
void resetCar(float x, float z) {
    car.x = x; car.z = z; car.y = 0.5f;
    car.angle = 0; car.vx = 0; car.vz = 0; car.speed = 0;
    car.health = 100 + car.healthBonus; // Apply health upgrade
    car.damage_flash = 0;
    car.drift_angle = 0; car.boost_timer = 0; car.shield_timer = 0; car.invuln_timer = 0;
    car.combo = 0; car.lastHitTime = 0;
    car.drifting = false; car.superMode = false; car.invincible = false;
    car.kills = 0;
    car.reverseGravityTimer = 0; car.confusionTimer = 0; car.ghostTimer = 0;
    car.timeRiftTimer = 0; car.empTimer = 0; car.oilSlickTimer = 0;
    car.shield_cooldown = 0; car.shockwave_cooldown = 0;
    car.fly_timer = 0; car.fly_cooldown = 0;
}


// NEW: Separate init functions for special levels
void initLevel15() {
    difficulty = 15;
    arenaSize = 250.0f;
    numObjects = 0; // NEW: No other objects

    // Init Dinos (T-Rexes and Raptors)
    for(int i = 0; i < MAX_DINOS; i++) {
        dinos[i].x = (rand() % (int)(arenaSize * 1.5f)) - arenaSize*0.75f;
        dinos[i].z = (rand() % (int)(arenaSize * 1.5f)) - arenaSize*0.75f;
        dinos[i].y = 1.5f;
        dinos[i].vx = 0; dinos[i].vz = 0;
        dinos[i].angle = rand() % 360;
        dinos[i].anim_timer = (float)(rand() % 360);
        dinos[i].active = true;
        dinos[i].state = 0;

        if (i < 20) { // 20 T-Rexes
            dinos[i].size = 3.0f + (rand() % 10)/10.0f;
            dinos[i].health = 200.0f;
        } else { // 80 Raptors
            dinos[i].size = 1.5f + (rand() % 10)/10.0f;
            dinos[i].health = 80.0f;
        }
    }

    // NEW: Init Flying Dinos
    for(int i = 0; i < MAX_FLYING_DINOS; i++) {
        flyingDinos[i].x = (rand() % (int)(arenaSize * 1.5f)) - arenaSize*0.75f;
        flyingDinos[i].z = (rand() % (int)(arenaSize * 1.5f)) - arenaSize*0.75f;
        flyingDinos[i].y = 20.0f + (rand() % 10); // Start high
        flyingDinos[i].vx = 0; flyingDinos[i].vz = 0;
        flyingDinos[i].angle = rand() % 360;
        flyingDinos[i].size = 1.2f;
        flyingDinos[i].anim_timer = (float)(rand() % 360);
        flyingDinos[i].active = true;
        flyingDinos[i].state = 0; // Hovering
        flyingDinos[i].health = 50.0f;
    }


    // Init 3 Targets
    targetsRemaining = 3;

    // NEW: Reset car position/health, keep upgrades
    resetCar(0, -(arenaSize - 20.f));

    // NEW: Targets start at y=1 (on the ground) and get 3 teleports
    targets[0] = (GameObject){0, 1, (arenaSize - 20.f), 0, 2.2f, 0, 0, 0, 0, 0, 0, 0, 100.0f, 0, true, false, false, 3, 0};
    targets[1] = (GameObject){(arenaSize - 20.f), 1, 0, 0, 2.2f, 0, 0, 0, 0, 0, 0, 0, 100.0f, 0, true, false, false, 3, 0};
    targets[2] = (GameObject){-(arenaSize - 20.f), 1, 0, 0, 2.2f, 0, 0, 0, 0, 0, 0, 0, 100.0f, 0, true, false, false, 3, 0};
    target.active = false;

    gameTime = 0;
    hits = 0;
    particleCount = 0;
    comboCounter = 0;
    nightMode = true;
    raining = true;
    meteorTimer = 0.0f;
    penaltyTimer = 120.0f; // 2 minutes

    for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
    // Spawn some health packs
    float spawnRange = arenaSize - 10.0f;
    for(int i=0; i<5; i++) {
        spawnPowerUp((rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange,
                     (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange);
    }
}

// NEW: Level 16 Init
void initLevel16() {
    difficulty = 16;
    arenaSize = 100.0f;
    numObjects = 0; // NEW: NO OBJECTS
    survivalTimer = 90.0f;
    alienSpawnTimer = 0.0f;
    targetsRemaining = 0;

    // NEW: Reset car position/health, keep upgrades
    resetCar(0, 0);

    target.active = false;
    for(int i=0; i<3; i++) targets[i].active = false;
    for(int i=0; i<MAX_DINOS; i++) dinos[i].active = false;
    for(int i=0; i<MAX_FLYING_DINOS; i++) flyingDinos[i].active = false;

    // NEW: Spawn Alien Ships
    for(int i=0; i < MAX_ALIEN_SHIPS; i++) {
        alienShips[i].x = (rand() % (int)(arenaSize * 1.5f)) - arenaSize*0.75f;
        alienShips[i].z = (rand() % (int)(arenaSize * 1.5f)) - arenaSize*0.75f;
        alienShips[i].y = 20.0f + (rand() % 10);
        alienShips[i].vx = 0; alienShips[i].vz = 0;
        alienShips[i].angle = rand() % 360;
        alienShips[i].size = 1.0f;
        alienShips[i].anim_timer = (float)(rand() % 360);
        alienShips[i].active = true;
        alienShips[i].state = 0; // Hunting
        alienShips[i].health = 100.0f;
        alienShips[i].attack_timer = (float)(rand() % 5);
        alienShips[i].type = (i % 3); // 0 = laser, 1 = bomber, 2 = spread
    }


    gameTime = 0;
    hits = 0;
    particleCount = 0;
    comboCounter = 0;
    nightMode = true;
    raining = false;
    meteorTimer = 999.0f; // No random meteors
    penaltyTimer = 999.0f; // No time limit

    for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;

    // Spawn a few health packs
    float spawnRange = arenaSize - 10.0f;
    for(int i=0; i<3; i++) {
        spawnPowerUp((rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange,
                     (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange);
    }
}


void initLevel(int level) {
    // NEW: Check for special levels
    if (level == 15) {
        initLevel15();
        return;
    }
    if (level == 16) {
        initLevel16();
        return;
    }

    srand(time(NULL));
    difficulty = level;
    arenaSize = 60.0f + level * 12.0f;
    numObjects = fmin(MAX_OBJECTS, 40 + level * 7);

    // NEW: Reset car position/health, keep upgrades
    resetCar(0, -(arenaSize - 20.f));

    // NEW: Target starts at y=1 (on the ground) and gets 3 teleports
    target = (GameObject){0, 1, (arenaSize - 20.f), 0, 2.2f, 0, 0, 0, 0, 0, 0, 0, 100.0f, 0, true, false, false, 3, 0};
    for(int i=0; i<3; i++) targets[i].active = false;
    targetsRemaining = 0;
    for(int i=0; i<MAX_DINOS; i++) dinos[i].active = false;
    for(int i=0; i<MAX_FLYING_DINOS; i++) flyingDinos[i].active = false;
    for(int i=0; i<MAX_ALIEN_SHIPS; i++) alienShips[i].active = false;


    gameTime = 0;
    if (level == 1) {
        // NEW: Reset score and upgrades on L1
        score = 0;
        car.healthBonus = 0;
        car.accelerationBonus = 0;
        car.turnSpeedBonus = 0;
        car.shieldCooldownReduction = 0;
        car.flyCooldownReduction = 0;
        costHealth = 2000;
        costAccel = 3000;
        costTurn = 3000;
        costShield = 5000;
        costFly = 5000;
    }
    hits = 0;
    particleCount = 0;
    comboCounter = 0;
    if (level == 1) maxCombo = 0;
    nightMode = false;
    raining = false;
    meteorTimer = 0.0f;
    penaltyTimer = fmax(15.0f, 50.0f - level * 2.0f);

    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerups[i].active = false;
    }

    float spawnRange = arenaSize - 10.0f;
    for (int i = 0; i < numObjects; i++) {
        objects[i].x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
        objects[i].y = 0.75f;
        objects[i].z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
        objects[i].angle = 0; objects[i].radius = 0.8f; objects[i].vx = 0;
        objects[i].vy = 0; objects[i].vz = 0; objects[i].rotY = rand() % 360;
        objects[i].timer = 0; objects[i].active = true; objects[i].targetX = 0;
        objects[i].targetZ = 0; objects[i].revealed = false; objects[i].type = 0;
        objects[i].health = 100.0f; objects[i].isAngry = false;
        objects[i].teleportCharges = 0; // Ensure obstacles don't teleport
        objects[i].teleportCooldown = 0;

        while (distance(objects[i].x, objects[i].z, car.x, car.z) < 15 ||
               distance(objects[i].x, objects[i].z, target.x, target.z) < 15) {
            objects[i].x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
            objects[i].z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
        }

        int r = rand() % 100;
        if (r < 12) { objects[i].type = 1; float speed = 0.14f + (rand() % 14) / 100.0f; speed *= (1.0f + level * 0.1f); objects[i].vx = (rand() % 2) ? speed : -speed; objects[i].vz = (rand() % 2) ? speed : -speed; objects[i].radius = 0.7f;
        } else if (r < 20) { objects[i].type = 2; objects[i].radius = 1.5f + level * 0.05f;
        } else if (r < 28) { objects[i].type = 3; objects[i].radius = 1.4f;
        } else if (r < 36) { objects[i].type = 4; objects[i].radius = 0.95f;
        } else if (r < 48) { objects[i].type = 5; objects[i].radius = 1.15f; objects[i].health = 50.0f + level * 12.0f;
        } else if (r < 55) { objects[i].type = 6; objects[i].radius = 1.25f;
        } else if (r < 62) { objects[i].type = 7; objects[i].radius = 1.3f;
        } else if (r < 69) { objects[i].type = 8; objects[i].radius = 1.0f; objects[i].vy = 0.1f;
        } else if (r < 77) { objects[i].type = 9; objects[i].radius = 1.1f; objects[i].timer = 0;
        } else if (level >= 3 && r < 83) { objects[i].type = 10; objects[i].radius = 1.1f;
        } else if (level >= 5 && r < 89) { objects[i].type = 11; objects[i].radius = 1.0f;
        } else if (level >= 4 && r < 95) { objects[i].type = 12; objects[i].radius = 1.8f;
        } else { objects[i].type = 0; objects[i].radius = 0.8f; }
    }

    glClearColor(0.06f, 0.08f, 0.12f, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.008f - level * 0.0003f);
    GLfloat fogColor[] = {0.06f, 0.08f, 0.12f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);

    GLfloat light0[] = {0, 45, 0, 1}, ambient0[] = {0.3f, 0.3f, 0.4f, 1}, diffuse0[] = {0.8f, 0.8f, 0.9f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, light0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
}

bool checkCollision(float x1, float z1, float r1, float x2, float z2, float r2) {
    return distance(x1, z1, x2, z2) < (r1 + r2);
}

void drawCar() {
    if (car.ghostTimer > 0) {
        float alpha = 0.2f + sin(anim * 0.5f) * 0.1f;
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glEnable(GL_BLEND);
    }

    glPushMatrix();
    glTranslatef(car.x, car.y + sin(anim * 0.15f) * 0.05f, car.z);
    glRotatef(car.angle, 0, 1, 0);
    if (car.drifting) glRotatef(car.drift_angle * 14, 0, 0, 1);

    if(car.reverseGravityTimer > 0) {
        glPushMatrix();
        float pulse = 0.6f + sin(anim * 0.3f) * 0.4f;
        glColor4f(0.8f, 0.2f, 1.0f, pulse * 0.5f);
        glRotatef(90, 1, 0, 0);
        glScalef(1.5f, 1.5f, 1.5f);
        glutSolidTorus(0.3, 1.0, 10, 16);
        glPopMatrix();
    }
    if(car.confusionTimer > 0) {
        float s = 0.2f + sin(anim * 0.5f) * 0.1f;
        glColor3f(1.0f, 0.1f, 0.1f);
        glPushMatrix();
        glTranslatef(0, 1.5f, 0);
        glRotatef(anim*5, 1, 1, 0);
        glScalef(s,s,s);
        glutSolidCone(1, 2, 4, 1);
        glPopMatrix();
    }
    if(car.empTimer > 0) {
        float pulse = 0.5f + sin(anim * 0.4f) * 0.5f;
        glColor3f(0.8f * pulse, 0.8f * pulse, 0.1f * pulse);
        glPushMatrix();
        glTranslatef(0, 0.5f, 0);
        glRotatef(anim * 8, 0, 1, 0);
        glScalef(0.5f, 2.0f, 0.5f);
        glutSolidCube(1);
        glPopMatrix();
    }
    if(car.oilSlickTimer > 0) {
        for(int i = 0; i < 5; i++) {
            glColor3f(0.1f, 0.1f, 0.1f);
            glPushMatrix();
            glTranslatef((rand() % 20 - 10)/10.0f, -0.3f, (rand() % 20 - 10)/10.0f);
            glutSolidSphere(0.1f + (rand() % 10)/50.0f, 6, 6);
            glPopMatrix();
        }
    }
    if (car.superMode) {
        glPushMatrix();
        glColor4f(1.0f, 0.8f, 0.0f, 0.4f + sin(anim * 0.4f) * 0.2f);
        glScalef(1.3f, 1.3f, 1.3f);
        glutSolidSphere(1.5f, 16, 16);
        glPopMatrix();
    }
    if (car.shield_timer > 0) {
        glPushMatrix();
        glColor4f(0.3f, 0.7f, 1.0f, 0.3f + sin(anim * 0.3f) * 0.15f);
        glRotatef(anim * 2, 0, 1, 0);
        glutSolidSphere(2.0f, 16, 16);
        glPopMatrix();
    }
    if (car.invincible) {
        glPushMatrix();
        glColor4f(1.0f, 0.3f, 1.0f, 0.35f + sin(anim * 0.5f) * 0.2f);
        glRotatef(anim * -3, 0, 1, 0);
        glutSolidSphere(1.8f, 14, 14);
        glPopMatrix();
    }

    // float healthRatio = car.health / 100.0f; // <-- FIX: This line is unused
    float glow = car.superMode ? 1.5f : 1.0f;

    // NEW: Car Body (Dark Grey)
    glColor3f(0.3f * glow, 0.3f * glow, 0.35f * glow);
    glPushMatrix(); glScalef(1.1f, 0.45f, 2.0f); glutSolidCube(1); glPopMatrix();

    // NEW: Car Cabin (Darker Grey)
    glColor3f(0.2f * glow, 0.2f * glow, 0.2f * glow);
    glPushMatrix(); glTranslatef(0, 0.4f, -0.15f); glScalef(0.7f, 0.45f, 1.1f); glutSolidCube(1); glPopMatrix();

    // NEW: Demon Wings
    glColor3f(0.6f, 0.1f, 0.1f); // Dark Red wings
    glPushMatrix();
    glTranslatef(-0.6f, 0.5f, -1.0f);
    glRotatef(-30, 0, 1, 0);
    glRotatef(20, 1, 0, 0);
    glScalef(0.1f, 0.6f, 0.9f);
    glutSolidCube(1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.6f, 0.5f, -1.0f);
    glRotatef(30, 0, 1, 0);
    glRotatef(20, 1, 0, 0);
    glScalef(0.1f, 0.6f, 0.9f);
    glutSolidCube(1);
    glPopMatrix();

    // NEW: Spikes
    glColor3f(0.5f, 0.5f, 0.5f); // Spike color
    // Hood spikes
    glPushMatrix(); glTranslatef(0, 0.3f, 0.7f); glRotatef(-90, 1,0,0); glScalef(0.1f, 0.1f, 0.3f); glutSolidCone(1, 1, 8, 1); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.3f, 0.3f, 0.8f); glRotatef(-90, 1,0,0); glScalef(0.1f, 0.1f, 0.3f); glutSolidCone(1, 1, 8, 1); glPopMatrix();
    glPushMatrix(); glTranslatef(0.3f, 0.3f, 0.8f); glRotatef(-90, 1,0,0); glScalef(0.1f, 0.1f, 0.3f); glutSolidCone(1, 1, 8, 1); glPopMatrix();
    // Roof spikes
    glPushMatrix(); glTranslatef(-0.2f, 0.65f, -0.2f); glRotatef(-90, 1,0,0); glScalef(0.15f, 0.15f, 0.4f); glutSolidCone(1, 1, 8, 1); glPopMatrix();
    glPushMatrix(); glTranslatef(0.2f, 0.65f, -0.2f); glRotatef(-90, 1,0,0); glScalef(0.15f, 0.15f, 0.4f); glutSolidCone(1, 1, 8, 1); glPopMatrix();

    // Spoiler
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(0, 0.5f, -1.0f); glScalef(1.0f, 0.08f, 0.2f); glutSolidCube(1); glPopMatrix();

    // Turbo
    if (car.boost_timer > 0) {
        float intensity = 1.0f + sin(anim * 0.5f) * 0.3f;
        // NEW: Fiery orange/red turbo
        glColor3f(1.0f * intensity, 0.2f * intensity, 0.0f);
        for (int i = 0; i < 2; i++) {
            glPushMatrix();
            glTranslatef(i == 0 ? -0.3f : 0.3f, 0, -1.05f);
            glScalef(0.18f, 0.18f, 0.55f + sin(anim * 0.7f + i) * 0.3f);
            glutSolidCone(1, 2.5f, 8, 1);
            glPopMatrix();
        }
    }

    // NEW: Red Headlights
    float lightIntensity = 1.0f + sin(anim * 0.2f) * 0.15f;
    glColor3f(1.0f * lightIntensity, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(-0.35f, 0.08f, 1.05f); glutSolidSphere(0.13, 10, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.35f, 0.08f, 1.05f); glutSolidSphere(0.13, 10, 10); glPopMatrix();

    // Wheels
    glColor3f(0.15f, 0.15f, 0.15f);
    float wheelPos[][2] = {{-0.6f, 0.75f}, {0.6f, 0.75f}, {-0.6f, -0.75f}, {0.6f, -0.75f}};
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(wheelPos[i][0], -0.28f, wheelPos[i][1]);
        glRotatef(90, 0, 0, 1);
        glRotatef(anim * car.speed * 45, 0, 1, 0);
        glutSolidTorus(0.12, 0.22, 10, 14);
        glPopMatrix();
    }

    glPopMatrix();

    if (car.ghostTimer > 0) {
        glDisable(GL_BLEND);
    }
}

void drawEnhancedCone(float r, float g, float b) {
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix(); glTranslatef(0, -0.5f, 0); glScalef(1.5f, 0.22f, 1.5f); glutSolidCube(1); glPopMatrix();
    for (int i = 0; i < 6; i++) {
        glColor3f(i % 2 ? r : 1, i % 2 ? g : 1, i % 2 ? b : 0.95f);
        glPushMatrix(); glTranslatef(0, -0.32f + i*0.26f, 0); glRotatef(-90, 1, 0, 0);
        glutSolidCone(0.68f - i*0.08f, 0.26f, 14, 2); glPopMatrix();
    }
    float glow = 0.6f + sin(anim * 0.22f) * 0.4f;
    glColor3f(1, 0.85f * glow, 0);
    glPushMatrix(); glTranslatef(0, 1.25f, 0); glutSolidSphere(0.2f, 10, 10); glPopMatrix();
}

void drawBarrel(bool explosive) {
    if (explosive) {
        float pulse = sin(anim * 0.14f) * 0.1f + 0.9f;
        glColor3f(0.95f * pulse, 0.22f, 0.15f);
    } else {
        glColor3f(0.85f, 0.5f, 0.18f);
    }
    glPushMatrix(); glScalef(0.85f, 1.3f, 0.85f); glutSolidSphere(0.8f, 14, 14); glPopMatrix();
    glColor3f(0.12f, 0.12f, 0.12f);
    for (int i = -1; i <= 1; i++) {
        glPushMatrix(); glTranslatef(0, i*0.38f, 0); glScalef(0.88f, 0.14f, 0.88f);
        glutSolidSphere(0.8f, 14, 10); glPopMatrix();
    }
    if (explosive) {
        glColor3f(1, 1, 0.2f);
        glPushMatrix(); glTranslatef(0, 0, 0.85f); glRotatef(anim * 2, 0, 0, 1);
        glScalef(0.32f, 0.32f, 0.1f); glutSolidSphere(1, 3, 1); glPopMatrix();
    }
}

// NEW: drawPursuer (demonic theme)
void drawPursuer(bool angry) {
    glPushMatrix(); glRotatef(anim * (angry ? 5 : 3), 0, 1, 0);
    // NEW: Black body
    glColor3f(0.1f, 0.1f, 0.1f);
    glutSolidSphere(0.7f, 12, 12);
    for (int i = 0; i < 4; i++) {
        glPushMatrix(); glRotatef(i * 90 + anim * (angry ? 3 : 2), 0, 1, 0);
        glTranslatef(0.9f, 0, 0);
        // NEW: Dark grey arms
        glColor3f(0.2f, 0.2f, 0.2f);
        glutSolidSphere(0.3f, 8, 8); glPopMatrix();
    }
    // NEW: Red eye
    float eyeGlow = sin(anim * (angry ? 0.4f : 0.28f)) * 0.3f + 0.7f;
    glColor3f(1, eyeGlow * 0.2f, 0);
    glPushMatrix(); glTranslatef(0, 0.1f, 0.75f); glutSolidSphere(0.16f, 8, 8); glPopMatrix();
    glPopMatrix();
}

void drawSpinner() {
    glPushMatrix();
    glRotatef(anim * 6, 0, 1, 0);
    glColor3f(0.3f, 0.8f, 0.9f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glRotatef(i * 90, 0, 1, 0);
        glTranslatef(0, 0, 0);
        glScalef(0.3f, 0.3f, 1.8f);
        glutSolidCube(1);
        glPopMatrix();
    }
    glColor3f(0.5f, 0.95f, 1.0f);
    glutSolidSphere(0.4f, 10, 10);
    glPopMatrix();
}

void drawBouncer(float height) {
    glPushMatrix();
    glTranslatef(0, height - 0.75f, 0);
    glColor3f(0.9f, 0.5f, 0.2f);
    glScalef(1.0f, 0.8f + sin(anim * 0.3f) * 0.2f, 1.0f);
    glutSolidSphere(0.8f, 12, 12);
    glPopMatrix();

    glColor3f(0.3f, 0.3f, 0.3f);
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        glTranslatef(0, i * 0.15f - 0.4f, 0);
        glScalef(0.6f, 0.1f, 0.6f);
        glutSolidSphere(0.5f, 8, 8);
        glPopMatrix();
    }
}

void drawLaserTurret() {
    glColor3f(0.25f, 0.25f, 0.3f);
    glPushMatrix();
    glScalef(1.2f, 0.3f, 1.2f);
    glutSolidCube(1);
    glPopMatrix();

    glPushMatrix();
    glRotatef(anim * 1.5f, 0, 1, 0);
    glColor3f(0.4f, 0.4f, 0.5f);
    glutSolidSphere(0.6f, 10, 10);

    glColor3f(1.0f, 0.2f, 0.2f);
    glPushMatrix();
    glTranslatef(0, 0, 0.7f);
    glScalef(0.15f, 0.15f, 0.5f);
    glutSolidCube(1);
    glPopMatrix();

    float pulse = sin(anim * 0.3f) * 0.3f + 0.7f;
    glColor3f(1.0f * pulse, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0, 0, 0.95f);
    glutSolidSphere(0.12f, 8, 8);
    glPopMatrix();

    glPopMatrix();
}

void drawConfusionMine() {
    float pulse = 0.8f + sin(anim * 0.25f) * 0.2f;
    glColor3f(0.8f * pulse, 0.1f, 0.9f * pulse);
    glPushMatrix();
    glRotatef(anim * 2, 0, 1, 0);
    glScalef(pulse, pulse, pulse);
    glutSolidDodecahedron();
    glPopMatrix();

    glColor3f(1.0f, 0.2f, 0.2f);
    for(int i = 0; i < 6; i++) {
        glPushMatrix();
        glRotatef(i * 60 + anim * 3, 1, 1, 0);
        glTranslatef(0, 0, 1.0f);
        glutSolidSphere(0.2f, 6, 6);
        glPopMatrix();
    }
}

void drawEMPMine() {
    float pulse = 0.5f + sin(anim * 0.5f) * 0.5f;
    glColor3f(0.8f * pulse, 0.8f * pulse, 0.1f * pulse);
    glPushMatrix();
    glScalef(pulse, pulse, pulse);
    glutSolidOctahedron();
    glPopMatrix();

    for(int i = 0; i < 8; i++) {
        glPushMatrix();
        glRotatef(i * 45 + anim * 5, (i%2), 1, (i%3));
        glTranslatef(0, 0, 1.3f * pulse);
        glColor3f(1.0f * pulse, 1.0f * pulse, 0.5f * pulse);
        glutSolidSphere(0.2f, 6, 6);
        glPopMatrix();
    }
}

void drawOilSlick() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glColor4f(0.05f, 0.05f, 0.05f, 0.7f + sin(anim * 0.1f) * 0.1f);
    glPushMatrix();
    glTranslatef(0, 0.05f, 0);
    glScalef(1.8f, 0.1f, 1.8f);
    glutSolidSphere(1.0f, 16, 16);
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// NEW: Draw Dino (Level 15)
void drawDino(Dinosaur* d) {
    if (!d->active) return;

    glPushMatrix();
    glTranslatef(d->x, d->y, d->z);
    glRotatef(d->angle, 0, 1, 0);
    glScalef(d->size, d->size, d->size);

    float body_bob = sin(d->anim_timer * 0.3f) * 0.1f;

    // Body
    glColor3f(0.2f, 0.4f, 0.15f);
    glPushMatrix();
    glTranslatef(0, 1.0f + body_bob, 0);
    glScalef(1.0f, 0.8f, 1.8f);
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Head
    float head_bob = sin(d->anim_timer * 0.3f + 0.5f) * 0.2f;
    glPushMatrix();
    glTranslatef(0, 1.5f + head_bob, 1.6f);
    glScalef(0.6f, 0.6f, 0.6f);
    glutSolidSphere(1.0f, 10, 10);
    // Eyes
    glColor3f(1.0f, 1.0f, 0.0f);
    glPushMatrix(); glTranslatef(-0.3f, 0.2f, 0.5f); glutSolidSphere(0.2f, 6, 6); glPopMatrix();
    glPushMatrix(); glTranslatef(0.3f, 0.2f, 0.5f); glutSolidSphere(0.2f, 6, 6); glPopMatrix();
    glPopMatrix();

    // Legs
    glColor3f(0.15f, 0.3f, 0.1f);
    float leg_stomp = sin(d->anim_timer * 0.15f);
    glPushMatrix(); glTranslatef(-0.5f, 0.5f, 0.5f); glScalef(0.3f, 1.0f + leg_stomp*0.2f, 0.3f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0.5f, 0.5f, 0.5f); glScalef(0.3f, 1.0f - leg_stomp*0.2f, 0.3f); glutSolidCube(1); glPopMatrix();

    glPopMatrix();
}

// NEW: Draw Flying Dino
void drawFlyingDino(FlyingDinosaur* d) {
    if (!d->active) return;
    glPushMatrix();
    glTranslatef(d->x, d->y, d->z);
    glRotatef(d->angle, 0, 1, 0);
    glScalef(d->size, d->size, d->size);

    float flap = sin(d->anim_timer * 0.8f) * 40.0f; // Flap angle

    // Body
    glColor3f(0.5f, 0.3f, 0.2f); // Brownish body
    glPushMatrix();
    glScalef(0.8f, 0.8f, 1.5f);
    glutSolidSphere(1.0f, 10, 10);
    glPopMatrix();

    // Left Wing
    glPushMatrix();
    glTranslatef(-0.7f, 0, 0);
    glRotatef(-30.0f + flap, 0, 0, 1); // Flap
    glScalef(2.0f, 0.1f, 0.8f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Right Wing
    glPushMatrix();
    glTranslatef(0.7f, 0, 0);
    glRotatef(30.0f - flap, 0, 0, 1); // Flap
    glScalef(2.0f, 0.1f, 0.8f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

// NEW: Draw Alien Ship (Level 16)
void drawAlienShip(AlienShip* s) {
    if (!s->active) return;
    glPushMatrix();
    glTranslatef(s->x, s->y, s->z);
    glRotatef(s->angle, 0, 1, 0);
    glScalef(s->size, s->size, s->size);

    float bob = sin(s->anim_timer * 0.5f) * 0.2f;

    // Main Hull (Saucer)
    glColor3f(0.3f, 0.4f, 0.35f); // Dark alien green
    glPushMatrix();
    glTranslatef(0, bob, 0);
    glScalef(2.5f, 0.5f, 2.5f);
    glutSolidSphere(1.0f, 16, 12);
    glPopMatrix();

    // Cockpit
    glColor3f(0.6f, 1.0f, 0.7f); // Bright green glow
    glPushMatrix();
    glTranslatef(0, bob + 0.5f, 0);
    glutSolidSphere(0.6f, 12, 10);
    glPopMatrix();

    // NEW: Visual for Type 2 (Spread)
    if(s->type == 2) {
        glColor3f(0.8f, 0.8f, 0.2f); // Yellow
        glPushMatrix();
        glTranslatef(0.8f, bob, 0);
        glScalef(0.2f, 0.2f, 0.5f);
        glutSolidCube(1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-0.8f, bob, 0);
        glScalef(0.2f, 0.2f, 0.5f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // Underside glow (attack port)
    float pulse = 0.5f + sin(s->anim_timer * 0.3f) * 0.5f;
    glColor3f(1.0f * pulse, 0.2f * pulse, 0.2f); // Red
    glPushMatrix();
    glTranslatef(0, bob - 0.3f, 0);
    glutSolidSphere(0.3f, 8, 8);
    glPopMatrix();

    glPopMatrix();
}


void drawPowerUp(PowerUp* p) {
    glPushMatrix();
    glTranslatef(p->x, p->y + sin(anim * 0.15f + p->x) * 0.3f, p->z);
    glRotatef(anim * 3, 0, 1, 0);

    if (p->type == 0) { glColor3f(0.2f, 1.0f, 0.3f); glutSolidSphere(0.4f, 12, 12); glColor3f(0.4f, 1.0f, 0.5f); glPushMatrix(); glScalef(0.7f, 0.1f, 0.1f); glutSolidCube(1); glPopMatrix(); glPushMatrix(); glScalef(0.1f, 0.7f, 0.1f); glutSolidCube(1); glPopMatrix();
    } else if (p->type == 1) { glColor3f(1.0f, 0.8f, 0.2f); glutSolidCone(0.4f, 0.8f, 12, 1); glPushMatrix(); glRotatef(180, 1, 0, 0); glutSolidCone(0.4f, 0.8f, 12, 1); glPopMatrix();
    } else if (p->type == 2) { glColor3f(1.0f, 0.3f, 1.0f); glutSolidTorus(0.15f, 0.4f, 10, 16); glPushMatrix(); glRotatef(90, 1, 0, 0); glutSolidTorus(0.15f, 0.4f, 10, 16); glPopMatrix();
    } else if (p->type == 3) { glColor3f(0.3f, 0.8f, 1.0f); glutSolidOctahedron();
    } else if (p->type == 4) { glColor3f(0.8f, 0.2f, 1.0f); glScalef(0.8f, 0.8f, 0.8f); glutSolidIcosahedron();
    } else if (p->type == 5) { glColor3f(1.0f, 1.0f, 1.0f); glScalef(0.7f, 0.7f, 0.7f); glutSolidDodecahedron();
    } else if (p->type == 6) { glColor3f(0.2f, 0.8f, 1.0f); glPushMatrix(); glRotatef(anim * 5, 1, 1, 0); glutSolidTorus(0.1f, 0.5f, 8, 16); glPopMatrix(); glPushMatrix(); glRotatef(anim * -5, 1, -1, 0); glutSolidTorus(0.1f, 0.5f, 8, 16); glPopMatrix();
    }

    glPopMatrix();
}

void drawObject(GameObject* o) {
    if (!o->active) return;
    float dist = distance(o->x, o->z, car.x, car.z);

    glPushMatrix();
    glTranslatef(o->x, o->y, o->z);

    if (o->type == 1) { glPushMatrix(); glRotatef(o->rotY, 0, 1, 0); drawEnhancedCone(1, 0.12f, 0.12f); glPopMatrix();
    } else if (o->type == 2) {
        if (dist < 10) {
            float alpha = (10-dist)/10.0f;
            glColor4f(0.82f*alpha, 0.15f*alpha, 0.92f*alpha, 0.6f*alpha);
            glPushMatrix(); glTranslatef(0, 0.08f, 0); glRotatef(-90, 1, 0, 0);
            glutSolidTorus(0.42, o->radius, 18, 28); glPopMatrix();
            for (int i = 0; i < 10; i++) {
                glPushMatrix(); glRotatef(i*36 + anim*3, 0, 1, 0);
                glTranslatef(o->radius * 0.8f, 0.42f + sin(anim*0.1f + i)*0.22f, 0);
                glColor4f(0.92f*alpha, 0.25f*alpha, 1.0f*alpha, 0.7f*alpha);
                glutSolidSphere(0.2f, 10, 10); glPopMatrix();
            }
        }
    } else if (o->type == 3) {
        float reveal = o->revealed ? 1.0f : 0.0f;
        if (dist < 6) reveal = (6 - dist) / 6.0f;
        glPushMatrix(); glRotatef(anim*0.65f, 0, 1, 0);
        glColor3f(0.35f + 0.6f*reveal, 0.95f - 0.78f*reveal, 0.45f - 0.3f*reveal);
        glutSolidSphere(1.3f, 18, 18);
        glColor3f(0.5f + 0.45f*reveal, 0.82f - 0.65f*reveal, 0.6f - 0.45f*reveal);
        glutSolidSphere(0.88f, 14, 14); glPopMatrix();
    } else if (o->type == 4) { drawBarrel(true); }
    else if (o->type == 5) { drawPursuer(o->isAngry); }
    else if (o->type == 6) {
        float pulse = sin(anim * 0.1f);
        glColor4f(0.5f, 0.25f, 0.98f, 0.6f + pulse * 0.28f);
        glPushMatrix(); glTranslatef(0, 0.12f, 0); glRotatef(-90, 1, 0, 0);
        glutSolidTorus(0.38, 1.45f, 18, 28); glPopMatrix();
        for (int i = 0; i < 12; i++) {
            float angle = (i * 30 + anim * 3) * PI / 180.0f;
            glPushMatrix(); glTranslatef(cos(angle) * 1.3f, 0.52f + sin(anim * 0.18f + i) * 0.32f, sin(angle) * 1.3f);
            glColor4f(0.7f, 0.35f, 1.0f, 0.72f); glutSolidSphere(0.16f, 8, 8); glPopMatrix();
        }
    }
    else if (o->type == 7) { drawSpinner(); }
    else if (o->type == 8) { drawBouncer(o->y); }
    else if (o->type == 9) { drawLaserTurret(); }
    else if (o->type == 10) { drawConfusionMine(); }
    else if (o->type == 11) { drawEMPMine(); }
    else if (o->type == 12) { drawOilSlick(); }
    else { drawBarrel(false); }

    glPopMatrix();
}

void drawTarget(GameObject* t) {
    if (!t->active) return;

    glPushMatrix();
    glTranslatef(t->x, t->y + sin(anim * 0.12f) * 0.15f, t->z);

    glColor3f(0.92f, 0.82f, 0.35f);
    glPushMatrix(); glTranslatef(0, -0.75f, 0); glScalef(4.2f, 0.32f, 4.2f); glutSolidCube(1); glPopMatrix();

    glPushMatrix(); glRotatef(anim*0.5f, 0, 1, 0);

    float pulse = sin(anim * 0.08f) * 0.12f + 0.88f;
    glColor3f(0.35f * pulse, 0.98f * pulse, 0.48f * pulse);
    glutSolidSphere(1.85f, 24, 24);

    glColor3f(0.52f, 1.0f, 0.62f);
    glutSolidSphere(1.25f, 20, 20);

    for (int i = 0; i < 3; i++) {
        glPushMatrix(); glRotatef(i*120, 0, 1, 0); glRotatef(62, 1, 0, 0);
        glColor3f(0.45f, 0.92f, 0.55f); glutSolidTorus(0.13, 2.6f + i*0.32f, 12, 32); glPopMatrix();
    }
    glPopMatrix(); glPopMatrix();
}

void drawGround() {
    float size = arenaSize;
    float boundary = size - 8.0f;

    glColor3f(0.16f, 0.2f, 0.18f);
    glBegin(GL_QUADS); glNormal3f(0, 1, 0);
    glVertex3f(-size, 0, -size); glVertex3f(size, 0, -size);
    glVertex3f(size, 0, size); glVertex3f(-size, 0, size); glEnd();

    glColor3f(0.22f, 0.28f, 0.24f); glLineWidth(1.6f); glBegin(GL_LINES);
    for (int i = -size; i <= size; i += 2) {
        glVertex3f(i, 0.01f, -size); glVertex3f(i, 0.01f, size);
        glVertex3f(-size, 0.01f, i); glVertex3f(size, 0.01f, i);
    }
    glEnd();

    float boundGlow = 0.7f + sin(anim * 0.1f) * 0.3f;
    glColor3f(0.92f * boundGlow, 0.92f * boundGlow, 0.15f * boundGlow); glLineWidth(5.5f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-boundary, 0.02f, -boundary); glVertex3f(boundary, 0.02f, -boundary);
    glVertex3f(boundary, 0.02f, boundary); glVertex3f(-boundary, 0.02f, boundary); glEnd();
    glLineWidth(1.0f);
}

void drawParticles() {
    glDisable(GL_LIGHTING);
    for (int i = 0; i < particleCount; i++) {
        if (particles[i].life > 0) {
            glPushMatrix(); glTranslatef(particles[i].x, particles[i].y, particles[i].z);
            if (particles[i].type == 1) {
                glScalef(2.0f, 0.2f, 2.0f);
            }
            glColor4f(particles[i].r, particles[i].g, particles[i].b, particles[i].life);
            glutSolidSphere(particles[i].size, 6, 6); glPopMatrix();
        }
    }
    glEnable(GL_LIGHTING);
}

void drawText(const char* text, float x, float y, void* font, float r, float g, float b) {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glColor3f(r, g, b); glRasterPos2f(x, y);
    while (*text) glutBitmapCharacter(font, *text++);
    glEnable(GL_LIGHTING);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

void renderScene() {
    drawGround();
    for (int i = 0; i < numObjects; i++) drawObject(&objects[i]);
    for (int i = 0; i < MAX_POWERUPS; i++) if (powerups[i].active) drawPowerUp(&powerups[i]);

    if (currentLevel == 15) {
        for (int i = 0; i < 3; i++) drawTarget(&targets[i]);
        for (int i = 0; i < MAX_DINOS; i++) drawDino(&dinos[i]);
        for (int i = 0; i < MAX_FLYING_DINOS; i++) drawFlyingDino(&flyingDinos[i]);
    } else if (currentLevel == 16) {
        for (int i = 0; i < MAX_ALIEN_SHIPS; i++) drawAlienShip(&alienShips[i]);
    } else {
        drawTarget(&target);
    }

    drawCar();
    drawParticles();
}

// NEW: Draw Upgrade Shop
void drawUpgradeShop() {
    char hud[300];

    drawText("=== UPGRADE SHOP ===", WIDTH/2-150, HEIGHT-70, GLUT_BITMAP_TIMES_ROMAN_24, 1, 1, 0);
    sprintf(hud, "SCORE (CURRENCY): %d", score);
    drawText(hud, WIDTH/2-150, HEIGHT-110, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);

    float hudY = HEIGHT-200;

    sprintf(hud, "1. Upgrade Max Health (+20HP) --- Cost: %d", costHealth);
    drawText(hud, 150, hudY, GLUT_BITMAP_HELVETICA_18, (score >= costHealth) ? 0 : 1, 1, (score >= costHealth) ? 0 : 0.5f);
    hudY -= 50;

    sprintf(hud, "2. Upgrade Acceleration (+0.005) --- Cost: %d", costAccel);
    drawText(hud, 150, hudY, GLUT_BITMAP_HELVETICA_18, (score >= costAccel) ? 0 : 1, 1, (score >= costAccel) ? 0 : 0.5f);
    hudY -= 50;

    sprintf(hud, "3. Upgrade Turning (+0.2) --- Cost: %d", costTurn);
    drawText(hud, 150, hudY, GLUT_BITMAP_HELVETICA_18, (score >= costTurn) ? 0 : 1, 1, (score >= costTurn) ? 0 : 0.5f);
    hudY -= 50;

    sprintf(hud, "4. Upgrade Shield Cooldown (-0.5s) --- Cost: %d", costShield);
    drawText(hud, 150, hudY, GLUT_BITMAP_HELVETICA_18, (score >= costShield) ? 0 : 1, 1, (score >= costShield) ? 0 : 0.5f);
    hudY -= 50;

    sprintf(hud, "5. Upgrade Fly Cooldown (-0.5s) --- Cost: %d", costFly);
    drawText(hud, 150, hudY, GLUT_BITMAP_HELVETICA_18, (score >= costFly) ? 0 : 1, 1, (score >= costFly) ? 0 : 0.5f);
    hudY -= 50;

    hudY -= 100;
    drawText("Press 'Enter' to start next level...", WIDTH/2-150, hudY, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);

    // Draw current stats
    hudY = HEIGHT-200;
    sprintf(hud, "Current Max HP: %.0f", 100 + car.healthBonus);
    drawText(hud, WIDTH - 450, hudY, GLUT_BITMAP_HELVETICA_18, 0.7f, 0.7f, 1.0f);
    hudY -= 50;

    sprintf(hud, "Current Accel: %.3f", ACCELERATION + car.accelerationBonus);
    drawText(hud, WIDTH - 450, hudY, GLUT_BITMAP_HELVETICA_18, 0.7f, 0.7f, 1.0f);
    hudY -= 50;

    sprintf(hud, "Current Turn: %.1f", TURN_SPEED + car.turnSpeedBonus);
    drawText(hud, WIDTH - 450, hudY, GLUT_BITMAP_HELVETICA_18, 0.7f, 0.7f, 1.0f);
    hudY -= 50;

    sprintf(hud, "Shield Cooldown: %.1fs", fmax(1.0f, 10.0f - car.shieldCooldownReduction));
    drawText(hud, WIDTH - 450, hudY, GLUT_BITMAP_HELVETICA_18, 0.7f, 0.7f, 1.0f);
    hudY -= 50;

    sprintf(hud, "Fly Cooldown: %.1fs", fmax(1.0f, 10.0f - car.flyCooldownReduction));
    drawText(hud, WIDTH - 450, hudY, GLUT_BITMAP_HELVETICA_18, 0.7f, 0.7f, 1.0f);
    hudY -= 50;

}


void display() {
    // NEW: Check for UPGRADE_SHOP state
    if (gameState == UPGRADE_SHOP) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, WIDTH, HEIGHT);
        drawUpgradeShop();
        glutSwapBuffers();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float shake = cameraShake * (rand() % 200 - 100) / 100.0f * 0.6f;
    if (earthquake) shake += sin(anim * 0.5f) * 0.3f;

    glViewport(0, 0, WIDTH/2, HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(72, (float)(WIDTH/2)/HEIGHT, 0.1, 140 + arenaSize * 2);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(car.x + shake, 50 + arenaSize * 0.8f + shake, car.z + shake,
              car.x, car.y, car.z,
              0, 0, 1);
    renderScene();

    glViewport(WIDTH/2, 0, WIDTH/2, HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(78, (float)(WIDTH/2)/HEIGHT, 0.1, 140 + arenaSize * 2);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float camDist = 8.5f - car.speed * 3.2f;
    float camHeight = 5.0f + car.speed * 2.2f;
    float angle = car.angle * PI / 180.0f;
    float cx = car.x - camDist*sin(angle) + shake;
    float cz = car.z - camDist*cos(angle) + shake;
    gluLookAt(cx, camHeight + car.y * 0.5f, cz,
              car.x, car.y + 2.2f, car.z,
              0, 1, 0);
    renderScene();

    glViewport(0, 0, WIDTH, HEIGHT);
    char hud[300];

    // Different HUD for Level 16
    if (currentLevel == 16) {
        sprintf(hud, "LEVEL 16 - SURVIVE: %.1fs | HP: %.0f%% | Kills: %d | Score: %d",
                survivalTimer > 0 ? survivalTimer : 0.0f, car.health, car.kills, score);
    } else if (currentLevel == 15) {
        sprintf(hud, "LEVEL 15 - DESTROY TARGETS: %d / 3 | HP: %.0f%% | Kills: %d | Score: %d",
                3 - targetsRemaining, car.health, car.kills, score);
    } else {
        sprintf(hud, "LEVEL %d / 15 | Time Limit: %.1fs | HP: %.0f%% | Speed: %.0f | Kills: %d | Score: %d",
                currentLevel, penaltyTimer > 0 ? penaltyTimer : 0.0f,
                car.health, car.speed * 180, car.kills, score);
    }
    drawText(hud, 15, HEIGHT-35, GLUT_BITMAP_HELVETICA_18, 1, 1, 0.9f);


    if (comboCounter > 1) {
        sprintf(hud, "COMBO x%d!", comboCounter);
        drawText(hud, WIDTH/2-60, HEIGHT-75, GLUT_BITMAP_TIMES_ROMAN_24, 1, 0.5f, 0);
    }

    float hudY = HEIGHT-70;
    // NEW: Fly Ability HUD
    if (car.fly_timer > 0) {
        sprintf(hud, "FLY: %.1fs", car.fly_timer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 0.2f); hudY -= 35;
    } else if (car.fly_cooldown > 0) {
        sprintf(hud, "FLY COOLDOWN: %.1fs", fmax(0, car.fly_cooldown));
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.5f, 0.5f, 0.5f); hudY -= 35;
    }

    if (car.shield_timer > 0) {
        sprintf(hud, "SHIELD: %.1fs", car.shield_timer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.3f, 0.7f, 1); hudY -= 35;
    } else if (car.shield_cooldown > 0) {
        sprintf(hud, "SHIELD COOLDOWN: %.1fs", fmax(0, car.shield_cooldown));
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.5f, 0.5f, 0.5f); hudY -= 35;
    }
    if (car.shockwave_cooldown > 0) {
        sprintf(hud, "SHOCKWAVE COOLDOWN: %.1fs", fmax(0, car.shockwave_cooldown));
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.5f, 0.5f, 0.5f); hudY -= 35;
    }
    if (car.invincible) {
        sprintf(hud, "INVINCIBLE: %.1fs", car.invuln_timer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 1, 0.3f, 1); hudY -= 35;
    }
    if (car.ghostTimer > 0) {
        sprintf(hud, "GHOST: %.1fs", car.ghostTimer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.9f, 0.9f, 0.9f); hudY -= 35;
    }
    if (car.reverseGravityTimer > 0) {
        sprintf(hud, "REVERSE GRAVITY: %.1fs", car.reverseGravityTimer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.8f, 0.3f, 1); hudY -= 35;
    }
    if (car.timeRiftTimer > 0) {
        sprintf(hud, "TIME RIFT: %.1fs", car.timeRiftTimer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.3f, 0.8f, 0.9f); hudY -= 35;
    }
    if (car.confusionTimer > 0) {
        sprintf(hud, "CONTROLS REVERSED: %.1fs", car.confusionTimer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 1.0f, 0.2f, 0.2f); hudY -= 35;
    }
    if (car.empTimer > 0) {
        sprintf(hud, "SYSTEMS DISABLED: %.1fs", car.empTimer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 0.2f); hudY -= 35;
    }
    if (car.oilSlickTimer > 0) {
        sprintf(hud, "NO TRACTION: %.1fs", car.oilSlickTimer);
        drawText(hud, 15, hudY, GLUT_BITMAP_HELVETICA_18, 0.3f, 0.3f, 0.3f); hudY -= 35;
    }

    if (car.superMode) drawText("*** SUPER MODE ***", WIDTH/2-100, HEIGHT-110, GLUT_BITMAP_TIMES_ROMAN_24, 1, 0.8f, 0);
    if (car.boost_timer > 0) drawText(">>> TURBO <<<", WIDTH/2-80, HEIGHT-145, GLUT_BITMAP_HELVETICA_18, 1, 0.7f, 0.2f);

    if (nightMode) drawText("[NIGHT MODE]", WIDTH-180, HEIGHT-35, GLUT_BITMAP_HELVETICA_18, 0.5f, 0.5f, 0.8f);
    if (earthquake) drawText("[EARTHQUAKE!]", WIDTH-190, HEIGHT-70, GLUT_BITMAP_HELVETICA_18, 1, 0.3f, 0.3f);
    if (meteorTimer > 0) drawText("[METEOR SHOWER!]", WIDTH-220, HEIGHT-105, GLUT_BITMAP_HELVETICA_18, 1, 0.5f, 0.2f);


    if (gameState == WIN) {
        sprintf(hud, "=== LEVEL %d CLEARED! ===", currentLevel);
        drawText(hud, WIDTH/2-160, HEIGHT/2+45, GLUT_BITMAP_TIMES_ROMAN_24, 0, 1, 0);
        sprintf(hud, "Time: %.1fs | Score: %d | Max Combo: x%d", gameTime, score, maxCombo);
        drawText(hud, WIDTH/2-180, HEIGHT/2, GLUT_BITMAP_HELVETICA_18, 1, 1, 0.8f);

        // NEW: Changed to "Press Enter for Upgrade Shop"
        if (currentLevel < 15) {
             drawText("Press 'Enter' to enter Upgrade Shop...", WIDTH/2-180, HEIGHT/2 - 45, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);
        } else if (currentLevel == 15 && score < 25000) {
             drawText("Press 'Enter' to face your destiny... (Score < 25000)", WIDTH/2-220, HEIGHT/2 - 45, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);
        } else if (currentLevel == 15 && score >= 25000) {
             drawText("Press 'Enter' to face your destiny... (Score >= 25000)", WIDTH/2-220, HEIGHT/2 - 45, GLUT_BITMAP_HELVETICA_18, 1, 1, 0);
        }
    } else if (gameState == LOSE) {
        drawText("=== OBLITERATED ===", WIDTH/2-120, HEIGHT/2+45, GLUT_BITMAP_TIMES_ROMAN_24, 1, 0.2f, 0.2f);
        sprintf(hud, "Survived: %.1fs | Score: %d | Kills: %d", gameTime, score, car.kills);
        drawText(hud, WIDTH/2-160, HEIGHT/2, GLUT_BITMAP_HELVETICA_18, 1, 0.7f, 0.7f);

        // FIX: Consolidated "LOSE" text to one block
        // NEW: Penalty starts at 50 and goes up
        int penalty = 0;
        if (currentLevel == 16) {
            penalty = 5000;
        } else if (currentLevel > 1) {
            penalty = 50 * currentLevel;
        }

        if (penalty > 0) {
            sprintf(hud, "Press 'Enter' to restart level... (-%d Score)", penalty);
            drawText(hud, WIDTH/2-160, HEIGHT/2 - 45, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);
            if (currentLevel == 16 && score < penalty) {
                drawText("INSUFFICIENT SCORE! (Game Over)", WIDTH/2-160, HEIGHT/2 - 90, GLUT_BITMAP_HELVETICA_18, 1, 0.5f, 0.5f);
            }
        } else {
            drawText("Press 'Enter' to restart level...", WIDTH/2-140, HEIGHT/2 - 45, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);
        }
    } else if (gameState == FINAL_WIN) {
        drawText("=== LEGENDARY VICTORY ===", WIDTH/2-150, HEIGHT/2+45, GLUT_BITMAP_TIMES_ROMAN_24, 0, 1, 0);
        sprintf(hud, "YOU HAVE CONQUERED THE APOCALYPSE!");
        drawText(hud, WIDTH/2-220, HEIGHT/2, GLUT_BITMAP_HELVETICA_18, 1, 1, 0.8f);
        sprintf(hud, "Final Score: %d | Max Combo: x%d", score, maxCombo);
        drawText(hud, WIDTH/2-160, HEIGHT/2 - 45, GLUT_BITMAP_HELVETICA_18, 1, 1, 0.8f);
        drawText("Press 'Enter' to play again from Level 1...", WIDTH/2-180, HEIGHT/2 - 90, GLUT_BITMAP_HELVETICA_18, 1, 1, 1);
    } else if (gameState == SECRET_WIN) {
        // --- NEW: Artistic Win Screen ---
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, WIDTH, 0, HEIGHT);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

        // Pulsing side bars
        float pulse = 0.5f + sin(anim * 0.1f) * 0.5f;
        glColor4f(1.0f, 0.2f, 0.1f, 0.3f * pulse); // Fiery red, pulsing alpha

        float barWidth = 50;
        float barY = HEIGHT/2 - 150;
        float barHeight = 300;

        // Left Bar
        glBegin(GL_QUADS);
        glVertex2f(WIDTH/2 - 300, barY);
        glVertex2f(WIDTH/2 - 300 + barWidth, barY);
        glVertex2f(WIDTH/2 - 300 + barWidth, barY + barHeight);
        glVertex2f(WIDTH/2 - 300, barY + barHeight);
        glEnd();

        // Right Bar
        glBegin(GL_QUADS);
        glVertex2f(WIDTH/2 + 300 - barWidth, barY);
        glVertex2f(WIDTH/2 + 300, barY);
        glVertex2f(WIDTH/2 + 300, barY + barHeight);
        glVertex2f(WIDTH/2 + 300 - barWidth, barY + barHeight);
        glEnd();

        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_LIGHTING);
        // --- End Artistic Win Screen ---

        drawText("=== SECRET ENDING: SURVIVED ===", WIDTH/2-200, HEIGHT/2+45, GLUT_BITMAP_TIMES_ROMAN_24, 1, 0.5f, 0); // Fiery Orange
        sprintf(hud, "YOU ARE A TRUE LEGEND OF THE WASTELAND.");
        drawText(hud, WIDTH/2-220, HEIGHT/2, GLUT_BITMAP_HELVETICA_18, 1, 0.9f, 0.8f); // Hot White
        sprintf(hud, "Final Score: %d | Max Combo: x%d", score, maxCombo);
        drawText(hud, WIDTH - 450, hudY, GLUT_BITMAP_HELVETICA_18, 1, 0.9f, 0.8f); // Hot White
        drawText("You are a true legend. Press 'Enter' to play again...", WIDTH/2-180, HEIGHT/2 - 90, GLUT_BITMAP_HELVETICA_18, 1, 1, 1); // White
    }

    // NEW: Updated controls
    drawText("Arrows: Drive | Space: Turbo | S: Shield | D: Fly | B: Blink | V: Shockwave (10HP) | ESC: Quit", 15, 25, GLUT_BITMAP_9_BY_15, 0.9f, 0.9f, 0.9f);

    if (screenFlash > 0) {
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, WIDTH, 0, HEIGHT);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glColor4f(1, 0.8f, 0.35f, screenFlash);
        glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(WIDTH, 0);
        glVertex2f(WIDTH, HEIGHT); glVertex2f(0, HEIGHT);
        glEnd();
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_LIGHTING);
    }
    if (car.damage_flash > 0) {
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, WIDTH, 0, HEIGHT);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glColor4f(1, 0.25f, 0.25f, car.damage_flash * 0.4f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(WIDTH, 0);
        glVertex2f(WIDTH, HEIGHT); glVertex2f(0, HEIGHT);
        glEnd();
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_LIGHTING);
    }
    if (lightningTimer > 0) {
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, WIDTH, 0, HEIGHT);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glColor4f(0.9f, 0.95f, 1.0f, lightningTimer * 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(WIDTH, 0);
        glVertex2f(WIDTH, HEIGHT); glVertex2f(0, HEIGHT);
        glEnd();
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_LIGHTING);
    }

    glutSwapBuffers();
}

void update(int v) {
    if (gameState == PLAYING) {
        float localTimeWarp = 1.0f;
        if(car.timeRiftTimer > 0) localTimeWarp = 0.4f;
        float finalTimeWarp = timeWarpEffect * localTimeWarp;

        gameTime += 0.016f * finalTimeWarp;
        anim += 3.2f * finalTimeWarp;
        if (anim > 360) anim -= 360;

        // Level-specific timers
        if (currentLevel == 16) {
            if (survivalTimer > 0) {
                survivalTimer -= 0.016f * finalTimeWarp;
                alienSpawnTimer -= 0.016f * finalTimeWarp;

                // NEW: ALIEN INVASION MADNESS (Rain from sky)
                if (alienSpawnTimer <= 0) {
                    alienSpawnTimer = 0.2f - (gameTime / 90.0f) * 0.18f; // From 5/s to 10/s

                    // Predictive aiming
                    float predictTime = 0.5f + (rand() % 5) / 10.0f; // 0.5 to 1.0s prediction
                    float targetX = car.x + car.vx * (1.0f/0.016f) * predictTime + (rand() % 20 - 10);
                    float targetZ = car.z + car.vz * (1.0f/0.016f) * predictTime + (rand() % 20 - 10);

                    // Clamp to arena
                    targetX = fmax(-arenaSize, fmin(arenaSize, targetX));
                    targetZ = fmax(-arenaSize, fmin(arenaSize, targetZ));

                    // NEW: Fast green/purple alien projectiles
                    addParticle(targetX, 60.0f, targetZ, 0, -2.5f, 0, 0.5f, 1.0f, 0.2f, 0.6f, 40.0f, 2);
                }

                if (survivalTimer <= 0) {
                    gameState = SECRET_WIN;
                    score += 10000;
                }
            }
        } else if (currentLevel < 15) {
             if (penaltyTimer > 0) {
                penaltyTimer -= 0.016f * finalTimeWarp;
                if (penaltyTimer <= 0) {
                    applyPenaltyTeleport();
                }
            }
        }

        if (difficulty >= 7 && currentLevel != 16) {
            meteorTimer -= 0.016f * finalTimeWarp;
            if (meteorTimer <= 0) {
                meteorTimer = 15.0f + (rand() % 10);
            }
            if (meteorTimer > 0 && meteorTimer < 8.0f) {
                if(rand() % 5 == 0) {
                    float spawnRange = arenaSize - 10.0f;
                    float x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                    float z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                    addParticle(x, 60.0f, z, 0, -1.2f, 0, 1.0f, 0.5f, 0.2f, 0.5f, 50.0f, 2); // Type 2 = Meteor
                }
            }
        }

        if (fmod(gameTime, 15.0f) < 0.02f) {
            if (difficulty > 5 && !nightMode && rand() % 100 < 30) {
                nightMode = true;
                glClearColor(0.02f, 0.03f, 0.08f, 1);
            }
            if (difficulty > 8 && rand() % 100 < 25) {
                earthquake = !earthquake;
            }
            if (rand() % 100 < 20) {
                lightningTimer = 0.8f;
            }
        }

        // Update ALL Buff/Debuff timers
        if (car.reverseGravityTimer > 0) car.reverseGravityTimer -= 0.016f;
        if (car.confusionTimer > 0) car.confusionTimer -= 0.016f;
        if (car.ghostTimer > 0) car.ghostTimer -= 0.016f;
        if (car.timeRiftTimer > 0) car.timeRiftTimer -= 0.016f;
        if (car.empTimer > 0) car.empTimer -= 0.016f;
        if (car.oilSlickTimer > 0) car.oilSlickTimer -= 0.016f;

        if (car.boost_timer > 0) car.boost_timer -= 0.016f;
        if (car.shield_timer > 0) car.shield_timer -= 0.016f;
        if (car.invuln_timer > 0) car.invuln_timer -= 0.016f;
        else car.invincible = false;

        // NEW: Cooldowns
        if (car.shield_cooldown > 0) car.shield_cooldown -= 0.016f * finalTimeWarp;
        if (car.shockwave_cooldown > 0) car.shockwave_cooldown -= 0.016f * finalTimeWarp;
        if (car.fly_timer > 0) car.fly_timer -= 0.016f * finalTimeWarp;
        if (car.fly_cooldown > 0) car.fly_cooldown -= 0.016f * finalTimeWarp;


        if (car.damage_flash > 0) car.damage_flash -= 0.05f;
        if (cameraShake > 0) cameraShake -= 0.05f;
        if (screenFlash > 0) screenFlash -= 0.04f;
        if (lightningTimer > 0) lightningTimer -= 0.08f;

        if (gameTime - car.lastHitTime > 3.0f) { comboCounter = 0; }
        if (car.combo > 10 && !car.superMode) {
            car.superMode = true;
            createExplosion(car.x, car.y, car.z, 2.5f);
        }
        if (car.combo <= 0) car.superMode = false;
        if (car.superMode) car.combo -= 0.008f;

        for (int i = 0; i < particleCount; i++) {
            if (particles[i].life > 0) {
                particles[i].x += particles[i].vx * finalTimeWarp;
                particles[i].y += particles[i].vy * finalTimeWarp;
                particles[i].z += particles[i].vz * finalTimeWarp;

                float gravity = 0.014f * (car.reverseGravityTimer > 0 ? -0.5f : 1.0f);
                if(particles[i].type != 2) { // Meteors/Bombs have their own gravity
                     particles[i].vy -= gravity * finalTimeWarp;
                }
                particles[i].life -= 0.012f * finalTimeWarp;

                // Meteor/Bomb impact
                if(particles[i].type == 2 && particles[i].y <= 0.5f) {
                    createMeteorImpact(particles[i].x, 0.5f, particles[i].z);
                    // NEW: Check car Y-level for ground impact and increase Lvl 16 damage
                    if(checkCollision(car.x, car.z, 1.5f, particles[i].x, particles[i].z, 3.0f) && car.y < 3.0f) {

                        // Check for invincibility/shields
                        if (!car.invincible && car.invuln_timer <= 0 && car.shield_timer <= 0) {
                            float damage = (currentLevel == 16) ? 30.0f : 15.0f; // More damage on Lvl 16
                            car.health -= damage;
                            car.damage_flash = 1.0f;
                            car.invuln_timer = 0.5f; // Short invuln after hit

                            if (car.health <= 0) {
                                gameState = LOSE;
                                createExplosion(car.x, car.y, car.z, 3.0f);
                            }
                        }
                    }
                    particles[i].life = 0;
                }
            }
        }

        // NEW: Car Y-position physics (Fly ability)
        if (car.fly_timer > 0) {
            // Fly up
            car.y = fmin(15.0f, car.y + 0.2f * finalTimeWarp); // Fly up to 15 units
        } else if (car.reverseGravityTimer > 0) {
            car.y = fmin(12.0f, car.y + 0.08f * finalTimeWarp); // Float up
        } else {
            if (car.y > 0.5f) car.y -= 0.12f * finalTimeWarp; // Fall down
            else car.y = 0.5f;
        }

        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (powerups[i].active) {
                powerups[i].timer -= 0.016f * finalTimeWarp;
                if (powerups[i].timer <= 0) powerups[i].active = false;

                if (checkCollision(car.x, car.z, 1.0f, powerups[i].x, powerups[i].z, 0.6f)) {
                    if (powerups[i].type == 0) { car.health = fmin(100 + car.healthBonus, car.health + 30); score += 15; } // Use health bonus
                    else if (powerups[i].type == 1) { car.boost_timer = 3.0f; score += 10; }
                    else if (powerups[i].type == 2) { car.invincible = true; car.invuln_timer = 5.0f; score += 25; }
                    else if (powerups[i].type == 3) { timeWarpEffect = 0.5f; score += 20; }
                    else if (powerups[i].type == 4) { car.reverseGravityTimer = 10.0f; score += 30; }
                    else if (powerups[i].type == 5) { car.ghostTimer = 8.0f; score += 35; }
                    else if (powerups[i].type == 6) { car.timeRiftTimer = 6.0f; score += 40; }

                    createExplosion(powerups[i].x, powerups[i].y, powerups[i].z, 0.8f);
                    powerups[i].active = false;
                }
            }
        }

        if (rand() % (800 - difficulty * 30) == 0 && currentLevel < 15) {
            float spawnRange = arenaSize - 10.0f;
            spawnPowerUp((rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange,
                         (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange);
        }

        timeWarpEffect = fmin(1.0f, timeWarpEffect + 0.005f);

        float boundary = arenaSize - 9.0f;

        // Update Dinos (Level 15)
        if (currentLevel == 15) {
            for(int i = 0; i < MAX_DINOS; i++) {
                Dinosaur* d = &dinos[i];
                if(!d->active) continue;
                d->anim_timer += 0.016f * finalTimeWarp;

                float distToCar = distance(d->x, d->z, car.x, car.z);
                if(distToCar < 80.0f) d->state = 1; // Chase (more aggressive)
                if(distToCar > 100.0f) d->state = 0; // Idle

                if (d->state == 1) { // Chasing
                    float dx = car.x - d->x;
                    float dz = car.z - d->z;
                    d->angle = atan2(dx, dz) * 180.0f / PI;

                    float speed;
                    if (d->size > 2.5f) { // T-Rex
                        speed = 0.08f; // Slower
                    } else { // Raptor
                        speed = 0.15f; // Faster
                    }

                    d->x += (dx/distToCar) * speed * finalTimeWarp;
                    d->z += (dz/distToCar) * speed * finalTimeWarp;
                }

                // Dino collision with car
                if(checkCollision(car.x, car.z, 1.0f, d->x, d->z, d->size * 1.2f)) {
                    if (!car.invincible && car.invuln_timer <= 0 && car.shield_timer <= 0) {
                        float damage = (d->size > 2.5f) ? 50.0f : 25.0f; // T-Rex hurts!
                        car.health -= damage;
                        car.damage_flash = 1.0f;
                        car.invuln_timer = 0.5f;
                        // NEW: Added death check
                        if (car.health <= 0) {
                            gameState = LOSE;
                            createExplosion(car.x, car.y, car.z, 3.0f);
                        }
                    }
                }
            }

            // NEW: Update Flying Dinos
            for(int i = 0; i < MAX_FLYING_DINOS; i++) {
                FlyingDinosaur* d = &flyingDinos[i];
                if(!d->active) continue;

                d->anim_timer += 0.016f * finalTimeWarp;
                float distToCar = distance(d->x, d->z, car.x, car.z);

                if (d->state == 0) { // Hovering
                    d->x += (rand() % 20 - 10) / 100.0f; // Drift
                    d->z += (rand() % 20 - 10) / 100.0f;
                    if (d->y < 18.0f) d->y += 0.05f * finalTimeWarp; // Float up

                    // NEW: Increased aggression range & FIXED bug
                    // Now they will attack you *especially* if you are flying
                    if (distToCar < 80.0f) {
                        d->state = 1;
                    }
                } else if (d->state == 1) { // Diving
                    float dx = car.x - d->x;
                    float dz = car.z - d->z;
                    d->angle = atan2(dx, dz) * 180.0f / PI;

                    // NEW: More aggressive tracking
                    d->x += (dx/distToCar) * 0.3f * finalTimeWarp; // Dive speed
                    d->z += (dz/distToCar) * 0.3f * finalTimeWarp;
                    d->y -= 0.15f * finalTimeWarp; // Fall faster

                    // Hit check
                    if (checkCollision(car.x, car.z, 1.0f, d->x, d->z, d->size) && fabs(car.y - d->y) < 2.0f) {
                         if (!car.invincible && car.invuln_timer <= 0 && car.shield_timer <= 0) {
                            car.health -= 20.0f;
                            car.damage_flash = 1.0f;
                            car.invuln_timer = 0.5f;
                            // NEW: Added death check
                            if (car.health <= 0) {
                                gameState = LOSE;
                                createExplosion(car.x, car.y, car.z, 3.0f);
                            }
                         }
                         d->state = 0; // Return to hovering
                         d->y = 20.0f; // Reset height
                    }

                    // Missed, pull up
                    if (d->y < 1.0f) {
                        d->state = 0;
                    }
                }
            }
        }

        // NEW: Update Alien Ships (Level 16)
        if (currentLevel == 16) {
            for(int i = 0; i < MAX_ALIEN_SHIPS; i++) {
                AlienShip* s = &alienShips[i];
                if(!s->active) continue;

                s->anim_timer += 0.016f * finalTimeWarp;
                s->attack_timer -= 0.016f * finalTimeWarp;

                // --- AI Logic ---
                float distToCar = distance(s->x, s->z, car.x, car.z);
                float dx = car.x - s->x;
                float dz = car.z - s->z;
                s->angle = atan2(dx, dz) * 180.0f / PI;

                // Follow car
                float speed = 0.15f; // NEW: Faster
                if (distToCar > 10.0f) { // Move in
                    s->x += (dx/distToCar) * speed * finalTimeWarp;
                    s->z += (dz/distToCar) * speed * finalTimeWarp;
                } else { // Strafe
                    s->x -= (dz/distToCar) * speed * 0.5f * finalTimeWarp;
                    s->z += (dx/distToCar) * speed * 0.5f * finalTimeWarp;
                }

                // Bobbing
                s->y = 20.0f + sin(s->anim_timer * 0.2f + i) * 3.0f;

                // Attack Logic
                if(s->attack_timer <= 0) {
                    if (s->type == 0) { // Laser
                        s->attack_timer = 1.5f; // NEW: Faster cooldown
                        // Predictive shot
                        float predictTime = 0.3f;
                        float targetX = car.x + car.vx * (1.0f/0.016f) * predictTime;
                        float targetZ = car.z + car.vz * (1.0f/0.016f) * predictTime;
                        float pdx = targetX - s->x;
                        float pdz = targetZ - s->z;
                        // NEW: Aim at car's Y level
                        float pdy = car.y - s->y;
                        float pDist = sqrt(pdx*pdx + pdz*pdz + pdy*pdy);
                        // Fire 10 fast particles
                        for(int p=0; p<10; p++) {
                            addParticle(s->x, s->y, s->z, (pdx/pDist) * 1.5f, (pdy/pDist) * 1.5f, (pdz/pDist) * 1.5f,
                                0.2f, 1.0f, 0.3f, 0.2f, 2.0f, 0); // Green laser
                        }
                    } else if (s->type == 1) { // Bomber
                        s->attack_timer = 2.5f; // NEW: Faster cooldown
                        // NEW: Predictive drop
                        float predictTime = 0.8f;
                        float targetX = car.x + car.vx * (1.0f/0.016f) * predictTime;
                        float targetZ = car.z + car.vz * (1.0f/0.016f) * predictTime;

                        addParticle(targetX, 60.0f, targetZ, 0, -2.5f, 0, 1.0f, 0.2f, 0.5f, 0.6f, 40.0f, 2);
                    } else { // NEW: Type 2 (Spread Shot)
                        s->attack_timer = 2.0f; // 2s cooldown
                        float pdy = car.y - s->y;
                        float pDist = sqrt(dx*dx + dz*dz + pdy*pdy);

                        // Fire 5 particles in a spread
                        for(int p=-2; p<=2; p++) {
                            float spreadAngle = p * 0.1f; // 0.1 rad spread
                            float pvx = (dx/pDist) * cos(spreadAngle) - (dz/pDist) * sin(spreadAngle);
                            float pvz = (dx/pDist) * sin(spreadAngle) + (dz/pDist) * cos(spreadAngle);

                            addParticle(s->x, s->y, s->z, pvx * 1.2f, (pdy/pDist) * 1.2f, pvz * 1.2f,
                                1.0f, 1.0f, 0.2f, 0.2f, 2.0f, 0); // Yellow laser
                        }
                    }
                }

                // Ship collision with car (if car is flying)
                if(checkCollision(car.x, car.z, 1.0f, s->x, s->z, s->size * 1.5f) && fabs(car.y - s->y) < 3.0f) {
                    if (!car.invincible && car.invuln_timer <= 0 && car.shield_timer <= 0) {
                        car.health -= 40.0f;
                        car.damage_flash = 1.0f;
                        car.invuln_timer = 0.5f;
                        if (car.health <= 0) {
                            gameState = LOSE;
                            createExplosion(car.x, car.y, car.z, 3.0f);
                        }
                    }
                    s->health -= 50; // Ramming hurts ship too
                    if(s->health <= 0) {
                         s->active = false;
                         createExplosion(s->x, s->y, s->z, 2.0f);
                         car.kills++;
                         score += 250;
                    }
                }
            }
        }


        for (int i = 0; i < numObjects; i++) {
            if (!objects[i].active) continue;

            float objTimeWarp = finalTimeWarp;
            if(car.timeRiftTimer > 0 && distance(car.x, car.z, objects[i].x, objects[i].z) < 20.0f) {
                objTimeWarp = finalTimeWarp * 0.3f;
            }

            objects[i].rotY += 2.2f * objTimeWarp;

            if (objects[i].type == 1) {
                objects[i].x += objects[i].vx * objTimeWarp;
                objects[i].z += objects[i].vz * objTimeWarp;
                if (objects[i].x < -boundary || objects[i].x > boundary) objects[i].vx *= -1;
                if (objects[i].z < -boundary || objects[i].z > boundary) objects[i].vz *= -1;
            } else if (objects[i].type == 5) {
                float dx = car.x - objects[i].x;
                float dz = car.z - objects[i].z;
                float dist = sqrt(dx*dx + dz*dz);
                if (dist < 8 + difficulty) objects[i].isAngry = true;
                if (dist > 0.1f && dist < 22 + difficulty * 2) {
                    float speed = objects[i].isAngry ? 0.18f : 0.11f;
                    speed *= (0.85f + difficulty * 0.05f) * objTimeWarp;
                    objects[i].x += (dx/dist) * speed;
                    objects[i].z += (dz/dist) * speed;
                }
                objects[i].x = fmax(-boundary, fmin(boundary, objects[i].x));
                objects[i].z = fmax(-boundary, fmin(boundary, objects[i].z));
            } else if (objects[i].type == 8) {
                objects[i].timer += 0.08f * objTimeWarp;
                objects[i].vy = sin(objects[i].timer) * 0.8f;
                objects[i].y = 0.75f + fabs(objects[i].vy);
            } else if (objects[i].type == 9) {
                objects[i].timer += 0.016f * objTimeWarp;
                float fireTime = fmax(0.5f, 2.0f - difficulty * 0.1f);
                if (objects[i].timer > fireTime) {
                    objects[i].timer = 0;
                    float dx, dz, dist;
                    if(difficulty < 10) {
                        dx = car.x - objects[i].x;
                        dz = car.z - objects[i].z;
                        dist = sqrt(dx*dx + dz*dz);
                    } else {
                        float predictTime = 0.5f;
                        float targetX = car.x + car.vx * (1.0f/0.016f) * predictTime;
                        float targetZ = car.z + car.vz * (1.0f/0.016f) * predictTime;
                        dx = targetX - objects[i].x;
                        dz = targetZ - objects[i].z;
                        dist = sqrt(dx*dx + dz*dz);
                    }
                    if (dist < 18 + difficulty && !car.invincible) {
                        for (int j = 0; j < 15; j++) {
                            addParticle(objects[i].x, objects[i].y + 0.5f, objects[i].z,
                                        (dx/dist) * 0.4f, 0, (dz/dist) * 0.4f,
                                        1, 0.2f, 0.2f, 0.15f, 0.6f, 0);
                        }
                    }
                }
            }
        }

        // Update Target (Level 15)
        if (currentLevel == 15) {
            for (int i=0; i<3; i++) {
                GameObject* t = &targets[i];
                if (!t->active) continue;

                if (t->teleportCooldown > 0) t->teleportCooldown -= 0.016f * finalTimeWarp;

                float distToTarget = distance(car.x, car.z, t->x, t->z);
                // NEW: Target is aware of you from farther away AND if you are flying
                float aggroRange = (car.fly_timer > 0) ? 60.0f : 30.0f;
                aggroRange += difficulty * 2.0f;

                if (distToTarget < aggroRange && distToTarget > t->radius + 1.2f) {
                    float dx = t->x - car.x;
                    float dz = t->z - car.z;
                    float dist = sqrt(dx*dx + dz*dz);
                    // NEW: Target is MUCH faster
                    float escapeSpeed = 0.6f * (0.75f + difficulty * 0.05f) * finalTimeWarp;
                    if (distToTarget < aggroRange * 0.6f) escapeSpeed *= 3.5f;

                    t->x += (dx/dist) * escapeSpeed;
                    t->z += (dz/dist) * escapeSpeed;

                    if (distToTarget < aggroRange * 0.8f) {
                        // NEW: Randomly teleport!
                        if (t->teleportCharges > 0 && t->teleportCooldown <= 0 && (rand() % 500) < 2) { // 0.4% chance
                            t->teleportCharges--;
                            t->teleportCooldown = 5.0f; // 5s cooldown
                            createExplosion(t->x, t->y, t->z, 1.5f);
                            float spawnRange = arenaSize - 10.0f;
                            t->x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                            t->z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                            t->y = 1.0f; // Reset to ground
                            t->vy = 0;
                            createExplosion(t->x, t->y, t->z, 1.5f);
                            // continue; // <-- FIX: REMOVED THIS ILLEGAL 'continue'
                        } else {
                            // NEW: Target flies up MUCH faster
                            t->vy = fmin(15.0f, t->vy + 1.2f);
                        }
                    }
                }
                // NEW: Gravity logic
                if (t->y > 1.0f) {
                    t->vy -= 0.05f * finalTimeWarp; // Gravity is stronger
                } else {
                    t->vy = 0;
                    t->y = 1.0f;
                }
                t->y += t->vy * finalTimeWarp; // Apply velocity

                t->x = fmax(-boundary, fmin(boundary, t->x));
                t->z = fmax(-boundary, fmin(boundary, t->z));
            }
        }
        // Update Target (Normal Levels)
        else if (currentLevel < 15 && target.active) {
            if (target.teleportCooldown > 0) target.teleportCooldown -= 0.016f * finalTimeWarp;

            float distToTarget = distance(car.x, car.z, target.x, target.z);
            // NEW: Target is aware of you from farther away AND if you are flying
            float aggroRange = (car.fly_timer > 0) ? 60.0f : 30.0f;
            aggroRange += difficulty * 2.0f;

            if (distToTarget < aggroRange && distToTarget > target.radius + 1.2f) {
                float dx = target.x - car.x;
                float dz = target.z - car.z;
                float dist = sqrt(dx*dx + dz*dz);
                // NEW: Target is MUCH faster
                float escapeSpeed = 0.6f * (0.75f + difficulty * 0.05f) * finalTimeWarp;
                if (distToTarget < aggroRange * 0.6f) escapeSpeed *= 3.5f;

                target.x += (dx/dist) * escapeSpeed;
                target.z += (dz/dist) * escapeSpeed;

                // NEW: Floating logic (only Lvl 7+)
                if (currentLevel >= 7 && distToTarget < aggroRange * 0.8f) {
                    // NEW: Randomly teleport!
                    if (target.teleportCharges > 0 && target.teleportCooldown <= 0 && (rand() % 500) < 2) { // 0.4% chance
                        target.teleportCharges--;
                        target.teleportCooldown = 5.0f; // 5s cooldown
                        createExplosion(target.x, target.y, target.z, 1.5f);
                        float spawnRange = arenaSize - 10.0f;
                        target.x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                        target.z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                        target.y = 1.0f; // Reset to ground
                        target.vy = 0;
                        createExplosion(target.x, target.y, target.z, 1.5f);
                    } else {
                        // NEW: Target flies up MUCH faster
                        target.vy = fmin(15.0f, target.vy + 1.2f);
                    }
                }
            }
            // NEW: Gravity logic
            if (target.y > 1.0f) {
                target.vy -= 0.05f * finalTimeWarp; // Gravity is stronger
            } else {
                target.vy = 0;
                target.y = 1.0f;
            }
            target.y += target.vy * finalTimeWarp; // Apply velocity

            target.x = fmax(-boundary, fmin(boundary, target.x));
            target.z = fmax(-boundary, fmin(boundary, target.z));
        }


        float baseSpeed = ACCELERATION + car.accelerationBonus; // Apply upgrade
        if (car.boost_timer > 0) baseSpeed *= 2.5f;
        if (car.superMode) baseSpeed *= 1.5f;

        // NEW: Finicky turbo
        float turboFumble = (car.boost_timer > 0) ? (0.6f + sin(anim * 0.5f) * 0.4f) : 1.0f;
        float turn = (TURN_SPEED + car.turnSpeedBonus) * (1.0f + car.speed * 0.5f) * (car.reverseGravityTimer > 0 ? 0.8f : 1.0f) * turboFumble; // Apply upgrade
        float friction = (car.reverseGravityTimer > 0) ? 0.98f : FRICTION;
        // NEW: Fly ability reduces friction
        if (car.fly_timer > 0) {
            friction = 0.99f; // Almost no friction
        }

        if(car.oilSlickTimer > 0) {
            turn *= 0.1f;
            car.angle += (rand() % 20 - 10) * 0.1f * finalTimeWarp;
        }

        float turnDir = (car.confusionTimer > 0) ? -1.0f : 1.0f;

        if (keyStates[GLUT_KEY_LEFT]) {
            car.angle += turn * finalTimeWarp * turnDir;
            car.drifting = car.speed > 0.3f && car.fly_timer <= 0; // No drifting in air
            car.drift_angle = -1.0f * turnDir;
        }
        if (keyStates[GLUT_KEY_RIGHT]) {
            car.angle -= turn * finalTimeWarp * turnDir;
            car.drifting = car.speed > 0.3f && car.fly_timer <= 0; // No drifting in air
            car.drift_angle = 1.0f * turnDir;
        }
        if (!keyStates[GLUT_KEY_LEFT] && !keyStates[GLUT_KEY_RIGHT]) {
            car.drifting = false;
            car.drift_angle = 0;
        }

        if (keyStates[GLUT_KEY_UP]) {
            car.speed += baseSpeed * finalTimeWarp;
            float maxSpd = MAX_SPEED * (car.boost_timer > 0 ? 2.0f : 1.0f) * (car.superMode ? 1.4f : 1.0f);
            if (car.speed > maxSpd) car.speed = maxSpd;
        }
        if (keyStates[GLUT_KEY_DOWN]) {
            car.speed *= BRAKE_POWER;
        }

        car.speed *= friction;
        float angle = car.angle * PI / 180.0f;
        car.vx = sin(angle) * car.speed * finalTimeWarp;
        car.vz = cos(angle) * car.speed * finalTimeWarp;
        car.x += car.vx;
        car.z += car.vz;
        car.x = fmax(-boundary, fmin(boundary, car.x));
        car.z = fmax(-boundary, fmin(boundary, car.z));

        if (car.speed > 0.1f && rand() % 3 == 0 && car.fly_timer <= 0) { // No dust while flying
             addParticle(car.x - sin(angle) * 1.0f, car.y-0.2f, car.z - cos(angle) * 1.0f,
                        (rand() % 20 - 10) / 130.0f, 0.018f, (rand() % 20 - 10) / 130.0f,
                        0.5f, 0.5f, 0.6f, 0.16f, 0.4f, 0);
        }

        for (int i = 0; i < numObjects; i++) {
            if (!objects[i].active) continue;

            if (car.ghostTimer > 0 && objects[i].type != 4) {
                 continue;
            }

            // NEW: Don't check object collision while flying high
            if (car.fly_timer > 0 && car.y > 3.0f) {
                continue;
            }

            if (checkCollision(car.x, car.z, 1.0f, objects[i].x, objects[i].z, objects[i].radius)) {
                if (!car.invincible && car.invuln_timer <= 0) {
                    float damage = 10.0f;
                    bool destroyed = false;

                    if (objects[i].type == 4) {
                        damage = 38.0f; createExplosion(objects[i].x, objects[i].y, objects[i].z, 2.0f);
                        objects[i].active = false; destroyed = true; score -= 15;
                    } else if (objects[i].type == 3) {
                        damage = 24.0f; createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.4f);
                        objects[i].revealed = true; score -= 22;
                    } else if (objects[i].type == 5) {
                        if (car.superMode) {
                            objects[i].health -= 100;
                            if (objects[i].health <= 0) {
                                damage = 0; createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.6f);
                                objects[i].active = false; destroyed = true; car.kills++; score += 50;
                                comboCounter++; if (comboCounter > maxCombo) maxCombo = comboCounter;
                                car.combo += 2; car.lastHitTime = gameTime;
                                spawnPowerUp(objects[i].x, objects[i].z);
                            }
                        } else {
                            damage = 32.0f; createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.6f);
                            objects[i].active = false; destroyed = true; score -= 10;
                        }
                    } else if (objects[i].type == 6) {
                        float spawnRange = arenaSize - 10.0f;
                        car.x = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                        car.z = (rand() % (int)(spawnRange * 20)) / 10.0f - spawnRange;
                        createExplosion(car.x, car.y, car.z, 1.0f);
                        damage = 10.0f; score -= 7;
                    } else if (objects[i].type == 7) {
                        damage = 20.0f; float pushAngle = objects[i].rotY * PI / 180.0f;
                        car.x += cos(pushAngle) * 3.0f; car.z += sin(pushAngle) * 3.0f; score -= 8;
                    } else if (objects[i].type == 8) {
                        damage = 16.0f; car.y += 2.0f;
                        createExplosion(objects[i].x, objects[i].y, objects[i].z, 0.9f); score -= 6;
                    } else if (objects[i].type == 9) {
                        damage = 12.0f; score -= 5;
                    } else if (objects[i].type == 10) {
                        damage = 15.0f; car.confusionTimer = 5.0f + difficulty * 0.2f;
                        createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.2f);
                        objects[i].active = false; destroyed = true; score -= 20;
                    } else if (objects[i].type == 11) {
                        damage = 5.0f; car.empTimer = 6.0f + difficulty * 0.3f;
                        createExplosion(objects[i].x, objects[i].y, objects[i].z, 1.8f);
                        objects[i].active = false; destroyed = true; score -= 25;
                    } else if (objects[i].type == 12) {
                        damage = 0.0f; car.oilSlickTimer = 4.0f + difficulty * 0.1f; score -= 10;
                    } else {
                        createExplosion(objects[i].x, objects[i].y, objects[i].z, 0.8f); score -= 3;
                    }

                    if (damage > 0) {
                        if (car.shield_timer > 0) {
                            car.shield_timer = 0; damage = 0; score += 5;
                            car.shield_cooldown = fmax(1.0f, 10.0f - car.shieldCooldownReduction); // Use upgrade
                        } else {
                            car.health -= damage;
                            car.damage_flash = 1.0f;
                            car.invuln_timer = 0.5f;
                            hits++;
                            comboCounter = 0;
                            car.combo = 0;
                        }

                        if (!destroyed) {
                            float dx = car.x - objects[i].x;
                            float dz = car.z - objects[i].z;
                            float dist = sqrt(dx*dx + dz*dz);
                            if (dist > 0.1f) {
                                car.x += (dx/dist) * 1.5f;
                                car.z += (dz/dist) * 1.5f;
                            }
                        }

                        if (car.health <= 0) {
                            gameState = LOSE;
                            createExplosion(car.x, car.y, car.z, 3.0f);
                            messageTimer = 0;
                        }
                    }
                }
            }
        }

        // Win condition check
        if (currentLevel == 15) {
            for(int i=0; i<3; i++) {
                if(targets[i].active) {
                    float distToTarget3D = sqrt(pow(car.x - targets[i].x, 2) + pow(car.y - targets[i].y, 2) + pow(car.z - targets[i].z, 2));
                    if (distToTarget3D < (1.0f + targets[i].radius)) {
                        targets[i].active = false;
                        targetsRemaining--;
                        createExplosion(targets[i].x, targets[i].y, targets[i].z, 2.8f);
                        score += 500;
                    }
                }
            }
            if (targetsRemaining <= 0) {
                gameState = WIN;
                score += 5000;
            }
        } else if (currentLevel < 15) {
            float distToTarget3D = sqrt(pow(car.x - target.x, 2) + pow(car.y - target.y, 2) + pow(car.z - target.z, 2));
            if (distToTarget3D < (1.0f + target.radius)) {
                gameState = WIN;
                float timeBonus = 1000.0f / (1.0f + gameTime);
                score += (int)(timeBonus + 250 + car.kills * 20 - hits * 10 + maxCombo * 15);
                createExplosion(target.x, target.y, target.z, 2.8f);
                messageTimer = 0;
            }
        }

        if ((int)(gameTime * 10) % 10 == 0) {
            score += difficulty * 2;
        }

    } // <-- FIX: Added closing brace for 'if (gameState == PLAYING)'

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
} // <-- FIX: Added closing brace for 'void update(int v)'

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);

    // NEW: Handle Upgrade Shop
    if (gameState == UPGRADE_SHOP) {
        if (key == '1' && score >= costHealth) {
            score -= costHealth;
            car.healthBonus += 20;
            costHealth = (int)(costHealth * 1.5f);
        } else if (key == '2' && score >= costAccel) {
            score -= costAccel;
            car.accelerationBonus += 0.005f;
            costAccel = (int)(costAccel * 1.8f);
        } else if (key == '3' && score >= costTurn) {
            score -= costTurn;
            car.turnSpeedBonus += 0.2f;
            costTurn = (int)(costTurn * 1.8f);
        } else if (key == '4' && score >= costShield) {
            score -= costShield;
            car.shieldCooldownReduction += 0.5f;
            costShield = (int)(costShield * 1.5f);
        } else if (key == '5' && score >= costFly) {
            score -= costFly;
            car.flyCooldownReduction += 0.5f;
            costFly = (int)(costFly * 1.5f);
        } else if (key == 13) { // 'Enter'
            currentLevel++;
            initLevel(currentLevel);
            gameState = PLAYING;
        }
        glutPostRedisplay(); // Redraw shop UI
        return;
    }


    if (key == 13) { // 'Enter'
        if (gameState == WIN) {
            if (currentLevel < 15) {
                gameState = UPGRADE_SHOP; // Go to shop
            } else if (currentLevel == 15) {
                if (score >= 25000) { // NEW: Secret level check
                    gameState = UPGRADE_SHOP; // Go to shop before final level
                } else {
                    gameState = FINAL_WIN; // Not enough score, normal win
                }
            } else {
                 // This case should be WIN on level 16
                 gameState = SECRET_WIN;
            }

        } else if (gameState == LOSE) {
            // NEW: Score Penalty Logic (starts at 50)
            int penalty = 0;
            if (currentLevel == 16) {
                penalty = 5000;
            } else if (currentLevel > 1) {
                penalty = 50 * currentLevel;
            }

            if (score >= penalty) {
                score -= penalty;
                initLevel(currentLevel); // Restart current level
                gameState = PLAYING;
            } else if (currentLevel == 16) {
                // On Lvl 16 and can't pay: Game Over, boot to L1
                currentLevel = 1;
                initLevel(currentLevel); // This will reset score and upgrades
                gameState = PLAYING;
            } else {
                // On normal level and can't pay: restart for free
                initLevel(currentLevel);
                gameState = PLAYING;
            }
        } else if (gameState == FINAL_WIN || gameState == SECRET_WIN) {
            currentLevel = 1;
            initLevel(currentLevel); // This will reset score and upgrades
            gameState = PLAYING;
        }
    }

    if (gameState != PLAYING) return;

    bool empActive = car.empTimer > 0;

    // NEW: RE-ENABLED Turbo while flying
    if (key == ' ' && car.boost_timer <= 0 && !empActive) {
        car.boost_timer = 2.2f;
        createExplosion(car.x, car.y, car.z, 0.5f);
    }
    // NEW: Cooldown check
    if ((key == 's' || key == 'S') && car.shield_timer <= 0 && car.shield_cooldown <= 0 && !empActive) {
        car.shield_timer = 3.5f;
        car.shield_cooldown = fmax(1.0f, 10.0f - car.shieldCooldownReduction); // Use upgrade
        score -= 10;
    }
    // NEW: Replaced Dash with Fly
    if ((key == 'd' || key == 'D') && car.fly_timer <= 0 && car.fly_cooldown <= 0 && !empActive) {
        float flyTime = (currentLevel == 16) ? 15.0f : 5.0f;
        car.fly_timer = flyTime;
        car.fly_cooldown = flyTime + fmax(1.0f, 5.0f - car.flyCooldownReduction); // Use upgrade
        createExplosion(car.x, car.y, car.z, 0.8f);
        score -= 2;
    }
    if ((key == 'b' || key == 'B') && !empActive && car.health > 10) {
        float angle = car.angle * PI / 180.0f;
        createExplosion(car.x, car.y, car.z, 0.4f);
        car.x += sin(angle) * 8.0f;
        car.z += cos(angle) * 8.0f;
        createExplosion(car.x, car.y, car.z, 0.6f);
        car.health -= 5;
        score -= 1;
    }
    // NEW: Shockwave cost and cooldown
    if ((key == 'v' || key == 'V') && !empActive && car.health > 10 && car.shockwave_cooldown <= 0) {
        createShockwave(car.x, car.z);
        car.health -= 10; // Cost 10 HP
        car.shockwave_cooldown = 15.0f; // 15s cooldown
    }
}

void specialKeys(int key, int x, int y) { keyStates[key] = true; }
void specialKeysUp(int key, int x, int y) { keyStates[key] = false; }

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Cars From Hell - Apocalypse Edition");

    // Init car struct once
    car = (Car){0, 0.5f, -60.f, 0, 0, 0, 0, 100.0f, 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    initLevel(currentLevel); // This will set score to 0 and reset upgrades
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}
