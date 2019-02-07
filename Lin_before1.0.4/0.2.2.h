// lyrinka kernal version 0.2.2 
#ifndef __kernal_h__
#define __kernal_h__
#include "stm32f10x.h" 
// Macro Required: SVCn_GenesisInit, SVCn_Branch, PendSV_Hang() 
u32 * ptrC, * ptrN; 
extern void SVC_ProxyCaller(u8 , u32 *); // External Required 
void __svc(SVCn_GenesisInit) GenesisInit(u32 *, u32 *); // User Functions 
void __svc(SVCn_Branch) Branch(u32 *); // User Functions 
void Init_Kernal(void){ // User Functions 
	SCB->CCR |= 1 << 9; //Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); // Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; // PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); // SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4); // Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4) | 15; // Pp same as PendSV, Sp higher than PendSV 
}
__asm void StkInit(u32 * funcStk, void * funcPtr, u32 Arg0, u32 Arg1){ // User Functions - Stack Init 
		PUSH	{R4, LR} 
		LDR		R4, [R0] 
		SUBS	R4, R4, #64 
		STR		R4, [R0] 
		STR		R1, [R4, #56] 
		STR		R2, [R4, #32] 
		STR		R3, [R4, #36] 
		MOV		R1, #0xFFFFFFFF 
		STR		R1, [R4, #52] 
		MOV		R1, #0x01000000 
		STR		R1, [R4, #60] 
		POP		{R4, PC} 
}
void Genesis(u32 * funcStk, void * funcPtr, u32 * traceStk){ // User Functions - Genesis 
	StkInit(funcStk, funcPtr, (u32)traceStk, (u32)funcStk); 
	GenesisInit(traceStk, funcStk); 
}
void Branch_Core(u32 * ptrX){	// ISR Internals - Branch 
	ptrC = ptrN; 
	if(ptrC != ptrX){
		ptrN = ptrX; 
		PendSV_Hang();
	}
}
__asm void GenesisInit_Core(u32 * StkF){ // Internals 
		MSR		PSP,R0 
		LDR		R1, [R0, #-4] 
		ORR 	R1, #4 
		STR		R1, [R0, #-4] 
		LDR		R1, [R0] 
		LDR		R0, =ptrN 
		STR		R1, [R0]
		BX		LR 
}
void SVC_PreProxyCaller(u8 ID, u32 * StkF){ // Internals 
	if(ID == SVCn_Branch) Branch_Core((u32 *)StkF[0]);
	else if(ID == SVCn_GenesisInit){
		GenesisInit_Core(StkF);
		Branch_Core((u32 *)StkF[1]);
	}
	else if(1) SVC_ProxyCaller(ID, StkF);
}
__asm void SVC_Handler(void){ // System Handlers 
		TST		LR, #4 
		ITE 	EQ 
		MRSEQ R1, MSP 
		MRSNE R1, PSP 
		LDR		R0, [R1, #24] 
		LDRB	R0, [R0, #-2] 
		B 		__cpp(SVC_PreProxyCaller) 
}
__asm void PendSV_Handler(void){ // System Handlers 
		IMPORT	ptrC 
		IMPORT	ptrN 
		CPSID	I 
		MRS		R2, PSP 
		LDR		R0, =ptrC 
		LDR		R0,	[R0] 
		LDR		R1, =ptrN 
		LDR		R1,	[R1] 
		STMDB R2!,{R4-R11} 
		STR		R2, [R0] 
		LDR		R2,	[R1] 
		LDMIA R2!,{R4-R11} 
		MSR		PSP,R2 
		CPSIE	I 
		BX		LR 
		NOP 
}
#endif 
