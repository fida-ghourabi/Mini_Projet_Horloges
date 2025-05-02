#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 700
#define PORT_GUI 6005
#define IDC_START_ALL 101

HINSTANCE hInst;
SOCKET gui_socket;
HWND hwndEventLists[4];
HWND hwndLabels[4];
HWND hwndStartAll;
char event_logs[4][2048] = {0}; // Increased size for vector clock logs

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void StartServer(void);
DWORD WINAPI HandleConnections(LPVOID param);
void ProcessSocketData(char *data);
void LaunchProcess(const char *exePath);

void StartServer(void) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return;
    }

    gui_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (gui_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        return;
    }

    SOCKADDR_IN server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_GUI);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(gui_socket, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(gui_socket);
        return;
    }

    if (listen(gui_socket, 4) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(gui_socket);
        return;
    }

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HandleConnections, NULL, 0, NULL);
}

DWORD WINAPI HandleConnections(LPVOID param) {
    while (1) {
        SOCKADDR_IN client;
        int addr_len = sizeof(client);
        SOCKET client_socket = accept(gui_socket, (SOCKADDR*)&client, &addr_len);
        if (client_socket == INVALID_SOCKET) continue;

        char buffer[256];
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            ProcessSocketData(buffer);
        }
        closesocket(client_socket);
    }
    return 0;
}

void ProcessSocketData(char *data) {
    char *type = strtok(data, ":");
    if (type == NULL) return;

    if (strcmp(type, "LOG") == 0) {
        char *pid_str = strtok(NULL, ":");
        if (pid_str == NULL) return;

        int pid = atoi(pid_str);
        if (pid < 1 || pid > 4) return;

        char *log_message = pid_str + strlen(pid_str) + 1;
        if (*log_message == '\0') return;

        SendMessage(hwndEventLists[pid - 1], LB_ADDSTRING, 0, (LPARAM)log_message);
        strcat(event_logs[pid - 1], log_message);
        strcat(event_logs[pid - 1], "\n");
        InvalidateRect(GetActiveWindow(), NULL, TRUE);
    }
}

void LaunchProcess(const char *exePath) {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi = { 0 };
    char cmdLine[256];
    snprintf(cmdLine, sizeof(cmdLine), "%s", exePath);

    BOOL success = CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    if (!success) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to start %s: Error %lu\n", exePath, GetLastError());
        MessageBox(NULL, errorMsg, "Error", MB_OK | MB_ICONERROR);
    } else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBrushBg = NULL;
    static COLORREF label_colors[4] = {RGB(255, 99, 71), RGB(60, 179, 113), RGB(106, 90, 205), RGB(255, 215, 0)};
    static HBRUSH hBrushBtn = NULL;
    static int clientWidth = WINDOW_WIDTH;
    static int clientHeight = WINDOW_HEIGHT;

    switch (msg) {
    case WM_CREATE: {
        hBrushBg = CreateSolidBrush(RGB(240, 240, 240));
        hBrushBtn = CreateSolidBrush(RGB(70, 130, 180));

        const char *labels[4] = {"Processus 1", "Processus 2", "Processus 3", "Processus 4"};
        int positions[4][2] = {{0, 0}, {450, 0}, {0, 350}, {450, 350}};

        for (int i = 0; i < 4; i++) {
            hwndLabels[i] = CreateWindow("STATIC", labels[i], WS_VISIBLE | WS_CHILD | SS_CENTER,
                                        positions[i][0] + 10, positions[i][1] + 10, 430, 25, hwnd, NULL, hInst, NULL);
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                     DEFAULT_PITCH | FF_SWISS, "Arial");
            SendMessage(hwndLabels[i], WM_SETFONT, (WPARAM)hFont, TRUE);

            hwndEventLists[i] = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOSEL | WS_VSCROLL,
                                            positions[i][0] + 10, positions[i][1] + 40, 430, 310, hwnd, NULL, hInst, NULL);
        }

        // Add Start All button
        hwndStartAll = CreateWindow("BUTTON", "Start All", WS_VISIBLE | WS_CHILD | BS_FLAT,
                                    10, 630, 120, 30, hwnd, (HMENU)IDC_START_ALL, hInst, NULL);

        StartServer();
        break;
    }
    case WM_SIZE: {
        clientWidth = LOWORD(lParam);
        clientHeight = HIWORD(lParam);

        // Adjust positions based on new client size
        int halfWidth = clientWidth / 2;
        int halfHeight = (clientHeight - 50) / 2; // Leave space for button

        int positions[4][2] = {{0, 0}, {halfWidth, 0}, {0, halfHeight}, {halfWidth, halfHeight}};
        for (int i = 0; i < 4; i++) {
            MoveWindow(hwndLabels[i], positions[i][0] + 10, positions[i][1] + 10, halfWidth - 20, 25, TRUE);
            MoveWindow(hwndEventLists[i], positions[i][0] + 10, positions[i][1] + 40, halfWidth - 20, halfHeight - 50, TRUE);
        }

        // Reposition Start All button at the bottom
        MoveWindow(hwndStartAll, 10, clientHeight - 40, 120, 30, TRUE);
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;

        for (int i = 0; i < 4; i++) {
            if (hwndStatic == hwndLabels[i]) {
                SetTextColor(hdc, label_colors[i]);
                SetBkColor(hdc, RGB(240, 240, 240));
                return (LRESULT)hBrushBg;
            }
        }

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(240, 240, 240));
        return (LRESULT)hBrushBg;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(70, 130, 180));
        return (LRESULT)hBrushBtn;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_START_ALL) {
            LaunchProcess("prog1_vector.exe");
            LaunchProcess("prog2_vector.exe");
            LaunchProcess("prog3_vector.exe");
            LaunchProcess("prog4_vector.exe");
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HBRUSH hBgBrush = CreateSolidBrush(RGB(240, 240, 240));
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, hBgBrush);
        DeleteObject(hBgBrush);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        closesocket(gui_socket);
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DistributedSystemGUI";
    wc.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("DistributedSystemGUI", "Distributed System Monitor", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}