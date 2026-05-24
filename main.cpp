#include <windows.h>
#include <profileapi.h>
#include <vector>
#include <cmath>

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

std::vector<Position> positions;
std::vector<Velocity> velocities;
std::vector<Size> sizes;

std::vector<Position> targets;

void UpdateMovement(float delta);
void AddNPC(Position pos, Velocity vel, Size size);
void UpdatePathfinding();

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

    AddNPC({200.0f, 100.0f}, {0.0f, 0.0f}, {40.0f, 40.0f});
    AddNPC({100.0f, 100.0f}, {10000.0f, 5000.0f}, {40.0f, 40.0f});
    targets.push_back({600.0f, 500.0f});
    
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
            counter = now;
            InvalidateRect(hWnd, NULL, FALSE);
            UpdateMovement(delta);
            UpdatePathfinding();
        }
    }

    return 0;
}

void AddNPC(Position pos, Velocity vel, Size size){
    positions.push_back(pos);
    velocities.push_back(vel);
    sizes.push_back(size);
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

void UpdatePathfinding(){
    if(targets.size() != 0 && positions.size() != 0){
        float dx = targets[0].x - positions[0].x;
        float dy = targets[0].y - positions[0].y;
        float longueur = sqrt(dx*dx + dy*dy);

        if(longueur > 5.0f){
            velocities[0].vx = (dx / longueur) * 10.0f;
            velocities[0].vy = (dy / longueur) * 10.0f;
        } else {
            velocities[0].vx = 0.0f;
            velocities[0].vy = 0.0f;
        }
    }

    for(int i = 0; i < positions.size(); i++){
        for(int j = i + 1; j < positions.size(); j++){
            CheckCollision(i, j);
        }
    }
}