// Lin Architecture header file for Lin.c version 2.1.2 
#ifndef __Lin_H__ 
#define __Lin_H__ 

/* Comments: 
	DataStructure of the Stack: 
		... 				<- High Address 
		MsgLast	 12 						: points to the last element in the message list 
		MsgFirst 8 							: points to the first(front) element in the message list 
		StkBtm 	 4 							: indicates where the stack ends 
		StkPtr 	 0 	<- _TCB 		: the storage place for the stack pointer 
		Loop 		-4 							: Loop point 
		Counter -8 							: Loop counter 
		Arg1Bkp -12 						: Arg1 Reload Value 
		Arg0Bkp -26 <- _ECB 		: Arg0 Reload Value 
		xPSR 		-20 						: Initialized as 0x01000000 (Thumb State) 
		PC 			-24 						: Entrance Point 
		LR 			-28 						: ThreadExit Routine 
		Reg12 	-32 						: Context Storage 
		Reg3 		-36 						: reference of itself 
		Reg2 		-40 						: Loop counter value 
		Reg1 		-44 						: Arg1 
		Reg0 		-48 <- _HWStkF 	: Arg0 
		Reg11 	-52 						: Context Storage 
		Reg10 	-56 						: Context Storage 
		Reg9 		-60 						: Context Storage 
		Reg8 		-64 						: Context Storage 
		Reg7 		-68 						: Context Storage 
		Reg6 		-72 						: Context Storage 
		Reg5 		-76 						: Context Storage 
		Reg4 		-80 <- _CtxS 		: Context Storage 
		... 										: Stack Region 
														: ... 
														: ... 
		... 										: PCB Region 
								<- StkBtm 	: Start of the Process Control Block 
		...					<- Low Address 

	Task Function Params: 
   void funcTask(u32 Arg0, u32 Arg1, u32 Counter, Task_r Self); 
	 Arg0: user applied argument, default 0 
	 Arg1: user applied argument, default 0 
	 Counter: the loop counter, increaced after each ThreadExit routine 
	 Self: the reference of the Task itself 
	 The function does not return anything. 
*/ 


// Configurations 
#define SVCn_LinSwitch 0 
#define SVCn_LinTrigger 1 


// Definitions 
#define NULL ((void *)0) 


// Data Structures: 
typedef struct Msg_t{ 
	int Type; 
	struct _Task_r * Source; 
	int Value; 
	void * Content; 
}Msg_t; 
typedef struct _MsgBlk{ 
	struct _MsgBlk * Prev; 	
	struct _MsgBlk * Next; 
	Msg_t						 Msg; 
}_MsgBlk, * MsgBlk; 
typedef struct _Task_r{ 
	u32 *  StkPtr; 	 u32 *  StkBtm; 
	MsgBlk MsgFirst; MsgBlk MsgLast; 
}_Task_r, * TCB, * Task_r; 
typedef void (* TaskFunc_t)(u32, u32, u32, Task_r); 
typedef struct _ECB{ 
	u32 Arg0Bkp; u32 Arg1Bkp; 
	u32 counter; TaskFunc_t LoopEntrance; 
}_ECB, * ECB; 
typedef struct _HWStkF{ 
	u32 Reg0;  u32 		Reg1; u32 	 Reg2; u32 Reg3; 
	u32 Reg12; void * LR; 	void * PC; 	 u32 xPSR; 
}_HWStkF, * HWStkF; 
typedef struct _CtxS{ 
	u32 Reg4; u32 Reg5; u32 Reg6;  u32 Reg7; 
	u32 Reg8; u32 Reg9; u32 Reg10; u32 Reg11; 
}_CtxS, * CtxS; 


// External Interface: 
	extern Task_r 											_MainTask; 
	extern Task_r 											_CurrTask; 
	extern void 												Lin_Init							(void); 
	extern Task_r  											Lin_New								(u32, TaskFunc_t); 
	extern void 												Lin_SetArgs						(Task_r, u32, u32); 
	extern void 												Lin_SetSetup					(Task_r, TaskFunc_t); 
	extern void 												Lin_Delete						(Task_r); 
	
	extern int  												Lin_Do								(Task_r); 
	extern void __svc(SVCn_LinSwitch) 	Lin_Switch						(Task_r); 
	extern void 												Lin_SwitchISR					(Task_r); 
	#define 														Lin_Yield() 																Lin_Switch(_MainTask) 
	#define 														Lin_YieldISR() 															Lin_SwitchISR(_MainTask) 
	extern void __svc(SVCn_LinTrigger) 	Lin_Return						(int); 
	
	extern int  												Lin_MsgWrite					(Task_r, Msg_t); 
	extern int  												Lin_MsgWriteFront			(Task_r, Msg_t); 
	extern Msg_t 												Lin_MsgRead						(Task_r); 
	extern Msg_t 												Lin_MsgReadLast				(Task_r); 
	#define 														Lin_MsgSubmit(Msg_t) 												Lin_MsgWrite(_MainTask, Msg_t) 
	#define 														Lin_MsgSubmitFront(Msg_t) 									Lin_MsgWriteFront(_MainTask, Msg_t) 
	#define 														Lin_MsgEmpty() 															(_CurrTask->MsgFirst == NULL) 
	#define 														Lin_MsgRecv() 															Lin_MsgRead(_CurrTask) 
	#define 														Lin_MsgRecvLast() 													Lin_MsgReadLast(_CurrTask) 

/* The memory part to be continued.. */ 
	extern void * 											Lin_MemAlloc					(u32); 
	extern void 												Lin_MemFree						(void *); 
	extern void * 											Lin_MemRealloc				(void *, u32); 
	

// Internal Interface: 
	extern void 												Lin_InitSw						(void); 
	extern Task_r  											Lin_StackInit					(void *, u32, TaskFunc_t); 
	extern int  __svc(SVCn_LinTrigger)  Lin_Enter							(Task_r); 
	
	extern void 												Lin_MsgPut						(Task_r, MsgBlk); 
	extern void 												Lin_MsgPutFront				(Task_r, MsgBlk); 
	extern MsgBlk 											Lin_MsgGet						(Task_r); 
	extern MsgBlk 											Lin_MsgGetLast				(Task_r); 
	extern u32  												Lin_MsgCritical				(Task_r, MsgBlk, void *); 
	extern void 												Lin_MsgPut_core				(Task_r, MsgBlk); 
	extern void 												Lin_MsgPutFront_core	(Task_r, MsgBlk); 
	extern MsgBlk 											Lin_MsgGet_core				(Task_r); 
	extern MsgBlk 											Lin_MsgGetLast_core		(Task_r); 

/* The memory part to be continued.. */ 
	extern void 												Lin_InitMem						(void); 


// MicroLib: 
	extern void * 											malloc								(u32); 
	extern void 												free									(void *); 
	extern void * 											realloc								(void *, u32); 

#endif 
// End of file. 
