#include <windows.h>
#include <profileapi.h>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

HWND hWnd;

struct Position {
    float x;
    float y;
};

struct Velocity {
    float vx;
    float vy;
};

struct Size {
    float length;
    float height;
};

struct Personality {
    float open;
    float cons;
    float extr;
    float agre;
    float nevr;
};

struct Mood {
    float soci;
};

struct Activity{
    bool talk;
};

struct CurrentTasks {
    std::vector<Activity> activities;
};

std::vector<Position> positions;
std::vector<Velocity> velocities;
std::vector<Size> sizes;
std::vector<Personality> personalities;
std::vector<Mood> moods;
std::vector<CurrentTasks> currentTasks;

std::mt19937 rng(std::random_device{}());
std::normal_distribution<float> random_vx(0.0f, 40.0f);
std::normal_distribution<float> random_vy(0.0f, 40.0f);
std::uniform_real_distribution<float> random_positive_negative_percent(-1.0f, 1.0f);

void UpdateMovement(float delta);
void AddNPC(Position pos, Velocity vel, Size size, Personality pers, Mood mood, CurrentTasks currTasks);
void UpdatePathfinding(float delta);
void SociabilityChange(int i, float delta);
int FindClosest(int i);
void Wander(int i, float delta);

LRESULT CALLBACK Wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch (uMsg){
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                HDC hdcMem = CreateCompatibleDC(hdc);

                RECT rc;
                GetClientRect(hWnd, &rc);
                HBITMAP hBitmap = CreateCompatibleBitmap(hdcMem, rc.right, rc.bottom);
                
                SelectObject(hdcMem, hBitmap);

                FillRect(hdcMem, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

                for(int i = 0; i < positions.size(); i++){
                    Rectangle(hdcMem, (int) positions[i].x, (int) positions[i].y, (int) positions[i].x + (int) sizes[i].length, (int) positions[i].y + (int) sizes[i].height);
                }                

                BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
                DeleteObject(hBitmap);
                DeleteDC(hdcMem);

                EndPaint(hWnd, &ps);
            }
            return 0;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow){
    WNDCLASS cl;
    cl.style = 0;
    cl.lpfnWndProc = Wndproc;
    cl.cbClsExtra = 0;
    cl.cbWndExtra = 0;
    cl.hInstance = hInstance;
    cl.hIcon = 0;
    cl.hCursor = LoadCursor(NULL, IDC_ARROW);
    cl.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    cl.lpszMenuName = 0;
    cl.lpszClassName = "Test";
    RegisterClass(&cl);

    hWnd = CreateWindowA("Test", "Boing", WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    std::normal_distribution<float> stat(0.5f, 0.15f);
    std::uniform_real_distribution<float> dist_x(0.0f, 760.0f);
    std::uniform_real_distribution<float> dist_y(0.0f, 560.0f);
    for(int i = 0; i < 5; i++){                                                     // 3 positions sur x
        for(int j = 0; j < 3; j++){                                                 // 2 positions sur y
            Personality p;
            p.open = std::clamp(stat(rng), 0.0f, 1.0f);
            p.cons = std::clamp(stat(rng), 0.0f, 1.0f);
            p.extr = std::clamp(stat(rng), 0.0f, 1.0f);
            p.agre = std::clamp(stat(rng), 0.0f, 1.0f);
            p.nevr = std::clamp(stat(rng), 0.0f, 1.0f);

            float pos_x = dist_x(rng);
            float pos_y = dist_y(rng);

            Activity act;
            act.talk = false;
            std::vector<Activity> activities;
            activities.push_back(act);
            CurrentTasks currTasks;
            currTasks.activities = activities;
            AddNPC({pos_x, pos_y}, {random_vx(rng), random_vx(rng)}, {40.0f, 40.0f}, p, {p.extr}, currTasks);
        }
    }

    // Boucle de jeu
    while(true){
        MSG msg;

        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if(msg.message == WM_QUIT){
                break;
            }
        } else {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            float delta = (float) (now.QuadPart - counter.QuadPart) / frequency.QuadPart;
            SetWindowTextA(hWnd, std::to_string(delta).c_str());
            counter = now;
            InvalidateRect(hWnd, NULL, FALSE);
            UpdateMovement(delta);
            UpdatePathfinding(delta);
        }
    }

    return 0;
}

