// Igor Grinchenko
// EMBSYS105
// Assignment04
// 02/02/20

/************************************************************************************

Module Name:

    tasks.c

Module Description:

    Exercises several uCOS services

History:

    2015/1 Nick Strathy wrote it
    2016/2 Ported to Cortex-M4/IAR

************************************************************************************/

#include <stdarg.h>
#include <string.h>

#include "bsp.h"
#include "print.h"

#define BUFSIZE 150

#define TASKLOOPLIMIT 5

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/

static OS_STK   TaskMBTxStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   TaskMBRxAStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   TaskMBRxBStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   TaskQTxAStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   TaskQTxBStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   TaskQRxStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   TaskRxFlagsStk[APP_CFG_TASK_START_STK_SIZE];


// Task prototypes
void TaskMBTx(void* pdata);
void TaskMBRxA(void* pdata);
void TaskMBRxB(void* pdata);
void TaskQTxA(void* pdata);
void TaskQTxB(void* pdata);
void TaskQRx(void* pdata);
void TaskRxFlags(void* pdata);



// Useful functions
void printWithBuf(char *buf, int size, char *format, ...);

OS_EVENT * semPrint;
OS_EVENT * mboxA;
OS_EVENT * mboxB;


// message queue components

#define QMAXENTRIES 4            // maximum entries in the queue

typedef struct                   // a queue entry
{
	char *msg;
}QMsg_t;

OS_EVENT * qMsg;                 // pointer to a uCOS message queue
void * qMsgVPtrs[QMAXENTRIES];   // an array of void pointers which is the actual queue
QMsg_t qMsgBlocks[QMAXENTRIES];  // a pool of message nodes that may be used as queue entries
OS_MEM *qMsgMemPart;             // pointer to a uCOS memory partition to manage the pool of message nodes

// Event flags for synchronizing mailbox messages
OS_FLAG_GRP *rxFlags = 0;

INT32U TaskMBRxA_msgCount = 0;
INT32U TaskMBRxB_msgCount = 0;


/************************************************************************************

   This task is the initial task running, started by main(). It starts
   the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
	INT8U err;

	char buf[BUFSIZE];
           

    // TODO semPrint 01: add code here to create semaphore semPrint as a binary semaphore
    semPrint = OSSemCreate(1);
      
	printWithBuf(buf, BUFSIZE, "StartupTask: Begin\n");
	printWithBuf(buf, BUFSIZE, "StartupTask: Starting timer tick\n");

    // Start the system tick
    SysTick_Config(CLOCK_HSI / OS_TICKS_PER_SEC);

    printWithBuf(buf, BUFSIZE, "StartupTask: Creating application tasks\n");

	// TODO Mailbox 01: add code here to create 2 mailboxes mboxA and mboxB, initially empty
    mboxA = OSMboxCreate(NULL);
    mboxB = OSMboxCreate(NULL);
    
	// TODO Queue 01: add code here to create qMsg as a uCOS queue that uses qMsgVPtrs to store queue entry pointers
    qMsg = OSQCreate(qMsgVPtrs, QMAXENTRIES);
     
	// TODO Queue 02: add code here to create qMsgMemPart as a uCOS memory partition containing a pool
	// of QMAXENTRIES messages where each entry is of type QMsg_t
    qMsgMemPart = OSMemCreate(qMsgBlocks, QMAXENTRIES, sizeof(QMsg_t), &err);
    
	// TODO EventFlags 01: add code here to create event flag 'group' rxFlags
    rxFlags = OSFlagCreate(0, &err);        // 16bit flag
    
    // The maximum of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
	INT8U pri = APP_TASK_START_PRIO + 1;
    OSTaskCreate(TaskMBTx, (void*)0, (void*)&TaskMBTxStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);
    OSTaskCreate(TaskMBRxA, (void*)0, (void*)&TaskMBRxAStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);
    OSTaskCreate(TaskMBRxB, (void*)0, (void*)&TaskMBRxBStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);
    OSTaskCreate(TaskQTxA, (void*)0, (void*)&TaskQTxAStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);
    OSTaskCreate(TaskQTxB, (void*)0, (void*)&TaskQTxBStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);
    OSTaskCreate(TaskQRx, (void*)0, (void*)&TaskQRxStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);
    OSTaskCreate(TaskRxFlags, (void*)0, (void*)&TaskRxFlagsStk[APP_CFG_TASK_START_STK_SIZE-1], pri++);

    // Delete ourselves, letting the work be done in the new tasks.
    printWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
	OSTaskDel(OS_PRIO_SELF);
}

/************************************************************************************

TaskMBTx sends messages to TaskMBRxA and TaskMBRxB.

************************************************************************************/

