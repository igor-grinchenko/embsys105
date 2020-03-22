#ifndef __BSP_H
#define __BSP_H

// Add interfaces to the specific hardware for use by application code
#include <os_cpu.h>
#include <os_cfg.h>
#include <app_cfg.h>
#include <ucos_ii.h>

#include "nucleoboard.h"
#include "hw_init.h"
#include "uart.h"


void SetLED(BOOLEAN On);

#endif /* __BSP_H */