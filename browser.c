#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl.h>
#include <stdbool.h>
#include <raylib.h>

#define BROWSER_VIGNETTE_LENGTH 148

typedef enum {
    TAG_HTML,
    TAG_HEAD,
    TAG_BODY,
    TAG_P,
    TAG_DIV,
    TAG_OTHER
} tag_type;

typedef struct {
    char* data;
    size_t max_len;
    size_t current_len;
} html;

typedef struct {
    tag_type type;
    char* raw_type;
    // char* raw_content;
} tag;

size_t write_to_html (void *contents, size_t size, size_t nmemb, void *userp) {
    // contents - то, что мы добавляем; size = 1; nmemb - количество добавляемых элементов, userp - нужно для преобразования
    html *my_html = (html *)userp;

    // Размер того, что мы добавляем
    size_t total_size = size * nmemb; 

    // Проверка на переполнение буффера
    if ((my_html->current_len + total_size) > my_html->max_len) { 
        // Если текущий размер + добавляемый размер > максимальный размер = ничего не добавляем
        printf("ERROR: html is to large!");
        return 0;
    }

    // Копирует из входящего contents в [my_html->data + my_html->current_len], 
    // то есть адрес назначения = начало текущего контента + текущий размер, то есть конец текущего контента
    memcpy(my_html->data + my_html->current_len, contents, total_size);

    // К предыдущему размеру прибавляем размер добавленного контента
    my_html->current_len += total_size; 

    // Нулевой терминатор для строк
    my_html->data[my_html->current_len] = '\0';

    // Что бы curl не ругался
    return total_size;
}

 void delete_char(char* str, int len, int index) {
    if (index < 0 || index >= len) return; // Проверка границ

    for (int i = index; i < len; i++) {
        str[i] = str[i + 1]; // Сдвигаем влево
    }
}

void parse_html (html content, tag* tags, int t_index, int t_len, int* tags_total_number) { // Tags
    int t_x = t_index;

    for (int x = 0; x < content.current_len; x ++) {
        if(content.data[x] == '<') {
            printf("Finded tag: ");

            tag i_tag;
            char* raw_tag_type = calloc(20, sizeof(char)); // Calloc вместо malloc потому что если будет malloc
                                                           // то при удалении последнего символа в строке из-за сдвига влево будут добавлятся сырые байты

            delete_char(content.data, t_len, x);
            int z = 0; // индекс в raw_tag_type
            while (true) { 
                if (content.data[x] == '/') {
                    delete_char(content.data, t_len, x); 

                    while (content.data[x] != '>') {
                        delete_char(content.data, t_len, x); 
                    }

                    delete_char(content.data, t_len, x); 
                    break;
                }
                else if (content.data[x] == '>') {
                    delete_char(content.data, t_len, x); 
                    content.data[x] == '\n';
                    break;
                } else if (content.data[x] == ' ') {
                    // обработка атрибутов
                    break;
            
                } else {
                    raw_tag_type[z] = content.data[x];
                    delete_char(content.data, t_len, x);
                    z ++;
                }
            }

            printf("%s\n", raw_tag_type);

            // Обработка типа тега
            if (strcmp(raw_tag_type, "p") == 0) {
                i_tag.type = TAG_P;
            } else if (strcmp(raw_tag_type, "div") == 0) {
                i_tag.type = TAG_DIV;
            } else {
                i_tag.type = TAG_OTHER;
            }

            // Распознавание вложенных тегов нужно будет реализовать с помощью рекурсии

            i_tag.raw_type = raw_tag_type;

            tags_total_number += 1;
            free(raw_tag_type);
        }
    }
}

int main() {
    SetTraceLogLevel(LOG_ERROR);

    tag* tags = malloc(200);
    int tags_total_number = 0;

    for(int z = 0; z < (BROWSER_VIGNETTE_LENGTH/4); z ++) {
        printf("=");
    }
    printf("Welcome to my browser!");
    for(int z = 0; z < (BROWSER_VIGNETTE_LENGTH/4); z ++) {
        printf("=");
    }
    printf("\n");

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    html my_html = {malloc(1*1000*1000), 1*1000*1000, 0};
    char* text_for_drawing;

    printf("Setting connection parameters...");
    curl_easy_setopt(curl, CURLOPT_URL, "file://index.html"); // file:///home/fedor/Документы/Work/C/my_browser/index.html
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_html);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &my_html);

    printf("\nTry connecting to server...");
    CURLcode res = curl_easy_perform(curl);
    printf("\nServer sent callback\n");
    
    if (res == CURLE_OK) {
        printf("Request was completed successfully!\n");
        printf("--------------------------------------\n");

        text_for_drawing = my_html.data;

        parse_html(my_html, tags, 0, 200, &tags_total_number);

        for(int z = 0; z < BROWSER_VIGNETTE_LENGTH; z ++) {
            printf("=");
        }
        printf("\n");

        printf("BROWSER WINDOW|\n");
        printf("---------------\n");
        printf("%s\n", my_html.data);

        for(int z = 0; z < BROWSER_VIGNETTE_LENGTH; z ++) {
            printf("=");
        }
        printf("\nParameters of received html: %zu bytes, %d tags\n", my_html.current_len, tags_total_number);
    } else {
        printf("Error: %s\n", curl_easy_strerror(res));
    }

    InitWindow(1300, 800, "My browser");
    SetTargetFPS(120);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK); 

        DrawText("Raw html:", 10, 10, 16, RED);
        DrawLine(5, 26, 1295, 26, WHITE);

        DrawText(text_for_drawing, 10, 28, 16, WHITE);

        EndDrawing();
    }

    CloseWindow();

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    free(my_html.data);
    return 0;
}