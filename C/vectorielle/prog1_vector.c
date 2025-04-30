#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT1 6001
#define PORT2 6002
#define PORT3 6003
#define PORT4 6004
#define NB_PROC 4

int process_id = 0; // P1 -> index 0
int vector_clock[NB_PROC] = {0};
const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;

void display_vector(const char *event) {
    printf("[P%d] %s | Horloge vectorielle : [", process_id + 1, event);
    for (int i = 0; i < NB_PROC; i++) {
        printf("%d%s", vector_clock[i], (i < NB_PROC - 1) ? ", " : "");
    }
    printf("]\n");
}

void update_on_receive(int received_vector[NB_PROC]) {
    for (int i = 0; i < NB_PROC; i++) {
        if (vector_clock[i] < received_vector[i]) {
            vector_clock[i] = received_vector[i];
        }
    }
    vector_clock[process_id]++;
    display_vector("Reception de message");
}

void send_vector(int port) {
    vector_clock[process_id]++;

    SOCKET sock;
    SOCKADDR_IN server;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Erreur socket : %d\n", WSAGetLastError());
        return;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(IP);

    int try_count = 0;
    while (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR && try_count < 5) {
        Sleep(500);
        try_count++;
    }

    if (try_count == 5) {
        printf("Connexion echouee au port %d apres plusieurs tentatives (code %d)\n", port, WSAGetLastError());
        closesocket(sock);
        return;
    }

    send(sock, (char*)vector_clock, sizeof(int) * NB_PROC, 0);
    closesocket(sock);
    display_vector("Envoi de message");
}

DWORD WINAPI receive_thread(LPVOID lpParam) {
    SOCKADDR_IN server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    int buffer[NB_PROC];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Erreur creation socket serveur : %d\n", WSAGetLastError());
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT1);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Erreur bind : %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_socket, 4) == SOCKET_ERROR) {
        printf("Erreur listen : %d\n", WSAGetLastError());
        return 1;
    }

    printf("[P%d] Serveur pret sur le port %d\n", process_id + 1, PORT1);

    while (!stop) {
        SOCKET client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            if (stop) break;
            printf("Erreur accept : %d\n", WSAGetLastError());
            continue;
        }
        recv(client_socket, (char*)buffer, sizeof(int) * NB_PROC, 0);
        update_on_receive(buffer);
        closesocket(client_socket);
    }

    closesocket(server_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    Sleep(1000); // Assurer que le serveur est pret

    // 5 événements locaux
    vector_clock[process_id]++;
    display_vector("Evenement local 1 : Affichage");

    Sleep(500);
    vector_clock[process_id]++;
    int x = 4; x++;
    display_vector("Evenement local 2 : Incrementation");

    Sleep(500);
    vector_clock[process_id]++;
    time_t t = time(NULL);
    printf("Heure actuelle : %s", ctime(&t));
    display_vector("Evenement local 3 : Heure systeme");

    Sleep(500);
    vector_clock[process_id]++;
    int y = x + 5;
    display_vector("Evenement local 4 : Addition");

    Sleep(1000);
    vector_clock[process_id]++;
    display_vector("Evenement local 5 : Pause 1s");

    // Envois de messages
    send_vector(PORT2); Sleep(200);
    send_vector(PORT3); Sleep(200);
    send_vector(PORT4); Sleep(200);
    send_vector(PORT2); Sleep(200);

    Sleep(5000);
    stop = 1;
    closesocket(server_socket);
    Sleep(200);
    WSACleanup();
    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}
