/*
 * chunk.c 
 *
 * The chunk is our way of storing quantum program instructions in memory.
 * It's a dynamic array that grows as we add more instructions.
 *
 */
#include "quantum_interpreter.h"


#define CHUNK_INITIAL_CAPACITY 8


/* Initialize a new chunk with default capacity. */
void init_chunk(Chunk* chunk) {

    chunk->count = 0;
    chunk->capacity = CHUNK_INITIAL_CAPACITY;
    chunk->code = malloc(sizeof(Instruction) * chunk->capacity);
    
    if (chunk->code == NULL) {
        fprintf(stderr, "Failed to allocate memory for chunk\n");
        exit(EXIT_FAILURE);
    }
}

/* Double the chunk's capacity when it's full.*/
static void grow_capacity(Chunk* chunk) {
    int new_capacity = chunk->capacity * 2;
    chunk->code = realloc(chunk->code, sizeof(Instruction) * new_capacity);
    
    if (chunk->code == NULL) {
        fprintf(stderr, "Failed to reallocate memory for chunk\n");
        exit(EXIT_FAILURE);
    }
    
    chunk->capacity = new_capacity;
}

/*
* Adds a new instruction to the chunk.
* If we're out of space, grows the chunk first.
*/
void write_chunk(Chunk* chunk, Instruction instruction) {
    if (chunk->count >= chunk->capacity) {
        grow_capacity(chunk);
    }
    
    chunk->code[chunk->count] = instruction;
    chunk->count++;
}

/* Clean up the chunk's allocated memory. */
void free_chunk(Chunk* chunk) {
    free(chunk->code);
    chunk->code = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
}