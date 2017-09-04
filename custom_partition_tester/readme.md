# Custom Partition Table

This is a demo for a custom partition table for PlatformIO/esp-idf, it depends on a [pull request](https://github.com/platformio/platform-espressif32/pull/40).

The basic idea is that partitions_table.csv is detected in src folder it will compile and use it instad of the default. You also need to modify sdkconfig.h to use the partitions_table.csv.