void TaskMBTx(void* pdata)
{
	char buf[BUFSIZE];
	printWithBuf(buf, BUFSIZE, "TaskMBTx: starting\n");

	char msgA[TASKLOOPLIMIT][3] = {{0}};
	char msgB[TASKLOOPLIMIT][3] = {{0}};

	// initialize a set of 2-character null-terminated string messages to send to other tasks
	int i;
	for (i = 0; i < TASKLOOPLIMIT; i++)
	{
		msgA[i][0] = 'A';
		msgA[i][1] = '0' + (i % 10);

		msgB[i][0] = 'B';
		msgB[i][1] = '0' + (i % 10);
	}


    for (i = 0; i < TASKLOOPLIMIT; i++)
    {
    	// TODO Mailbox 02: add code here to send msgA[i] to TaskMBRxA using mailbox mboxA
    OSMboxPost(mboxA, (char*)msgA[i]);
      
        OSTimeDly(90);

    	// TODO Mailbox 03: add code here to send msgB[i] to TaskMBRxB using mailbox mboxB
    OSMboxPost(mboxB, (char*)msgB[i]);
        
        OSTimeDly(90);
    }
    printWithBuf(buf, BUFSIZE, "TaskMBTx: Done! Sent %d messages\n", 2 * TASKLOOPLIMIT);
    while (1) OSTimeDly(1000);
}

/************************************************************************************

TaskMBRxA receives messages sent to it by TaskMBTx.

************************************************************************************/

void TaskMBRxA(void* pdata)
{
	INT8U err;
	char buf[BUFSIZE];
	uint32_t errorCount = 0;
	const char *DummyMsg = "XX";

	char expected[3] = {"AA"};
	char *msgReceived = (char*) DummyMsg;

	printWithBuf(buf, BUFSIZE, "TaskMBRxA: starting\n");

	int i;
    for (i = 0; i < TASKLOOPLIMIT; i++)
    {
    	// TODO EventFlags 02: add code here to wait on bit 0 in rxFlags till TaskRxFlags clears it,
    	// indicating it is OK to receive the next message
          OSFlagPend(rxFlags, 0x1, OS_FLAG_WAIT_CLR_ALL, 0, &err);

    	// TODO Mailbox 04: add code here to receive a message in msgReceived from mailbox mboxA
           msgReceived = (char*)OSMboxPend(mboxA, 0, &err);
      
    	TaskMBRxA_msgCount += 1;
    	expected[1] = '0' + (i % 10);
    	if (strcmp(msgReceived, expected))
    	{
    		errorCount += 1;
    	}

    	// TODO EventFlags 03: add code here to set bit 0 in rxFlags, indicating to TaskRxFlags
    	// that we have finished receiving a message
           OSFlagPost(rxFlags, 0x1, OS_FLAG_SET, &err);

    	printWithBuf(buf, BUFSIZE, "TaskMBRxA: actual=%s, expected=%s, received=%d errors=%d\n",
    			msgReceived, expected, TaskMBRxA_msgCount, errorCount);
    	OSTimeDly(30);
    }
    printWithBuf(buf, BUFSIZE, "%sTaskMBRxA: Done! Received %d messages, errors=%d\n",
		errorCount > 0 ? "**ERROR:" : "", TASKLOOPLIMIT, errorCount);
    while (1) OSTimeDly(1000);
}



/************************************************************************************

TaskMBRxB receives messages sent to it by TaskMBTx.

************************************************************************************/

void TaskMBRxB(void* pdata)
{
	INT8U err;
	char buf[BUFSIZE];
	uint32_t errorCount = 0;
	const char *DummyMsg = "XX";

	char expected[3] = {"BB"};
	char *msgReceived = (char*) DummyMsg;

	printWithBuf(buf, BUFSIZE, "TaskMBRxB: starting\n");

	int i;
    for (i = 0; i < TASKLOOPLIMIT; i++)
    {
    	// TODO EventFlags 04: add code here to wait on bit 1 in rxFlags till TaskRxFlags clears it,
    	// indicating it is OK to receive the next message
           OSFlagPend(rxFlags, 0x2, OS_FLAG_WAIT_CLR_ALL, 0, &err);
      
    	// TODO Mailbox 05: add code here to receive a message in msgReceived from mboxB
           msgReceived = (char*)OSMboxPend(mboxB, 0, &err);
      
    	TaskMBRxB_msgCount += 1;
    	expected[1] = '0' + (i % 10);
    	if (strcmp(msgReceived, expected))
    	{
    		errorCount += 1;
    	}

    	// TODO EventFlags 05: add code here to set bit 1 in rxFlags, indicating to TaskRxFlags
    	// that we have finished receiving a message
           OSFlagPost(rxFlags, 0x2, OS_FLAG_SET, &err);

    	printWithBuf(buf, BUFSIZE, "TaskMBRxB: actual=%s, expected=%s, received=%d errors=%d\n",
    			msgReceived, expected, TaskMBRxB_msgCount, errorCount);
    	OSTimeDly(10);
    }
    printWithBuf(buf, BUFSIZE, "%sTaskMBRxB: Done! received %d messages, errors=%d\n",
		errorCount > 0 ? "**ERROR:" : "", TASKLOOPLIMIT, errorCount);
    while (1) OSTimeDly(1000);
}

