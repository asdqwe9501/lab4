#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void handle_get_request(int client_socket);
void handle_post_request(int client_socket, char *data);
void execute_cgi_program(int client_socket, char *data);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // 서버 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Server socket creation failed");
        exit(1);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 서버 소켓에 주소 바인딩
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }

    // 서버 소켓을 듣기 상태로 전환
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);

    // 클라이언트 연결 처리
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // 클라이언트로부터 요청 받기
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            close(client_socket);
            continue;
        }

        printf("Received request:\n%s\n", buffer);

        // HTTP 요청 처리
        if (strncmp(buffer, "GET", 3) == 0) {
            handle_get_request(client_socket);
        } else if (strncmp(buffer, "POST", 4) == 0) {
            // POST 메소드에서 데이터를 추출하여 처리
            char *data = strstr(buffer, "\r\n\r\n");
            if (data != NULL) {
                data += 4;  // 데이터 시작 위치
                handle_post_request(client_socket, data);
            }
        }

        // 클라이언트 소켓 닫기
        close(client_socket);
    }

    // 서버 소켓 닫기
    close(server_socket);
    return 0;
}

void handle_get_request(int client_socket) {
    const char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    const char *html_content = "<html><body><h1>GET Request Processed</h1></body></html>";
    send(client_socket, response_header, strlen(response_header), 0);
    send(client_socket, html_content, strlen(html_content), 0);
}

void handle_post_request(int client_socket, char *data) {
    char response_header[BUFFER_SIZE];
    char content[BUFFER_SIZE];

    // POST 데이터 처리
    snprintf(content, sizeof(content), "<html><body><h1>POST Request Received</h1><p>%s</p></body></html>", data);

    // 응답 헤더
    snprintf(response_header, sizeof(response_header), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");

    // 응답 전송
    send(client_socket, response_header, strlen(response_header), 0);
    send(client_socket, content, strlen(content), 0);
}

void execute_cgi_program(int client_socket, char *data) {
    FILE *fp;
    char result[BUFFER_SIZE];
    char *cgi_path = "/usr/lib/cgi-bin/test.cgi";

    // CGI 프로그램 실행
    fp = popen(cgi_path, "w");
    if (fp == NULL) {
        perror("Failed to execute CGI program");
        return;
    }

    // POST 데이터 전송
    fprintf(fp, "Content-Length: %ld\r\n\r\n", strlen(data));
    fprintf(fp, "%s", data);
    fclose(fp);

    // CGI 프로그램의 결과 처리
    fp = popen(cgi_path, "r");
    if (fp == NULL) {
        perror("Failed to read CGI program output");
        return;
    }

    // CGI 프로그램의 출력 읽기
    while (fgets(result, sizeof(result), fp) != NULL) {
        send(client_socket, result, strlen(result), 0);
    }

    fclose(fp);
}

