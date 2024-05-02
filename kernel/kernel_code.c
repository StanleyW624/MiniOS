/*
 * kernel_code.c - Project 2, CMPSC 473
 * Copyright 2023 Ruslan Nikolaev <rnikola@psu.edu>
 * Distribution, modification, or usage without explicit author's permission
 * is not allowed.
 */

#include <kernel.h>
#include <types.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>

void *page_table = NULL; /* TODO: Must be initialized to the page table address */
void *user_stack = NULL; /* TODO: Must be initialized to a user stack virtual address */
void *user_program = NULL; /* TODO: Must be initialized to a user program virtual address */

void kernel_init(void *ustack, void *uprogram, void *memory, size_t memorySize)
{
	// 'memory' points to the place where memory can be used to create
	// page tables (assume 1:1 initial virtual-to-physical mappings here)
	// 'memorySize' is the maximum allowed size, do not exceed that (given just in case)

	// TODO: Create a page table here
	
	// FOR PTE
	uint64_t *pte = (uint64_t*) memory;

	for (size_t i = 0; i < 1048576; i++) {
		pte[i] = (i<<12) | 3; 	// Note for self: i<<12 is 4096
	}

	// FOR PDE
	uint64_t* pde = pte + 1048576;
	for (size_t j = 0; j < 2048; j++) {
		uint64_t* spde = pte + 512 * j; //since each page is 512, we will increment with size of 512, thus brings us to the next page
		pde[j] = (uint64_t) spde | 3; //set the first 3 bits for spde
	}

	// Note: PDPE From entry 4 should all be 0 (< 512), PMLE4E from entry 1 should be all 0 (up to 510 aka < 511)
	// FOR PDPE
	uint64_t* pdpe = pde + (4*512);
	for (size_t k = 0; k < 4; k++) {
		uint64_t* spdpe = pde + 512 * k;
		pdpe[k] = (uint64_t) spdpe | 3;
	}

	// Set all other pages to 0
	for (size_t ki = 4; ki < 512; ki++) {
		pdpe[ki] = 0;
	}

	// FOR PMLE4E
	uint64_t* pmle4e = pdpe + 512;
	*pmle4e = (uint64_t) pdpe | 3;
	
	// Set all other pages to 0
	for (size_t l = 1; l < 511; l++) {
		pmle4e[l] = 0;
	}
	
	page_table = pmle4e;

	// TODO: It is OK for Q1-Q3, but should changed
	// for the final part's Q4 when creating your own user page table

	// User code starts from here
	
	// Get physical address
	uint64_t* user_PA = ustack - 4096;
	
	// For user_PDPE
	uint64_t *updpe = pmle4e + 512;
	// Only using 1 page therefore set all other page to 0 
	for (size_t x = 0; x < 512; x++) {
		updpe[x] = 0;
	}
	// Link level 4 user to level 3 user; initialize the last page of PMLE4E to PDPE
	pmle4e[511] = (uint64_t) updpe | 7; 

	// For user_PDE
	uint64_t* upde = updpe + 512;
	// Only using 1 page therefore set all other page to 0 
	for (size_t y = 0; y < 512; y++) {
		upde[y] = 0;
	}
	// Link level 3 user to level 2 user; initialize the last page of PDPE to PDE
	updpe[511] =  (uint64_t) upde | 7;

	// For user_PTE
	uint64_t* upte = upde + 512;
	// Only using 1 page therefore set all other page to 0 
	for (size_t z = 0; z < 512; z++) {
		upde[z] = 0;
	}
	// Link level 2 user to level 1 user; initialize the last page of PDE to PTE
	upde[511] = (uint64_t) upte | 7;

	// Set uprogram pointer (gives user access) [3rd page]
	upte[509] = (uint64_t) uprogram | 7;
	// Set physcial address pointer (gives user access)
	upte[511] = (uint64_t) user_PA | 7;


	// User Stack should point to the Virtual address + 4096, which user_PA is mapped as Virtual Address
	user_stack = user_PA + 4096;

	// TODO: It is OK for Q1-Q3, but should changed
	// for the final part's Q4 when creating your own user page table

	// This gets the 3rd page of the level 1 user table
	user_program = (uint64_t*) (0-12288);

	//printf("Hello from sgroup53, rml5846\n");
	// The remaining portion just loads the page table,
	// this does not need to be changed:
	// load 'page_table' into the CR3 register

	const char *err = load_page_table(page_table);
	if (err != NULL) {
		printf("ERROR: %s\n", err);
	}

	// The extra credit assignment
	mem_extra_test();
}

/* The entry point for all system calls */
long syscall_entry(long n, long a1, long a2, long a3, long a4, long a5)
{
	// TODO: the system call handler to print a message (n = 1)
	// the system call number is in 'n', make sure it is valid!

	// Arguments are passed in a1,.., a5 and can be of any type
	// (including pointers, which are casted to 'long')
	// For the project, we only use a1 which will contain the address
	// of a string, cast it to a pointer appropriately 

	// For simplicity, assume that the address supplied by the
	// user program is correct
	//
	// Hint: see how 'printf' is used above, you want something very
	// similar here

	// Check if n = 1
	if  (n==1) {
		// Using char pointer to cast a1
		char *scm = (char *) a1;
		printf("%s", scm);
		return 0;
	}
	return -1; /* Success: 0, Failure: -1 */
}