void AddNPC(Position pos, Velocity vel, Size size, Personality pers, Mood mood, CurrentTasks currTasks){
    positions.push_back(pos);
    velocities.push_back(vel);
    sizes.push_back(size);
    personalities.push_back(pers);
    moods.push_back(mood);
    currentTasks.push_back(currTasks);
}

bool CheckCollision(int i, int j){
    bool ret = false;
    if(positions[i].x <= positions[j].x + sizes[j].length && positions[i].y <= positions[j].y + sizes[j].height &&
    positions[i].x + sizes[i].length > positions[j].x && positions[i].y + sizes[i].height > positions[j].y){

        float overlapX = std::min(positions[i].x + sizes[i].length, positions[j].x + sizes[j].length) - 
                        std::max(positions[i].x, positions[j].x);
        float overlapY = std::min(positions[i].y + sizes[i].height, positions[j].y + sizes[j].height) - 
                        std::max(positions[i].y, positions[j].y);

        if(overlapX < overlapY){
            float sep = (overlapX + 1.0f) * 0.5f;
            if(positions[i].x < positions[j].x){
                positions[i].x -= sep;
                positions[j].x += sep;
            } else {
                positions[i].x += sep;
                positions[j].x -= sep;
            }
            velocities[i].vx = velocities[i].vx * -1.0f;
            velocities[j].vx = velocities[j].vx * -1.0f;
        } else if(overlapX > overlapY){
            float sep = (overlapY + 1.0f) * 0.5f;
            if(positions[i].y < positions[j].y){
                positions[i].y -= sep;
                positions[j].y += sep;
            } else {
                positions[i].y += sep;
                positions[j].y -= sep;
            }
            velocities[i].vy = velocities[i].vy * -1.0f;
            velocities[j].vy = velocities[j].vy * -1.0f;
        } else {
            float sepX = (overlapX + 1.0f) * 0.5f;
            float sepY = (overlapY + 1.0f) * 0.5f;
            if(positions[i].x < positions[j].x){
                positions[i].x -= sepX;
                positions[j].x += sepX;
            } else {
                positions[i].x += sepX;
                positions[j].x -= sepX;
            }
            if(positions[i].y < positions[j].y){
                positions[i].y -= sepY;
                positions[j].y += sepY;
            } else {
                positions[i].y += sepY;
                positions[j].y -= sepY;
            }
            velocities[i].vx = velocities[i].vx * -1.0f;
            velocities[j].vx = velocities[j].vx * -1.0f;
            velocities[i].vy = velocities[i].vy * -1.0f;
            velocities[j].vy = velocities[j].vy * -1.0f;
        }
        
        ret = true;
    }

    return ret;
}

void UpdateMovement(float delta){
    RECT rc;
    GetClientRect(hWnd, &rc);

    for(int i = 0; i < positions.size(); i++){
        positions[i].x += velocities[i].vx * delta;
        positions[i].y += velocities[i].vy * delta;

        if(positions[i].x <= 0 || positions[i].x >= rc.right - sizes[i].length){
            if(positions[i].x <= 0){
                positions[i].x = 0;
            } else {
                positions[i].x = rc.right - sizes[i].length;
            }
            velocities[i].vx = velocities[i].vx * -1.0f;
        }
        if(positions[i].y <= 0 || positions[i].y >= rc.bottom - sizes[i].height){
            if(positions[i].y <= 0){
                positions[i].y = 0;
            } else {
                positions[i].y = rc.bottom - sizes[i].height;
            }
            velocities[i].vy = velocities[i].vy * -1.0f;
        }
    }

    for(int i = 0; i < positions.size(); i++){
        for(int j = i + 1; j < positions.size(); j++){
            CheckCollision(i, j);
        }
    }
}

