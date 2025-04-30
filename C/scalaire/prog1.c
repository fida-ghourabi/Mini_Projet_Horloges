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

int scalar_clock = 0;
const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;  // utilisé pour fermer proprement le serveur

void display_clock(const char *event) {
    printf("[Processus 1] %s | Horloge scalaire : %d\n", event, scalar_clock);
}

void update_on_receive(int received_clock) {
    scalar_clock = (scalar_clock > received_clock ? scalar_clock : received_clock) + 1;
    display_clock("Reception de message");
}

void send_message(int port, int message) {
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

    send(sock, (char*)&message, sizeof(int), 0);
    closesocket(sock);
    display_clock("Envoi de message");
    scalar_clock++;
}

DWORD WINAPI receive_thread(LPVOID lpParam) {
    SOCKADDR_IN server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    int buffer;

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

    printf("[Processus 1] Serveur pret sur le port %d\n", PORT1);

    while (!stop) {
        SOCKET client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            if (stop) break;  // arrêt normal
            int err = WSAGetLastError();
            printf("Erreur accept : %d\n", err);
            continue;
        }
        recv(client_socket, (char*)&buffer, sizeof(int), 0);
        update_on_receive(buffer);
        closesocket(client_socket);
    }

    closesocket(server_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    Sleep(1000); // Attente pour laisser le serveur démarrer

    scalar_clock++;
    display_clock("Evenement local 1 : Affichage");

    scalar_clock++;
    int x = 4;
    x++;
    display_clock("Evenement local 2 : Incrementation");

    scalar_clock++;
    time_t t = time(NULL);
    printf("Heure actuelle : %s", ctime(&t));
    display_clock("Evenement local 3 : Heure systeme");

    scalar_clock++;
    int y = x + 5;
    display_clock("Evenement local 4 : Addition");

    scalar_clock++;
    Sleep(1000);
    display_clock("Evenement local 5 : Pause 1s");

    send_message(PORT2, scalar_clock);
    Sleep(200);
    send_message(PORT3, scalar_clock);
    Sleep(200);
    send_message(PORT4, scalar_clock);
    Sleep(200);
    send_message(PORT2, scalar_clock);

    Sleep(5000);
    stop = 1;
    closesocket(server_socket);  // Interruption propre du accept()
    Sleep(200);
    WSACleanup();
    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}
