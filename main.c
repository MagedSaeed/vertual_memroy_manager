//
// Created by group6 on 23/12/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <math.h>


// all these caonstants are extracted from the problem statement details

#define FRAME_SIZE 256          //FRAME SIZE is 256 bytes 2^8
#define FRAMES 256              //Number of frames
#define PAGE_SIZE 256           // PAGE SIZE is 256 bytes 2^8
#define TLB_SIZE 16             //Number of TLB Entries
#define PAGE_MASKER 0xFFFF      // This masker will be explained later.
#define OFFSET_MASKER 0xFF      // This masker will be explained later.
#define ADDRESS_SIZE 10
#define CHUNK 256

// input files
FILE *address_file;
FILE *backing_store;

// struct to store TLB and page table
struct page_frame
{
   int page_number;
   int frame_number;
};

// initial global variables
int Physical_Memory [FRAMES][FRAME_SIZE];
struct page_frame TLB[TLB_SIZE];
struct page_frame PAGE_TABLE[FRAMES]; // the size of the page table is the number of the available frames in the system.
int translated_addresses = 0;
char address [ADDRESS_SIZE]; // to store the address coming from the input file
int TLBHits = 0;
int page_faults = 0;
signed char buffer[CHUNK];
int firstAvailableFrame = 0;
int firstAvailablePageTableIndex = 0;
signed char value;
int TLB_Full_Entries = 0;

// functions placeholders
void get_page(int logical_address);
int read_from_store(int pageNumber);
void insert_into_TLB(int pageNumber, int frameNumber);

