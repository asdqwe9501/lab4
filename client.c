#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <gtk/gtk.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int sockfd;
GtkWidget *chat_view;
GtkTextBuffer *chat_buffer;
GtkWidget *entry;
char username[50];  // 사용자 이름을 저장할 변수

// 메시지 수신 후 UI 업데이트
void update_chat_window(const char *message) {
    gtk_text_buffer_insert_at_cursor(chat_buffer, message, -1);
    gtk_text_buffer_insert_at_cursor(chat_buffer, "\n", -1);
}

// 메시지 수신 스레드
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        // 메시지를 GUI에 추가하기 위해 메인 스레드에서 실행되도록 큐에 추가
        g_idle_add((GSourceFunc)update_chat_window, strdup(buffer));
    }
    close(sockfd);
    return NULL;
}

// 메시지 전송 핸들러
void send_message(GtkWidget *widget, gpointer data) {
    const char *message = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(message) > 0) {
        // 사용자 이름과 메시지 결합
        char full_message[BUFFER_SIZE];
        snprintf(full_message, sizeof(full_message), "%s: %s", username, message);
        
        // 메시지 전송
        send(sockfd, full_message, strlen(full_message), 0);
        
        // GUI 창에 클라이언트 메시지 추가
        g_idle_add((GSourceFunc)update_chat_window, strdup(full_message));

        // 메시지 창 비우기
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

// GUI 초기화 및 실행
void start_gui() {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *scroll;
    GtkWidget *send_button;

    gtk_init(NULL, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    chat_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(chat_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scroll), chat_view);

    chat_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);

    send_button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, TRUE, 0);

    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t recv_thread;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 사용자 이름 입력
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // 개행 제거

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // 사용자 이름을 서버에 전송
    send(sockfd, username, strlen(username), 0);

    // 수신 스레드 시작
    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    // GUI 시작
    start_gui();

    pthread_join(recv_thread, NULL);
    close(sockfd);
    return 0;
}

