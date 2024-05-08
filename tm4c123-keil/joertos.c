#include <stdint.h>
#include "joertos.h"
#include "TM4C123GH6PM.h" /* the TM4C MCU Peripheral Access Layer (TI) */
#include "qassert.h"

Q_DEFINE_THIS_FILE

OSThread * volatile OS_curr; // Pointer to the current thread
OSThread * volatile OS_next; // Pointer to the next thread

OSThread *OS_thread[32 + 1]; // array of threads started so far
uint32_t OS_readySet; // bitmask of threads that are ready to run
uint32_t OS_delayedSet; // bitmask of threads that are delayed

#define LOG2(x) (32U - __builtin_clz(x))

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
								 0U, /* idle thread priority */
								 &main_idleThread,
							   stkSto, stkSize);
}

void OS_sched(void){
	/* choose next thread to execute */
	OSThread *next;
	if (OS_readySet == 0U) { /* idle condition? */
		next = OS_thread[0]; /* the idle thread */
	}
	else {
		next = OS_thread[LOG2(OS_readySet)];
		Q_ASSERT(next != (OSThread *)0);
	}
	/* trigger PendSV, if needed */
	if (next != OS_curr){
		OS_next = next;
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
	uint32_t workingSet = OS_delayedSet;
    while (workingSet != 0U) {
        OSThread *t = OS_thread[LOG2(workingSet)];
        uint32_t bit;
        Q_ASSERT((t != (OSThread *)0) && (t->timeout != 0U));

        bit = (1U << (t->prio - 1U));
        --t->timeout;
        if (t->timeout == 0U) {
            OS_readySet   |= bit;  /* insert to set */
            OS_delayedSet &= ~bit; /* remove from set */
        }
        workingSet &= ~bit; /* remove from working set */
    }
}

void OS_delay(uint32_t ticks) {
    uint32_t bit;
    __asm volatile ("cpsid i");

    /* never call OS_delay from the idleThread */
    Q_REQUIRE(OS_curr != OS_thread[0]);

    OS_curr->timeout = ticks;
    bit = (1U << (OS_curr->prio - 1U));
    OS_readySet &= ~bit;
    OS_delayedSet |= bit;
    OS_sched();
    __asm volatile ("cpsie i");
}

void OSThread_start(
	OSThread *me,
	uint8_t  prio, /* thread priority */
	OSThreadHandler threadHandler,
	void *stkSto, uint32_t stkSize)
	{
		uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
		uint32_t *stk_limit;
		
		/* priority must be in range and the priority level must be unused */
		Q_REQUIRE((prio < Q_DIM(OS_thread))
							&& (OS_thread[prio] == (OSThread *)0));
		
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

    /* register the thread with the OS */
    OS_thread[prio] = me;
		me->prio = prio;
		/* make the thread ready to run */
		if (prio > 0U) {
			OS_readySet |= (1U << (prio - 1U));
		}
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