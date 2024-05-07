#include <stdint.h>
#include "miros.h"
#include "TM4C123GH6PM.h" /* the TM4C MCU Peripheral Access Layer (TI) */
#include "qassert.h"

Q_DEFINE_THIS_FILE

OSThread * volatile OS_curr; // Pointer to the current thread
OSThread * volatile OS_next; // Pointer to the next thread

OSThread *OS_thread[32 + 1]; // array of threads started so far
uint8_t OS_threadNum; // number of threads started so far
uint8_t OS_currIdx; // current thread index for round robin
uint32_t OS_readySet; // bitmask of threads that are ready to run

OSThread idleThread;
void main_idleThread() {
    while (1) {
        OS_onIdle();
    }
}

void OS_init(void *stkSto, uint32_t stkSize) {
	/* set the PendSV interrupt priority to the lowest level */
	*(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16);
	
	/* start idle thread */
	OSThread_start(&idleThread,
								 &main_idleThread,
							   stkSto, stkSize);
}

void OS_sched(void){
	/* OS_next = ... */
	if (OS_readySet == 0U) {
		OS_currIdx = 0U;
	}
	else {
		do {
			++OS_currIdx;
			if(OS_currIdx == OS_threadNum) {
				OS_currIdx = 1U;
			}
		} while ((OS_readySet & (1U << (OS_currIdx - 1U))) == 0U);
		
	}
	OS_next = OS_thread[OS_currIdx];
	if (OS_next != OS_curr){
		*(uint32_t volatile *)0xE000ED04 = (1U << 28);
	}
}



void OS_run(void) {
	/* callback to configure and start interrupts */
	OS_onStartup();
	
	__asm volatile ("cpsid i");
    OS_sched();
    __asm volatile ("cpsie i");
	
	/* The following should never execute */
	Q_ERROR();
}

void OS_tick(void) {
	uint8_t n;
	for(n = 1U; n < OS_threadNum; ++n) {
		if (OS_thread[n]->timeout != 0U){
			--OS_thread[n]->timeout;
			if (OS_thread[n]->timeout == 0U){
				OS_readySet |= (1U << (n - 1U));
			}
		}
	}
}

void OS_delay(uint32_t ticks) {
    __asm volatile ("cpsid i");

    /* never call OS_delay from the idleThread */
    Q_REQUIRE(OS_curr != OS_thread[0]);

    OS_curr->timeout = ticks;
    OS_readySet &= ~(1U << (OS_currIdx - 1U));
    OS_sched();
    __asm volatile ("cpsie i");
}


void OSThread_start(
	OSThread *me,
	OSThreadHandler threadHandler,
	void *stkSto, uint32_t stkSize)
	{
		uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
		uint32_t *stk_limit;
		/* fabricate Cortex-M ISR stack frame for blinky1 */
    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)threadHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */
		
		/* save the top of the stack in the thread's attribute */
		me->sp = sp;
		/* round up the bottom of the stack to the 8-byte boundary */
		stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) /8) + 1U) * 8);
		
		/* pre-fill the unused part of the stack with 0xDEADBEEF */
		for (sp = sp - 1U; sp >= stk_limit; --sp) {
			*sp = 0xDEADBEEFU;
		}
	
		Q_ASSERT(OS_threadNum < Q_DIM(OS_thread));

    /* register the thread with the OS */
    OS_thread[OS_threadNum] = me;
		/* make the thread ready to run */
		if (OS_threadNum > 0U) {
			OS_readySet |= (1U << (OS_threadNum - 1U));
		}
		++OS_threadNum;
	}
	
/* inline assembly syntax for Compiler 6 (ARMCLANG) */
__attribute__ ((naked))
void PendSV_Handler(void) {
__asm volatile (
    /* __disable_irq(); */
    "  CPSID         I                 \n"

    /* if (OS_curr != (OSThread *)0) { */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CBZ           r1,PendSV_restore \n"

    /*     push registers r4-r11 on the stack */
    "  PUSH          {r4-r11}          \n"

    /*     OS_curr->sp = sp; */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  STR           sp,[r1,#0x00]     \n"
    /* } */

    "PendSV_restore:                   \n"
    /* sp = OS_next->sp; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           sp,[r1,#0x00]     \n"

    /* OS_curr = OS_next; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=OS_curr       \n"
    "  STR           r1,[r2,#0x00]     \n"

    /* pop registers r4-r11 */
    "  POP           {r4-r11}          \n"

    /* __enable_irq(); */
    "  CPSIE         I                 \n"

    /* return to the next thread */
    "  BX            lr                \n"
    );
}