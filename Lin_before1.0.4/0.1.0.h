#ifndef __kernal_h__
#define __kernal_h__

/*
#define SVCn_B .....
#define SVCn_GenesisInit .....
#define Stack_Top 0x20000000 + MemSize 
*/

//--User Functions 
u32 Imain; // The stack pointer value for the very first process 

void Init_KernalInit(void); // At Very First System Startup 
/* Containments: 
	Disable Stack DW Align. 
	Set Priority Group 5, 2bits+2bits. 
	PendSV Pend Clear. 
	SVC Pend Clear. 
	PendSV Has The Lowest Priority. 
	SVC Has A Priority which: 
			Preemption Priority same as PendSV, 
			Sub Priority higher than PendSV by 1. 
 */

#define Yield() B(&Imain) // Branch back to Imain 
void StackInit(u32 *, void *, u32, u32); // Initialize the stack for a new process with incoming parametres 
void Genesis(void *); // Branch from main to the first process 

//--Externs 
extern void SVC_Handler_Main(u8 ID, u32 * StkF); //-- SVC Proxy Caller 
extern void __svc(SVCn_GenesisInit) GenesisInit(u32 *); //-- SVC Call for: GenesisInit_Core(StkF); 
extern void __svc(SVCn_B) B(u32 *); //-- SVC Call for: Branch_Core(StkF[0]); 

//--System Functions 
u32 ptrC; // Parametres for PendSV: Current Stack Pointer's Pointer casted to u32 
u32 ptrN; // Parametres for PendSV: Target Stack Pointer's Pointer casted to u32 

#define PendSV_Hang() SCB->ICSR = 1 << 28; // PendSV Pend Set 
void GenesisInit_Core(u32 *); // Called By SVC Proxy Caller: number SVCn_GenesisInit 
void Branch_Core(u32); // Called By SVC Proxy Caller: number SVCn_B 

void PendSV_Handler(void); // NVIC Handlers: PendSV 
void SVC_Handler(void); // NVIC Handlers: SVCall 

//--Start of Code 
#include "stm32f10x.h" 

//-SVC Cores 
__asm void GenesisInit_Core(u32 * StkF){
		MSR		PSP,R0 
		LDR		R1, [R0, #-4] 
		ORR 	R1, #4 
		STR		R1, [R0, #-4] 
		LDR		R1, [R0] 
		LDR		R0, =ptrN 
		STR		R1, [R0]
		BX		LR 
}

void Branch_Core(u32 p){
	ptrC = ptrN; 
	if(ptrC != p){
		ptrN = p; 
		PendSV_Hang();
	}
}

//-Handlers 
__asm void PendSV_Handler(void){
		IMPORT	ptrC 
		IMPORT	ptrN 
		MRS		R2, PSP 
		CPSID	I 
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

__asm void SVC_Handler(void){
		TST		LR, #4 
		ITE 	EQ 
		MRSEQ R1, MSP 
		MRSNE R1, PSP 
		LDR		R0, [R1, #24] 
		LDRB	R0, [R0, #-2] 
		B 		__cpp(SVC_Handler_Main) 
}

////-User Main Functions 
void Init_KernalInit(void){
	SCB->CCR |= 1 << 9; //Disable Stack DW Align 
	SCB->AIRCR = (5 << 8) | (0x05FA << 16); // Priority Group 5, 2bits+2bits 
	SCB->ICSR = 1 << 27; // PendSV Pend Clear 
	SCB->SHCSR &= ~(1 << 15); // SVC Pend Clear 
	SCB->SHP[12+PendSV_IRQn] = (3 << 6) | (3 << 4); // Lowest Priority 
	SCB->SHP[12+SVCall_IRQn] = (3 << 6) | (2 << 4); // Pp same as PendSV, Sp higher than PendSV 
}

void StackInit(u32 * StkF, void * func, u32 Arg0, u32 Arg1){
	*StkF -= 4*16; 
	((u32 *)(*StkF))[8] = Arg0; 
	((u32 *)(*StkF))[9] = Arg1; 
	((u32 *)(*StkF))[13] = 0xFFFFFFFF; 
	((u32 *)(*StkF))[14] = (u32)func; 
	((u32 *)(*StkF))[15] = 0x01000000;
}

void Genesis(void * Imain_func){
	u32 Iload; 
	Imain = Stack_Top; // Maxium Stack 
	StackInit(&Imain, Imain_func, (u32)&Iload, (u32)Imain_func); 
	GenesisInit(&Iload); 
	B(&Imain); 
}

#endif 

//--EOF 
