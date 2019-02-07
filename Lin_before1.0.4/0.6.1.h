// lyrinka kernal version 0.6.1 
#ifndef __kernal_h__
#define __kernal_h__
#include <stm32f10x.h>
																												// Macro Required: SVCn_KernalTrig, SVCn_KernalSwitch 
extern void SVC_ProxyCaller(u8 , u32 *); 								// External Required - SVC Proxy Caller 
void __svc(SVCn_KernalTrig) Kernal_Enter(u32 *, u32 *); // User Functions - Enter First Thread from Main 
void __svc(SVCn_KernalTrig) Kernal_Exit(u32 *); 				// User Functions - Exit from Thread to Main 
void __svc(SVCn_KernalSwitch) Kernal_Switch(u32 *); 		// User Functions - Switch to any Thread from Thread 
u32 * ptrC, * ptrN; 																		// Internals - CtxSw Stack Pointer's Pointer 

void Kernal_Init(void){ // User Functions - NVIC and Handler Initialization 
	SCB->CCR |= 1 << 9; 																	// Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); 							// Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; 																	// PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); 														// SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4) | 15; 	// PendSV Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4); 			// SVC Pp same as PendSV, Sp higher than PendSV 
}

__asm void Kernal_StackInit(u32 * funcStk, void * funcPtr, u32 Arg0, u32 Arg1){ // User Functions - New Thread Stack Init 
		PUSH	{R4,LR} 					// !!PENDING IMPROVEMENT!! 
		LDR		R4, [R0] 
		SUBS	R4,  R4, #64 			// Stack growth by 16 words 
		STR		R4, [R0] 					// Stack store back 
		STR		R1, [R4, #56] 		// Set PC 
		STR		R2, [R4, #32] 		// Set R0 (Arg0) 
		STR		R3, [R4, #36] 		// Set R1 (Arg1) 
		MOV		R1,  #0xFFFFFFFF 
		STR		R1, [R4, #52] 		// Set LR  !!PENDING IMPROVEMENT!! 
		MOV		R1,  #0x01000000 
		STR		R1, [R4, #60] 		// Set Initial PSR 
		POP		{R4,PC} 
}

__asm void Kernal_ISRSwitch(u32 * ptrX){ // ISR Functions - Switch to any Thread from ISR 
		LDR		R2, =ptrN 
		LDR		R1, [R2] 					// Get ptrN 
		CMP		R0,  R1 					// Stack Pointers Compared 
		ITTTT	NE 								// If not equal- 
		STRNE	R0, [R2] 					//-- ptrN has ptrX value 
		LDRNE	R1, =0xE000ED00 
		MOVNE	R0,  #0x10000000 
		STRNE	R0, [R1, #4] 			//== PendSV Pend Set (SCB->ICSR.28) 
		BX		LR 								// Then PendSV executes on demand 
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
		BEQ		Cut 
		CMP		R0,  #SVCn_KernalTrig 
		BNE		__cpp(SVC_ProxyCaller) 	// No match, go to SVC_ProxyCaller 
		TST		LR,  #4 								// 1. Match KernalTrig(<R1> u32 * StkF) 
		BNE		Shut 										// 1A.Match KernalTrig -> KernalEnter 
		MSR		PSP, R1 								//-- Set CtxS PSP 
		ADD		R2,  R1, #32 
		MSR		MSP, R2 								//-- MSP Stack Growth for CtxS (8 words) 
		ORR 	LR,  #4 								//-- Modify LR, use PSP 
		LDR		R0, [R1] 
		LDR		R2, =ptrC 
		STR		R0, [R2] 								//-- Set Main CtxS Ptr to ptrC 
		ADD		R1,  R1, #04 						//== Modify Parametre Pointer 
Cut																// 2. Match Switch or FinState of OsEnter 
		LDR		R0, [R1] 								//-- Pointer to Stack Pointer 
		B			__cpp(Kernal_ISRSwitch) //== Context Switching  !!PENDING OPTIMIZATION!! 
Shut															// 1B.Match KernalTrig -> KernalExit 
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

#endif 
