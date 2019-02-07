// lyrinka kernal version 1.0.3 
#ifndef __kernal_h__
#define __kernal_h__
#include <stm32f10x.h>
////////////																						// Macro Required: SVCn_KernalTrig, SVCn_KernalSwitch 
extern void SVC_ProxyCaller(u8 ID, u32 * StkF); 				// External Functions - SVC Proxy Caller 
void Kernal_Init(void); 																// User Functions - NVIC and Handler Initialization 
void Kernal_ThreadInit(u32 *, void *, u32, u32); 				// User Functions - New Thread Stack Init 
void __svc(SVCn_KernalTrig) Kernal_Enter(u32 *, u32 *); // User Functions - Enter First Thread from Main 
void __svc(SVCn_KernalSwitch) Kernal_Switch(u32 *); 		// User Functions - Switch to any Thread from Thread 
void Kernal_ISRSwitch(u32 *); 													// ISR  Functions - Switch to any Thread from ISR 
void __svc(SVCn_KernalTrig) Kernal_Exit(u32 *); 				// User Functions - Exit from Thread to Main 
//////////// 
u32 * ptrC, * ptrN; 																		// Internal Variables - CtxSw Stack Pointer's Pointer 
__asm void Kernal_ThreadInit(u32 * funcStk, void * funcPtr, u32 Arg0, u32 Arg1){ // User Functions - New Thread Stack Init 
		PUSH	{R4,LR} 
		LDR		R4, [R0] 
		SUBS	R4,  R4, #68 			// Stack growth by 17 words 
		STR		R4, [R0] 					// Stack store back 
		STR		R1, [R4, #56] 		// Set PC 
		STR		R1, [R4, #64] 		// Set EntrancePoint for ThreadExit Routine 
		STR		R2, [R4, #32] 		// Set R0 (Arg0) 
		STR		R3, [R4, #36] 		// Set R1 (Arg1) 
		LDR		R1, =ThreadExit 
		STR		R1, [R4, #52] 		// Set LR as ThreadExit Routine 
		MOV		R1,  #0x01000000 
		STR		R1, [R4, #60] 		// Set Initial PSR 
		POP		{R4,PC} 
ThreadExit 
		LDR		LR, =ThreadExit 
		LDR		R0, [SP] 
		BX		R0 
}
__asm void PendSV_Handler(void){ // PendSV Handler (Context Switch Performer) 
		IMPORT		ptrC 
		IMPORT		ptrN 
		LDR		R2, =ptrC 
		LDR		R1, =ptrN 
		CPSID	I 						// Critical Enter 
		LDR		R0, [R2] 			// P1 Get-  PSP1 MemLocation - ptrC 
		LDR		R1, [R1] 			// P2 Get-  PSP2 MemLocation - ptrN 
		STR		R1, [R2] 			// P3 Swap- ptrC has ptrN value 
		CPSIE	I 						// Critical Exit 
		MRS		R2,  PSP 			// A1 Get   PSP1 from Processor 
		STMDB R2!,{R4-R11} 	// A2 Push  Context {R4-R11} to PSP1  
		STR		R2, [R0] 			// A3 Store PSP1 via ptrC 
		LDR		R2, [R1] 			// B1 Get   PSP2 via ptrN 
		LDMIA R2!,{R4-R11} 	// B2 Pop   Context {R4-R11} from PSP2 
		MSR		PSP, R2 			// B3 Set   PSP2 to Processor 
		BX		LR						// Pop HW Context {R0-R3,R12,LR,PC,PSR} 
}
__asm void SVC_Handler(void){ // SVC Call Handler 
		TST		LR,  #4 
		ITE 	EQ 
		MRSEQ R1,  MSP 
		MRSNE R1,  PSP 
		LDR		R0, [R1, #24] 					// Get HW Stack Frame Pointer 
		LDRB	R0, [R0, #-2] 					// Got HW StkF ptr in R1, SVC ID in R0. 
		CMP		R0,  #SVCn_KernalSwitch 
		BNE		Next 
Cut																// 1. Match Switch or FinState of OsEnter 
		LDR		R0, [R1] 								//-- Pointer to Stack Pointer 
		EXPORT	Kernal_ISRSwitch 
Kernal_ISRSwitch 							// ISR  Functions - Switch to any Thread from ISR 
		LDR		R2, =ptrN 
		LDR		R1, [R2] 						// Get ptrN 
		CMP		R0,  R1 						// Stack Pointers Compared 
		ITTTT	NE 									// If not equal- 
		STRNE	R0, [R2] 						//-- ptrN has ptrX value 
		LDRNE	R1, =0xE000ED00 
		MOVNE	R0,  #0x10000000 
		STRNE	R0, [R1, #4] 				//== PendSV Pend Set (SCB->ICSR.28) 
		BX		LR 									// Then PendSV executes on demand 
Next 
		CMP		R0,  #SVCn_KernalTrig 
		BNE		__cpp(SVC_ProxyCaller) 	// No match, go to SVC_ProxyCaller 
		TST		LR,  #4 								// 2. Match KernalTrig(<R1> u32 * StkF) 
		BNE		Shut 										// 2A.Match KernalTrig -> KernalEnter 
		MSR		PSP, R1 								//-- Set CtxS PSP 
		ADD		R2,  R1, #32 
		MSR		MSP, R2 								//-- MSP Stack Growth for CtxS (8 words) 
		ORR 	LR,  #4 								//-- Modify LR, use PSP 
		LDR		R0, [R1] 
		LDR		R2, =ptrC 
		STR		R0, [R2] 								//-- Set Main CtxS Ptr to ptrC 
		ADD		R1,  R1, #04 						//== Modify Parametre Pointer 
		B			Cut 
Shut															// 2B.Match KernalTrig -> KernalExit 
		BIC		LR,  #4 								//-- Modify LR, use MSP 
		LDR		R2, =ptrN 
		LDR		R1, [R1] 
		STR		R1, [R2] 								//-- Store Main CtxS Ptr to ptrN 
		MOV		R3,  LR 								//-- PendSV didn't use R3 
		BL    __cpp(PendSV_Handler) 	//-- Do CtxSw 
		MOV		LR,  R3 
		MRS		R0,  PSP 
		MSR		MSP, R0 								//-- MSP Stack Shrink for CtxS 
		BX		LR 											//== Return 
		NOP 
}
void Kernal_Init(void){ // User Functions - NVIC and Handler Initialization 
	SCB->CCR |= 1 << 9; 																	// Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); 							// Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; 																	// PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); 														// SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4) | 15; 	// PendSV Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4); 			// SVC Pp same as PendSV, Sp higher than PendSV 
}
#endif
