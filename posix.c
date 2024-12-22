#include <pthread.h>
#include <stdio.h>

void* print_message(void* message) {
    printf("%s\n", (char*)message);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, print_message, "Thread 1");
    pthread_create(&thread2, NULL, print_message, "Thread 2");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}

