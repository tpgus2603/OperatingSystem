/**********************************************************************
 * Copyright (c) 2019-2024
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;

/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];

/**
 * Monotonically increasing ticks. Do not modify it
 */
extern unsigned int ticks;

/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;

/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/

static bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_BLOCKED;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) { 
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_BLOCKED);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

#include "sched.h"

/***********************************************************************
 * FCFS scheduler
 ***********************************************************************/
static int fcfs_initialize(void)
{
	return 0;
}

static void fcfs_finalize(void)
{
}

static struct process *fcfs_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, the current process can be blocked
	 * while acquiring a resource. In this case just pick the next as well.
	 */
	if (!current || current->status == PROCESS_BLOCKED) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note that we use 
		 * list_del_init() over list_del() to maintain the list head tidy.
		 * Otherwise, the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list); 
	}

	/* Return the process to run next */
	return next;
}

struct scheduler fcfs_scheduler = {
	.name = "FCFS",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fcfs_initialize,
	.finalize = fcfs_finalize,
	.schedule = fcfs_schedule,
};

/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	
struct process *next = NULL;

	
	if (!current || current->status == PROCESS_BLOCKED) {
		goto pick_next;
	}
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	
	if (!list_empty(&readyqueue)) {
		struct process *node;
		struct process *min_node=list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(node,&readyqueue,list){
			if(node->lifespan<min_node->lifespan)
			{
				min_node=node;
			}
		}
		next=min_node;
		list_del_init(&next->list);
	}
	return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire,	/* Use the default FCFS acquire() */
	.release = fcfs_release,	/* Use the default FCFS release() */
	.schedule = sjf_schedule,			
								/* TODO: Assign your schedule function  
							  // to this function pointer to activate
								   SJF in the simulation system */
};

/***********************************************************************
 * STCF scheduler
 ***********************************************************************/

static struct process *stcf_schedule(void)
{

	struct process *next = NULL;
	if (!current || current->status == PROCESS_BLOCKED)
	{
		goto pick_next;
	}
	if (!list_empty(&readyqueue))
	{

		struct process *node;
		struct process *min_node=list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(node,&readyqueue,list){
			if((node->lifespan-node->age)<min_node->lifespan-min_node->age)
			{
				min_node=node;
			}
		}
		if((min_node->lifespan-min_node->age)<current->lifespan-current->age)
		{
			list_add_tail(&current->list,&readyqueue);
			list_del_init(&min_node->list);
			return min_node;
		}
	}

	if (current->age < current->lifespan)
	{
		return current;
	}

pick_next:

		if (!list_empty(&readyqueue)) {
			
		struct process *node;
		struct process *min_node=list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(node,&readyqueue,list){
			if(node->lifespan-node->age<min_node->lifespan-min_node->age)
			{
				min_node=node;
			}
		}
		next=min_node;
		list_del_init(&next->list);
	}
	return next;
}

struct scheduler stcf_scheduler = {
	.name = "Shortest Time-to-Complete First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule= stcf_schedule,
	/* You need to check the newly created processes to implement STCF.
	 * Have a look at @forked() callback.
	 */


	/* Obviously, you should implement stcf_schedule() and attach it here */
};



/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/

static struct process *rr_schedule(void){
	struct process *next = NULL;
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
		if(current!=NULL&&current->age!=current->lifespan)
		{
			list_add_tail(&current->list,&readyqueue); 
		}
	}
	else
	{
		if(current->age<current->lifespan)
			return current;
	}
	return next;
}


struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule=rr_schedule
	/* Obviously, ... */
};

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
static struct process *findMax(struct list_head *head)
{
	struct process *node;
	unsigned int max_priority = 0;
	list_for_each_entry(node, head, list)
	{
		if (node->prio >=max_priority)
		{
			max_priority = node->prio;
		}
	}
	list_for_each_entry(node, head, list)
	{
		if (node->prio == max_priority)
		{
			return node;
		}
	}
	return NULL;
}
static struct process *prio_schedule()
{
	struct process *max_node=NULL;
	if (!list_empty(&readyqueue))
	{
		max_node=findMax(&readyqueue);
	}
	if (!current|| current->status == PROCESS_BLOCKED||current->age >= current->lifespan)
	{
		if(max_node)
			list_del_init(&max_node->list);
		return max_node;
	}
	if(max_node&&current->prio>max_node->prio)
	{
		return current;
	}
	else if(max_node&&current->prio<=max_node->prio)
	{
		list_del_init(&max_node->list);
		if(current->age < current->lifespan)
			list_add_tail(&current->list,&readyqueue);
		return max_node;
	}
	if (current->age < current->lifespan)
	{
		return current;
	}
	
	return max_node;
}

static void prio_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;
	if (!list_empty(&r->waitqueue)) { 
		struct process *waiter ;
		waiter=findMax(&r->waitqueue);
		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}
struct scheduler prio_scheduler = {
	.name = "Priority",
	.schedule=prio_schedule,
	.acquire=fcfs_acquire, //same with fcfs
	.release=prio_release,
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/

static struct process *pa_schedule()
{
	struct process*node;
	list_for_each_entry(node,&readyqueue,list)
	{
		if(node->prio<MAX_PRIO)
			node->prio++;
	}
	struct process *max_node=NULL;
	if (!list_empty(&readyqueue))
	{
		max_node=findMax(&readyqueue);
	}
	if (!current|| current->status == PROCESS_BLOCKED||current->age >= current->lifespan)
	{
		if(max_node)
		{	
			list_del_init(&max_node->list);
			max_node->prio=max_node->prio_orig;
		}
		return max_node;
	}
	if(max_node&&current->prio>max_node->prio)
	{
		return current;
	}
	else if(max_node&&current->prio<=max_node->prio)
	{
		list_del_init(&max_node->list);
		if(current->age < current->lifespan)
			list_add_tail(&current->list,&readyqueue);
		max_node->prio=max_node->prio_orig;
		return max_node;
	}
	if (current->age < current->lifespan)
	{
		return current;
	}
	
	return max_node;
}



struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	.schedule=pa_schedule,
	.acquire=fcfs_acquire, //same with fcfs
	.release=prio_release, //same with prio
};

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
static bool pcp_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		r->owner = current;
		current->prio=MAX_PRIO;
		return true;
	}
	current->status = PROCESS_BLOCKED;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

static void pcp_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;
	current->prio=current->prio_orig;
	if (!list_empty(&r->waitqueue)) { 
		struct process *waiter ;
		waiter=findMax(&r->waitqueue);
		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}


struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.schedule=prio_schedule, //same with prio
	//.forked=pa_forked, //same with pa
	.acquire=pcp_acquire,
	.release=pcp_release,

};

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
static bool pip_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		r->owner = current;
		return true;
	}
	current->status = PROCESS_BLOCKED;
	struct process *node;
	list_for_each_entry(node,&readyqueue,list)
	{
		if(node->pid==r->owner->pid)
		{	
			node->prio=current->prio;
		}
	}
	list_add_tail(&current->list, &r->waitqueue);

	return false;
}
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.schedule=prio_schedule, // same with prio
	.acquire=pip_acquire, 
	.release=pcp_release, //same with pcp 

};
