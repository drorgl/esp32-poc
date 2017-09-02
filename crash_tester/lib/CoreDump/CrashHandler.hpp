#ifndef _CRASH_HANDLER_H_
#define _CRASH_HANDLER_H_

#include <freertos/FreeRTOSConfig.h>
#include <functional>
#include <esp_partition.h>
#include <rom/rtc.h>
#include <cmath>

// void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
// {
//     panicPutStr("***ERROR*** A stack overflow in task ");
//     panicPutStr((char *)pcTaskName);
//     panicPutStr(" has been detected.\r\n");
//     abort();
// }

#include <rom/rtc.h>
#include <string>

class CrashHanlder
{
private:
	

	bool execute_in_partition_context(std::function<void(void*, uint32_t)> fn){
		auto core_part = get_coredump_partition();
		if (core_part == NULL)
		{
			return false;
		}

		auto core_part_size = floor(core_part->size / SPI_FLASH_SEC_SIZE) * SPI_FLASH_SEC_SIZE;

		//check if core dump is present in the eeprom
		const void *result = NULL;
		spi_flash_mmap_handle_t ota_data_map;

		auto ret = esp_partition_mmap(core_part, 0, core_part_size, SPI_FLASH_MMAP_DATA, &result, &ota_data_map);
		if (ret != ESP_OK)
		{
			printf("unable to map partition %d\r\n", ret);
			result = NULL;
			return false;
		}
		else
		{
			fn((void*)result,core_part_size);

			spi_flash_munmap(ota_data_map);
		}
		return true;
	}

	const esp_partition_t* get_coredump_partition(){
		auto core_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
		if (!core_part)
		{
			printf("No core dump partition found!\r\n");
			return NULL;
		}
		printf("Found partition '%s' @ %x %d bytes\r\n", core_part->label, core_part->address, core_part->size);

		//align partition to SPI_FLASH_SEC_SIZE to avoid problems


		return core_part;
	}


  public:
	RESET_REASON RESET_REASONS[portNUM_PROCESSORS];
	bool crash_detected;

	CrashHanlder()
	{
		crash_detected = false;
		for (auto i = 0; i < portNUM_PROCESSORS;i++){
			RESET_REASONS[i] = rtc_get_reset_reason(i);
			if (RESET_REASONS[i] != RESET_REASON::NO_MEAN){
				crash_detected = true;
			}
		}
	}

	const char * get_reset_reason(RESET_REASON reason)
	{
	  switch ( reason)
	  {
		case RESET_REASON::NO_MEAN                : return "NO_MEAN";
		case RESET_REASON::POWERON_RESET          : return "POWERON_RESET";    /**<1, Vbat power on reset*/
		case RESET_REASON::SW_RESET               : return "SW_RESET";    /**<3, Software reset digital core*/
		case RESET_REASON::OWDT_RESET             : return "OWDT_RESET";    /**<4, Legacy watch dog reset digital core*/
		case RESET_REASON::DEEPSLEEP_RESET        : return "DEEPSLEEP_RESET";    /**<3, Deep Sleep reset digital core*/
		case RESET_REASON::SDIO_RESET             : return "SDIO_RESET";    /**<6, Reset by SLC module, reset digital core*/
		case RESET_REASON::TG0WDT_SYS_RESET       : return "TG0WDT_SYS_RESET";    /**<7, Timer Group0 Watch dog reset digital core*/
		case RESET_REASON::TG1WDT_SYS_RESET       : return "TG1WDT_SYS_RESET";    /**<8, Timer Group1 Watch dog reset digital core*/
		case RESET_REASON::RTCWDT_SYS_RESET       : return "RTCWDT_SYS_RESET";    /**<9, RTC Watch dog Reset digital core*/
		case RESET_REASON::INTRUSION_RESET        : return "INTRUSION_RESET";    /**<10, Instrusion tested to reset CPU*/
		case RESET_REASON::TGWDT_CPU_RESET        : return "TGWDT_CPU_RESET";    /**<11, Time Group reset CPU*/
		case RESET_REASON::SW_CPU_RESET           : return "SW_CPU_RESET";    /**<12, Software reset CPU*/
		case RESET_REASON::RTCWDT_CPU_RESET       : return "RTCWDT_CPU_RESET";    /**<13, RTC Watch dog Reset CPU*/
		case RESET_REASON::EXT_CPU_RESET          : return "EXT_CPU_RESET";    /**<14, for APP CPU, reseted by PRO CPU*/
		case RESET_REASON::RTCWDT_BROWN_OUT_RESET : return "RTCWDT_BROWN_OUT_RESET";    /**<15, Reset when the vdd voltage is not stable*/
		case RESET_REASON::RTCWDT_RTC_RESET       : return "RTCWDT_RTC_RESET";    /**<16, RTC Watch dog reset digital core and rtc module*/
	  }
	  return "";
	}

	void display_crash_reason(){
		for (auto i = 0; i < portNUM_PROCESSORS;i++){
			printf("Crash %s on core %d\r\n", get_reset_reason(RESET_REASONS[i]), i);
		}
	}

	
	

	uint32_t get_coredump_size()
	{
		uint32_t checksum = 0;
		uint32_t length = 0;

		execute_in_partition_context([&](void * part_addr, uint32_t part_size){
			for (auto i = 0; i < part_size; i++)
			{
				auto value = ((uint8_t *)part_addr)[i];
				checksum += ~value;
				if (value != 0xFF){
					length = i;
				}
			}

			printf("Core Dump Partition Checksum %d, length %d bytes\r\n", checksum, length);
		});

		return length;
	}

	bool save_coredump_to_file(const char *filename)
	{
		execute_in_partition_context([&](void * part_addr, uint32_t part_size){
			FILE *file = fopen(filename, "wb");
			fwrite(part_addr, part_size, 1, file);
			fclose(file);
		});
		return true;
	}

	void dump_coredump_to_console(){
		const static DRAM_ATTR char b64[] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		auto src_len = get_coredump_size();
		
		execute_in_partition_context([&](void * part_addr, uint32_t part_size){
			int i, j, a, b, c;
			
			char * src = (char*)part_addr;

			for (i = j = 0; i < src_len; i += 3) {
				a = src[i];
				b = i + 1 >= src_len ? 0 : src[i + 1];
				c = i + 2 >= src_len ? 0 : src[i + 2];
		
				printf("%c",b64[a >> 2]);
				printf("%c", b64[((a & 3) << 4) | (b >> 4)]);
				if (i + 1 < src_len) {
					printf("%c", b64[(b & 0x0F) << 2 | (c >> 6)]);
				}
				if (i + 2 < src_len) {
					printf("%c", b64[c & 0x3F]);
				}
			}
			while (j % 4 != 0) {
				printf("%c", '=');
			}
		});
	}
};
#endif
