//Igor Grinchenko
//EMBSYS105
//Assignment 1
//01/11/20

#include "bsp.h"
#include "print.h"


char clr_scrn[] = { 27, 91, 50, 74, 0 };              // esc[2J
/* Public variables ----------------------------------------------------------*/
PRINT_DEFINEBUFFER();           /* required for lightweight sprintf */
/* Private prototype ---------------------------------------------------------*/

void SystemInit(void);

// Generates a fault exception
void GenerateFault()
{
    static int count = 0;

    count++;

    if (count > 10)
    {
        PrintString("\nSuccess!\n");
        while (1);
    }

    int *a = (int*) 0x00080000;  // Set reserved memory address to cause a hard fault
    int b = *a;
    b = b; // This line just avoids a compiler warning

    PrintString("\nFail! Should not arrive here.\n");
    while (1);
    
}

void main() {
    Hw_init();
    
    PrintString(clr_scrn); /* Clear entire screen */
    PrintString("University of Washington - Debugger Test Application \n");

    GenerateFault();

    PrintString("\nFail! Should not arrive here.\n");
    while(1);
}


void SystemInit(void) {
  // System init is called from the assembly .s file
  // We will configure the clocks in hw_init
  asm("nop");
}
  