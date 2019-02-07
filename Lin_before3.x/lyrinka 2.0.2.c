// lyrinka Context Switching program file ver 2.0.2 final release fixing 
/* 
	Context Switching Framework for lyrinka systems. 
	Manually optimized for fastest execution speed. 

	Release Notes: 
	
	<2.0.2>[181014] 4th parametre in the task function now passes the TCB of itself. 
	<2.0.1>[181014] Fixed errors in TCB typedef. 
	<2.0.0>[181014] Initial release. 
*/
#include <stm32f10x.h> 
#include <lysw.h> 

// Initialization of a new thread. 
/* Initialize the task and associated control blocks 
    in a given part of memory(from Memory with size Size) 
		and return the Task Control Block. 
*/ 
// local version 1.2 debugged. 181014 
// Some types in header file. 
__asm TCB lysw_New(void * Memory, u32 Size, void * funcPtr){ 
		ADD		R3,  R0, R1 		
		SUB		R3,  #16 				// TCB = (TCB)((u8 *)Memory + Size - sizeof(_TCB)); 
		STR		R0, [R3, #4] 		// TCB->StkBtm = Memory; 
		SUB		R0,  R3, #80 
		STR		R0, [R3] 				// TCB->StkPtr = (u32 *)((u8 *)PCB - sizeof(_ECB) - sizeof(_HWStkF) - sizeof(_CtxS)); 
		MOV		R0,  #0 
		STR		R0, [R3, #12] 	// TCB->reserved2 = 0; 
		STR		R0, [R3, #8] 		// TCB->reserved1 = 0; 
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
__asm void lysw_SetArgs(TCB Task, u32 Arg0, u32 Arg1){ 
		STR		R1, [R0, #-16] 	// ECB->Arg0Bkp = Arg0; 
		STR		R1, [R0, #-48] 	// HWStkF->Arg0 = Arg0; 
		STR		R2, [R0, #-12] 	// ECB->Arg1Bkp = Arg1; 
		STR		R2, [R0, #-44] 	// HWStkF->Arg1 = Arg1; 
		BX		LR 
}
__asm void lysw_SetSetup(TCB Task, void * funcSetup){ 
		STR		R1, [R0, #-24] 	// HWStkF->PC = funcSetup; 
		BX		LR 
}
// End of a section. 

// General Context Switching Operation. 
/* PendSV Handler for context switching. 
    Current Task Handle in GV ptrC, 
    Next Task to be run Handle in GV ptrN. 
*/ 
// local version 1.1 debugged. 181014 
TCB ptrC, ptrN; // Current Running & Next to be Run 
__asm void PendSV_Handler(void){ // The 1.0.6 version framework not changed 
		// PendSV Handler (Context Switch Performer) 
		// Disable interrupts will prevent any context switches. 
	IMPORT	ptrC 
	IMPORT	ptrN 
		LDR		R2, =ptrC 
		LDR		R1, =ptrN 
		CPSID	I 						// Critical Enter 
		LDR		R0, [R2] 			// CurrTaskHandle in R0 
		LDR		R1, [R1] 			// NextTaskHandle in R1 
		STR		R1, [R2] 			// NextTaskHandle stored in ptrC 
		CPSIE	I 						// Critical Exit 
		MRS		R2,  PSP 			// A1 Get   PSP1 from Processor 
		STMDB R2!,{R4-R11} 	// A2 Push  Context {R4-R11} to PSP1  
		STR		R2, [R0]			// A3 Store PSP1 via CurrTaskHandle->StkPtr 
		LDR		R2, [R1]			// B1 Get   PSP2 via NextTaskHandle->StkPtr 
		LDMIA R2!,{R4-R11} 	// B2 Pop   Context {R4-R11} from PSP2 
		MSR		PSP, R2 			// B3 Set   PSP2 to Processor 
		BX		LR 						// Pop HW Context {R0-R3,R12,LR,PC,PSR} 
}
// End of a section. 

// SVC Call Handler 
// local version 1.1 debugged. 181014 
/* SVC Call Handler for 
    Switch(including SwitchISR), Enter and Return. 
    other SVC Calls goes to SVC_ProxyCaller(u8 ID, u32 * StkF); . 
    The SVC numbers are inside the header file. 
*/ 
// Some definitions in header file. 
void __svc(SVCn_CtxSwSwitch) lysw_Switch(TCB); 
void lysw_SwitchISR(TCB); 
void __svc(SVCn_CtxSwTrig) lysw_Enter(TCB); 
void __svc(SVCn_CtxSwTrig) lysw_Return(void); 
extern void SVC_ProxyCaller(u8 ID, u32 * StkF); 
u32 * Tloader; // storing handle for the context of the main function 
__asm void SVC_Handler(void){ // The 1.0.6 version framework not changed 
		// SVC Call Handler 
		// Other SVC Numbers will trigger SVC_ProxyCaller(u8 ID, u32 * StkF); 
		TST		LR,  #4 
		ITE 	EQ 
		MRSEQ R1,  MSP 
		MRSNE R1,  PSP 								// Get HW Stack Frame Pointer. 
		LDR		R0, [R1, #24] 
		LDRB	R0, [R0, #-2] 					// Got HW StkF ptr in R1, SVC ID in R0. 
		CMP		R0,  #SVCn_CtxSwSwitch 
		BNE		NotSwitch 							// Is Switch Op? 
Switch 														//  Do Switch Op 
		LDR		R0, [R1] 								//  Get Target Task Handle 
	EXPORT	lysw_SwitchISR 
lysw_SwitchISR 										//  Switches from ISR directly come here 
		LDR		R2, =ptrN 
		LDR		R1, [R2] 								//  Get Handle in ptrN 
		CMP		R0,  R1 								//  Handles Compared 
		ITTTT	NE 											//  If not equal- 
		STRNE	R0, [R2] 								//   put new handle in ptrN 
		LDRNE	R1, =0xE000ED00 
		MOVNE	R0,  #0x10000000 
		STRNE	R0, [R1, #4] 						//   PendSV Pend Set (SCB->ICSR.28) 
		BX		LR 											//  Then PendSV executes on demand 
NotSwitch 												// Not Switch Op 
		CMP		R0,  #SVCn_CtxSwTrig 		//  Is Trigger Op? 
		BNE.W	SVC_ProxyCaller 				//   No, go to SVC_ProxyCaller 
		TST		LR,  #4 								//  Is Trigger Op Enter or Return? 
		BNE		Return 									//   Trigger Op is Enter 
		MSR		PSP, R1 								//    Set PSP as current pointer for CtxS 
		SUB		R2,  R1, #32 
		MSR		MSP, R2 								//    MSP Stack Growth for CtxS (8 words) 
		ORR 	LR,  #4 								//    Modify LR to use PSP 
	IMPORT	Tloader 
		LDR		R0, =Tloader 
		LDR		R2, =ptrC 
		STR		R0, [R2] 								//    Set the storing handle to ptrC 
		B			Switch 
Return 														//   Trigger Op is Return 
		BIC		LR,  #4 								//    Modify LR, use MSP 
		LDR		R1, =Tloader 
		LDR		R2, =ptrN 
		STR		R1, [R2] 								//    Store the storage handle to ptrN 
		MOV		R3,  LR 								//    PendSV didn't use R3 
		BL 		__cpp(PendSV_Handler) 	//    Do CtxSw 
		MOV		LR,  R3 
		MRS		R0,  PSP 
		MSR		MSP, R0 								//    MSP Stack Shrink for CtxS 
		BX		LR 
	EXPORT	SVC_ProxyCaller		[WEAK] 
SVC_ProxyCaller 									// If SVC_ProxyCaller not present- 
		BX		LR 											// Return 
}
// End of a section. 

// Initialization 
// local version 0 not changed. debugged 181014 
void lysw_Init(void){ 
	// User Functions - NVIC and Handler Initialization 
	SCB->CCR |= 1 << 9; 																	// Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); 							// Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; 																	// PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); 														// SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4) | 15; 	// PendSV Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4); 			// SVC Pp same as PendSV, Sp higher than PendSV 
}
// End of a section. 

// End of file. 