/************************************************************************************

TaskQTxA sends messages to TaskQRx.

************************************************************************************/

void TaskQTxA(void* pdata)
{
	INT8U err;
	char buf[BUFSIZE];
	QMsg_t *pQmsgToSend;

	printWithBuf(buf, BUFSIZE, "TaskQTxA: starting\n");

	char msg[TASKLOOPLIMIT][3] = {{0}};

	// initialize a set of 2-character null-terminated string messages to send to other tasks
	int i;
	for (i = 0; i < TASKLOOPLIMIT; i++)
	{
		msg[i][0] = 'A';
		msg[i][1] = '0' + (i % 10);
	}

    for (i = 0; i < TASKLOOPLIMIT; i++)
    {
//		// allocate a queue entry
		pQmsgToSend = (QMsg_t*) OSMemGet(qMsgMemPart, &err);
		if (err != OS_ERR_NONE)
		{
			printWithBuf(buf, BUFSIZE, "Not enough message blocks");
			while (OS_TRUE);
		}

		pQmsgToSend->msg = (char*)msg[i];
		err = OS_ERR_NONE;
		do
		{
			OSTimeDly(5);

//             // TODO Queue 03: first uncomment this code block (highlight and hit Ctrl-Shift-K)
//    	    // then add code here to send pQmsgToSend to TaskQRx using qMsg
                      err = OSQPost(qMsg, (void*)pQmsgToSend);      
                                 
		} while (err == OS_ERR_Q_FULL);

        printWithBuf(buf, BUFSIZE, "TaskQTxA: sent msg %s\n", msg[i]);

        OSTimeDly(90);
    }
    printWithBuf(buf, BUFSIZE, "TaskQTxA: Done! Sent %d messages\n", TASKLOOPLIMIT);
    while (1) OSTimeDly(1000);
}


/************************************************************************************

TaskQTxB sends messages to TaskQRx.

************************************************************************************/

void TaskQTxB(void* pdata)
{
	INT8U err;
	char buf[BUFSIZE];
	printWithBuf(buf, BUFSIZE, "TaskQTxB: starting\n");

	char msg[TASKLOOPLIMIT][3] = {{0}};

	// initialize a set of 2-character null-terminated string messages to send to other tasks
	int i;
	for (i = 0; i < TASKLOOPLIMIT; i++)
	{
		msg[i][0] = 'B';
		msg[i][1] = '0' + (i % 10);
	}

    for (i = 0; i < TASKLOOPLIMIT; i++)
    {
		// allocate a queue entry
		QMsg_t *pQmsgToSend = OSMemGet(qMsgMemPart, &err);
		if (err != OS_ERR_NONE)
		{
			printWithBuf(buf, BUFSIZE, "Not enough message blocks");
			while (OS_TRUE);
		}

		pQmsgToSend->msg = (char*)msg[i];
		do
		{
			OSTimeDly(5);

//             // TODO Queue 04: first uncomment this code block (highlight and hit Ctrl-Shift-K)
//    	    // then add code here to send pQmsgToSend to TaskQRx using qMsg
                      err = OSQPost(qMsg, (void*)pQmsgToSend);
                                 
		} while (err == OS_ERR_Q_FULL);

		printWithBuf(buf, BUFSIZE, "TaskQTxB: sent msg %s\n", msg[i]);

        OSTimeDly(30);
    }
    printWithBuf(buf, BUFSIZE, "TaskQTxB: Done! Sent %d messages\n", TASKLOOPLIMIT);
    while (1) OSTimeDly(1000);
}

/************************************************************************************

TaskQRx receives messages from TaskQTxA and TaskQTxB.

************************************************************************************/

