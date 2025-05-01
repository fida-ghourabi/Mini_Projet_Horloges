#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 700
#define IDC_START 101
#define IDC_STOP 102
#define IDC_SEND 103
#define IDC_RESET 104
#define PORT_GUI 6005

HINSTANCE hInst;
SOCKET gui_socket;
HWND hwndClocks[4], hwndEventLists[4], hwndGlobalLog;
HWND hwndLabels[4];
int scalar_clocks[4] = {0, 0, 0, 0};
char event_logs[4][1024] = {0};
char global_log[4096] = {0};

typedef struct {
    int sender;
    int receiver;
    int clock;
} MessageEvent;
MessageEvent message_events[100];
int message_count = 0;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void StartServer(void);
DWORD WINAPI HandleConnections(LPVOID param);
void ProcessSocketData(char *data);
void UpdateGUI(int pid, const char *event, int clock);
void DrawMessages(HDC hdc);

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

    if (strcmp(type, "UPDATE") == 0) {
        char *pid_str = strtok(NULL, ":");
        if (pid_str == NULL) return;

        char *last_colon = strrchr(data, ':');
        if (last_colon == NULL) return;
        *last_colon = '\0';
        last_colon++;
        int clock = atoi(last_colon);

        char *event_start = pid_str + strlen(pid_str) + 1;
        if (event_start >= last_colon) return;

        int pid = atoi(pid_str);
        UpdateGUI(pid, event_start, clock);
    } else if (strcmp(type, "MSG") == 0) {
        char *sender_str = strtok(NULL, ":");
        char *receiver_str = strtok(NULL, ":");
        char *clock_str = strtok(NULL, ":");
        if (sender_str && receiver_str && clock_str) {
            int sender = atoi(sender_str);
            int receiver = atoi(receiver_str);
            int clock = atoi(clock_str);
            if (message_count < 100) {
                message_events[message_count].sender = sender;
                message_events[message_count].receiver = receiver;
                message_events[message_count].clock = clock;
                message_count++;
            }
            char msg[256];
            sprintf(msg, "[P%d] Sent to P%d | Clock: %d", sender, receiver, clock);
            UpdateGUI(sender, msg, clock);
            InvalidateRect(GetActiveWindow(), NULL, TRUE);
        }
    }
}

void UpdateGUI(int pid, const char *event, int clock) {
    if (pid < 1 || pid > 4) return;

    scalar_clocks[pid - 1] = clock;

    char clock_text[32];
    sprintf(clock_text, "Clock: %d", scalar_clocks[pid - 1]);
    SetWindowText(hwndClocks[pid - 1], clock_text);

    char event_text[256];
    sprintf(event_text, "%s | Clock: %d", event, scalar_clocks[pid - 1]);
    strcat(event_logs[pid - 1], event_text);
    strcat(event_logs[pid - 1], "\n");
    SendMessage(hwndEventLists[pid - 1], LB_ADDSTRING, 0, (LPARAM)event_text);

    strcat(global_log, event_text);
    strcat(global_log, "\n");
    SendMessage(hwndGlobalLog, LB_ADDSTRING, 0, (LPARAM)event_text);

    InvalidateRect(GetActiveWindow(), NULL, TRUE);
}

