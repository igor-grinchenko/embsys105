/*
    mp3Util.h
    Some utility functions for controlling the MP3 decoder.

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
    2020/3 Igor Grinchenko added enum InputCommandEnum
*/

#ifndef __MP3UTIL_H
#define __MP3UTIL_H


PjdfErrCode Mp3GetRegister(HANDLE hMp3, INT8U *cmdInDataOut, INT32U bufLen);
void Mp3Init(HANDLE hMp3);
void Mp3Test(HANDLE hMp3);
void Mp3Stream(HANDLE hMp3, INT8U *pBuf, INT32U bufLen);
void Mp3StreamSDFile(HANDLE hMp3, char *pFilename);

typedef enum
{
  INPUTCOMMAND_NONE,
  INPUTCOMMAND_PLAY,
  INPUTCOMMAND_PAUSE,
  INPUTCOMMAND_STOP,  
} InputCommandEnum;

#endif