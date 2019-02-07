// lyrinka kernal version 0.4.2 
#ifndef __kernal_h__
#define __kernal_h__
#include <stm32f10x.h>
																																// Macro Required: SVCn_GenesisInit, SVCn_Branch 
extern void SVC_ProxyCaller(u8 , u32 *); 												// External Required 
void __svc(SVCn_GenesisInit) GenesisInit(u32 *, u32 *); 				// User Functions 
void __svc(SVCn_Branch) Branch(u32 *); 													// User Functions 
u32 * ptrC, * ptrN; 																						// Internals 

__asm void StkInit(u32 * funcStk, void * funcPtr, u32 Arg0, u32 Arg1){ // User Functions - Stack Init 
		PUSH	{R4,LR} 
		LDR		R4, [R0] 
		SUBS	R4,  R4, #64 			// Stack growth by 16 words 
		STR		R4, [R0] 					// Stack store back 
		STR		R1, [R4, #56] 		// Set PC 
		STR		R2, [R4, #32] 		// Set R0 (Arg0) 
		STR		R3, [R4, #36] 		// Set R1 (Arg1) 
		MOV		R1, #0xFFFFFFFF 
		STR		R1, [R4, #52] 		// Set LR  !!PENDING IMPROVEMENT!! 
		MOV		R1, #0x01000000 
		STR		R1, [R4, #60] 		// Set Initial PSR 
		POP		{R4,PC} 
}

__asm void Branch_Core(u32 * ptrX){ // ISR Functions - Branch 
		LDR		R1, =ptrN 
		LDR		R2, =ptrC 
		LDR		R3, [R1] 
		STR		R3, [R2] 					// ptrC has ptrN value 
		CMP		R0,  R3 					// Stack Pointers Compared 
		ITTTT	NE 								// If not equal- 
		STRNE	R0, [R1]					//-- ptrN has ptrX value 
		LDRNE	R2, =0xE000ED00 
		MOVNE	R3,  #0x10000000 
		STRNE	R3, [R2, #4] 			//-- PendSV Pend Set (SCB->ICSR.28) 
		BX		LR 								// Then PendSV executes on demand 
}

__asm void SVC_Handler(void){ // SVC Call Handler 
		TST		LR,  #4 								// Get HW Stack Frame Pointer 
		ITE 	EQ 
		MRSEQ R3,  MSP 
		MRSNE R3,  PSP 
		LDR		R0, [R3, #24] 
		LDRB	R0, [R0, #-2] 					// Got HW StkF ptr in R1, SVC ID in R0. 
		CMP		R0,  #SVCn_Branch 
		BEQ		Cut 										// Match Branch Functions - Label "Cut" 
		CMP		R0,  #SVCn_GenesisInit 
		BNE		__cpp(SVC_ProxyCaller) 	// No match, go to SVC_ProxyCaller 
		MSR		PSP, R3 								// Match Genesis Init 
		ORR 	LR,  #4 								//-- Change Link Register 
		LDR		R0, [R3] 								//-- Swap Pointers 
		LDR		R1, =ptrN 
		STR		R0, [R1] 
		ADD		R3,  R3, #04 						//-- Modify Parametre Pointer 
Cut
		LDR		R0, [R3] 								// Pointer to Stack Pointer 
		B			__cpp(Branch_Core) 			// Context Switching  !!PENDING OPTIMIZATION!! 
}

__asm void PendSV_Handler(void){ // PendSV Handler (Context Switching)  
		IMPORT		ptrC 
		IMPORT		ptrN 
		CPSID	I 						// Turn off Interrupts 
		LDR		R0, =ptrC 
		LDR		R0,	[R0] 			// P1 Get   PSP1 MemLocation - ptrC 
		LDR		R1, =ptrN 
		LDR		R1,	[R1] 			// P2 Get   PSP2 MemLocation - ptrN 
		MRS		R2,  PSP 			// A1 Get   PSP1 from Processor 
		STMDB R2!,{R4-R11} 	// A2 Push  Context to PSP1 {R4-R11} 
		STR		R2, [R0] 			// A3 Store PSP1 via ptrC 
		LDR		R2,	[R1] 			// B1 Get   PSP2 via ptrN 
		LDMIA R2!,{R4-R11} 	// B2 Pop   Context from PSP2 {R4-R11} 
		MSR		PSP,R2 				// B3 Set   PSP2 to Processor 
		CPSIE	I 						// Turn on Interrupts 
		BX		LR						// Pop HW Context {R0-R3,R12,LR,PC,PSR} 
		NOP 								// Make the compiler happy 
}

void Init_Kernal(void){ // System and NVIC Initialization 
	SCB->CCR |= 1 << 9; 																	// Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); 							// Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; 																	// PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); 														// SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4) | 15; 	// Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4); 			// Pp same as PendSV, Sp higher than PendSV 
}

#endif 