void UpdatePathfinding(float delta){
    for(int i = 0; i < positions.size(); i++){
        int j = FindClosest(i);
        float dx = positions[j].x - positions[i].x;
        float dy = positions[j].y - positions[i].y;
        float longueur = sqrt(dx*dx + dy*dy);
        if(moods[i].soci > 0.5f){                           // Si on a encore de la batterie sociale
            if(longueur > 50.0f){                           // Et qu'on n'est pas assez proche du plus proche
                velocities[i].vx = (dx / longueur) * 40.0f; // On se rapproche à une vitesse de 40f
                velocities[i].vy = (dy / longueur) * 40.0f;
                currentTasks[i].activities[0].talk = false;
            } else {                                        // Si on est assez proche
                velocities[i].vx = 0.0f;                    // On s'arrête
                velocities[i].vy = 0.0f;
                currentTasks[i].activities[0].talk = true;
            }
        } else if (moods[i].soci < 0.2f && longueur < 150.0f){                                            // Si on n'a plus de batterie sociale
            velocities[i].vx = (dx / longueur) * -80.0f;    // ON FUIT
            velocities[i].vy = (dy / longueur) * -80.0f;
            currentTasks[i].activities[0].talk = false;
        } else {
            if(longueur > 50.0f){
                Wander(i, delta);
                currentTasks[i].activities[0].talk = false;
            } else {
                velocities[i].vx = 0.0f;
                velocities[i].vy = 0.0f;
                currentTasks[i].activities[0].talk = true;
            }
        }
        SociabilityChange(i, delta);
    }

    /*
    if(targets.size() != 0 && positions.size() != 0){
        targets[0] = positions[1];
        float dx = targets[0].x - positions[0].x;
        float dy = targets[0].y - positions[0].y;
        float longueur = sqrt(dx*dx + dy*dy);

        if(longueur > 5.0f){
            velocities[0].vx = (dx / longueur) * 80.0f;
            velocities[0].vy = (dy / longueur) * 80.0f;
        } else {
            velocities[0].vx = 0.0f;
            velocities[0].vy = 0.0f;
        }
    }

    for(int i = 0; i < positions.size(); i++){
        for(int j = i + 1; j < positions.size(); j++){
            CheckCollision(i, j);
        }
    }*/
}

int FindClosest(int i){
    float minDist = 100000.0f;
    int closest = -1;
    for(int j = 0; j < positions.size(); j++){
        if(j != i){
            float dx = positions[j].x - positions[i].x;
            float dy = positions[j].y - positions[i].y;
            float longueur = sqrt(dx*dx + dy*dy);
            if(longueur < minDist){
                minDist = longueur;
                closest = j;
            }
        }
    }
    return closest;
}

float GetDistance(int i, int j){
    float dx = positions[j].x - positions[i].x;
    float dy = positions[j].y - positions[i].y;
    return sqrt(dx*dx + dy*dy);
}

void SociabilityChange(int i, float delta){
    float extraversion = personalities[i].extr;

    if(currentTasks[i].activities[0].talk){
        moods[i].soci -= 0.1f * personalities[i].extr * delta;
        moods[i].soci = std::clamp(moods[i].soci, 0.0f, personalities[i].extr);
    } else {
        if(moods[i].soci < personalities[i].extr && GetDistance(i, FindClosest(i)) > 100.0f){
            moods[i].soci += 0.1f * personalities[i].extr * delta;
            moods[i].soci = std::clamp(moods[i].soci, 0.0f, personalities[i].extr);
        }
    }
}

void Wander(int i, float delta){
    velocities[i].vx += 1000.0f * delta * random_positive_negative_percent(rng);
    velocities[i].vy += 1000.0f * delta * random_positive_negative_percent(rng);

    float speed = sqrt(velocities[i].vx * velocities[i].vx + velocities[i].vy * velocities[i].vy);

    if(speed > 50.0f) {
        velocities[i].vx = (velocities[i].vx / speed) * 50.0f;
        velocities[i].vy = (velocities[i].vy / speed) * 50.0f;
    } else if(speed < 20.0f){
        velocities[i].vx = (velocities[i].vx / speed) * 20.0f;
        velocities[i].vy = (velocities[i].vy / speed) * 20.0f;
    }
}