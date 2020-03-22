
#include "nucleoboard.h"

/**
  * @brief  Print a character on the HyperTerminal
  * @param  c: The character to be printed
  * @retval None
  */
void PrintByte(char c)
{
  USART_SendData(COMM, c);
  while (USART_GetFlagStatus(COMM, USART_FLAG_TXE) == RESET)
  {
  }
}