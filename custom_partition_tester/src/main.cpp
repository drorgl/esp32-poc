#include <esp_log.h>
#include <esp_partition.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAGSPIFF "main"

extern "C" {

	void app_main()
	{
		esp_log_level_set("*", ESP_LOG_VERBOSE);
		esp_log_level_set("*", esp_log_level_t::ESP_LOG_DEBUG);
		ESP_LOGI(TAGSPIFF, "Partition Table:");
		printf("- label           type/subtype\t address:size\t\t encrypted\r\n");
		{
			auto it = esp_partition_find(esp_partition_type_t::ESP_PARTITION_TYPE_APP, esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_ANY, NULL);
			while (it != NULL) {
				const esp_partition_t *p = esp_partition_get(it);
				printf("- %-15s 0x%02x/0x%02x\t 0x%08x:0x%08x\t %d  \r\n", p->label, p->type, p->subtype, p->address, p->size, p->encrypted);

				it = esp_partition_next(it);
			}
			esp_partition_iterator_release(it);
		}
		{
			auto it = esp_partition_find(esp_partition_type_t::ESP_PARTITION_TYPE_DATA, esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_ANY, NULL);
			while (it != NULL) {
				esp_partition_t *p = const_cast<esp_partition_t*>(esp_partition_get(it));
				printf("- %-15s 0x%02x/0x%02x\t 0x%08x:0x%08x\t %d  \r\n", p->label, p->type, p->subtype, p->address, p->size, p->encrypted);

				it = esp_partition_next(it);
			}
			esp_partition_iterator_release(it);
		}

		vTaskDelay(60000 / portTICK_PERIOD_MS);
	}

}