// get a page for a given virtual address
void get_page(int logical_address){

    int pageNumber = ((logical_address & PAGE_MASKER)>>8); // this expression will mask the value of the logical address then shifting it 8 bits to the right to retireve the page number. In this case m = 16 and n = 8.
    int offset = (logical_address & OFFSET_MASKER); // This expression retrives the last 8 bits of the address as the page offset.

    // first try to get page from TLB
    int frameNumber = -1; // initialized to -1 to tell if it's valid in the conditionals below

    int i;  // look through TLB for a match
    for(i = 0; i < TLB_SIZE; i++){
        if(TLB[i].page_number == pageNumber){   // if the TLB key is equal to the page number.
            frameNumber = TLB[i].frame_number;  // then the frame number value is extracted.
            TLBHits++;                          // and the TLBHit counter is incremented.
            
        }
    }

     // if the frameNumber was not int the TLB:
    if(frameNumber == -1){
        int i;                                                       // walk the contents of the page table
        for(i = 0; i < firstAvailablePageTableIndex; i++){           // The size of the page table is the same number as the frames number.
            if(PAGE_TABLE[i].page_number == pageNumber){             // if the page is found in those contents
                frameNumber = PAGE_TABLE[i].frame_number;            // extract the frameNumber from its twin array
            }
        }
        
        if(frameNumber == -1){                                      // if the page is not found in those contents
            frameNumber = read_from_store(pageNumber);              // page fault, call to read_from_store to get the frame into physical memory and the page table and set the frameNumber to the current firstAvailableFrame index
            page_faults++;                                          // increment the number of page faults
        }
    }

    insert_into_TLB(pageNumber, frameNumber);                       // call to function to insert the page number and frame number into the TLB
    value = Physical_Memory[frameNumber][offset];                   // frame number and offset used to get the signed value stored at that address

    // for testing before outputing values to the file:

    //printf("frame number: %d\n", frameNumber);
    //printf("offset: %d\n", offset); 
    // output the virtual address, physical address and value of the signed char to the console
    //printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNumber << 8) | offset, value);

    // output final values to a file named results
    FILE *res = fopen("results.txt","a");
    fprintf(res,"Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNumber << 8) | offset, value);
    fclose(res);
}

int read_from_store(int pageNumber){

    // first seek to byte CHUNK in the backing store
    // SEEK_SET in fseek() seeks from the beginning of the file
    if (fseek(backing_store, pageNumber * CHUNK, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in backing store\n");
    }
    
    // now read CHUNK bytes from the backing store to the buffer
    if (fread(buffer, sizeof(signed char), CHUNK, backing_store) == 0) {
        fprintf(stderr, "Error reading from backing store\n");        
    }
    
    // load the bits into the first available frame in the physical memory 2D array
    int i;
    for(i = 0; i < CHUNK; i++){
        Physical_Memory[firstAvailableFrame][i] = buffer[i];
    }
    
    // and then load the frame number into the page table in the first available frame
    PAGE_TABLE[firstAvailablePageTableIndex].page_number = pageNumber;
    PAGE_TABLE[firstAvailablePageTableIndex].frame_number = firstAvailableFrame;
    
    // increment the counters that track the next available frames
    firstAvailableFrame++;
    firstAvailablePageTableIndex++;
    return PAGE_TABLE[firstAvailablePageTableIndex-1].frame_number;
}

void insert_into_TLB(int pageNumber, int frameNumber){

    /*
    The algorithm to insert a page into the TLB using FIFO replacement. It goes as follows:
    - First check if the page is already in the TLB table. If it is in the TLB Table, then:
        move it to the end of the TLB. "following the FIFO policies."

    - If it is not in the TLB:
        If there is a free cell then add it to that free cell at the end of the queue
        else, move every thing one step to the left to free up the last cell and put the page into this last cell.
    */
    
   int i;  
   // first check if it is found in the TLB. If so, then move every thing one step to the left in insert it in the last cell.
   // remever that i iterates upto one last cell in the TLB for a maximum TLB_FULL_Entries.
    for(i = 0; i < TLB_Full_Entries; i++){
        if(TLB[i].page_number == pageNumber){               // if it is in the TLB, move it to the last cell.
            for(i = i; i < TLB_Full_Entries - 1; i++)      // iterate from the current cell to one less than the number of entries
                TLB[i] = TLB[i + 1];                        // move everything over in the arrays
            break;
        }
    }
    
    // if the previous loop did not break, then the page is not in the TLB. We need to insert it.
    if(i == TLB_Full_Entries){
        int j;
        for (j=0; j<i; j++)
            TLB[j] = TLB[j+1];

    }
    TLB[i].page_number = pageNumber;        // and insert the page and frame on the end
    TLB[i].frame_number = frameNumber;

    if(TLB_Full_Entries < TLB_SIZE-1){     // if there is still room in the arrays, increment the number of entries
        TLB_Full_Entries++;
    }    
}

int main(int argc, char *argv[]) {
    printf("Hello, This is the ICS431 Project\n");
    printf("results are printed to the \"resutls.txt\" file.\n");
    printf("The command \"diff results.txt correct.txt\" might be usefull to check whether the results of the program and the correct results are identical.\n");

    // perform basic error checking
    if (argc != 2) {
        fprintf(stderr,"Usage: ./a.out [input file]\n");
        return -1;
    }

    // Read Adresses from input file. The file is retrived from the command line arguments
    address_file = fopen(argv[1], "r");

    backing_store = fopen("BACKING_STORE.bin", "rb");

    if (address_file == NULL) {
        fprintf(stderr, "Error opening addresses.txt %s\n",argv[1]);
        return -1;
    }

    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin %s\n","BACKING_STORE.bin");
        return -1;
    }
    int translated_addresses = 0;
    int logical_address;
    // read through the input file and output each logical address
    while (fgets(address, ADDRESS_SIZE, address_file) != NULL) {
        logical_address = atoi(address); // this function converts numeric strings to int datatype.

        // get the physical address and value stored at that address
        get_page(logical_address);
        translated_addresses++;  // increment the number of translated addresses
    }

     // calculate and print out the stats
    printf("Number of translated addresses = %d\n", translated_addresses);
    double pfRate = page_faults / (double)translated_addresses;
    double TLBRate = TLBHits / (double)translated_addresses;
    
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n",pfRate);
    printf("TLB Hits = %d\n", TLBHits);
    printf("TLB Hit Rate = %.3f\n", TLBRate);
    
    // close the input file and backing store
    fclose(address_file);
    fclose(backing_store);

    return 0;
}