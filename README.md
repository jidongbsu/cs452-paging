# Overview

In this assignment, we will write a Linux kernel module called infiniti. This module implements a page fault handler for a 64-bit x86 system, which runs multiple level page tables. You should still use the cs452 VM which you used for your tesla and lexus, as loading and unloading the kernel module requires the root privilege.

## Learning Objectives

 - Get further familiar with the frequently used system call function: the ioctl() system call.
 - Understand the x86 64-bit multiple level page table structures.

## Important Notes

You MUST build against the kernel version (3.10.0-1160.el7.x86\_64), which is the default version of the kernel installed on the cs452 VM.

## Book References

Read these chapters carefully in order to prepare yourself for this project:

 - Operating Systems: Three Easy Pieces: [Chapter 18: Introduction to Paging](https://pages.cs.wisc.edu/~remzi/OSTEP/vm) (also known as "Paging: Introduction").
 - Operating Systems: Three Easy Pieces: [Chapter 19: Translation Lookaside Buffers](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-tlbs.pdf) (also known as "Paging: Faster Translations (TLBs)").
 - Operating Systems: Three Easy Pieces: [Chapter 20: Advanced Page Tables](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-smalltables.pdf) (also known as "Paging: Smaller Tables").

# Specification

You will develop a page fault handler in a Linux system. Please refer to the book chapters to have a basic understanding of what page fault handler does.

Your page fault handler will work as a kernel module. The Linux kernel has its own page fault handler. In this assignment, we do not intend to take over the default page fault handler, rather we try to maintain a seperate handler, this handler will only handle memory pages mapped into a specific reserved memory region which the kernel will ignore.

## The Starter Code

The starter code already provides you with the code for a kernel module called infiniti, and the code for a user-level library which interacts with the kernel module via ioctls. The kernel module implements a memory manager system, which manages the above reserved memory region. To install the module, run *make* and then *sudo insmod infiniti.ko*; to remove it, run *sudo rmmod infiniti*. Yes, in rmmod, whether or not you specify *ko* does not matter; but in insmod, you must have that *ko*.

What this module currently does is: create a file called /dev/infiniti, which provides an inteface for applications to communicate with the kernel module. One way to communicate between applications and the kernel module, is applications issue ioctl() system calls to this device file (i.e., /dev/infiniti), and the kernel module will handle these ioctl commands. A list of commands are currently supported:

 - LAZY\_ALLOC: Applications call library function *infiniti_malloc*(), which sends this command to the kernel module, so as to allocate memory.
 - LAZY\_FREE: Applications call library function *infiniti_free*(), which sends this command to the kernel module, so as to free memory.
 - DUMP\_STATE: Applications call library function *infiniti_dump*(), which sends this command to the kernel module, so as to dump the state of our reserved memory region.
 - PAGE\_FAULT: Applications call library functions *init_infiniti*(), which registers the application into our memory manager system. Applications managed by our system need to use *infiniti_malloc*() to allocate dynamic memory, and use *infiniti_free*() to free dynamic memory. Such applications need to have their own page fault handler, because we only allocate memory from the aforementioned reserved memory region.

The starter code also includes a user-level library, which implements functions such as *init_infiniti*(), *infiniti_malloc*(), *infiniti_free*(), *infiniti_dump*(). Several testing programs (infiniti-test1.c, infiniti-test2.c, infiniti-test3.c, infiniti-test4.c) are also provided. The user-level library, as well as the test programs, are located in the **user** folder. Once you navigate into the **user** folder, you need to run *make* to compile these test programs, the user-level library will be automatically compiled and linked into the resulted binary of the test programs.

## Functions You Need to Implement

You need to implement the following 2 functions in the kernel module:
 - *infiniti_do_page_fault*(): This function will be called when the application triggers a page fault. In this function you should ask physical memory from the kernel and then update the page tables.
 - *infiniti_free_pa*(): this function will be called when the application calls *infiniti_free*(). In this function you should give the physical memory back to the kernel and then update the page tables.

The only file you should modify is fault.c.

## Predefined Data Structures, Global Variables, and Provided Helper Functions
 - *invlpg*().
 - *get_cr3*().

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

the address returned by *get_zeroed_page*() is a page aligned address, which means its lower 12 bits are 0, and its corresponding physical address also has its lower 12 bits be 0. For example, if you add a printk statement to print the address represented by the above *kernel_vaddr* variable:

```c
printk(KERN_INFO "kernel address is %lx, and its physical address is %lx\n", kernel_vaddr, __pa(kernel_vaddr));
```

in your log, you will see some message like this:

```console
kernel address is ffff880077313000, and its physical address is 77313000
kernel address is ffff880059eca000, and its physical address is 59eca000
kernel address is ffff880075faf000, and its physical address is 75faf000
```

 - *__va()* and *__pa()*. You may only need these two functions for debugging purpose, given a physical address, *__va*() gives you its virtual address; given a virtual address, *__pa*() gives you its physical address. Of course, the virtual address gets involved here must be a kernel virtual address. For user virtual address, we still need to walk the page table to do the translation.

## Expected Results

If you compile the stater code, install the default infiniti kernel module, and run the tests, you will get the following results:

```console
[cs452@localhost user]$ ./infiniti-test1
Segmentation fault (core dumped)
[cs452@localhost user]$ ./infiniti-test2
Segmentation fault (core dumped)
[cs452@localhost user]$ ./infiniti-test3
Segmentation fault (core dumped)
[cs452@localhost user]$ ./infiniti-test4
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
infiniti-test4 is just for demonstration purpose. When you run it, in you dmesg log, or /var/log/messages, you will see the dump of the reserved virtual memory region.

Note that seg fault message showed when running infiniti-test2, is expected, and is intentionally showed. It happens because the test program tries to access a freed buffer. If yours does not show this seg fault message, it means your *infiniti_free_pa*() function is not implemented correctly. For example, if your *infiniti_free_pa*() function is completely empty, but your *infiniti_do_page_fault*() works correctly, then this is what you will see when running infiniti-test2.

```console
[cs452@localhost user]$ ./infiniti-test2
Hello Boise!
Hello Boise!
```

The logic here is, the test program allocates a buffer to store this message, and then the program prints this message; and then the program tries to free its memory, however, because your *infiniti_free_pa*() does nothing, the memory will not be freed, and the page table mappings are therefore still there, thus the program can still print this same message. A fully functioning system would tell the program this is not allowed, and would kill the program and show a seg fault message.

## Walk Through the 4-Level Page Tables

In this assignment, we only consider 4KB pages, i.e., each page is 4KB. By default, a 4-level page table structure is used in the Linux kernel running on 64-bit x86 platforms. In Intel's terminology, these 4-level tables are called:

 - PML4, or page map level 4; each entry in this table is called a PML4E, i.e., a page map level 4 entry.
 - PDPT, or page directory pointer table; each entry in this table is called a PDPTE, i.e., a page directory pointer table entry.
 - PDT, or page directory table; each entry in this table is called a PDTE, i.e., a page directory table entry.
 - PT, or page table; each entry in this table is called a PTE, i.e., a page table entry.

Each of these table has 512 entries, and each entry is 8 bytes, and thus in total it's 4KB per table, which is one page.

But Linux kernel has its own naming conventions. The 4-level page tables are known as:

 - Page Global Directory (PGD)
 - Page Upper Directory (PUD).
 - Page Middle Directory (PMD).
 - Page Table Entry directory (PTE).

In this assignment, we will use the Linux kernel naming convention.

We need to walk through all of these tables whenever we need to translate a virtual address to a physical address. This whole process starts from the register *CR3* - this register always points to the physical address of the PML4 table base, i.e., the starting address of the PML4 table. 

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
