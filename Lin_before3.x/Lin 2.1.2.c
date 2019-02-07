// Lin Architecture version 2.1.2 
// ********************************************************************************************** 
/* 
	The Lin Architecture. 
	Mostly optimized for fastest execution speed. 
	
	Release Notes: 
	
	<2.1.2>[181020] Added centered framework to Messaging features. 
									Some GV changed names. 
									Changed message format, added features for source identification. 
	<2.1.1>[181019] Added Low-level Messaging features in C Code. 
	<2.1.0>[181019] Combined lysw & lyct, added messaging and other features: 
	                Some functions changed names. 
	                The Do (Enter) function can receive an int returned with the Return SVC Call. 
									Added DataStructures for messaging. 
									Added functions operate on double linked lists for messaging. 
									We can use this version like an ordinary Lin Switching framework right now. 
*/ 
#include <stm32f10x.h> 
#include <Lin.h> 


// Functions: 

// ********************************************************************************************** 
// Lin Switching Environment Initialization 
// local version 0 not changed. debugged 181014 
void Lin_InitSw(void){ 
	// User Functions - NVIC and Handler Initialization 
	SCB->CCR |= 1 << 9; 																	// Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); 							// Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; 																	// PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); 														// SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4) | 15; 	// PendSV Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4); 			// SVC Pp same as PendSV, Sp higher than PendSV 
}
// End of a section. 

