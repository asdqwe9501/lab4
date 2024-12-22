#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CLIENTS 3

pthread_mutex_t mutex;
char message[256] = "";

void* client_thread(void* arg) {
    int id = *(int*)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        if (strlen(message) > 0) {
            printf("Client %d received message: %s\n", id, message);
            message[0] = '\0'; // 메시지 읽은 후 초기화
        }
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

void* server_thread(void* arg) {
    while (1) {
        char buffer[256];
        printf("Server: Enter message to broadcast: ");
        fgets(buffer, sizeof(buffer), stdin);
        pthread_mutex_lock(&mutex);
        strncpy(message, buffer, sizeof(message));
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t clients[MAX_CLIENTS], server;
    int client_ids[MAX_CLIENTS] = {1, 2, 3};
    
    pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&server, NULL, server_thread, NULL);
    for (int i = 0; i < MAX_CLIENTS; i++)
        pthread_create(&clients[i], NULL, client_thread, &client_ids[i]);
    
    pthread_join(server, NULL);
    for (int i = 0; i < MAX_CLIENTS; i++)
        pthread_join(clients[i], NULL);

    pthread_mutex_destroy(&mutex);
    return 0;
}

