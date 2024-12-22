#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// 클라이언트 정보 구조체
typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    char username[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// 클라이언트에 메시지 브로드캐스트
void broadcast_message(const char *message, int sender_sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].sockfd != sender_sockfd) {
            send(clients[i].sockfd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// 서버가 메시지에 답장하는 함수
void send_reply(int sockfd, const char *message) {
    char reply[BUFFER_SIZE];
    snprintf(reply, sizeof(reply), "Server: %s", message);
    send(sockfd, reply, strlen(reply), 0);
}

// 클라이언트 처리 스레드
void *handle_client(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char username[50];

    // 사용자 이름 수신
    recv(sockfd, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0; // 개행 제거

    pthread_mutex_lock(&clients_mutex);
    strcpy(clients[client_count].username, username);
    clients[client_count].sockfd = sockfd;
    client_count++;
    pthread_mutex_unlock(&clients_mutex);

    // 접속 메시지 브로드캐스트
    sprintf(buffer, "%s joined the chat.\n", username);
    broadcast_message(buffer, sockfd);

    // 메시지 수신 및 브로드캐스트
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // 클라이언트 연결 종료
            break;
        }
        buffer[bytes_received] = '\0';

        char message[BUFFER_SIZE + 50];
        sprintf(message, "%s: %s", username, buffer);  // 클라이언트 이름을 앞에 붙여서 메시지 전송
        broadcast_message(message, sockfd);

        // 서버가 답장
        send_reply(sockfd, "Message received!");
    }

    // 클라이언트 연결 해제
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].sockfd == sockfd) {
            for (int j = i; j < client_count - 1; ++j) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    sprintf(buffer, "%s left the chat.\n", username);
    broadcast_message(buffer, sockfd);
    close(sockfd);
    return NULL;
}

// 서버에서 입력을 받아서 클라이언트에게 메시지를 전송하는 함수
void *server_input_thread(void *arg) {
    char message[BUFFER_SIZE];
    while (1) {
        printf("Enter message for all clients: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0; // 개행 제거

        if (strlen(message) > 0) {
            // "Server"라는 이름을 붙여서 메시지를 전송
            char full_message[BUFFER_SIZE];
            snprintf(full_message, sizeof(full_message), "Server: %s", message);
            broadcast_message(full_message, -1);  // 모든 클라이언트에게 메시지 전송
        }
    }
}

int main() {
    int server_sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // 소켓 생성
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인딩
    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 연결 대기
    if (listen(server_sockfd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Chat server started on port %d\n", PORT);

    // 서버 입력 스레드 시작
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, server_input_thread, NULL);

    // 클라이언트 연결 처리
    while (1) {
        addr_size = sizeof(client_addr);
        new_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (new_sockfd < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)&new_sockfd) != 0) {
            perror("Thread creation failed");
            continue;
        }
        pthread_detach(tid);
    }

    close(server_sockfd);
    return 0;
}

