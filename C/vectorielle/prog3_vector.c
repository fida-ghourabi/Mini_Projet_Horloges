#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT3 6003
#define PORT1 6001
#define PORT2 6002
#define PORT4 6004
#define MON_ID 3
#define NB_PROC 4

const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;

int vector_clock[NB_PROC] = {0};

typedef struct {
    int sender_id;
    int vector[NB_PROC];
} Message;

void print_vector(const int *v) {
    printf("[");
    for (int i = 0; i < NB_PROC; i++) {
        printf("%d", v[i]);
        if (i < NB_PROC - 1) printf(", ");
    }
    printf("]");
}

void display_clock(const char *event) {
    printf("[P%d] %s | Horloge vectorielle : ", MON_ID, event);
    print_vector(vector_clock);
    printf("\n");
}

void update_on_receive(Message msg) {
    printf("[P%d] Recu de P%d | Vecteur recu : ", MON_ID, msg.sender_id);
    print_vector(msg.vector);
    printf("\n");

    for (int i = 0; i < NB_PROC; i++) {
        if (vector_clock[i] < msg.vector[i]) {
            vector_clock[i] = msg.vector[i];
        }
    }
    vector_clock[MON_ID - 1]++;

    printf("[P%d] Nouvelle horloge : ", MON_ID);
    print_vector(vector_clock);
    printf("\n");
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

    vector_clock[MON_ID - 1]++;

    Message msg;
    msg.sender_id = MON_ID;
    for (int i = 0; i < NB_PROC; i++) {
        msg.vector[i] = vector_clock[i];
    }

    printf("[P%d] Envoi a P%d | Horloge envoyee : ", MON_ID, dest_id);
    print_vector(msg.vector);
    printf("\n");

    send(sock, (char*)&msg, sizeof(msg), 0);
    closesocket(sock);
  
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
    server_addr.sin_port = htons(PORT3);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Erreur bind : %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_socket, 4) == SOCKET_ERROR) {
        printf("Erreur listen : %d\n", WSAGetLastError());
        return 1;
    }

    printf("[P%d] Serveur pret sur le port %d\n", MON_ID, PORT3);

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
    WSAStartup(MAKEWORD(2,2), &wsa);

    CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    Sleep(1000);

    vector_clock[MON_ID - 1]++;
    display_clock("Evenement local 1 : Affichage");

    vector_clock[MON_ID - 1]++;
    int x = 7; x--;
    display_clock("Evenement local 2 : Decrementation");

    vector_clock[MON_ID - 1]++;
    time_t t = time(NULL);
    printf("[P%d] Heure systeme : %s", MON_ID, ctime(&t));
    display_clock("Evenement local 3 : Heure systeme");

    vector_clock[MON_ID - 1]++;
    int y = x * x;
    display_clock("Evenement local 4 : Carre");

    vector_clock[MON_ID - 1]++;
    Sleep(1000);
    display_clock("Evenement local 5 : Pause 1s");

    send_message(PORT1, 1); Sleep(200);
    send_message(PORT2, 2); Sleep(200);
    send_message(PORT4, 4); Sleep(200);
    send_message(PORT2, 2);

    Sleep(5000);
    stop = 1;
    closesocket(server_socket);
    Sleep(200);
    WSACleanup();

    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}