// ********************************************************************************************** 
// Initialization of the stack of a new task. 
/* Initialize the task stack and associated control blocks 
    in a given part of memory(from Memory with size Size) 
		and return the Task Control Block. 
*/ 
// local version 1.2 debugged. 181014 
__asm Task_r Lin_StackInit(void * Memory, u32 Size, TaskFunc_t funcPtr){ 
		ADD		R3,  R0, R1 		
		SUB		R3,  #16 				// TCB = (TCB)((u8 *)Memory + Size - sizeof(_TCB)); 
		STR		R0, [R3, #4] 		// TCB->StkBtm = Memory; 
		SUB		R0,  R3, #80 
		STR		R0, [R3] 				// TCB->StkPtr = (u32 *)((u8 *)PCB - sizeof(_ECB) - sizeof(_HWStkF) - sizeof(_CtxS)); 
		MOV		R0,  #0 
		STR		R0, [R3, #12] 	// TCB->MsgLast = NULL; 
		STR		R0, [R3, #8] 		// TCB->MsgFirst = NULL; 
		STR		R2, [R3, #-4] 	// ECB->LoopEntrance = funcPtr; 
		STR		R0, [R3, #-8]  	// ECB->Counter = 0; 
		STR		R0, [R3, #-12] 	// ECB->Arg1Bkp = 0; 
		STR		R0, [R3, #-16] 	// ECB->Arg0Bkp = 0; 
		MOV		R1,  #0x01000000 
		STR		R1, [R3, #-20] 	// HWStkF->xPSR = 0x01000000; 
		STR		R2, [R3, #-24] 	// HWStkF->PC = funcPtr; 
		LDR		R1, =ThreadExit 
		STR		R1, [R3, #-28] 	// HWStkF->LR = ThreadExit; 
		STR		R0, [R3, #-32] 	// HWStkF->Reg12 = 0; 
		STR		R3, [R3, #-36] 	// HWStkF->Reg3 = TCB; 
		STR		R0, [R3, #-40] 	// HWStkF->Reg2 = 0; 
		STR		R0, [R3, #-44] 	// HWStkF->Reg1 = 0; 
		STR		R0, [R3, #-48] 	// HWStkF->Reg0 = 0; 
		MOV		R0,  R3 				// return TCB 
		BX		LR 
		NOP 
ThreadExit 
		LDR		LR, =ThreadExit 
		LDMIA	SP, {R0-R3} 
		ADD		R2,  #1 
		STR		R2, [SP, #8] 
		MOV		R12, R3 
		ADD		R3,  SP, #16 
		BX		R12 
}
__asm void Lin_SetArgs(Task_r Task, u32 Arg0, u32 Arg1){ 
		STR		R1, [R0, #-16] 	// ECB->Arg0Bkp = Arg0; 
		STR		R1, [R0, #-48] 	// HWStkF->Arg0 = Arg0; 
		STR		R2, [R0, #-12] 	// ECB->Arg1Bkp = Arg1; 
		STR		R2, [R0, #-44] 	// HWStkF->Arg1 = Arg1; 
		BX		LR 
}
__asm void Lin_SetSetup(Task_r Task, TaskFunc_t funcSetup){ 
		STR		R1, [R0, #-24] 	// HWStkF->PC = funcSetup; 
		BX		LR 
}
// End of a section. 

// ********************************************************************************************** 
// General Context Switching Operation. 
/* PendSV Handler for context switching. 
    Current Task Handle in GV _CurrTask, 
    Next Task to be run Handle in GV ptrN. 
*/ 
// local version 1.2 debugged. 181019 
Task_r _CurrTask, _NextTask; // Current Running & Next to be Run 
__asm void PendSV_Handler(void){ // The 1.0.6 version framework not changed 
		// PendSV Handler (Context Switch Performer) 
		// Disable interrupts will prevent any context switches. 
	IMPORT	_CurrTask 
	IMPORT	_NextTask 
		LDR		R2, =_CurrTask 
		LDR		R1, =_NextTask 
		LDR		R0, [R2] 			// CurrTaskHandle in R0 
		LDR		R1, [R1] 			// NextTaskHandle in R1 
		STR		R1, [R2] 			// NextTaskHandle stored in _CurrTask 
		MRS		R2,  PSP 			// A1 Get   PSP1 from Processor 
		STMDB R2!,{R4-R11} 	// A2 Push  Context {R4-R11} to PSP1  
		STR		R2, [R0]			// A3 Store PSP1 via CurrTaskHandle->StkPtr 
		LDR		R2, [R1]			// B1 Get   PSP2 via NextTaskHandle->StkPtr 
		LDMIA R2!,{R4-R11} 	// B2 Pop   Context {R4-R11} from PSP2 
		MSR		PSP, R2 			// B3 Set   PSP2 to Processor 
		BX		LR 						// Pop HW Context {R0-R3,R12,LR,PC,PSR} 
}
// End of a section. 

// ********************************************************************************************** 
// SVC Call Handler 
// local version 1.2 debugged. 181018 
/* SVC Call Handler for 
    Switch(including SwitchISR), Enter and Return. 
    other SVC Calls goes to SVC_ProxyCaller(u8 ID, u32 * StkF); . 
    The SVC numbers are defined by macros. 
*/ 
void __svc(SVCn_LinSwitch) 	Lin_Switch		(Task_r); 
void 												Lin_SwitchISR	(Task_r); 
int  __svc(SVCn_LinTrigger) Lin_Enter			(Task_r); 
void __svc(SVCn_LinTrigger) Lin_Return		(int); 
extern void SVC_ProxyCaller(u8 ID, u32 * StkF); 
u32 * _TaskLoader; // storing handle for the context of the main function 
__asm void SVC_Handler(void){ // The 1.0.6 version framework not changed 
		// SVC Call Handler 
		// Other SVC Numbers will trigger SVC_ProxyCaller(u8 ID, u32 * StkF); 
		TST		LR,  #4 
		ITE 	EQ 
		MRSEQ R1,  MSP 
		MRSNE R1,  PSP 								// Get HW Stack Frame Pointer. 
		LDR		R0, [R1, #24] 
		LDRB	R0, [R0, #-2] 					// Got HW StkF ptr in R1, SVC ID in R0. 
		CMP		R0,  #SVCn_LinSwitch 
		BNE		NotSwitch 							// Is Switch Op? 
Switch 														//  Do Switch Op 
		LDR		R0, [R1] 								//  Get Target Task Handle 
	EXPORT	Lin_SwitchISR 
Lin_SwitchISR 										//  Switches from ISR directly come here 
		LDR		R2, =_NextTask 
		LDR		R1, [R2] 								//  Get Handle in _NextTask 
		CMP		R0,  R1 								//  Handles Compared 
		ITTTT	NE 											//  If not equal- 
		STRNE	R0, [R2] 								//   put new handle in _NextTask 
		LDRNE	R1, =0xE000ED00 
		MOVNE	R0,  #0x10000000 
		STRNE	R0, [R1, #4] 						//   PendSV Pend Set (SCB->ICSR.28) 
		BX		LR 											//  Then PendSV executes on demand 
NotSwitch 												// Not Switch Op 
		CMP		R0,  #SVCn_LinTrigger		//  Is Trigger Op? 
		BNE.W	SVC_ProxyCaller 				//   No, go to SVC_ProxyCaller 
		TST		LR,  #4 								//  Is Trigger Op Enter or Return? 
		BNE		Return 									//   Trigger Op is Enter 
		MSR		PSP, R1 								//    Set PSP as current pointer for CtxS 
		SUB		R2,  R1, #32 
		MSR		MSP, R2 								//    MSP Stack Growth for CtxS (8 words) 
		ORR 	LR,  #4 								//    Modify LR to use PSP 
	IMPORT	_TaskLoader 
		LDR		R0, =_TaskLoader 
		LDR		R2, =_CurrTask 
		STR		R0, [R2] 								//    Set the storing handle to _CurrentTask 
		B			Switch 
Return 														//   Trigger Op is Return 
		BIC		LR,  #4 								//    Modify LR, use MSP 
		LDR		R3, [R1] 								//    Get return value, PensSV didn't use R3 
		LDR		R1, =_TaskLoader 
		LDR		R2, =_NextTask  
		STR		R1, [R2] 								//    Store the storage handle to _NextTask 
		MOV		R12,  LR 								//    PendSV didn't use R12 
		BL 		__cpp(PendSV_Handler) 	//    Do CtxSw 
		MOV		LR,  R12 
		MRS		R0,  PSP 
		MSR		MSP, R0 								//    MSP Stack Shrink for CtxS 
		STR		R3, [R0] 								//    Return value stored in HWStkF 
		BX		LR 
	EXPORT	SVC_ProxyCaller		[WEAK] 
SVC_ProxyCaller 									// If SVC_ProxyCaller not present- 
		BX		LR 											// Return 
}
// End of a section. 


// ********************************************************************************************** 
// Lin Messaging Framework 
// local version 1.1 debugged. 181019 
/* Message Blocks can be chained to a task as Lists. 
    These operations are present: 
    AddToFront, AddToBack, 
    RemoveFromFront, RemoveFromBack. 
    Pending improvements of converting to assembly code. 
*/ 
__asm void Lin_MsgPut(Task_r Task, MsgBlk Block){ 
	IMPORT	Lin_MsgPut_core 
		LDR		R2, =Lin_MsgPut_core 
		B.W		__cpp(Lin_MsgCritical) 
		NOP 
}
__asm void Lin_MsgPutFront(Task_r Task, MsgBlk Block){ 
	IMPORT	Lin_MsgPutFront_core 
		LDR		R2, =Lin_MsgPutFront_core
		B.W		__cpp(Lin_MsgCritical) 
		NOP 
}
__asm MsgBlk Lin_MsgGet(Task_r Task){ 
	IMPORT	Lin_MsgGet_core 
		LDR		R2, =Lin_MsgGet_core 
		B.W		__cpp(Lin_MsgCritical) 
		NOP 
}
__asm MsgBlk Lin_MsgGetLast(Task_r Task){ 
	IMPORT	Lin_MsgGetLast_core 
		LDR		R2, =Lin_MsgGetLast_core 
		B.W		__cpp(Lin_MsgCritical) 
		NOP 
}
// The critical region wrapper 
__asm u32 Lin_MsgCritical(Task_r Task, MsgBlk optBlock, void * func){ 
		PUSH	{R4, LR} 
		MRS		R4, PRIMASK 
		CPSID	I 
		BLX		R2 
		MSR		PRIMASK, R4 
		POP		{R4, PC} 
}
// The C version (without critical regions) 
void Lin_MsgPut_core(Task_r Task, MsgBlk Block){ 
	MsgBlk Curr = Task->MsgLast; 
	if(Curr != NULL) Curr->Next = Block; 
	else Task->MsgFirst = Block; 
	Task->MsgLast = Block; 
	Block->Prev = Curr; 
	Block->Next = NULL; 
}
void Lin_MsgPutFront_core(Task_r Task, MsgBlk Block){ 
	MsgBlk Curr = Task->MsgFirst; 
	if(Curr != NULL) Curr->Prev = Block; 
	else Task->MsgLast = Block; 
	Task->MsgFirst = Block; 
	Block->Prev = NULL; 
	Block->Next = Curr; 
}
MsgBlk Lin_MsgGet_core(Task_r Task){ 
	MsgBlk This = Task->MsgFirst; 
	if(This == NULL) return NULL; 
	MsgBlk That = This->Next; 
	if(That != NULL) That->Prev = NULL; 
	else Task->MsgLast = NULL; 
	Task->MsgFirst = That; 
	return This; 
}
MsgBlk Lin_MsgGetLast_core(Task_r Task){ 
	MsgBlk This = Task->MsgLast; 
	if(This == NULL) return NULL; 
	MsgBlk That = This->Prev; 
	if(That != NULL) That->Next = NULL; 
	else Task->MsgFirst = NULL; 
	Task->MsgLast = That; 
	return This; 
}
// End of a section. 


// ********************************************************************************************** 
// Lin Memory Managements (part) 
/* using the microlib. 
    pending improvements of quick memory pools. 
*/ 
__asm void Lin_InitMem(void){ 
		BX		LR 
}
__asm void * Lin_MemAlloc(u32 Size){ 
		PUSH	{R4, LR} 
		MRS		R4, PRIMASK 
		CPSID	I 
		BL.W	__cpp(malloc)  
		MSR		PRIMASK, R4 
		POP		{R4, PC} 
}
__asm void * Lin_MemRealloc(void * Mem, u32 Size){ 
		PUSH	{R4, LR} 
		MRS		R4, PRIMASK 
		CPSID	I 
		BL.W	__cpp(realloc) 
		MSR		PRIMASK, R4 
		POP		{R4, PC} 
}
__asm void Lin_MemFree(void * Mem){ 
		PUSH	{R4, LR} 
		MRS		R4, PRIMASK 
		CPSID	I 
		BL.W	__cpp(free) 
		MSR		PRIMASK, R4 
		POP		{R4, PC} 
}
// End of a section. 

// ********************************************************************************************** 
// Lin Memory Layer 
// local version 1.0 debugged. 181020 
/* Combine memory functions and operating functions together 
    for a unified external interface. 
*/ 
void Lin_Init(void){ 
	Lin_InitSw(); 
	Lin_InitMem(); 
}
// Switching Memory Layer 
Task_r Lin_New(u32 Size, TaskFunc_t func){ 
	void * memory = Lin_MemAlloc(Size); 
	if(memory == NULL) return NULL; 
	return Lin_StackInit(memory, Size, func); 
}
void Lin_Delete(Task_r Task){ 
	while(Task->MsgFirst) Lin_MsgRead(Task);  
	Lin_MemFree(Task->StkBtm); 
}
// Messaging Memory Layer 
// Not Using QuickMemPool 
int Lin_MsgWrite(Task_r Task, Msg_t Msg){ 
//if(Task == NULL) return -1; 
	MsgBlk This = (MsgBlk)Lin_MemAlloc(sizeof(_MsgBlk)); 
	if(This == NULL) return -1; 
	Msg.Source = _CurrTask; 
	This->Msg = Msg; 
	Lin_MsgPut(Task, This); 
	return 0; 
}
int Lin_MsgWriteFront(Task_r Task, Msg_t Msg){ 
//if(Task == NULL) return -1; 
	MsgBlk This = (MsgBlk)Lin_MemAlloc(sizeof(_MsgBlk)); 
	if(This == NULL) return -1; 
	Msg.Source = _CurrTask; 
	This->Msg = Msg; 
	Lin_MsgPutFront(Task, This); 
	return 0; 
}
Msg_t Lin_MsgRead(Task_r Task){ 
//if(Task == NULL) return -1; 
	MsgBlk That = Lin_MsgGet(Task); 
	Msg_t Msg; 
	if(That == NULL) return Msg; 
	Msg = That->Msg; 
	Lin_MemFree(That); 
	return Msg; 
}
Msg_t Lin_MsgReadLast(Task_r Task){ 
//if(Task == NULL) return -1; 
	MsgBlk That = Lin_MsgGetLast(Task); 
	Msg_t Msg; 
	if(That == NULL) return Msg; 
	Msg = That->Msg; 
	Lin_MemFree(That); 
	return Msg; 
}
// End of a section. 


// ********************************************************************************************** 
// Lin Centralized Framework 
// local version 1.0 debugged. previously 
/* The first task entered by Do function becomes the main task 
    which can be accessed easily by Yield functions. 
   Messaging functions can use Submit to Write the main task 
    and Recv functions reads the current task by _CurrTask GV Handle. 
*/ 
// The other Yield functions are in macros. 
// The other Messaging functions are in macros. 
Task_r _MainTask; 
int Lin_Do(Task_r Task){ 
	_MainTask = Task; 
	return Lin_Enter(Task); 
}
// End of a section. 


// End of file. 
