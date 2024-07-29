/**********************************************************************
 * Copyright (c) 2020-2024
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
#include <string.h>

#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;

/**
 * TLB of the system.
 */
extern struct tlb_entry tlb[];


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * lookup_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Translate @vpn of the current process through TLB. DO NOT make your own
 *   data structure for TLB, but should use the defined @tlb data structure
 *   to translate. If the requested VPN exists in the TLB and it has the same
 *   rw flag, return true with @pfn is set to its PFN. Otherwise, return false.
 *   The framework calls this function when needed, so do not call
 *   this function manually.
 *
 * RETURN
 *   Return true if the translation is cached in the TLB.
 *   Return false otherwise
 */
bool lookup_tlb(unsigned int vpn, unsigned int rw, unsigned int *pfn)
{
	for(int i=0 ;i<NR_TLB_ENTRIES;i++)
	{
		if(tlb[i].vpn==vpn&&tlb[i].valid==true)
		{
			if(tlb[i].rw>=rw)
			{
				*pfn=tlb[i].pfn;
				return true;
			}
			else
				return false;
		}
	}
	return false;
}


/**
 * insert_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Insert the mapping from @vpn to @pfn for @rw into the TLB. The framework will
 *   call this function when required, so no need to call this function manually.
 *   Note that if there exists an entry for @vpn already, just update it accordingly
 *   rather than removing it or creating a new entry.
 *   Also, in the current simulator, TLB is big enough to cache all the entries of
 *   the current page table, so don't worry about TLB entry eviction. ;-)
 */
void insert_tlb(unsigned int vpn, unsigned int rw, unsigned int pfn)
{
	for(int i=0;i<NR_TLB_ENTRIES;i++)
	{
		if(tlb[i].vpn==vpn&&tlb[i].valid)
		{
			if(tlb[i].pfn==pfn&&tlb[i].rw==rw)
				return;
			else // if different update 
			{
				tlb[i].pfn=pfn;
				tlb[i].rw=rw;
				return ;
			}
		}
	}
	for(int i=0;i<NR_TLB_ENTRIES;i++)
	{
		if(tlb[i].valid==false)
		{
			tlb[i].vpn=vpn;
			tlb[i].rw=rw;
			tlb[i].pfn=pfn;
			tlb[i].valid=true;
			return ;
		}
	}
}


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with ACCESS_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with ACCESS_READ should not be accessible with
 *   ACCESS_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */


unsigned int alloc_page(unsigned int vpn, unsigned int rw) {
    int pd_index = vpn / NR_PTES_PER_PAGE;
    int pte_index = vpn % NR_PTES_PER_PAGE;

    struct pagetable *pt = ptbr;
    struct pte_directory *pd;
    struct pte *pte;
    
    pd = pt->pdes[pd_index];
    if (!pd) {
        pd = malloc(sizeof(struct pte_directory));
        pt->pdes[pd_index] = pd;  // index update
    }
    pte = &pd->ptes[pte_index];
    for (int i = 0; i < NR_PAGEFRAMES; i++) {
        if (mapcounts[i] == 0) {
            pte->pfn = i;
            mapcounts[i]++;
            pte->rw = rw;
            pte->valid = 1;
            return i;
        }
    }
    return -1; // No free page frame found
}

/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, rw, pfn) is set @false or 0.
 *   Also, consider the case when a page is shared by two processes,
 *   and one process is about to free the page. Also, think about TLB as well ;-)
 */
void free_page(unsigned int vpn)
{
	int pd_index = vpn / NR_PTES_PER_PAGE;
	int pte_index = vpn % NR_PTES_PER_PAGE;

	struct pagetable *pt = ptbr;
	struct pte_directory *pd;
	struct pte *pte;
	pd = pt->pdes[pd_index];
	pte = &pd->ptes[pte_index];
	if(mapcounts[pte->pfn]>0)
		mapcounts[pte->pfn]--;
	for(int i=0;i<NR_TLB_ENTRIES;i++)
	{
		if(tlb[i].valid&&tlb[i].vpn==vpn)
		{
			tlb[i].valid=false;
			break;
		}
	}
	pte->valid=0;
	pte->rw=ACCESS_NONE;
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	int pd_index = vpn / NR_PTES_PER_PAGE;
	int pte_index = vpn % NR_PTES_PER_PAGE;

	struct pagetable *pt = ptbr;
	struct pte_directory *pd;
	struct pte *pte;
	pd = pt->pdes[pd_index];
	pte = &pd->ptes[pte_index];
	if(rw==ACCESS_WRITE&&pte->private)
	{
		if(mapcounts[pte->pfn]>=2)
		{
			mapcounts[pte->pfn]--;
			for(int i=0;i<NR_PAGEFRAMES;i++)
			{
				if(mapcounts[i]==0)
				{
					pte->pfn=i;
					mapcounts[i]++;
					break;
				}
			}
		}
		pte->rw=3; //read and write
		return true;
	}
	return false;
}


/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. 
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for 
 *   storing some useful information :-)
 */
void switch_process(unsigned int pid) {
    
	for(int i=0;i<NR_TLB_ENTRIES;i++)
	{
		tlb[i].valid=false;
	}
	struct process *node, *temp;
    int found = 0;

    list_for_each_entry_safe(node, temp, &processes, list) {
        if (node->pid == pid) {
            list_add_tail(&current->list, &processes);
            list_del_init(&node->list);
            current = node;
            ptbr = &current->pagetable;
            found = 1;
            break;
        }
    }
    if (!found) { // Fork
        struct process *fork_process = malloc(sizeof(struct process));
        fork_process->pid = pid;
        INIT_LIST_HEAD(&fork_process->list);
        for (int i = 0; i < NR_PDES_PER_PAGE; i++) { //deep copy 
            if (current->pagetable.pdes[i]) {
                fork_process->pagetable.pdes[i] = malloc(sizeof(struct pte_directory));
                memcpy(fork_process->pagetable.pdes[i], current->pagetable.pdes[i], sizeof(struct pte_directory)); //copy pde
                for (int j = 0; j < NR_PTES_PER_PAGE; j++) {
                    struct pte *pte = &fork_process->pagetable.pdes[i]->ptes[j];
					struct pte *pte2=&current->pagetable.pdes[i]->ptes[j];
                    if (pte->valid) {  //for copy on write
                        if(pte->rw>=2) // rw-> r
						{
							pte->private=1;
							pte2->private=1;
						}
						pte->rw = ACCESS_READ;
						pte2->rw=ACCESS_READ;
                        mapcounts[pte->pfn]++;
                    }
                }
            } else {
                fork_process->pagetable.pdes[i] = NULL;
            }
        }
        list_add_tail(&current->list, &processes);
        current = fork_process;
        ptbr = &current->pagetable;
    }
}

