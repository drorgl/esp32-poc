/* Application For Core Dumps Generation

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <nvs_flash.h>
// #include <rom/rtc.h>
// #include <esp_partition.h>
// #include <rom/crc.h>
// #include <mbedtls/base64.h>
// #include <esp_spi_flash.h>

#include <CrashHandler.hpp>

// task crash indicators
#define TCI_NULL_PTR    0x1
#define TCI_UNALIGN_PTR 0x2
#define TCI_FAIL_ASSERT 0x4

volatile unsigned long crash_flags = TCI_UNALIGN_PTR;

// void check_core_dump(){
//     auto reset_reason0 = rtc_get_reset_reason(0);
//     #if !CONFIG_FREERTOS_UNICORE
//     auto reset_reason1 = rtc_get_reset_reason(1);
//     #else
//     auto reset_reason1 = RESET_REASON::NO_MEAN;
//     #endif

//     if ((reset_reason0 != RESET_REASON::NO_MEAN) ||
//         (reset_reason1 != RESET_REASON::NO_MEAN)){
//             //crash detected
//             printf("reset reason detected %d and %d\r\n",reset_reason0, reset_reason1);
//     }

//     //check for core dump partition
//     auto core_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
//     if (!core_part)
//     {
//         printf("No core dump partition found!\r\n");
//         return;
//     }
//     printf("Found partition '%s' @ %x %d bytes\r\n", core_part->label, core_part->address, core_part->size);

//     //check if core dump is present in the eeprom
//     const void *result = NULL;
//     spi_flash_mmap_handle_t ota_data_map;

//     auto ret = esp_partition_mmap(core_part, 0, core_part->size, SPI_FLASH_MMAP_DATA, &result, &ota_data_map);
//     if (ret != ESP_OK) {
//         result = NULL;
//         return;
//     } else {
//         uint32_t checksum = 0;
//         for (auto i = 0; i <core_part->size;i++ ){
//             checksum += ((uint8_t*)result)[i];
//             printf("%02X",((uint8_t*)result)[i]);
//             if ((i % 64) == 0){
//                 printf("\r\n");
//                 vTaskDelay(0);
//             }
            
//         }
//         printf("Core Dump Partition Checksum %d\r\n", checksum);

//         //usable size: SPI_FLASH_SEC_SIZE
        
//         if (checksum != 0){
//             spi_flash_munmap(ota_data_map);
//             printf("Core Dump Partition is not empty, erasing...");
//             auto erase_result = esp_partition_erase_range(core_part,0,core_part->size);
//             printf("done: %d\r\n", erase_result);
//             vTaskDelay(10000 / portTICK_PERIOD_MS);


//             auto ret = esp_partition_mmap(core_part, 0, core_part->size, SPI_FLASH_MMAP_DATA, &result, &ota_data_map);
//             if (ret != ESP_OK) {
//                 result = NULL;
//                 return;
//             } else {
                
//                 // for (auto i = 0; i <core_part->size;i+=sizeof(uint32_t) ){
//                 //     ((uint32_t*)result)[i] = 0;        
//                 // }

//                 uint32_t checksum = 0;
//                 for (auto i = 0; i <core_part->size;i++ ){
//                     checksum += ((uint8_t*)result)[i];
//                     printf("%02X",((uint8_t*)result)[i]);
//                     if ((i % 64) == 0){
//                         printf("\r\n");
//                         vTaskDelay(0);
//                     }
//                 }
//                 printf("Core Dump Partition Checksum after clear %d\r\n", checksum);
//             }
//         }



//         spi_flash_munmap(ota_data_map);
//     }
    

//     //core_dump.address
//     //core_dump.size
//     //if so, dump to screen and clean up

// }



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