void TaskQRx(void* pdata)
{
	INT8U err;
	char buf[BUFSIZE];
	char expectedMsg[2 * TASKLOOPLIMIT][3] = {{0}};
	INT32U msgCount[2 * TASKLOOPLIMIT] = {0};
	BOOLEAN isError = OS_FALSE;
	QMsg_t *pQMsgReceived;
	char msgReceived[3] = {"XX"};

	printWithBuf(buf, BUFSIZE, "TaskQRx: starting\n");

	// initialize a set of expected 2-character null-terminated string messages we should receive
	int i;
	for (i = 0; i < TASKLOOPLIMIT; i++)
	{
		expectedMsg[i][0] = 'A';
		expectedMsg[i][1] = '0' + (i % 10);
		expectedMsg[i+TASKLOOPLIMIT][0] = 'B';
		expectedMsg[i+TASKLOOPLIMIT][1] = '0' + (i % 10);
	}

	// Receive messages from 2 tasks hence cycle TASKLOOPLIMIT twice
    for (i = 0; i < TASKLOOPLIMIT * 2; i++)
    {
//    	// TODO Queue 05: first uncomment this code block (highlight and hit Ctrl-Shift-K)
//    	// then add code here to receive a message in pQMsgReceived from qMsg
           pQMsgReceived = OSQPend(qMsg, 0, &err);
      
    	memcpy(msgReceived, pQMsgReceived->msg, 3);

//		// free the queue msg so it can be reused
		OSMemPut(qMsgMemPart, pQMsgReceived);

		printWithBuf(buf, BUFSIZE, "TaskQRx: received msg %s\n", msgReceived);

 		// Verify that we received one of the expected messages
 		int j;
 		for (j = 0; j < 2 * TASKLOOPLIMIT; j++)
 		{
 			if (!strcmp(msgReceived, expectedMsg[j]))
 			{
 				msgCount[j]++;
 				if (msgCount[j] > 1)
 				{
 					isError = OS_TRUE;
 				}
 				break;
 			}
 		}
 		if (j == 2 * TASKLOOPLIMIT)
 		{
 			isError = OS_TRUE;
 		}

		OSTimeDly(10);
    }
    if (isError)
    {
    	for (i = 0; i < 2 * TASKLOOPLIMIT; i++)
    	{
    		if (msgCount[i] != 1)
    		{
    	    	printWithBuf(buf, BUFSIZE,
					"**ERROR: TaskQRx: msg=%s: expected to receive 1 instance, actual=%d\n", expectedMsg[i], msgCount[i]);
    		}
    	}
    }
    else
    {
		printWithBuf(buf, BUFSIZE, "TaskQRx: Done! Received %d/%d messages.\n",
			2 * TASKLOOPLIMIT, 2 * TASKLOOPLIMIT);
    }
    while (1) OSTimeDly(1000);
}


/************************************************************************************

TaskRxFlags synchronizes TaskMBRxA and TaskMBRxB so they each must receive their current message
before either can receive their next message.

************************************************************************************/

void TaskRxFlags(void* pdata)
{
	INT8U err;
	INT8U iError = 0;
	char buf[BUFSIZE];
	BOOLEAN isError = OS_FALSE;

	printWithBuf(buf, BUFSIZE, "TaskRxFlags: starting\n");

	int i;
    for (i = 1; i <= TASKLOOPLIMIT; i++)
    {
    	// TODO EventFlags 06: add code here to wait for both TaskMBRxA and TaskMBRxB to signal that they have
    	// received a message      
           OSFlagPend(rxFlags, 0x3, OS_FLAG_WAIT_SET_ALL, 0, &err);
           
    	isError = (i != TaskMBRxA_msgCount) || (i != TaskMBRxB_msgCount);
    	if (isError && !iError)
    	{
    		iError = i;
    	}

    	printWithBuf(buf, BUFSIZE,
			"TaskRxFlags: (TaskMBRxA_msgCount expected=%d actual=%d) (TaskMBRxB_msgCount expected=%d actual=%d)\n",
			i, TaskMBRxA_msgCount, i, TaskMBRxB_msgCount);

    	// TODO EventFlags 07: add code here to signal to both TaskMBRxA and TaskMBRxB that they can
    	// go ahead and receive their next message.
           OSFlagPost(rxFlags, 0x3, OS_FLAG_CLR, &err);
           
           
    	OSTimeDly(10);
    }
    if (iError)
    {
    	printWithBuf(buf, BUFSIZE, "**ERROR: TaskRxFlags: Done! Out of sync beginning at message %d\n", iError);
    }
    else
    {
    	printWithBuf(buf, BUFSIZE,
			"TaskRxFlags: Done! (TaskMBRxA_msgCount expected=%d actual=%d) (TaskMBRxB_msgCount expected=%d actual=%d)\n",
			i-1, TaskMBRxA_msgCount, i-1, TaskMBRxB_msgCount);
    }
    while (1) OSTimeDly(1000);
}


/************************************************************************************

   Print a formated string with the given buffer.
   Each task should use its own buffer to prevent data corruption.

************************************************************************************/
void printWithBuf(char *buf, int size, char *format, ...)
{
	INT8U err;
    va_list args;
    va_start(args, format);
    vsnprintf(buf, size, format, args);

    // TODO semPrint 02: place the call to PrintString in a critical section using semaphore semPrint
    OSSemPend(semPrint, 0, &err);
    PrintString(buf);
    OSSemPost(semPrint);
    
    va_end(args);
}

