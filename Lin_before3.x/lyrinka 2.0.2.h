// lyrinka Context Switching header file ver 2.0.2 final release fixing 
// "stm32f10x.h" or "LVTypes.h" must be included before this file. 
#ifndef __lysw_H__ 
#define __lysw_H__ 

#define SVCn_CtxSwSwitch 0 
#define SVCn_CtxSwTrig 1 

/* DataStructure of the Stack: 
		... 				<- High Address 
		com1 		 12 						: reserved for inter-task communications 
		com0 		 8 							: reserved for inter-task communications 
		StkBtm 	 4 							: indicates where the PCB locates 
		StkPtr 	 0 	<- _TCB 		: the storage place for the stack pointer 
		Loop 		-4 							: Loop point 
		Counter -8 							: Loop counter 
		Arg1Bkp -12 						: Arg1 Reload Value 
		Arg0Bkp -26 <- _ECB 		: Arg0 Reload Value 
		xPSR 		-20 						: Initialized as 0x01000000 (Thumb State) 
		PC 			-24 						: Entrance Point 
		LR 			-28 						: ThreadExit Routine 
		Reg12 	-32 						: Context Storage 
		Reg3 		-36 						: TCB of itself 
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
*/ 
/* Task Function Params: 
   void funcTask(u32 Arg0, u32 Arg1, u32 Counter, TCB Self); 
	 Arg0: user applied argument, default 0 
	 Arg1: user applied argument, default 0 
	 Counter: the loop counter, increaced after each ThreadExit routine 
	 Self: the TCB of the Task itself 
	 The function does not return anything. 
*/ 
typedef struct{ // Task Control Block 
	u32 * StkPtr; u32 * StkBtm; u32 Com0; u32 Com1; 
} _TCB, * TCB; 
typedef struct{ // Execution Control Block 
	u32 Arg0Bkp; u32 Arg1Bkp; u32 counter; void * LoopEntrance; 
} _ECB, * ECB; 
typedef struct{ // Hardware Stack Frame 
	u32 Reg0; 	u32 Reg1; 	u32 Reg2; 	u32 Reg3; 
	u32 Reg12; 	void * LR; 	void * PC; 	u32 xPSR; 
} _HWStkF, * HWStkF; 
typedef struct{ // Context Storage 
	u32 Reg4; 	u32 Reg5; 	u32 Reg6; 	u32 Reg7; 
	u32 Reg8; 	u32 Reg9; 	u32 Reg10; 	u32 Reg11; 
} _CtxS, * CtxS; 

// extern TCB ptrC, ptrN; 
// extern u32 * Tloader; 
extern void 												lysw_Init 			(void); 
extern TCB 													lysw_New 				(void * Memory, u32 Size, void * funcPtr); 
extern void 												lysw_SetArgs		(TCB Task, u32 Arg0, u32 Arg1); 
extern void 												lysw_SetSetup		(TCB Task, void * funcSetup); 
extern void __svc(SVCn_CtxSwSwitch) lysw_Switch			(TCB); 
extern void 												lysw_SwitchISR	(TCB); 
extern void __svc(SVCn_CtxSwTrig) 	lysw_Enter			(TCB); 
extern void __svc(SVCn_CtxSwTrig) 	lysw_Return			(void); 

#endif 
