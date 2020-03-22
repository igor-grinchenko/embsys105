/************************************************************************************

Copyright (c) 2001-2016  University of Washington Extension.

Module Name:

    tasks.c

Module Description:

    The tasks that are executed by the test application.

2016/2 Nick Strathy adapted it for NUCLEO-F401RE 
2020/3 Igor Grinchenko added MP3 player tasks

************************************************************************************/
#include <stdarg.h>

#include "bsp.h"
#include "print.h"
#include "mp3Util.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>

Adafruit_ILI9341 lcdCtrl = Adafruit_ILI9341(); // The LCD controller
Adafruit_FT6206 touchCtrl = Adafruit_FT6206(); // The touch controller

#define PENRADIUS 3

long MapTouchToScreen(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#include "PopcornRemix.h"

#define BUFSIZE 256

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/

static OS_STK   LcdTouchTaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   Mp3TaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   LcdStatusTaskStk[APP_CFG_TASK_START_STK_SIZE];

// Task prototypes
void LcdTouchTask(void* pdata);
void Mp3Task(void* pdata);
void LcdStatusTask(void* pdata);

// Useful functions
void PrintToLcdWithBuf(char *buf, int size, char *format, ...);

// Globals
BOOLEAN nextSong = OS_FALSE;

OS_EVENT *commandMsg;

InputCommandEnum InputCommand;

/************************************************************************************

   This task is the initial task running, started by main(). It starts
   the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
    char buf[BUFSIZE];

    PjdfErrCode pjdfErr;
    INT32U length;
    static HANDLE hSD = 0;
    static HANDLE hSPI = 0;

    PrintWithBuf(buf, BUFSIZE, "StartupTask: Begin\n");
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Starting timer tick\n");

    // Start the system tick
    OS_CPU_SysTickInit(OS_TICKS_PER_SEC);
    
    // Initialize SD card
    PrintWithBuf(buf, PRINTBUFMAX, "Opening handle to SD driver: %s\n", PJDF_DEVICE_ID_SD_ADAFRUIT);
    hSD = Open(PJDF_DEVICE_ID_SD_ADAFRUIT, 0);
    if (!PJDF_IS_VALID_HANDLE(hSD)) while(1);


    PrintWithBuf(buf, PRINTBUFMAX, "Opening SD SPI driver: %s\n", SD_SPI_DEVICE_ID);
    // We talk to the SD controller over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to 
    // the SD driver.
    hSPI = Open(SD_SPI_DEVICE_ID, 0);
    if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);
    
    length = sizeof(HANDLE);
    pjdfErr = Ioctl(hSD, PJDF_CTRL_SD_SET_SPI_HANDLE, &hSPI, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);
    
    // Create the test tasks
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");

    // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
    OSTaskCreate(Mp3Task, (void*)0, &Mp3TaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST1_PRIO);
    OSTaskCreate(LcdTouchTask, (void*)0, &LcdTouchTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST2_PRIO);
    OSTaskCreate(LcdStatusTask, (void*)0, &LcdStatusTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST3_PRIO);
    
    commandMsg = OSMboxCreate(NULL);
    
    // Delete ourselves, letting the work be done in the new tasks.
    PrintWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
	OSTaskDel(OS_PRIO_SELF);
}

Adafruit_GFX_Button buttonPlay = Adafruit_GFX_Button();
Adafruit_GFX_Button buttonPause = Adafruit_GFX_Button();
Adafruit_GFX_Button buttonStop = Adafruit_GFX_Button();

static void DrawLcdContents()
{
    lcdCtrl.fillScreen(ILI9341_WHITE);
    
    buttonPlay.initButton(
                          &lcdCtrl,
                          41, 125,         // x, y coordinates
                          73, 50,           // width, height
                          ILI9341_BLACK,    // outline
                          ILI9341_WHITE,    // fill
                          ILI9341_BLACK,    // text color
                          "PLAY",           // label
                          2);               // text size
    
    buttonPause.initButton(
                          &lcdCtrl,
                          119, 125,         // x, y coordinates
                          73, 50,           // width, height
                          ILI9341_BLACK,    // outline
                          ILI9341_WHITE,    // fill
                          ILI9341_BLACK,    // text color
                          "PAUSE",           // label
                          2);               // text size  
    
    buttonStop.initButton(
                          &lcdCtrl,
                          197, 125,         // x, y coordinates
                          73, 50,           // width, height
                          ILI9341_BLACK,    // outline
                          ILI9341_WHITE,    // fill
                          ILI9341_BLACK,    // text color
                          "STOP",           // label
                          2);               // text size
    
    buttonPlay.drawButton();
    buttonPause.drawButton();
    buttonStop.drawButton();

}

void LcdStatusTask(void* pdata)
{
  char buf[BUFSIZE];
  OS_CPU_SR  cpu_sr = 0u;
  
  InputCommandEnum previousInputCommand;
  
 while (1) { 
  
  if (InputCommand == previousInputCommand)
  {
    OSTimeDly(50); // do nothing
  }
  
  if (InputCommand != previousInputCommand)
  {
    OS_ENTER_CRITICAL();
    switch (InputCommand)
      {
        case INPUTCOMMAND_PLAY:
          lcdCtrl.fillRect(0, 280, 240, 20, ILI9341_WHITE);
          lcdCtrl.setCursor(75, 280);
          lcdCtrl.setTextColor(ILI9341_BLACK);  
          lcdCtrl.setTextSize(2);
          PrintToLcdWithBuf(buf, BUFSIZE, "Playing..."); 
          break;
        case INPUTCOMMAND_PAUSE:
          lcdCtrl.fillRect(0, 280, 240, 20, ILI9341_WHITE);
          lcdCtrl.setCursor(75, 280);
          lcdCtrl.setTextColor(ILI9341_BLACK);  
          lcdCtrl.setTextSize(2);
          PrintToLcdWithBuf(buf, BUFSIZE, "Paused");
          break;
        case INPUTCOMMAND_STOP:
          lcdCtrl.fillRect(0, 280, 240, 20, ILI9341_WHITE);
          lcdCtrl.setCursor(75, 280);
          lcdCtrl.setTextColor(ILI9341_BLACK);  
          lcdCtrl.setTextSize(2);
          PrintToLcdWithBuf(buf, BUFSIZE, "Stopped");
          break;
      default:
          break;
       } 
    previousInputCommand = InputCommand;
    OS_EXIT_CRITICAL();
  }
}
}

/************************************************************************************

   Runs LCD/Touch code

************************************************************************************/
void LcdTouchTask(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT32U length;
    
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "LcdTouchTask: starting\n");

    PrintWithBuf(buf, BUFSIZE, "Opening LCD driver: %s\n", PJDF_DEVICE_ID_LCD_ILI9341);
    // Open handle to the LCD driver
    HANDLE hLcd = Open(PJDF_DEVICE_ID_LCD_ILI9341, 0);
    if (!PJDF_IS_VALID_HANDLE(hLcd)) while(1);

    PrintWithBuf(buf, BUFSIZE, "Opening LCD SPI driver: %s\n", LCD_SPI_DEVICE_ID);
    // We talk to the LCD controller over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to 
    // the LCD driver.
    HANDLE hSPI = Open(LCD_SPI_DEVICE_ID, 0);
    if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);

    length = sizeof(HANDLE);
    pjdfErr = Ioctl(hLcd, PJDF_CTRL_LCD_SET_SPI_HANDLE, &hSPI, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    PrintWithBuf(buf, BUFSIZE, "Initializing LCD controller\n");
    lcdCtrl.setPjdfHandle(hLcd);
    lcdCtrl.begin();

    DrawLcdContents();
    
    HANDLE htouch = Open(PJDF_DEVICE_ID_I2C1, 0);
    if (!PJDF_IS_VALID_HANDLE(htouch)) while(1);
   
    pjdfErr = Ioctl(htouch, PJDF_CTRL_I2C_SET_DEVICE_ADDRESS, (void*)FT6206_ADDR, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);
    
    touchCtrl.setPjdfHandle(htouch);
    
    PrintWithBuf(buf, BUFSIZE, "Initializing FT6206 touchscreen controller\n");
    if (! touchCtrl.begin(40)) {  // pass in 'sensitivity' coefficient
      PrintWithBuf(buf, BUFSIZE, "Couldn't start FT6206 touchscreen controller\n");
      while (1);
    }

    while (1) { 
        boolean touched = false;
        
        touched = touchCtrl.touched();
                
        if (! touched) {
            OSTimeDly(5);
            continue;
        }
        
        TS_Point rawPoint;
       
        if (touchCtrl.touched()) {
        rawPoint = touchCtrl.getPoint();
        }

        if (rawPoint.x == 0 && rawPoint.y == 0)
        {
            continue; // usually spurious, so ignore
        }
        
        // PLAY Command
        if (rawPoint.x >= 162 && rawPoint.x <= 240 && rawPoint.y >= 170 && rawPoint.y <= 220)
        {
        InputCommand = INPUTCOMMAND_PLAY;
        OSMboxPost(commandMsg, (void*)&InputCommand);
        }
        
        // PAUSE Command
        if (rawPoint.x >= 84 && rawPoint.x <= 157 && rawPoint.y >= 170 && rawPoint.y <= 220 && InputCommand == INPUTCOMMAND_PLAY)
        {
        InputCommand = INPUTCOMMAND_PAUSE;
        OSMboxPost(commandMsg, (void*)&InputCommand);
        }
        
        // STOP Command
        if (rawPoint.x >= 0 && rawPoint.x <= 79 && rawPoint.y >= 170 && rawPoint.y <= 220 && (InputCommand == INPUTCOMMAND_PLAY || InputCommand == INPUTCOMMAND_PAUSE))
        {
        InputCommand = INPUTCOMMAND_STOP;
        OSMboxPost(commandMsg, (void*)&InputCommand);
        }
    }
}
/************************************************************************************

   Runs MP3 code

************************************************************************************/
void Mp3Task(void* pdata)
{
    PjdfErrCode pjdfErr;
    INT32U length;

    OSTimeDly(2000); // Allow other task to initialize LCD before we use it.
    
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "Mp3Task: starting\n");

    PrintWithBuf(buf, BUFSIZE, "Opening MP3 driver: %s\n", PJDF_DEVICE_ID_MP3_VS1053);
    // Open handle to the MP3 decoder driver
    HANDLE hMp3 = Open(PJDF_DEVICE_ID_MP3_VS1053, 0);
    if (!PJDF_IS_VALID_HANDLE(hMp3)) while(1);

    PrintWithBuf(buf, BUFSIZE, "Opening MP3 SPI driver: %s\n", MP3_SPI_DEVICE_ID);
    // We talk to the MP3 decoder over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to 
    // the MP3 driver.
    HANDLE hSPI = Open(MP3_SPI_DEVICE_ID, 0);
    if (!PJDF_IS_VALID_HANDLE(hSPI)) while(1);

    length = sizeof(HANDLE);
    pjdfErr = Ioctl(hMp3, PJDF_CTRL_MP3_SET_SPI_HANDLE, &hSPI, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    // Send initialization data to the MP3 decoder and run a test
    PrintWithBuf(buf, BUFSIZE, "Starting MP3 device\n");
    Mp3Init(hMp3);
    int count = 0;
    
    while (1)
    {
      OSTimeDly(500);
      if (InputCommand == INPUTCOMMAND_PLAY) 
      {        
        PrintWithBuf(buf, BUFSIZE, "Begin streaming sound file  count=%d\n", ++count);
        Mp3Stream(hMp3, (INT8U*)Popcorn, sizeof(Popcorn));
        PrintWithBuf(buf, BUFSIZE, "Done streaming sound file  count=%d\n", count);
        OSMboxPost(commandMsg, (void*)&InputCommand);
      }
    }
}


// Renders a character at the current cursor position on the LCD
static void PrintCharToLcd(char c)
{
    lcdCtrl.write(c);
}

/************************************************************************************

   Print a formated string with the given buffer to LCD.
   Each task should use its own buffer to prevent data corruption.

************************************************************************************/
void PrintToLcdWithBuf(char *buf, int size, char *format, ...)
{
    va_list args;
    va_start(args, format);
    PrintToDeviceWithBuf(PrintCharToLcd, buf, size, format, args);
    va_end(args);
}
