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
#define MON_ID 1

int scalar_clock = 0;
const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;

typedef struct {
    int sender_id;
    int scalar_clock;
} Message;

void display_clock(const char *event) {
    printf("[P%d] %s | Horloge scalaire : %d\n", MON_ID, event, scalar_clock);
}

void update_on_receive(Message msg) {
    scalar_clock = (scalar_clock > msg.scalar_clock ? scalar_clock : msg.scalar_clock) + 1;
    printf("[P%d] Recu de P%d | Horloge recue : %d -> Nouvelle horloge : %d\n",
           MON_ID, msg.sender_id, msg.scalar_clock, scalar_clock);
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

    // Préparer et afficher le message AVANT envoi
    Message msg;
    msg.sender_id = MON_ID;
    msg.scalar_clock = scalar_clock;

    printf("[P%d] Envoi a P%d | Horloge scalaire envoyee : %d\n", MON_ID, dest_id, scalar_clock);

    send(sock, (char*)&msg, sizeof(msg), 0);
    scalar_clock++;  // Mise à jour locale après l’envoi

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

    printf("[P%d] Serveur en ecoute sur le port %d\n", MON_ID, PORT1);

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
    Sleep(1000); // Attendre que tous les serveurs soient prêts

    scalar_clock++;
    display_clock("Evenement local 1 : Affichage");

    scalar_clock++;
    int x = 4; x++;
    display_clock("Evenement local 2 : Incrementation");

    scalar_clock++;
    time_t t = time(NULL);
    printf("[P%d] Heure systeme : %s", MON_ID, ctime(&t));
    display_clock("Evenement local 3 : Heure systeme");

    scalar_clock++;
    int y = x + 5;
    display_clock("Evenement local 4 : Addition");

    scalar_clock++;
    Sleep(1000);
    display_clock("Evenement local 5 : Pause 1s");

    // Envois
    send_message(PORT2, 2); Sleep(200);
    send_message(PORT3, 3); Sleep(200);
    send_message(PORT4, 4); Sleep(200);
    send_message(PORT2, 2);

    Sleep(5000); // Temps pour réceptionner tous les messages
    stop = 1;
    closesocket(server_socket);
    Sleep(200);
    WSACleanup();

    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}
