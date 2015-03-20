/** @file proc.h
 *  @brief Prototypes for managing kernel processes and threads.
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#ifndef _PROC_H
#define _PROC_H

#include <linklist.h>
#include <hashtable.h>

#define PCB_HT_SIZE 128
#define TCB_HT_SIZE 128

typedef struct regs {
    unsigned ebx; //0
    unsigned esi; //4
    unsigned edi; //8
    unsigned esp; //12
    unsigned ebp; //16
    unsigned eflags; //20
    unsigned cr0; //24
    unsigned cr2; //28
    unsigned cr3; //32
    unsigned cr4; //36
    unsigned cs; //40
    unsigned ds; //44
    unsigned es; //48
    unsigned fs; //52
    unsigned gs; //56
    unsigned ss; //60
} regs_t;

typedef struct pcb {
    int pid;
    linklist_t threads;
} pcb_t;

typedef struct tcb {
    int tid;
    int pid;
    regs_t regs;
} tcb_t;

extern hashtable_t pcbs;
extern hashtable_t tcbs;
extern int cur_tid;

/* Process and thread functions */
int proc_init();
int proc_new_process();
int proc_new_thread();

#endif /* _PROC_H */