void DrawMessages(HDC hdc) {
    int process_positions[4] = {50, 250, 450, 650};
    int base_y = 220;

    // Draw background for Message Flow section
    HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 245));
    RECT msg_flow_rect = {10, 190, 890, 290};
    FillRect(hdc, &msg_flow_rect, hBrush);
    DeleteObject(hBrush);

    HPEN border_pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    SelectObject(hdc, border_pen);
    Rectangle(hdc, 10, 190, 890, 290);
    DeleteObject(border_pen);

    HPEN timeline_pen = CreatePen(PS_SOLID, 2, RGB(0, 128, 0));
    SelectObject(hdc, timeline_pen);
    for (int i = 0; i < 4; i++) {
        MoveToEx(hdc, process_positions[i], 200, NULL);
        LineTo(hdc, process_positions[i], 280);
    }
    DeleteObject(timeline_pen);

    HPEN arrow_pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));
    SelectObject(hdc, arrow_pen);
    for (int i = 0; i < message_count; i++) {
        int sender = message_events[i].sender - 1;
        int receiver = message_events[i].receiver - 1;
        int y_pos = base_y + (i * 10) % 60;

        MoveToEx(hdc, process_positions[sender], y_pos, NULL);
        LineTo(hdc, process_positions[receiver], y_pos);

        if (sender < receiver) {
            MoveToEx(hdc, process_positions[receiver] - 10, y_pos - 5, NULL);
            LineTo(hdc, process_positions[receiver], y_pos);
            MoveToEx(hdc, process_positions[receiver] - 10, y_pos + 5, NULL);
            LineTo(hdc, process_positions[receiver], y_pos);
        } else {
            MoveToEx(hdc, process_positions[receiver] + 10, y_pos - 5, NULL);
            LineTo(hdc, process_positions[receiver], y_pos);
            MoveToEx(hdc, process_positions[receiver] + 10, y_pos + 5, NULL);
            LineTo(hdc, process_positions[receiver], y_pos);
        }
    }
    DeleteObject(arrow_pen);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBrushBtn = NULL;
    static HBRUSH hBrushBg = NULL;
    static COLORREF label_colors[4] = {RGB(255, 99, 71), RGB(60, 179, 113), RGB(106, 90, 205), RGB(255, 215, 0)};

    switch (msg) {
    case WM_CREATE: {
        hBrushBtn = CreateSolidBrush(RGB(70, 130, 180));
        hBrushBg = CreateSolidBrush(RGB(240, 240, 240));

        for (int i = 0; i < 4; i++) {
            char label[10];
            sprintf(label, "P%d", i + 1);
            hwndLabels[i] = CreateWindow("STATIC", label, WS_VISIBLE | WS_CHILD | SS_CENTER,
                                         10 + i * 220, 10, 200, 25, hwnd, NULL, hInst, NULL);
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                     DEFAULT_PITCH | FF_SWISS, "Arial");
            SendMessage(hwndLabels[i], WM_SETFONT, (WPARAM)hFont, TRUE);

            hwndClocks[i] = CreateWindow("STATIC", "Clock: 0", WS_VISIBLE | WS_CHILD | SS_CENTER,
                                         10 + i * 220, 40, 200, 25, hwnd, NULL, hInst, NULL);
            SendMessage(hwndClocks[i], WM_SETFONT, (WPARAM)hFont, TRUE);

            hwndEventLists[i] = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOSEL | WS_VSCROLL,
                                             10 + i * 220, 70, 200, 120, hwnd, NULL, hInst, NULL);
        }

        HWND hwndMsgFlow = CreateWindow("STATIC", "Message Flow", WS_VISIBLE | WS_CHILD | SS_CENTER,
                                        10, 200, 860, 25, hwnd, NULL, hInst, NULL);
        HFONT hFontTitle = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                      DEFAULT_PITCH | FF_SWISS, "Arial");
        SendMessage(hwndMsgFlow, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

        CreateWindow("BUTTON", "Start All", WS_VISIBLE | WS_CHILD | BS_FLAT,
                     10, 300, 120, 30, hwnd, (HMENU)IDC_START, hInst, NULL);
        CreateWindow("BUTTON", "Stop All", WS_VISIBLE | WS_CHILD | BS_FLAT,
                     140, 300, 120, 30, hwnd, (HMENU)IDC_STOP, hInst, NULL);
        CreateWindow("BUTTON", "Send Message", WS_VISIBLE | WS_CHILD | BS_FLAT,
                     270, 300, 120, 30, hwnd, (HMENU)IDC_SEND, hInst, NULL);
        CreateWindow("BUTTON", "Reset Clocks", WS_VISIBLE | WS_CHILD | BS_FLAT,
                     400, 300, 120, 30, hwnd, (HMENU)IDC_RESET, hInst, NULL);

        hwndGlobalLog = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOSEL | WS_VSCROLL,
                                     10, 340, 860, 350, hwnd, NULL, hInst, NULL);
        StartServer();
        break;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(70, 130, 180));
        return (LRESULT)hBrushBtn;
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
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_START:
            system("start prog1.exe");
            system("start prog2.exe");
            system("start prog3.exe");
            system("start prog4.exe");
            break;
        case IDC_STOP: {
            SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == INVALID_SOCKET) break;
            SOCKADDR_IN server;
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr("127.0.0.1");
            for (int port = 6001; port <= 6004; port++) {
                server.sin_port = htons(port);
                if (connect(sock, (SOCKADDR*)&server, sizeof(server)) == 0) {
                    send(sock, "CONTROL:STOP", 12, 0);
                }
                closesocket(sock);
                sock = socket(AF_INET, SOCK_STREAM, 0);
            }
            break;
        }
        case IDC_RESET:
            for (int i = 0; i < 4; i++) {
                scalar_clocks[i] = 0;
                char clock_text[32];
                sprintf(clock_text, "Clock: %d", scalar_clocks[i]);
                SetWindowText(hwndClocks[i], clock_text);
                SendMessage(hwndEventLists[i], LB_RESETCONTENT, 0, 0);
                event_logs[i][0] = '\0';
            }
            SendMessage(hwndGlobalLog, LB_RESETCONTENT, 0, 0);
            global_log[0] = '\0';
            message_count = 0;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDC_SEND:
            break;
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HBRUSH hBgBrush = CreateSolidBrush(RGB(240, 240, 240));
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, hBgBrush);
        DeleteObject(hBgBrush);
        DrawMessages(hdc);
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