# Overview

In this assignment, we will write a Linux kernel module called infiniti. This module implements a page fault handler for a 64-bit x86 system, which runs multiple level page tables. You should still use the cs452 VM which you used for your tesla and lexus, as loading and unloading the kernel module requires the root privilege.

## Learning Objectives

 - Get further familiar with the frequently used system call function: the ioctl() system call.
 - Understand the x86 64-bit multiple level page table structures.

## Important Notes

You MUST build against the kernel version (3.10.0-1160.el7.x86\_64), which is the default version of the kernel installed on the cs452 VM.

While working on this assignment, please keep in mind, in current Linux systems, a virtual address is just 48 bits, not 64 bits. And your physical address is at most 52 bits.

## Book References

Read these chapters carefully in order to prepare yourself for this project:

 - Operating Systems: Three Easy Pieces: [Chapter 18: Introduction to Paging](https://pages.cs.wisc.edu/~remzi/OSTEP/vm)
 - Operating Systems: Three Easy Pieces: [Chapter 19: Translation Lookaside Buffers](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-tlbs.pdf)
 - Operating Systems: Three Easy Pieces: [Chapter 20: Advanced Page Tables](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-smalltables.pdf)
 - Operating Systems: Three Easy Pieces: [Chapter 21: Swapping: Mechanisms](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-beyondphys.pdf)

# Specification

According to the textbook chapter 21: "**The act of accessing a page that is not in physical memory is commonly referred to as a page fault**". When a page fault happens, a kernel level function will be called to handle it, and this function is known as the page fault handler. In this assignment, you will develop a page fault handler in a Linux system. The Linux kernel has its own page fault handler. In this assignment, we do not intend to take over the default page fault handler, rather we try to maintain a seperate handler, this handler will only handle memory pages mapped into a specific reserved memory region which the kernel will ignore. This memory region is in between virtual address 0x1000000000LL and virtual address 0x3000000000LL, and it is in the user space.

## The Starter Code

The starter code already provides you with the code for a kernel module called infiniti, and the code for a user-level library which interacts with the kernel module via ioctls. The kernel module implements a memory manager system, which manages the aforementioned reserved memory region. This is a region the kernel will not use. To install the module, run *make* and then *sudo insmod infiniti.ko*; to remove it, run *sudo rmmod infiniti*. Yes, in rmmod, whether or not you specify *ko* does not matter; but in insmod, you must have that *ko*.

What this module currently does is: create a file called /dev/infiniti, which provides an inteface for applications to communicate with the kernel module. One way to communicate between applications and the kernel module, is applications issue ioctl() system calls to this device file (i.e., /dev/infiniti), and the kernel module will handle these ioctl commands. A list of commands are currently supported:

 - LAZY\_ALLOC: Applications call library function *infiniti_malloc*(), which sends this LAZY\_ALLOC command to the kernel module, so as to allocate memory.
 - LAZY\_FREE: Applications call library function *infiniti_free*(), which sends this LAZY\_FREE command to the kernel module, so as to free memory.
 - DUMP\_STATE: Applications call library function *infiniti_dump*(), which sends this DUMP\_STATE command to the kernel module, so as to dump the state of our reserved memory region to the kernel log file /var/log/messages.
 - PAGE\_FAULT: Applications call library functions *init_infiniti*(), which registers the application into our memory manager system. Applications managed by our system need to use *infiniti_malloc*() to allocate dynamic memory, and use *infiniti_free*() to free dynamic memory. Such applications need to have their own page fault handler, because we only allocate memory from the aforementioned reserved memory region, which is a memory region the kernel will not handle.

The starter code will manage the reserved memory region, but it will not map any virtual address into a physical address. Thus, when the application tries to call *infiniti_malloc*(), if memory in the reserved memory region is available, the malloc function will succeed, and a pointer will be returned, just like the regular malloc() function. However, because the virtual address pointed to by this pointer is not mapped into anywhere in the physical memory, any access to such an address will just fail. When that access fails, the kernel will deliver a signal to the process (i.e., the application), normally this siginal will kill the process, but the process is configured (in *init_infiniti*())to intercept such signals and when such signals are received, the process will deliver a PAGE\_FAULT command to the kernel module, and now in this kernel module, your page fault handler *infiniti_do_page_fault*() will be called. In this handler function, you need to create the map, between this user space address, and a physical address. To achieve this, you call *get_zeroed_page*() to allocate physical memory, and then you update the page table so that that user space address is mapped to this physical address. After that, your *infiniti_do_page_fault*() will return, and the applicantion will try to access that user space address again, and this time it will succeed - if your *infiniti_do_page_fault*() function has updated the page table correctly.

The starter code also includes a user-level library, which implements functions such as *init_infiniti*(), *infiniti_malloc*(), *infiniti_free*(), *infiniti_dump*(). Several testing programs (infiniti-test1.c, infiniti-test2.c, infiniti-test3.c) are also provided. The user-level library, as well as the test programs, are located in the **user** folder. Once you navigate into the **user** folder, you need to run *make* to compile these test programs, and at the same time the user-level library will be automatically compiled and linked into the resulted binary of the test programs.

## Functions You Need to Implement

The only file you should modify in this assignment is fault.c. You need to implement the following 2 functions in this file:

 - *infiniti_do_page_fault*(): This function will be called when the application triggers a page fault. In this function you should ask physical memory from the kernel and then update the page tables. The prototype of this function is:

```c
int infiniti_do_page_fault(struct infiniti_vm_area_struct *infiniti_vma, uintptr_t fault_addr, u32 error_code)
```
this function should return 0 if a page fault is handled successfully, and return -1 if not. *fault_addr* is the user space address the application is trying to access. To handle the page fault, you need to update the page tables so that a mapping between the *fault_addr* and the physical address you allocated via *get_zero_page*() is created.

 - *infiniti_free_pa*(): this function will be called when the application calls *infiniti_free*(). In this function you should give the physical memory back to the kernel and then update the page tables. The prototype of this function is:

```c
void infiniti_free_pa(uintptr_t user_addr)
```
 Before the application calls your *infiniti_free_pa*() (via *infiniti_free*()), this *user_addr* is mapped to some physical address - thank to your *infiniti_do_page_fault*(). Now when the application calls your *infiniti_free_pa*() (via *infiniti_free*()), you should update the page tables so as to destroy the mapping. In other words, when the application calls *infiniti_free*(), the mapping will no longer exist, and the application should no longer be able to access that same physical address. 

## Predefined Data Structures, Global Variables, and Provided Helper Functions
 - *invlpg*(): we call this function to invalidate the tlb entry for one specific page. This function takes a virtual address, the processor determines the page that contains that virtual address and flushes all TLB entries for that page.
 - *get_cr3*(): we call this function to get the content of the cr3 register.

## Related Kernel APIs

I used the following APIs. 
 - *get_zeroed_page*()and *free_page*(). You call *get_zeroed_page*() to get a free memory page (filled with zeros) from the kernel, later on you call *free_page*() to give the memory back to the kernel. This is how you use *get_zeroed_pages*():

```c
uintptr_t kernel_addr = 0;
kernel_addr = (uintptr_t)get_zeroed_page(GFP_KERNEL);
if (!kernel_vaddr) {
	printk(KERN_INFO "failed to allocate one page\n");
	return -ENOMEM;
}
```

and then later this is how you use *free_page*().

```c
free_page(kernel_addr);
```

the address returned by *get_zeroed_page*() is a page aligned virtual address (in kernel space), which means its lowest 12 bits are 0, and its corresponding physical address also has its lowest 12 bits be 0. For example, if you add a printk statement to print the address represented by the above *kernel_vaddr* variable:

```c
printk(KERN_INFO "kernel address is %lx, and its physical address is %lx\n", kernel_vaddr, __pa(kernel_vaddr));
```

in your log, you will see some messages like this:

```console
kernel address is 0xffff880077313000, and its physical address is 0x77313000
kernel address is 0xffff880059eca000, and its physical address is 0x59eca000
kernel address is 0xffff880075faf000, and its physical address is 0x75faf000
```

 - *__va()* and *__pa()*. Given a physical address, *__va*() gives you its virtual address; given a virtual address, *__pa*() gives you its physical address. Of course, the virtual address gets involved here must be a kernel virtual address. For user virtual address, we still need to walk the page table to do the translation.

## Expected Results

If you compile the stater code, install the default infiniti kernel module, and run the tests, you will get the following results:

```console
[cs452@localhost user]$ ./infiniti-test1
Segmentation fault (core dumped)
[cs452@localhost user]$ ./infiniti-test2
Segmentation fault (core dumped)
[cs452@localhost user]$ ./infiniti-test3
Segmentation fault (core dumped)
```

Once your implementation is complete, you install the infiniti kernel module, and run the tests, you should get the following results:

```console
[cs452@localhost user]$ ./infiniti-test1
Hello Boise!
[cs452@localhost user]$ ./infiniti-test2
Hello Boise!
Segmentation fault (core dumped)
[cs452@localhost user]$ ./infiniti-test3
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
buf is a
test
success
```
Note that seg fault message showed when running infiniti-test2, is expected, and is intentionally showed. It happens because the test program tries to access a freed buffer. If yours does not show this seg fault message, it means your *infiniti_free_pa*() function is not implemented correctly. For example, if your *infiniti_free_pa*() function is completely empty, but your *infiniti_do_page_fault*() works correctly, then this is what you will see when running infiniti-test2.

```console
[cs452@localhost user]$ ./infiniti-test2
Hello Boise!
Hello Boise!
```

The logic here is, the test program allocates a buffer to store this message, and then the program prints this message; and then the program tries to free its memory, however, because your *infiniti_free_pa*() does nothing, the memory will not be freed, and the page table mappings are therefore still there, thus the program can still print this same message. A fully functioning system would tell the program this is not allowed, and would kill the program and show a seg fault message.

## Walk Through the 4-Level Page Tables

In this assignment, we only consider 4KB pages, i.e., each page is 4KB. By default, a 4-level page table structure is used in the Linux kernel running on 64-bit x86 platforms. In Intel's terminology, these 4-level tables are called:

 - PML4, or page map level 4; each entry in this table is called  a page map level 4 entry, or PML4E.
 - PDPT, or page directory pointer table; each entry in this table is called a page directory pointer table entry, or PDPTE.
 - PDT, or page directory table; each entry in this table is called a page directory table entry, or PDTE.
 - PT, or page table; each entry in this table is called a page table entry, or PTE.

<!The 4-level page tables are known as:

 - Page Global Directory (PGD)
 - Page Upper Directory (PUD).
 - Page Middle Directory (PMD).
 - Page Table Entry directory (PTE).->

Each of these tables has 512 entries, and each entry is 8 bytes, and thus in total it's 512x8=4KB per table, which is one page.

Whenever we need to translate a virtual address to a physical address, we need to walk through all 4 tables. This whole process starts from the register *CR3* - this register always points to the starting physical address of the PML4 table. Remember, your physical address has at most 52 bits, thus *CR3*, which is a 64-bit register, has its bit 52-63 all 0s. In addition, all 4-level page tables are stored at page-aligned address, which means their starting physical address has it lowest 12 bits all 0s: because one page is 4KB, which is 2^12. Therefore, for a virtual machine whose total memory size is 2GB, the following is typical value stored in *CR3*:

```console
0x77272000
```

Note that this address has its lowest 12 bits all 0s, and if we convert this address to binary, it will be: 0b 111 0111 0010 0111 0010 0000 0000 0000. From this binary number, we can see this address has 31 bits, which is reasonable when the total memory size is 2GB, because 2GB=2^31.

To translate a virtual address to a physical address,

1. we get the physical address of the PML4 table from CR3, and use bit 47 to bit 39 of the virtual address to get the correct PML4E.
2. we use the PML4E to get the physical address of the PDPT table, and use bit 38 to bit 30 of the virtual address to get the correct PDPTE.
3. we use the PDPTE to get the physical address of the PDT table, and use bit 29 to bit 21 of the virtual address to get the correct PDTE.
4. we use the PDTE to get the physical address of the page table, and use bit 20 to bit 12 of the virtual address to get the correct PTE.
5. we use the PTE to get the page frame number, and use bit 11 to bit 0 of the virtual address to get the offset.
6. we concatenate the page frame number with the offset to get the physical address.

Note: bit 47 to bit 39 are 9 bits; bit 38 to bit 30 are 9 bits, bit 29 to bit 21 are 9 bits, bit 20 to bit 12 are 9 bits. And 2^9 is 512, this matches with the fact that each of these tables has 512 entries: you only need 9 bits to index a table when the table has 512 entries in total.

Keep in mind that in this assignment, your job is NOT to translate a virtual address to a physical address, but rather is to update these tables so that CPU will be able to walk through the tables and translate a virtual address to a physical address. CPU will follow the above 6 steps to walk a 4-level page table structure, your job is just to make sure these page tables contain the correct information. More specifically, when your *infiniti_do_page_fault*() is called, you should update these page tables so that the CPU, via walking the page tables, can translate the fault address to the newly allocated physical address. And then when your *infiniti_free_pa*() is called, you should update these page tables again, so that the mapping between that user space address and its corresponding physical address will be destroyed.

To achieve the goals we just described, your *infiniti_do_page_fault*() should do the following:

1. find the PML4E, check its present bit, which is bit 0 of the entry, if it is 1, then move on to step 2; if it is 0, then we need to call *get_zeroed_page*() to allocate a page for the PDPT table, and update the PML4E entry to reflect that this PDPT table is present, is writable, is a user page. This requires you to change the PML4E's bit 0, bit 1, bit 2 to 1. Also, store the physical frame number of the allocated page into the PML4E entry's bit 12 to bit 51. 
2. find the PDPTE, check its present bit, which is bit 0 of the entry, if it is 1, then move on to step 3; if it is 0, then we need to call *get_zeroed_page*() to allocate a page for the PDT table, and update the PDPTE entry to reflect that this PDT table is present, is writable, is a user page. This requires you to change the PDPTE's bit 0, bit 1, bit 2 to 1. Also, store the physical frame number of the allocated page into the PDPTE entry's bit 12 to bit 51.
3. find the PDTE, check its present bit, which is bit 0 of the entry, if it is 1, then move on to step 4; if it is 0, then we need to call *get_zeroed_page*() to allocate a page for the page table, and update the PDTE entry to reflect that this page table is present, is writable, is a user page. This requires you to change the PDTE's bit 0, bit 1, bit 2 to 1. Also, store the physical frame number of the allocated page into the PDPE entry's bit 12 to bit 51.
4. find the PTE, check its present bit, which is bit 0 of the entry, if it is 1, then move on to step 5; if it is 0, then we need to call *get_zeroed_page*() to allocate a page for the physical page, and update the PTE entry to reflect that this physical page is present, is writable, is a user page. This requires you to change the PTE's bit 0, bit 1, bit 2 to 1. Also, store the physical frame number of the allocated page into the PTE entry's bit 12 to bit 51.
5. return 0.

And your *infiniti_free_pa*(), which takes *uintptr_t user_addr* as its parameter, should do the following:

1. find the PML4E, check its present bit, which is bit 0 of the entry, if it is 0, then there is nothing you need to free - there is no valid mapping, so just return; if it is 1, then move on to step 2.
2. find the PDPTE, check its present bit, which is bit 0 of the entry, if it is 0, then there is nothing you need to free - there is no valid mapping, so just return; if it is 1, then move on to step 3.
3. find the PDTE, check its present bit, which is bit 0 of the entry, if it is 0, then there is nothing you need to free - there is no valid mapping, so just return; if it is 1, then move on to step 4.
4. find the PTE, check its present bit, which is bit 0 of the entry, if it is 0, then there is nothing you need to free - there is no valid mapping, so just return; if it is 1, then move on to step 5.
5. now that you are here, you actually have just "accidentally" walked the whole page tables, and now the PTE contains the physical frame number of the page the application wants to free, so get the offset from *user_addr*, and concatenate the physical frame number with the offset, will give you the physical address you should free, convert this physical address to its kernel space address (via *__va*()), and call *free_page*() to free it.
6. now that the physical memory page is freed, you need to update the page tables to destroy the mapping. following steps are needed:
  6.1. set the entire PTE entry to 0. check if the entire page table is free, i.e., if in this page table, every entry's present bit is 0, then we can say this page table is not used at all, and therefore its memory should be freed. call *free_page*() to free this page table.
  6.2. set the entire PDTE entry to 0. check if the entire PDT table is free, i.e., if in this PDT table, every entry's present bit is 0, then we can say this PDT table is not used at all, and therefore its memory should be freed. call *free_page*() to free this PDT table.
  6.3. set the entire PDPTE entry to 0. check if the entire PDPT table is free, i.e., if in this page table, every entry's present bit is 0, then we can say this page table is not used at all, and therefore its memory should be freed. call *free_page*() to free this PDPT table.
  6.4. set the entire PML4E entry to 0. check if the entire PML4 table is free, i.e., if in this page table, every entry's present bit is 0, then we can say this page table is not used at all, and therefore its memory should be freed. call *free_page*() to free this PML4 table.

# Submission

Due: 23:59pm, March 1st, 2022. Late submission will not be accepted/graded.

# Grading Rubric (Undergraduate and Graduate)
Grade: /100

- [ 70 pts] Functional Requirements: page faults handled correctly:
    - infiniti-test1 runs and ends smoothly: message printed correctly, no program crash, no kernel crash. /20
    - infiniti-test2 runs and ends with a seg fault: message printed, then seg fault, but no kernel crash. /30
    - infiniti-test3 runs and ends smoothly: messages printed correctly, no program crash, no kernel crash. /20

- [10 pts] Module can be installed and removed without crashing the system: 
   - You won't get these points if your module doesn't implement any of the above functional requirements.

- [10 pts] Compiling:
   - Each compiler warning will result in a 3-point deduction.
   - You are not allowed to suppress warnings. (you won't get these points if your module doesn't implement any of the above functional requirements.)

- [10 pts] Documentation:
   - README.md file (replace this current README.md with a new one using the README template. You do not need to check in this current README file.)
   - You are required to fill in every section of the README template, missing 1 section will result in a 2-point deduction.
