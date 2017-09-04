/* Application For Core Dumps Generation

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
//#define __STDC__

#include <stdio.h>

#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <nvs_flash.h>
// #include <esp_spi_flash.h>

#include <CrashHandler.hpp>

// task crash indicators
#define TCI_NULL_PTR    0x1
#define TCI_UNALIGN_PTR 0x2
#define TCI_FAIL_ASSERT 0x4

volatile unsigned long crash_flags = TCI_UNALIGN_PTR;

void bad_ptr_func()
{
    unsigned long *ptr = (unsigned long *)0;
    volatile int cnt;
    int i = 0;

    for (i = 0; i < 1000; i++) {
        cnt++;
    }

    if(crash_flags & TCI_NULL_PTR) {
        printf("Write to bad address 0x%lx.\n", (unsigned long)ptr);
        *ptr = 0xDEADBEEF;
    }
}

void bad_ptr_task(void *pvParameter)
{
    printf("Task 'bad_ptr_task' start.\n");
    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("Task 'bad_ptr_task' run.\n");
        bad_ptr_func();
    }
    fflush(stdout);
    while(1){
        vTaskDelay(1000);
    }
}

void recur_func()
{
    static int rec_cnt;
    unsigned short *ptr = (unsigned short *)0x5;
    volatile int cnt;
    int i = 0;

    if (rec_cnt++ > 2) {
        return;
    }
    for (i = 0; i < 4; i++) {
        cnt++;
        if(i == 2) {
            recur_func();
            break;
        }
    }

    if(crash_flags & TCI_UNALIGN_PTR) {
        printf("Write to unaligned address 0x%lx.\n", (unsigned long)ptr);
        *ptr = 0xDEAD;
    }
}

void unaligned_ptr_task(void *pvParameter)
{
    printf("Task 'unaligned_ptr_task' start.\n");
    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("Task 'unaligned_ptr_task' run.\n");
        recur_func();
    }
    fflush(stdout);
    while(1){
        vTaskDelay(1000);
    }
}

void failed_assert_task(void *pvParameter)
{
    printf("Task 'failed_assert_task' start.\n");
    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("Task 'failed_assert_task' run.\n");
        if(crash_flags & TCI_FAIL_ASSERT) {
            printf("Assert.\n");
            assert(0);
        }
    }
    fflush(stdout);
    while(1){
        vTaskDelay(1000);
    }
}

void init_analysis_task(void *pvParameter)
{
    CrashHanlder crash_handler;
    if (crash_handler.crash_detected){
        crash_handler.display_crash_reason();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    auto cd_size = crash_handler.get_coredump_size();
    if (cd_size > 0){
        printf("core dump size: %d\r\n", cd_size);
        crash_handler.dump_coredump_to_console();
    }

    while(1){
        vTaskDelay(1000);
    }
}

extern "C"{

void app_main()
{
    

    nvs_flash_init();
    esp_log_level_set("*",ESP_LOG_VERBOSE);

    xTaskCreate(& init_analysis_task,"init_analysis_task", 2048,NULL,5,NULL);
    //check_core_dump();

    vTaskDelay(60000 / portTICK_PERIOD_MS);

    xTaskCreate(&bad_ptr_task, "bad_ptr_task", 2048, NULL, 5, NULL);
    xTaskCreatePinnedToCore(&unaligned_ptr_task, "unaligned_ptr_task", 2048, NULL, 7, NULL, 1);
    xTaskCreatePinnedToCore(&failed_assert_task, "failed_assert_task", 2048, NULL, 10, NULL, 0);
}
}