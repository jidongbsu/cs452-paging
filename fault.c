#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/pgtable.h>
#include <asm/pgtable_types.h>

#include "infiniti.h"
#include "fault.h"

#define DEBUG 1

int is_valid_address(struct infiniti_vm_area_struct *infiniti_vma, uintptr_t user_addr){
	struct list_head *pos, *next;
 	struct infiniti_vm_area_struct *node;
	/* traverse the list of nodes, and see if there is one which is reserved, and its range includes this address. */
	list_for_each_safe(pos, next, &(infiniti_vma->list)) {
		node = list_entry(pos, struct infiniti_vm_area_struct, list);
        if(node->status == RESERVED && user_addr >= node->start && user_addr < (node->start + 4096 * node->num_pages)){
            return 1;
        }
	}
    return 0;
}

int is_entire_page_table_free(pte_t *table){
	int i = 0;
	/* all four tables have 512 entries, each has 8 bytes, and thus 4KB per entry.. */
	for(i = 0; i < 512; i++){
		/* each entry is 8 bytes. */
		int offset = i * 8;
		pte_t *pte = (table + offset);
		/* as long as one of them is one, then we can't free them or invalidate the tlb entries. */
		if(pte_present(*pte) == 1){
			/* not entirely free */
			return 0;
		}
	}
	/* table entirely free */
	return 1;
}

int is_entire_pmd_table_free(pmd_t *table){
	int i = 0;
	/* all four tables have 512 entries, each has 8 bytes, and thus 4KB per entry.. */
	for(i = 0; i < 512; i++){
		/* each entry is 8 bytes. */
		int offset = i * 8;
		pmd_t *pmd = (table + offset);
		/* as long as one of them is one, then we can't free them or invalidate the tlb entries. */
		if(pmd_present(*pmd) == 1){
			/* not entirely free */
			return 0;
		}
	}
	/* table entirely free */
	return 1;
}

int is_entire_pud_table_free(pud_t *table){
	int i = 0;
	/* all four tables have 512 entries, each has 8 bytes, and thus 4KB per entry.. */
	for(i = 0; i < 512; i++){
		/* each entry is 8 bytes. */
		int offset = i * 8;
		pud_t *pud = (table + offset);
		/* as long as one of them is one, then we can't free them or invalidate the tlb entries. */
		if(pud_present(*pud) == 1){
			/* not entirely free */
			return 0;
		}
	}
	/* table entirely free */
	return 1;
}

int is_entire_pgd_table_free(pgd_t *table){
	int i = 0;
	/* all four tables have 512 entries, each has 8 bytes, and thus 4KB per entry.. */
	for(i = 0; i < 512; i++){
		/* each entry is 8 bytes. */
		int offset = i * 8;
		pgd_t *pgd = (table + offset);
		/* as long as one of them is one, then we can't free them or invalidate the tlb entries. */
		if(pgd_present(*pgd) == 1){
			/* not entirely free */
			return 0;
		}
	}
	/* table entirely free */
	return 1;
}

/*
 * error_code:
 * 1 == not present
 * 2 == permissions error
 * return 0 if handled successful; otherwise return -1.
 * */

int infiniti_do_page_fault(struct infiniti_vm_area_struct *infiniti_vma, uintptr_t fault_addr, u32 error_code) {
    printk("Page fault!\n");
    return -1;
}

/* this function takes a user VA and free its PA as well as its kernel va. */
void infiniti_free_pa(uintptr_t user_addr){
	return;
}

/* vim: set ts=4: */
