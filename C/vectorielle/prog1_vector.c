#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>


#define PORT1 6001
#define PORT2 6002
#define PORT3 6003
#define PORT4 6004
#define MON_ID 1
#define NB_PROC 4

const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;

int vector_clock[NB_PROC] = {0}; // Initialisé à [0,0,0,0]

// Structure du message contenant un vecteur d'horloge
typedef struct {
    int sender_id;
    int vector[NB_PROC];
} Message;

void send_to_gui(const char *message) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return;

    SOCKADDR_IN gui;
    gui.sin_family = AF_INET;
    gui.sin_port = htons(6005); // GUI port
    gui.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (SOCKADDR*)&gui, sizeof(gui)) == 0) {
        send(sock, message, strlen(message), 0);
    }
    closesocket(sock);
}

void log_to_console_and_gui(const char *format, ...) {
    va_list args;
    char buffer[256];

    // Print to console
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // Send to GUI
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    char gui_message[256];
    snprintf(gui_message, sizeof(gui_message), "LOG:%d:%s", MON_ID, buffer);
    send_to_gui(gui_message);
    va_end(args);
}

void print_vector(const int *v) {
    char vector_str[32];
    snprintf(vector_str, sizeof(vector_str), "[%d, %d, %d, %d]", v[0], v[1], v[2], v[3]);
    log_to_console_and_gui("%s", vector_str);
}

void display_clock(const char *event) {
    log_to_console_and_gui("[P%d] %s | Horloge vectorielle : ", MON_ID, event);
    print_vector(vector_clock);
    log_to_console_and_gui("\n");
}

// Mise à jour à la réception : max composant par composant, puis incrément propre case
void update_on_receive(Message msg) {
    log_to_console_and_gui("[P%d] Recu de P%d | Vecteur recu : ", MON_ID, msg.sender_id);
    print_vector(msg.vector);
    log_to_console_and_gui("\n");

    for (int i = 0; i < NB_PROC; i++) {
        if (vector_clock[i] < msg.vector[i]) {
            vector_clock[i] = msg.vector[i];
        }
    }
    vector_clock[MON_ID - 1]++; // Incrément propre case après réception

    log_to_console_and_gui("[P%d] Nouvelle horloge : ", MON_ID);
    print_vector(vector_clock);
    log_to_console_and_gui("\n");
}

// Envoi : incrément propre case puis envoyer le vecteur
void send_message(int port, int dest_id) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        log_to_console_and_gui("Erreur socket : %d\n", WSAGetLastError());
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
        log_to_console_and_gui("[P%d] Connexion echouee au port %d (code %d)\n", MON_ID, port, WSAGetLastError());
        closesocket(sock);
        return;
    }

    vector_clock[MON_ID - 1]++; // Incrément avant l'envoi

    Message msg;
    msg.sender_id = MON_ID;
    for (int i = 0; i < NB_PROC; i++) {
        msg.vector[i] = vector_clock[i];
    }

    log_to_console_and_gui("[P%d] Envoi a P%d | Horloge envoyee : ", MON_ID, dest_id);
    print_vector(msg.vector);
    log_to_console_and_gui("\n");

    send(sock, (char*)&msg, sizeof(msg), 0);
    closesocket(sock);
}

DWORD WINAPI receive_thread(LPVOID lpParam) {
    SOCKADDR_IN server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    Message msg;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        log_to_console_and_gui("Erreur creation socket serveur : %d\n", WSAGetLastError());
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT1);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        log_to_console_and_gui("Erreur bind : %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_socket, 4) == SOCKET_ERROR) {
        log_to_console_and_gui("Erreur listen : %d\n", WSAGetLastError());
        return 1;
    }

    log_to_console_and_gui("[P%d] Serveur pret sur le port %d\n", MON_ID, PORT1);

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

    vector_clock[MON_ID - 1]++;
    display_clock("Evenement local 1 : Affichage");

    vector_clock[MON_ID - 1]++;
    int x = 4; x++;
    display_clock("Evenement local 2 : Incrementation");

    vector_clock[MON_ID - 1]++;
    time_t t = time(NULL);
    log_to_console_and_gui("[P%d] Heure systeme : %s", MON_ID, ctime(&t));
    display_clock("Evenement local 3 : Heure systeme");

    vector_clock[MON_ID - 1]++;
    int y = x + 5;
    printf("[P%d] Valeur de y : %d\n", MON_ID, y); // Use y to avoid warning
    display_clock("Evenement local 4 : Addition");

    
    Sleep(1000);
    vector_clock[MON_ID - 1]++;
    display_clock("Evenement local 5 : Pause 1s");

    // Envois
    send_message(PORT2, 2); Sleep(200);
    send_message(PORT3, 3); Sleep(200);
    send_message(PORT4, 4); Sleep(200);
    send_message(PORT2, 2);

    Sleep(5000); // Laisser le temps de recevoir tous les messages
    stop = 1;
    closesocket(server_socket);
    Sleep(200);
    WSACleanup();

    log_to_console_and_gui("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}