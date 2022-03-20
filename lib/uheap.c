#include <inc/lib.h>

// malloc()
//	This function use FIRST FIT strategy to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
void* FindAllocationAddress(uint32 size);
void heapAllocation(uint32 va , uint32 size);
void* BESTFIT(uint32 numOfPages);
void DeAllocation(uint32 selected_allocation_start_va, uint32 rounded_size);

uint32 totalSize = USER_HEAP_MAX-USER_HEAP_START;
uint32 total_num_of_pages = (USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE;
uint32 nextAllocationAddress;

//Holds the indices of all heap pages
uint32 pageindex[(USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE] = {0};

//Holds the info of allocated blocks
struct AllocationInfo
{
	uint32 startAddress;
	uint32 size;

}allocInfo[(USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE];

//Holds num of consecutive empty pages and the start address of the first one
struct freePages
{
	int numOfFreePages;
	uint32* startAddress;

}freepages[(USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE];




void* BESTFIT(uint32 numOfPages)
{
        uint32 index=0, count=0, freeidx=0,idx=-1;
		uint32 startAddress;
		for(uint32 i=USER_HEAP_START; i < USER_HEAP_MAX; i+=PAGE_SIZE)
		{
			if(pageindex[index] == 0)
			{
				if(count == 0)
				{

					//saves the address of the first page in each group of consecutive pages
					startAddress = i;
				}
	            ++count;
			}
			else
			{

				if(count != 0)
				{
					freepages[freeidx].numOfFreePages = count;
					freepages[freeidx].startAddress = (uint32*)startAddress;
				    count = 0;
					++freeidx;
				}
			}
			++index;
		}
		//to handle if there are any free pages that weren't added in the loop
		if(freeidx == 0 || count > 0 )
		{
			freepages[freeidx].numOfFreePages = count;
		    freepages[freeidx].startAddress = (uint32*)startAddress;
		    ++freeidx;

		}
		int temp = total_num_of_pages + 5;
		uint32* startAd = NULL;
		for(int i = 0; i < freeidx; i++)
		{
			if(numOfPages == freepages[i].numOfFreePages)
			{
				return (void*)freepages[i].startAddress;
			}
			if(freepages[i].numOfFreePages > numOfPages)
		    {

				temp = MIN(freepages[i].numOfFreePages,temp);
				if(freepages[i].numOfFreePages == temp)
				{
					startAd = freepages[i].startAddress;
					idx = i;
				}

			}

		}
		if(idx!=-1 )
        {
			if(freepages[idx].numOfFreePages > numOfPages)
			{
				freepages[idx].numOfFreePages-=numOfPages;
				freepages[idx].startAddress+=(numOfPages*PAGE_SIZE);
			}
			return (void*)startAd;
        }
		return NULL;

}




void* FindAllocationAddress(uint32 size)
{
	uint32 required_pages = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	uint32 selectedVirtualAdd = 0;

	selectedVirtualAdd = (uint32) BESTFIT(required_pages);

	return (void*) selectedVirtualAdd;
}

void heapAllocation(uint32 startVirtualAdd, uint32 size)
{

	//fill the allocated pages in the array
	startVirtualAdd = ROUNDDOWN(startVirtualAdd, PAGE_SIZE);
	for (uint32 virtualAddress = startVirtualAdd; virtualAddress < (startVirtualAdd + size); virtualAddress += PAGE_SIZE)
	{
		pageindex[(virtualAddress - USER_HEAP_START) / PAGE_SIZE] = 1;
	}

	//save the data of the allocated block
	unsigned int index = (startVirtualAdd - USER_HEAP_START)/ PAGE_SIZE;
	allocInfo[index].startAddress = startVirtualAdd;
	allocInfo[index].size = size;

}

void DeAllocation(uint32 startVirtualAdd, uint32 rounded_size)
{
	int index = (startVirtualAdd - USER_HEAP_START) / PAGE_SIZE;

	//erase the info of the allocated block
	allocInfo[index].size = 0;
	allocInfo[index].startAddress = 0;

	//empty the allocated pages from the array
	for (uint32 virtualAddress = startVirtualAdd; virtualAddress < (startVirtualAdd + rounded_size); virtualAddress += PAGE_SIZE)
	{
		pageindex[(virtualAddress - USER_HEAP_START) / PAGE_SIZE] = 0;
	}
}


void* malloc(uint32 size)
{
	//TODO: [PROJECT 2021 - [2] User Heap] malloc() [User Side]
	// Write your code here, remove the panic and write your code

	void* returnedAdd = FindAllocationAddress(size);
	uint32 totalNumOfPages = ROUNDUP(size, PAGE_SIZE)/ PAGE_SIZE;


	if (returnedAdd != NULL)
	{
		heapAllocation((uint32) returnedAdd, ROUNDUP(size, PAGE_SIZE));
		sys_allocateMem((uint32) returnedAdd, ROUNDUP(size, PAGE_SIZE));
		return returnedAdd;
	}
	else
		return (void*) NULL;
}

// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

void free(void* virtual_address)
{
	//TODO: [PROJECT 2021 - [2] User Heap] free() [User Side]
	// Write your code here, remove the panic and write your code
	//you should get the size of the given allocation using its address
	//refer to the project presentation and documentation for details
	virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);
	uint32 index = (uint32) (virtual_address - USER_HEAP_START)/ PAGE_SIZE;

	if (pageindex[index] == 0)
			return;

	uint32 size = allocInfo[index].size;
	DeAllocation((uint32) virtual_address, size);
	sys_freeMem((uint32) virtual_address, size);

}

//==================================================================================//
//================================ OTHER FUNCTIONS =================================//
//==================================================================================//

void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	panic("this function is not required...!!");
	return 0;
}

void* sget(int32 ownerEnvID, char *sharedVarName)
{
	panic("this function is not required...!!");
	return 0;
}

void sfree(void* virtual_address)
{
	panic("this function is not required...!!");
}

void *realloc(void *virtual_address, uint32 new_size)
{
	panic("this function is not required...!!");
	return 0;
}

void expand(uint32 newSize)
{
	panic("this function is not required...!!");
}
void shrink(uint32 newSize)
{
	panic("this function is not required...!!");
}

void freeHeap(void* virtual_address)
{
	panic("this function is not required...!!");
}
