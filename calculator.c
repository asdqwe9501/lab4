#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>

// 버튼 클릭 이벤트 핸들러
void on_calculate_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *num1_text = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *num2_text = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    // 입력 값이 비어 있으면 0으로 설정
    if (*num1_text == '\0' || *num2_text == '\0') {
        gtk_label_set_text(GTK_LABEL(entries[2]), "Invalid input. Please enter numbers.");
        return;
    }

    double num1 = atof(num1_text);
    double num2 = atof(num2_text);
    double result = num1 + num2;

    // 결과 표시
    char result_text[100];
    snprintf(result_text, sizeof(result_text), "Result: %.2f", result);

    GtkWidget *result_label = entries[2];
    gtk_label_set_text(GTK_LABEL(result_label), result_text);
}

int main(int argc, char *argv[]) {
    GtkWidget *window, *grid, *entry1, *entry2, *button, *result_label;

    gtk_init(&argc, &argv);

    // 창 생성
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    // 그리드 레이아웃
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // 첫 번째 입력창
    entry1 = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry1, 0, 0, 1, 1);

    // 두 번째 입력창
    entry2 = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry2, 1, 0, 1, 1);

    // 계산 버튼
    button = gtk_button_new_with_label("Calculate");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 2, 1);

    // 결과 레이블
    result_label = gtk_label_new("Result: ");
    gtk_grid_attach(GTK_GRID(grid), result_label, 0, 2, 2, 1);

    // 버튼 클릭 이벤트 연결
    GtkWidget *widgets[] = {entry1, entry2, result_label};
    g_signal_connect(button, "clicked", G_CALLBACK(on_calculate_button_clicked), widgets);

    // 창 닫기 이벤트 연결
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

