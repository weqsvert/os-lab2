#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proclaunch.h"

// Вспомогательная функция для сна
void sleep_ms(int milliseconds) {
#ifdef _WIN32
    #include <windows.h>
    Sleep(milliseconds);
#else
    #include <unistd.h>
    #include <time.h>
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

int main(int argc, char *argv[]) {
    // ---------------------------------------------------------
    // РЕЖИМ РЕБЕНКА: Если программа запущена с флагом --child
    // ---------------------------------------------------------
    if (argc >= 4 && strcmp(argv[1], "--child") == 0) {
        int exit_code = atoi(argv[2]);
        int sleep_time = atoi(argv[3]);
        
        // Если передан 5-й аргумент (для Теста 3), проверяем его
        if (argc >= 5) {
            const char *expected = "hello \"big\" world\\";
            if (strcmp(argv[4], expected) == 0) {
                return 1; // Успех: аргумент не искажён
            } else {
                printf("Child: got '%s', expected '%s'\n", argv[4], expected);
                return 0; // Провал: аргумент искажён
            }
        }
        
        sleep_ms(sleep_time);
        return exit_code; 
    }

    // ---------------------------------------------------------
    // РЕЖИМ ТЕСТИРОВАНИЯ (Родитель)
    // ---------------------------------------------------------
    printf("=== Starting tests ===\n");
    const char *self = argv[0]; // Путь к текущей программе
    int tests_failed = 0;

    // --- ТЕСТ 1: Проверка фонового запуска и pl_poll ---
    printf("\n[TEST 1] Background launch and pl_poll...\n");
    const char *args1[] = { self, "--child", "42", "1000", NULL }; // Ребенок спит 1 сек, вернет 42
    pl_process proc1;
    
    if (pl_start(args1, &proc1) != 0) {
        printf("[FAIL] pl_start failed\n");
        tests_failed++;
    } else {
        int iterations = 0;
        int code = -1;
        int status;
        
        // Проверяем 15 раз с интервалом 100 мс
        while (iterations < 15) {
            status = pl_poll(&proc1, &code);
            if (status == 1) { // Завершился
                break;
            } else if (status == -1) {
                printf("[FAIL] pl_poll returned error\n");
                tests_failed++;
                break;
            }
            sleep_ms(100);
            iterations++;
        }
        
        if (status == 1 && code == 42 && iterations > 3) {
            printf("[ OK ] Finished with code 42 after %d iterations\n", iterations);
        } else {
            printf("[FAIL] Expected code 42, got %d after %d iterations\n", code, iterations);
            tests_failed++;
        }
    }

    // --- ТЕСТ 2: Проверка pl_wait (ожидание) ---
    printf("\n[TEST 2] Blocking wait...\n");
    const char *args2[] = { self, "--child", "7", "300", NULL }; // Ребенок спит 0.3 сек, вернет 7
    pl_process proc2;
    
    if (pl_start(args2, &proc2) != 0) {
        printf("[FAIL] pl_start failed\n");
        tests_failed++;
    } else {
        int code = -1;
        if (pl_wait(&proc2, &code) == 0 && code == 7) {
            printf("[ OK ] Finished with code 7\n");
        } else {
            printf("[FAIL] Expected code 7, got %d\n", code);
            tests_failed++;
        }
    }

    // --- ТЕСТ 3: Аргумент с пробелами ---
    printf("\n[TEST 3] Argument with spaces...\n");
    // Передаем аргумент: hello "big" world\
    const char *args3[] = { self, "--child", "0", "0", "hello \"big\" world\\", NULL };
    pl_process proc3;
    
    if (pl_start(args3, &proc3) != 0) {
        printf("[FAIL] pl_start failed\n");
        tests_failed++;
    } else {
        int code = -1;
        if (pl_wait(&proc3, &code) == 0 && code == 1) {
            printf("[ OK ] Child received argument without distortion\n");
        } else {
            printf("[FAIL] Argument was distorted or child failed (code %d)\n", code);
            tests_failed++;
        }
    }

    // --- ТЕСТ 4: Запуск несуществующей программы ---
    printf("\n[TEST 4] Non-existent program...\n");
    const char *args4[] = { "no_such_program_12345", NULL };
    pl_process proc4;
    
    if (pl_start(args4, &proc4) == 0) {
        printf("[FAIL] pl_start returned success for non-existent program\n");
        int code;
        pl_wait(&proc4, &code); // Убираем зомби
        tests_failed++;
    } else {
        printf("[ OK ] pl_start correctly returned error\n");
    }
    
    // --- ТЕСТ 5: Код возврата больше 255 (Linux обрезает до 8 бит) ---
    printf("\n[TEST 5] Exit code > 255...\n");
    const char *args5[] = { self, "--child", "300", "0", NULL }; // Вернуть 300
    pl_process proc5;
    
    if (pl_start(args5, &proc5) == 0) {
        int code = -1;
        pl_wait(&proc5, &code);
#ifdef _WIN32
        if (code == 300) {
            printf("[ OK ] Got 300 (Windows keeps 32-bit code)\n");
        } else {
            printf("[FAIL] Expected 300, got %d\n", code);
            tests_failed++;
        }
#else
        // 300 % 256 = 44
        if (code == 44) {
            printf("[ OK ] Got 44 (300 truncated to 8 bits)\n");
        } else {
            printf("[FAIL] Expected 44, got %d\n", code);
            tests_failed++;
        }
#endif
    }

    printf("\n=== Tests finished. Failed: %d ===\n", tests_failed);
    return tests_failed > 0 ? 1 : 0;
}