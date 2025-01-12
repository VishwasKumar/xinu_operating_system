#include <prodcons.h>
#include <stdlib.h>
/*
*Description
*Get the value of a future set by an operation and change the state of future from FUTURE_VALID to FUTURE_EMPTY.
*Parameters
*future *f: future on which to wait for and obtain value
*int *value: variable in which to obtain the value from a future
*Return
*syscall: SYSERR or OK
*/

int get_exclusive(future *f, int *value){
    if(f->state==FUTURE_EMPTY){
        //set tid:
        f->tid=getpid();
        //set state:
        f->state=FUTURE_WAITING;
        //disable interrupts
        intmask im=disable();
        //sleep:
        put_thread_to_sleep(getpid());
        restore(im);
        if(f->state==FUTURE_VALID){
            f->state=FUTURE_EMPTY;
            *value=f->value;
            //reset tid:
            f->tid=NULL;
            return OK;
        }
    }
    else if (f->state==FUTURE_VALID){
        //just get value:
        intmask im=disable();
        f->state=FUTURE_EMPTY;
        *value=f->value;
        //reset tid:
        f->tid=NULL;
        restore(im);
        return OK;

    }
}

int get_shared(future *f, int * value){
    while(1){
        //printf("getter tid=%d\n\r",getpid());
        if(f->state==FUTURE_VALID){
            //get value:
            *value=f->value;
            //last in queue?
            if(f->get_queue!=NULL && f->get_queue->thread!=-1){
                //wake up next consumer:
                intmask im=disable();
                //printf("valid, check getqueue\n\r");
                //print_queue(f->get_queue);
                //printf("end getqueue, first in line:%d\n\r",f->get_queue->thread);
                //Dequeue first in queue:
                int first=check_thread(f->get_queue);
                //Dequeue:
                Dequeue(f->get_queue);
                ready(first);
                restore(im);
                return OK;
            }
            else{
                //last one: 
                //get value:
                *value=(f->value);
                (f->value)=0xddddffff;
                f->state=FUTURE_EMPTY;
                return OK;
            }
        }
        else if(f->state==FUTURE_WAITING){
            //printf("waiting at tid %d\n\r",getpid());
            //add yourself to queue:
            intmask im=disable();
            Enqueue(f->get_queue,getpid());
            //print_queue(f->get_queue);
            put_thread_to_sleep(getpid());
            restore(im);
        }
        else if(f->state==FUTURE_EMPTY){
            //printf("empty at tid %d\n\r",getpid());
            intmask im=disable();
            //make queue:
            //build head node:
            f->get_queue=(queue*)getmem(sizeof(queue));
            f->get_queue->thread=getpid();
            f->get_queue->next=NULL;
            f->state=FUTURE_WAITING;
            put_thread_to_sleep(getpid());
            restore(im);
        }
    }
}

int get_queue(future *f, int * value){
    if (set_head != set_tail)
   {
       intmask im = disable();
       future *temp = dq_set();
       printf("%d\n", temp->tid);
       temp->state = FUTURE_VALID;
       *value = temp->value;
       resume(temp->tid);
       restore(im);
   }
   else{
        intmask im = disable();
        if (f->state == FUTURE_VALID)
        {
           *value = f->value;
           return OK; 
        }
        else if (f->state == FUTURE_EMPTY || f-> state == FUTURE_WAITING)
        {
            f->state = FUTURE_WAITING;
            f-> value = *value;
            f->tid = getpid();
            nq_get(f);
            put_thread_to_sleep(f->tid);
        }
        restore(im);
   }
}

syscall future_get(future *f, int *value){
    //if tid id not set and state is empty
    if(f->flag==FUTURE_EXCLUSIVE){
        return get_exclusive(f,value);
    }
    else if (f->flag==FUTURE_SHARED){
        return get_shared(f,value);
    }
    else if (f->flag==FUTURE_QUEUE){
        return get_queue(f,value);
    }
    return SYSERR;
}