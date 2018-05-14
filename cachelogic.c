#include "tips.h"

/* Global variable for tracking the least recently used block */
unsigned int lru_count[MAX_SETS];

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
char *lfu_to_string(int assoc_index, int block_index) {
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
char *lru_to_string(int assoc_index, int block_index) {
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
void init_lfu(int assoc_index, int block_index) {
    cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index) {
    cache[assoc_index].block[block_index].lru.value = 0;
    lru_count[assoc_index] = 0;
}

/*
  This function find the least recently used block in the enumerated set

    set_index - the index of the set in which to find the least recently used block

  returns the index of the least recently used block in the enumerated set
*/
int find_lru_block(int set_index) {
    int min_lru = 0;
    for(int i = 0; i < assoc; ++i) {
        if(cache[set_index].block[i].valid == INVALID)
            return i;
        if(cache[set_index].block[i].lru.value < cache[set_index].block[min_lru].lru.value)
            min_lru = i;
    }
    return min_lru;
}

/*
  This function increments the lru value of the accessed block using the lru counter
  for the enumerated set. If overflow occurs, the lru values are shifted maintaining
  the integrity of the lru counter.

    set_index - the index of the set in which to find the least recently used block
    block_number - the index of the block to be modified

*/
void increment_lru(int set_index, int block_index) {
    int min_lru_val;
    cache[set_index].block[block_index].lru.value = ++lru_count[set_index];
    if(lru_count[set_index]-1 > lru_count[set_index]) {
        min_lru_val = cache[set_index].block[find_lru_block(set_index)].lru.value;
        for(int i = 0; i < assoc; ++i) {
            cache[set_index].block[i].lru.value += (0xFFFFFFFF - min_lru_val);
        }
    }
}

/*
  This function find the least frequently used block in the enumerated set

    set_index - the index of the set in which to find the least frequently used block

  returns the index of the least frequently used block in the enumerated set
*/
int find_lfu_block(int set_index) {
    int min_lfu = 0;
     for(int i = 0; i < assoc; ++i) {
        if(cache[set_index].block[i].valid == INVALID)
            return i;
        if(cache[set_index].block[i].accessCount < cache[set_index].block[min_lfu].accessCount)
            min_lfu = i;
    }
    return min_lfu;
}

/*
  This function increments the lru value of the accessed block using the lru counter
  for the enumerated set. If overflow occurs, the lru values are shifted maintaining
  the integrity of the lru counter.

    set_index - the index of the set in which to find the least recently used block
    block_number - the index of the block to be modified

*/
void increment_lfu(int set_index, int block_index) {
    cache[set_index].block[block_index].accessCount++;
    // Handle overflow
    if(cache[set_index].block[block_index].accessCount-1 > cache[set_index].block[block_index].accessCount) {
        cache[set_index].block[block_index].accessCount--;
        for(int i = 0; i < assoc; ++i) {
            cache[set_index].block[block_index].accessCount /= 2;
        }
    }
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
void accessMemory(address addr, word *data, WriteEnable we) {

    /* Declare variables here */
    unsigned int i_bit_count, t_bit_count, o_bit_count;
    unsigned int set, tag, offset;
    unsigned int hit = 0, accessed_block, write_back_addr;
    TransferUnit transfer_unit = 0;

    /* handle the case of no cache at all - leave this in */
    if (assoc == 0) {
        accessDRAM(addr, (byte *) data, WORD_SIZE, we);
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
    transfer_unit = uint_log2(block_size);

    i_bit_count = uint_log2(set_count);
    o_bit_count = uint_log2(block_size);
    t_bit_count = 32 - (i_bit_count + o_bit_count);
    
    offset = addr & ((1 << o_bit_count) - 1);
    set = (addr >> o_bit_count) & ((1 << i_bit_count) - 1);
    tag = addr >> (o_bit_count + i_bit_count);

    char msg[1024];
    sprintf(msg, "Index (%u bits): %u\n", i_bit_count, set);
    append_log(msg);
    sprintf(msg, "Offset (%u bits): %u\n", o_bit_count, offset);
    append_log(msg);
    sprintf(msg, "Tag (%u bits): %u\n", t_bit_count, tag);
    append_log(msg);

    for (int i = 0; i < assoc; ++i) {
        if(cache[set].block[i].tag == tag && cache[set].block[i].valid) {
            accessed_block = i;
            hit = 1;
            highlight_offset(set, accessed_block, offset, HIT);
            break;
        }
    }

    if(!hit) {
        if(policy == LRU)
            accessed_block = find_lru_block(set);
        else if (policy == LFU)
            accessed_block = find_lfu_block(set);
        else
            accessed_block = randomint(assoc);

        highlight_block(set, accessed_block);
        highlight_offset(set, accessed_block, offset, MISS);

        if(memory_sync_policy == WRITE_BACK && cache[set].block[accessed_block].dirty) {
            write_back_addr = (cache[set].block[accessed_block].tag << (i_bit_count + o_bit_count)) + (set << o_bit_count);
            accessDRAM(write_back_addr, cache[set].block[accessed_block].data, transfer_unit, WRITE);
        }

        accessDRAM(addr, cache[set].block[accessed_block].data, transfer_unit, READ);

        if(policy == LFU)
            cache[set].block[accessed_block].accessCount = 0;

        cache[set].block[accessed_block].tag = tag;
        cache[set].block[accessed_block].valid = VALID;

        if(memory_sync_policy == WRITE_BACK)
            cache[set].block[accessed_block].dirty = VIRGIN;
    }

    if (we == READ) {
        memcpy(data, cache[set].block[accessed_block].data + offset, 4);
    } else if (we == WRITE) {
        memcpy(cache[set].block[accessed_block].data + offset, data, 4);
        if(memory_sync_policy == WRITE_THROUGH)
            accessDRAM(write_back_addr, cache[set].block[accessed_block].data, transfer_unit, WRITE);
        else
            cache[set].block[accessed_block].dirty = DIRTY;
    }

    if(policy == LRU)
        increment_lru(set, accessed_block);
    else if(policy == LFU)
        increment_lfu(set, accessed_block);

}
