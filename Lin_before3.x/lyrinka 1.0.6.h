// lyrinka header for lyrinka 1.0.6.c 
#ifndef __lyrinka_1_0_5_H__ 
#define __lyrinka_1_0_5_H__ 
#include <stm32f10x.h> 

// Declaration 
#include <lyrinka config.h> 
extern void SVC_ProxyCaller(u8 ID, u32 * StkF); 										// External Functions - SVC Proxy Caller 
extern void lyrinka_Init(void); 																		// User Functions - NVIC and Handler Initialization 
extern void lyrinka_ThreadInitLoop(u32 **, void *, void *); 				// User Functions - New Thread Stack Init with Loop Function 
extern void lyrinka_ThreadSetArgs(u32 **, u32, u32); 							// User Functions - New Thread Set Arguments 
extern void __svc(SVCn_lyrinkaTrig) lyrinka_Enter(u32 **, u32 **); 	// User Functions - Enter First Thread from Main 
extern void __svc(SVCn_lyrinkaSwitch) lyrinka_Switch(u32 **); 			// User Functions - Switch to any Thread from Thread 
extern void lyrinka_ISRSwitch(u32 **); 															// ISR  Functions - Switch to any Thread from ISR 
extern void __svc(SVCn_lyrinkaTrig) lyrinka_Exit(u32 **); 					// User Functions - Exit from Thread to Main 

// Macros 
#define lyrinka_ThreadInit(a, b, c, d) 														\
										lyrinka_ThreadInitLoop(a, b, b); 							\
										lyrinka_ThreadSetArgs(a, (u32)c, (u32)d) 				// User Functions - New Thread Stack Init 
#endif 
