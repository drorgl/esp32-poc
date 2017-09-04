#include <stdint.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h> 
#include <esp_system.h>

uint32_t last_clock_time[portNUM_PROCESSORS];
uint32_t total_clock_time[portNUM_PROCESSORS] = {0};
uint8_t loaded_clock_time[portNUM_PROCESSORS] = {0};

volatile unsigned long ticker_value = 0;

void init_run_time_counter()
{
    int id = xPortGetCoreID();
    last_clock_time[id] = xthal_get_ccount();
    loaded_clock_time[id] = 1;
}

unsigned long get_run_time_counter_value()
{
    //return xTaskGetTickCount() * 100;
    //struct timeval tv;
     //gettimeofday(&tv, NULL); 
     //curtime=tv.tv_sec;
     //return tv.tv_usec;
    return system_get_time();
    //return ticker_value;
    // struct timespec ts_start;
    // clock_gettime(CLOCK_MONOTONIC, &ts_start);
    // return ts_start.tv_nsec;

//     int id = xPortGetCoreID();
//     uint32_t now = xthal_get_ccount();

//     if (!loaded_clock_time[id])
//     {
//         last_clock_time[id] = now;
//         loaded_clock_time[id] = 1;
//         return total_clock_time[id];
//     }

//     int32_t dt = (int32_t)(now - last_clock_time[id]);
//     last_clock_time[id] = now;

// #if defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_240)
//     total_clock_time[id] += dt / 240;
// #elif defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_160)
//     total_clock_time[id] += dt / 160;
// #else
//     total_clock_time[id] += dt / 80;
// #endif

//     //ets_printf("id=%d, %d\n", id, now);

//     return total_clock_time[id];
}

void low_load()
{
    while (1)
    {
        uint32_t load;
        for (int i = 0; i < 100000; i++)
        {
            load += 1;
            if ((i % 1000) == 0){
                taskYIELD();
            }
        }
        
        vTaskDelay(1);
    }
}

void medium_load()
{
    while (1)
    {
        uint32_t load;
        for (int i = 0; i < 100000; i++)
        {
            load += 1;
            if ((i % 1000) == 0){
                taskYIELD();
            }
        }
        //taskYIELD();
        vTaskDelay(1);
    }
}

void high_load()
{
    while (1)
    {
        uint32_t load;
        for (int i = 0; i < 100000; i++)
        {
            load += 1;
            if ((i % 1000) == 0){
                taskYIELD();
            }
        }
        //taskYIELD();
        vTaskDelay(1);
    }
}


void display_stats()
{
    

    while (1)
    {
        char* pcWriteBuffer;
        char WriteBuffer[1024]={0};
        pcWriteBuffer = &WriteBuffer[0];

        printf("Printing Stats:\r\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        vTaskGetRunTimeStats(pcWriteBuffer);
        for (int i = 0; i < 1024;i++){
            printf("%c", WriteBuffer[i]);
            if (WriteBuffer[i] == '\0'){
                break;
            }
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void ticker(){
    while(1){
        //ticker_value = xthal_get_ccount();
        ticker_value ++;
        vTaskDelay(1000);
    }
}

// extern "C" {

void app_main()
{
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    nvs_flash_init();
    esp_log_level_set("*", ESP_LOG_VERBOSE);

    printf("Starting ticker\r\n");
    xTaskCreatePinnedToCore(&ticker,"ticker",1024,NULL,configMAX_PRIORITIES ,NULL,0);

    printf("Starting display_stats\r\n");
    xTaskCreate(&display_stats, "display_stats", 2048, NULL, 5, NULL);

    printf("Starting low_load\r\n");
    xTaskCreate(&low_load, "low_load1", 2048, NULL, 4, NULL);
    xTaskCreate(&low_load, "low_load2", 2048, NULL, 4, NULL);

    printf("Starting medium_load\r\n");
    xTaskCreate(&medium_load, "medium_load1", 2048, NULL, 10, NULL);
    xTaskCreate(&medium_load, "medium_load2", 2048, NULL, 5, NULL);

    printf("Starting high_load\r\n");
    xTaskCreate(&high_load, "high_load1", 2048, NULL, 16, NULL);
    xTaskCreate(&high_load, "high_load2", 2048, NULL, 16, NULL);
}
//}