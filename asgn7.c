/*
Jasmine Jans (submitter)
Kayline Hunter
Jessica Peretti

Program that simulates using virtual memory

11/30/2016
OS - Sec. 2

compile: gcc asgn7.c
run: ./a.out BACKING_STORE.bin

*/

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255
#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255
#define MEMORY_SIZE PAGES * PAGE_SIZE


//structs for a tlb entry
struct tlbentry {
    unsigned char logical;
    unsigned char physical;
};
typedef struct tlbentry tlb;

int* getOffsetAndPageNumber(int);
int* readAddressFileToArray(FILE*, int);
int countLines(FILE*);
int getPhysicalPage(int);

/*
runs the loop that gets all the addresses from the address file
and get the necessary data from those addresses
*/
int main(int argc, const char *argv[])
{
	 tlb tlb[TLB_SIZE];
	 int pagetable[PAGES];
	 signed char RAM[MEMORY_SIZE];
	 signed char *backing;
	 
	 //gets secondary storage name from command line
    const char *backing_filename = argv[1]; 
    int backing_fd = open(backing_filename, O_RDONLY);
    
    //secondary storage file can now be viewed as an array
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
    
    //creates a file from the addresses file
    FILE *addressFile = fopen("addresses.txt", "r");
    int numLines = countLines(addressFile);
    int *addressArray = readAddressFileToArray(addressFile, numLines);
    
    int physical_page = 0;
    
    //statistic variables
    int tlbIndex = 0; 
    float tlbHit = 0;
    float pageFaultCount = 0; 
    
	 for(int i = 0; i<numLines; i++)
	 {
    	int logicalAddress = addressArray[i];  //The first line of addresses.txt
    	
    	//sends in the logical address to be broken down into offset and pagenumber
		int *OPN = getOffsetAndPageNumber(logicalAddress);
		int offset = OPN[1];
		int pageNumber = OPN[0];
		
		//handles checking the tlb
		int inTLB = 0;
		int indexInTLB = 0;
	   for( int k = 0; k < 15; k++)
	   {
	   	if(tlb[k].logical == pageNumber)
	   	{
	   		inTLB = 1;
	   		indexInTLB = k;
	   		tlbHit++;
	   	}
	   	else
	   	{
	   	 	tlb[k].physical = physical_page;
	   	 	tlb[k].logical = pageNumber; 
	   	 	tlbIndex++; 
	   	}   
	   }  
	   
	   //in tlb
		if(inTLB)
		{
			int frameNumber = tlb[indexInTLB].logical;
			signed char value = RAM[pageNumber * PAGE_SIZE + offset];
			printf("Virtual address : %d, Physical address : %d, Value : %d \n", 
					logicalAddress, ((physical_page << OFFSET_BITS) | offset), value);
		} 
		//not in page table
		else if(pagetable[pageNumber] == 0)
      {
      	 //gets the value form secondary storage and places it in RAM
    		 memcpy(RAM + physical_page * PAGE_SIZE, 
           			backing + pageNumber * PAGE_SIZE, PAGE_SIZE);	

  			 //Shift the physical page left 8 bits and or with the offset
    		 int  physical_address = (physical_page << OFFSET_BITS) | offset;
    		 
  			 signed char value = RAM[physical_page * PAGE_SIZE + offset]; 
  			 
  			 pagetable[pageNumber] = physical_page;
  			 
  			 printf("Virtual address : %d, Physical address : %d, Value : %d \n", 
  			 		logicalAddress, ((physical_page << OFFSET_BITS) | offset), value);
  			 		
  			 pageFaultCount++;
  			 physical_page++;
  			         
		}
		//in page table
		else 
		{
			int frameNumber = pagetable[pageNumber] >> OFFSET_BITS;
			int offsetBits = pagetable[pageNumber] | offset;
			
			signed char value = RAM[pageNumber * PAGE_SIZE + offset];
			printf("Virtual address : %d, Physical address : %d, Value : %d \n", 
					logicalAddress, ((physical_page << OFFSET_BITS) | offset), value);
		}
    }
	 
	 //calculate statistics
	 float pfr = pageFaultCount/numLines;
	 float tlbhr = tlbHit/numLines;
	 printf("Number of Translated Addreses = %d \n", numLines);
	 printf("Page Faults = %.0f \n", pageFaultCount);
	 printf("Page Fault Rate = %.3f \n", pfr);
	 printf("TLB Hits = %.0f \n", tlbHit);
	 printf("TLB Hit Rate = %.3f \n", tlbhr);
	 
    return 0;
}

/* 
returns the offset and page number from a logical_address 
in an array [pageNumber, offset].
*/
int* getOffsetAndPageNumber(int logicalAddress)
{
	 //extract low order 8 bits from the logical_address. This is the offset
    int offset = logicalAddress & OFFSET_MASK;

    //extract bits 8 through 15. This is the page number 
    int pageNumber = (logicalAddress >> OFFSET_BITS) & PAGE_MASK;
    
    int *offsetAndPageNumber = malloc(2*sizeof(int));
    offsetAndPageNumber[0] = pageNumber;
    offsetAndPageNumber[1] = offset;
    
    return offsetAndPageNumber;
}


/*
This file take, an input file that (FILE*input)
 is a pass by reference parameter. It is then read, and the 
integers are placed into the array. 
*/
int* readAddressFileToArray(FILE* input, int inNumLines)
{
	int numLines = inNumLines;
	
	int *array = malloc(numLines*sizeof(int));
	int i = 0;
	
	int k;
	int c;
	
	while((k = fscanf(input, "%d", &c)) != EOF)
	{
		array[i] = c;
		i++;
	}
	
	fclose(input);
	return array;
}

/*
Counts the number of lines in a file
*/
int countLines(FILE* file)
{
	int numLines = 0;
	char ch;
	while(!feof(file))
	{
		ch = fgetc(file);
		if(ch == '\n')
			numLines++;
	}
	
	fseek(file, 0, SEEK_SET);
	return numLines;
}


