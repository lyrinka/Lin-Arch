// CtxSw kernal CtxSw Framework header for version 1.1.0 
#ifndef __ctxsw_H__
#define __ctxsw_H__
#include <stm32f10x.h> 

// Declaration 
#include <CtxSw config.h> 
extern void 												SVC_ProxyCaller(u8 ID, u32 * StkF); 			// External Functions - SVC Proxy Caller 
extern void 												CtxSw_Init(void); 												// User Functions - NVIC and Handler Initialization 
extern void 												CtxSw_ThreadInit(u32 **, void *, void *); // User Functions - New Thread Stack Init with Loop Function 
extern void 												CtxSw_ThreadSetArgs(u32 **, u32, u32); 		// User Functions - New Thread Set Arguments 
extern void __svc(SVCn_CtxSwTrig) 	CtxSw_Enter(u32 **, u32 **); 							// User Functions - Enter First Thread from Main 
extern void __svc(SVCn_CtxSwSwitch) CtxSw_Switch(u32 **); 										// User Functions - Switch to any Thread from Thread 
extern void 												CtxSw_SwitchISR(u32 **); 									// ISR  Functions - Switch to any Thread from ISR 
extern void __svc(SVCn_CtxSwTrig) 	CtxSw_Exit(u32 **); 											// User Functions - Exit from Thread to Main 

#endif 
