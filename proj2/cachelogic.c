#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w);

/* return random int from 0..x-1 */
int randomint(int x);

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
    /* Buffer to print lfu information -- increase size as needed. */
    static char buffer[9];
    sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

    return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
    /* Buffer to print lru information -- increase size as needed. */
    static char buffer[9];
    sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

    return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
    cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
    cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/

//Helper function that sets a bit to 0 (aka dirty or virgin)
void SetVirg(unsigned int index, unsigned int valueLRU, unsigned int tag) {
    cache[index].block[valueLRU].lru.value = 0;
    cache[index].block[valueLRU].valid = 1;
    cache[index].block[valueLRU].dirty = VIRGIN;
    cache[index].block[valueLRU].tag = tag;
}

void accessMemory(address addr, word* data, WriteEnable we)
{
    /* Declare variables here */
    unsigned int tag;
    unsigned int tagSize;
    unsigned int index;
    unsigned int indexSize;
    unsigned int offset;
    unsigned int offsetSize;
    unsigned int Hit = 0;
    unsigned int counterLRU = 0;
    unsigned int valueLRU = 0;

    //creates a transfer unit named byteSize and set to default value 0
    TransferUnit byteSize = 0;

    address outdatedAdr = 0;

    /* handle the case of no cache at all - leave this in */
    if (assoc == 0) {
        accessDRAM(addr, (byte*)data, WORD_SIZE, we);
        return;
    }

    /*
    You need to read/write between memory (via the accessDRAM() function) and
    the cache (via the cache[] global structure defined in tips.h)

    Remember to read tips.h for all the global variables that tell you the
    cache parameters

    The same code should handle random, LFU, and LRU policies. Test the policy
    variable (see tips.h) to decide which policy to execute. The LRU policy
    should be written such that no two blocks (when their valid bit is VALID)
    will ever be a candidate for replacement. In the case of a tie in the
    least number of accesses for LFU, you use the LRU information to determine
    which block to replace.

    Your cache should be able to support write-through mode (any writes to
    the cache get immediately copied to main memory also) and write-back mode
    (and writes to the cache only gets copied to main memory when the block
    is kicked out of the cache.

    Also, cache should do allocate-on-write. This means, a write operation
    will bring in an entire block if the block is not already in the cache.

    To properly work with the GUI, the code needs to tell the GUI code
    when to redraw and when to flash things. Descriptions of the animation
    functions can be found in tips.h
    */

    /* Start adding code here */

    //find the size of the offset and index
    offsetSize = uint_log2(block_size);
    indexSize = uint_log2(set_count);

    //tag is the difference of total size(32) - index - offset
    tagSize = 32 - indexSize - offsetSize;

    //create masks of offset, index, and tag
    offset = (addr) & ((1 << offsetSize) - 1);
    index = (addr >> offsetSize) & ((1 << indexSize) - 1);
    tag = addr >> (offsetSize + indexSize);

    //zero
    if (block_size == 1) {
        byteSize = 0;
    }

    //half
    else if (block_size == 2) {
        byteSize = 1;
    }

    //full
    else if (block_size == 4) {
        byteSize = 2;
    }

    //double
    else if (block_size == 8) {
        byteSize = 3;
    }

    //triple
    else if (block_size == 16) {
        byteSize = 4;
    }

    //quad
    else if (block_size == 32) {
        byteSize = 5;
    }

    //Block size is outside scope of project
    else {
        printf("ERROR: Invalid Block Size!");
    }

    if (policy == LRU) {
        //sets the value for lru in relaiton to the associativity
        for (int i = 0; i < assoc; i++) {
            cache[index].block[i].lru.value++;
        }
    }

    switch (we) {
        case READ:
            Hit = 0;
            for (int i = 0; i < assoc; i++) {
                if (tag == cache[index].block[i].tag) {
                    if (cache[index].block[i].valid == 1) {
                        cache[index].block[i].lru.value = 0;
                        cache[index].block[i].valid = 1;
                        Hit = 1;
                        memcpy(data, (cache[index].block[i].data + offset), 4);
                    }
                }
            }

            //If it is a miss
            if (Hit == 0) {
                //Replacement Policy
                switch (policy) {
                    case LRU:
                        for (int i = 0; i < assoc; i++) {
                            if (valueLRU < cache[index].block[i].lru.value) {
                                counterLRU = i;
                                valueLRU = cache[index].block[i].lru.value;
                            }
                        }
                        break;

                    //Not truly random, but random enough for my liking :P
                    case RANDOM:
                        counterLRU = assoc;
                        break;

                    //LFU policy is not implemented because it is out of the scope of this project
                    default:
                        printf("Replacement policy is not supported");
                        break;
                }

                if (cache[index].block[counterLRU].dirty == DIRTY) {
                    outdatedAdr = cache[index].block[counterLRU].tag << ((indexSize + offsetSize) + (index << offsetSize));
                    accessDRAM(outdatedAdr, (cache[index].block[counterLRU].data), byteSize, WRITE);
                }

                accessDRAM(addr, (cache[index].block[counterLRU].data), byteSize, READ);
                SetVirg(index, counterLRU, tag);
                memcpy(data, (cache[index].block[counterLRU].data + offset), 4);
                break;
            }

        case WRITE:
            Hit = 0;

            //write back
            if (memory_sync_policy == WRITE_BACK) {
                for (int i = 0; i < assoc; i++) {
                    if (tag == cache[index].block[i].tag && cache[index].block[i].valid == 1) {
                        memcpy((cache[index].block[i].data + offset), data, 4);
                        cache[index].block[i].dirty = DIRTY;
                        cache[index].block[i].lru.value = 0;
                        cache[index].block[i].valid = 1;
                        Hit = 1;
                    }
                }

                //miss case
                if (Hit == 0) {
                    //Replacement Policy
                    switch (policy) {
                        case LRU:
                            for (int i = 0; i < assoc; i++) {
                                if (valueLRU < cache[index].block[i].lru.value) {
                                    counterLRU = i;
                                    valueLRU = cache[index].block[i].lru.value;
                                }
                            }

                        //Not truly random
                        case RANDOM:
                            counterLRU = assoc;
                            break;

                        //LFU is out of the scope of this project
                        default:
                            printf("Replacement policy is not supported");
                            break;
                    }

                    if (cache[index].block[counterLRU].dirty == DIRTY) {
                        outdatedAdr = cache[index].block[counterLRU].tag << (indexSize + offsetSize) + (index << offsetSize);
                        accessDRAM(outdatedAdr, (cache[index].block[counterLRU].data), byteSize, WRITE);
                    }

                    SetVirg(index, counterLRU, tag);
                    accessDRAM(addr, (cache[index].block[counterLRU].data), byteSize, READ);
                    memcpy((cache[index].block[counterLRU].data + offset), data, 4);
                }
            }

            //write through
            else {
                for (int i = 0; i < assoc; i++) {
                    if (tag == cache[index].block[i].tag && cache[index].block[i].valid == 1) {
                        memcpy((cache[index].block[i].data + offset), data, 4);
                        cache[index].block[i].dirty = VIRGIN;
                        cache[index].block[i].lru.value = 0;
                        cache[index].block[i].valid = 1;
                        Hit = 1;
                        accessDRAM(addr, (cache[index].block[counterLRU].data), byteSize, WRITE);
                    }

                    //miss case
                    if (Hit == 0) {
                        switch (policy) {
                            case LRU:
                                for (int i = 0; i < assoc; i++) {
                                    if (valueLRU < cache[index].block[i].lru.value) {
                                        counterLRU = i;
                                        valueLRU = cache[index].block[i].lru.value;
                                    }
                                }

                                //Not truly random
                            case RANDOM:
                                counterLRU = assoc;
                                break;

                            default:
                                printf("LFU is not implemented");
                                break;
                        }

                        accessDRAM(addr, (cache[index].block[counterLRU].data), byteSize, READ);
                        SetVirg(index, counterLRU, tag);
                        memcpy((cache[index].block[counterLRU].data + offset), data, 4);
                    }
                }
                break;
            }

        default:
            printf("WriteEnable is not supported");
            break;
    }
}
