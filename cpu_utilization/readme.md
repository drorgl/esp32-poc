# CPU Utilization

This is a demo/workaround for using the FreeRTOS vTaskGetRunTimeStats

The workaround includes modifying portmacro.h:
```
#ifndef portGET_RUN_TIME_COUNTER_VALUE
#define portGET_RUN_TIME_COUNTER_VALUE()  xthal_get_ccount()
#endif 
```

If you're experiencing crashes, try to increase the stack size for the task running vTaskGetRunTimeStats.

