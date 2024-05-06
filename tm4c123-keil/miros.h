#ifndef MIROS_H
#define MIROS_H

/* Thread Control Block (TCB) */
typedef struct {
	void *sp; /* stack pointer */
	/* ...other attributes associated with a thread */
} OSThread;

typedef void (*OSThreadHandler)();

void OS_init(void);

/* This function must be called with interrupts DISABLED */ 
void OS_sched(void);

void OSThread_start(
	OSThread *me,
	OSThreadHandler threadHandler,
	void *stkSto, uint32_t stkSize);
#endif /* MIROS_H */
