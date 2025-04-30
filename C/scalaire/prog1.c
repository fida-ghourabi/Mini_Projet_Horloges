#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>
#include <gtk/gtk.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT1 6001
#define PORT2 6002
#define PORT3 6003
#define PORT4 6004

int scalar_clock = 0;
const char *IP = "127.0.0.1";
volatile int stop = 0;
SOCKET server_socket;
GtkWidget *text_view, *clock_label;
GtkTextBuffer *text_buffer;

typedef struct {
    char *text;
    GtkTextBuffer *buffer;
} TextUpdate;

gboolean update_text_view(gpointer data) {
    TextUpdate *update = (TextUpdate *)data;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(update->buffer, &end);
    gtk_text_buffer_insert(update->buffer, &end, update->text, -1);
    g_free(update->text);
    g_free(update);
    return FALSE;
}

gboolean update_clock_label(gpointer data) {
    char clock_text[32];
    snprintf(clock_text, sizeof(clock_text), "Scalar Clock: %d", scalar_clock);
    gtk_label_set_text(GTK_LABEL(clock_label), clock_text);
    return FALSE;
}

void display_clock(const char *event) {
    char msg[256];
    snprintf(msg, sizeof(msg), "[Processus 1] %s | Horloge scalaire : %d\n", event, scalar_clock);
    TextUpdate *update = g_new(TextUpdate, 1);
    update->text = g_strdup(msg);
    update->buffer = text_buffer;
    g_idle_add(update_text_view, update);
    g_idle_add(update_clock_label, NULL);
}

void update_on_receive(int received_clock, const char *client_ip, int client_port) {
    scalar_clock = (scalar_clock > received_clock ? scalar_clock : received_clock) + 1;
    char msg[256];
    snprintf(msg, sizeof(msg), "Reçu de %s:%d, valeur = %d\n", client_ip, client_port, received_clock);
    TextUpdate *update = g_new(TextUpdate, 1);
    update->text = g_strdup(msg);
    update->buffer = text_buffer;
    g_idle_add(update_text_view, update);
    display_clock("Reception de message");
}

void send_message(int port, int message) {
    SOCKET sock;
    SOCKADDR_IN server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Erreur socket : %d\n", WSAGetLastError());
        TextUpdate *update = g_new(TextUpdate, 1);
        update->text = g_strdup(msg);
        update->buffer = text_buffer;
        g_idle_add(update_text_view, update);
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
        char msg[256];
        snprintf(msg, sizeof(msg), "Connexion échouée au port %d (code %d)\n", port, WSAGetLastError());
        TextUpdate *update = g_new(TextUpdate, 1);
        update->text = g_strdup(msg);
        update->buffer = text_buffer;
        g_idle_add(update_text_view, update);
        closesocket(sock);
        return;
    }

    send(sock, (char*)&message, sizeof(int), 0);
    closesocket(sock);
    char msg[256];
    snprintf(msg, sizeof(msg), "Envoyé au port %d, valeur = %d\n", port, message);
    TextUpdate *update = g_new(TextUpdate, 1);
    update->text = g_strdup(msg);
    update->buffer = text_buffer;
    g_idle_add(update_text_view, update);
    display_clock("Envoi de message");
    scalar_clock++;
}

DWORD WINAPI receive_thread(LPVOID lpParam) {
    SOCKADDR_IN server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    int buffer;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Erreur création socket serveur : %d\n", WSAGetLastError());
        TextUpdate *update = g_new(TextUpdate, 1);
        update->text = g_strdup(msg);
        update->buffer = text_buffer;
        g_idle_add(update_text_view, update);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT1);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Erreur bind : %d\n", WSAGetLastError());
        TextUpdate *update = g_new(TextUpdate, 1);
        update->text = g_strdup(msg);
        update->buffer = text_buffer;
        g_idle_add(update_text_view, update);
        return 1;
    }

    if (listen(server_socket, 4) == SOCKET_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Erreur listen : %d\n", WSAGetLastError());
        TextUpdate *update = g_new(TextUpdate, 1);
        update->text = g_strdup(msg);
        update->buffer = text_buffer;
        g_idle_add(update_text_view, update);
        return 1;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "[Processus 1] Serveur prêt sur le port %d\n", PORT1);
    TextUpdate *update = g_new(TextUpdate, 1);
    update->text = g_strdup(msg);
    update->buffer = text_buffer;
    g_idle_add(update_text_view, update);

    while (!stop) {
        SOCKET client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            if (stop) break;
            char msg[256];
            snprintf(msg, sizeof(msg), "Erreur accept : %d\n", WSAGetLastError());
            TextUpdate *update = g_new(TextUpdate, 1);
            update->text = g_strdup(msg);
            update->buffer = text_buffer;
            g_idle_add(update_text_view, update);
            continue;
        }
        int bytes_received = recv(client_socket, (char*)&buffer, sizeof(int), 0);
        if (bytes_received != sizeof(int)) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Erreur: Données incomplètes (%d octets)\n", bytes_received);
            TextUpdate *update = g_new(TextUpdate, 1);
            update->text = g_strdup(msg);
            update->buffer = text_buffer;
            g_idle_add(update_text_view, update);
            closesocket(client_socket);
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        update_on_receive(buffer, client_ip, ntohs(client_addr.sin_port));
        closesocket(client_socket);
    }

    closesocket(server_socket);
    return 0;
}

void init_gui() {
    gtk_init(NULL, NULL);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Processus 1");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    clock_label = gtk_label_new("Scalar Clock: 0");
    gtk_box_pack_start(GTK_BOX(vbox), clock_label, FALSE, FALSE, 5);

    text_view = gtk_text_view_new();
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 5);

    gtk_widget_show_all(window);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    init_gui();
    CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    Sleep(3000);

    scalar_clock++;
    display_clock("Evenement local 1 : Affichage");

    scalar_clock++;
    int x = 4;
    x++;
    display_clock("Evenement local 2 : Incrementation");

    scalar_clock++;
    time_t t = time(NULL);
    char time_msg[256];
    snprintf(time_msg, sizeof(time_msg), "Heure actuelle : %s", ctime(&t));
    TextUpdate *update = g_new(TextUpdate, 1);
    update->text = g_strdup(time_msg);
    update->buffer = text_buffer;
    g_idle_add(update_text_view, update);
    display_clock("Evenement local 3 : Heure systeme");

    scalar_clock++;
    int y = x + 5;
    printf("y = %d\n", y);
    display_clock("Evenement local 4 : Addition");

    scalar_clock++;
    Sleep(1000);
    display_clock("Evenement local 5 : Pause 1s");

    send_message(PORT2, scalar_clock); Sleep(200);
    send_message(PORT3, scalar_clock); Sleep(200);
    send_message(PORT4, scalar_clock); Sleep(200);
    send_message(PORT2, scalar_clock);

    Sleep(5000);
    stop = 1;
    closesocket(server_socket);
    WSACleanup();
    gtk_main();
    return 0;
}