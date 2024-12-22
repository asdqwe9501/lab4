#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS]; // 클라이언트 소켓 배열
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 뮤텍스

// 클라이언트에게 메시지 브로드캐스트
void broadcast_message(const char* message, int sender_fd) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && client_sockets[i] != sender_fd) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

// 클라이언트 쓰레드 핸들러
void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];

    printf("Thread created for client %d\n", client_fd); // 쓰레드 생성 로그

    while (1) {
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            printf("Client disconnected: %d\n", client_fd);
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == client_fd) {
                    client_sockets[i] = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            close(client_fd);
            break;
        }
        buffer[bytes_read] = '\0';
        printf("Message from client %d: %s\n", client_fd, buffer);
        broadcast_message(buffer, client_fd); // 메시지를 다른 클라이언트에 전송
    }

    printf("Thread for client %d exiting...\n", client_fd); // 쓰레드 종료 로그
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 주소와 포트 바인딩
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 연결 대기
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    pthread_t thread_id;
    while ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) >= 0) {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                pthread_create(&thread_id, NULL, handle_client, &new_socket);
                pthread_detach(thread_id);
                printf("New client connected: %d\n", new_socket);
                break;
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    close(server_fd);
    return 0;
}

