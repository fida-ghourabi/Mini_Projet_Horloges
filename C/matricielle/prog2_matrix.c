#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT2 6002
#define PORT1 6001
#define PORT3 6003
#define PORT4 6004
#define PORT_GUI 6005
#define MON_ID 2
#define NB_PROC 4

const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;

int matrix_clock[NB_PROC][NB_PROC] = {0};

typedef struct {
    int sender_id;
    int matrix[NB_PROC][NB_PROC];
} Message;

void send_to_gui(const char *update) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return;

    SOCKADDR_IN server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_GUI);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) {
        closesocket(sock);
        return;
    }

    send(sock, update, strlen(update), 0);
    closesocket(sock);
}

void print_matrix(int matrix[NB_PROC][NB_PROC]) {
    for (int i = 0; i < NB_PROC; i++) {
        printf("[");
        for (int j = 0; j < NB_PROC; j++) {
            printf("%d", matrix[i][j]);
            if (j < NB_PROC - 1) printf(", ");
            else printf("]");
        }
        printf("\n");
    }
}

void display_clock(const char *event) {
    printf("[P%d] %s\nHorloge matricielle :\n", MON_ID, event);
    print_matrix(matrix_clock);
    printf("\n");

    char update[512];
    char matrix_str[4][32];
    for (int i = 0; i < NB_PROC; i++) {
        sprintf(matrix_str[i], "[%d, %d, %d, %d]", 
                matrix_clock[i][0], matrix_clock[i][1], matrix_clock[i][2], matrix_clock[i][3]);
    }
    sprintf(update, "UPDATE:%d:%s:%s:%s:%s:%s", 
            MON_ID, event, matrix_str[0], matrix_str[1], matrix_str[2], matrix_str[3]);
    send_to_gui(update);
}

void update_on_receive(Message msg) {
    printf("[P%d] Recu de P%d | Matrice recue :\n", MON_ID, msg.sender_id);
    print_matrix(msg.matrix);

    for (int j = 0; j < NB_PROC; j++) {
        for (int k = 0; k < NB_PROC; k++) {
            if (matrix_clock[j][k] < msg.matrix[j][k]) {
                matrix_clock[j][k] = msg.matrix[j][k];
            }
        }
    }

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    printf("[P%d] Nouvelle horloge :\n", MON_ID);
    print_matrix(matrix_clock);
    printf("\n");

    char update[512];
    char matrix_str[4][32];
    for (int i = 0; i < NB_PROC; i++) {
        sprintf(matrix_str[i], "[%d, %d, %d, %d]", 
                matrix_clock[i][0], matrix_clock[i][1], matrix_clock[i][2], matrix_clock[i][3]);
    }
    sprintf(update, "UPDATE:%d:Received from P%d:%s:%s:%s:%s", 
            MON_ID, msg.sender_id, matrix_str[0], matrix_str[1], matrix_str[2], matrix_str[3]);
    send_to_gui(update);
}

void send_message(int port, int dest_id) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Erreur socket : %d\n", WSAGetLastError());
        return;
    }

    SOCKADDR_IN server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(IP);

    int try_count = 0;
    while (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR && try_count < 5) {
        Sleep(500);
        try_count++;
    }

    if (try_count == 5) {
        printf("[P%d] Connexion echouee au port %d (code %d)\n", MON_ID, port, WSAGetLastError());
        closesocket(sock);
        return;
    }

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    matrix_clock[MON_ID - 1][dest_id - 1]++;

    Message msg;
    msg.sender_id = MON_ID;
    for (int i = 0; i < NB_PROC; i++)
        for (int j = 0; j < NB_PROC; j++)
            msg.matrix[i][j] = matrix_clock[i][j];

    printf("[P%d] Envoi a P%d | Matrice envoyee :\n", MON_ID, dest_id);
    print_matrix(msg.matrix);
    printf("\n");

    send(sock, (char*)&msg, sizeof(msg), 0);
    closesocket(sock);

    char gui_msg[256];
    sprintf(gui_msg, "MSG:%d:%d", MON_ID, dest_id);
    send_to_gui(gui_msg);
}

DWORD WINAPI receive_thread(LPVOID lpParam) {
    SOCKADDR_IN server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    Message msg;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Erreur creation socket serveur : %d\n", WSAGetLastError());
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT2);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Erreur bind : %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_socket, 4) == SOCKET_ERROR) {
        printf("Erreur listen : %d\n", WSAGetLastError());
        return 1;
    }

    printf("[P%d] Serveur pret sur le port %d\n", MON_ID, PORT2);

    while (!stop) {
        SOCKET client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            if (stop) break;
            continue;
        }
        recv(client_socket, (char*)&msg, sizeof(msg), 0);
        update_on_receive(msg);
        closesocket(client_socket);
    }

    closesocket(server_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    Sleep(1000);

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    display_clock("Evenement local 1 : Affichage");

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    int x = 10; x += 2;
    display_clock("Evenement local 2 : Incrementation");

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    time_t t = time(NULL);
    printf("[P%d] Heure systeme : %s", MON_ID, ctime(&t));
    display_clock("Evenement local 3 : Heure systeme");

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    int y = x * 3;
    display_clock("Evenement local 4 : Multiplication");

    matrix_clock[MON_ID - 1][MON_ID - 1]++;
    Sleep(1000);
    display_clock("Evenement local 5 : Pause 1s");

    send_message(PORT1, 1); Sleep(200);
    send_message(PORT3, 3); Sleep(200);
    send_message(PORT4, 4); Sleep(200);
    send_message(PORT1, 1);

    Sleep(5000);
    stop = 1;
    closesocket(server_socket);
    Sleep(200);
    WSACleanup();

    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}