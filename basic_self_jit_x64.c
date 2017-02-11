//------------------------------------------------------------------------------
// Simple JITing of code into memory.
//
// This example doesn't actually use libjit - it's much more basic.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


// Allocates RWX memory of given size and returns a pointer to it. On failure,
// prints out the error and returns NULL.
void* alloc_executable_memory(size_t size) {
  void* ptr = mmap(0, size,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void*)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

// Allocates RW memory of given size and returns a pointer to it. On failure,
// prints out the error and returns NULL. mmap is used to allocate, so
// deallocation has to be done with munmap, and the memory is allocated
// on a page boundary so it's suitable for calling mprotect.
void* alloc_writable_memory(size_t size) {
  void* ptr = mmap(0, size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void*)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

// Sets a RX permission on the given memory, which must be page-aligned. Returns
// 0 on success. On failure, prints out the error and returns -1.
int make_memory_executable(void* m, size_t size) {
  if (mprotect(m, size, PROT_READ | PROT_EXEC) == -1) {
    perror("mprotect");
    return -1;
  }
  return 0;
}

// Emits code into the given memory, assuming enough was allocated. Returns the
// number of bytes emitted.
size_t emit_code_into_memory(unsigned char* m) {
  unsigned char code[] = {
    0x48, 0x89, 0xf8,                   // mov %rdi, %rax
    0x48, 0x83, 0xc0, 0x04,             // add $4, %rax
    0xc3                                // ret
  };
  memcpy(m, code, sizeof(code));
  return sizeof(code);
}

const size_t SIZE = 1024;
typedef long (*JittedFunc)(long);

// Allocates RWX memory directly.
void run_from_rwx() {
  void* m = alloc_executable_memory(SIZE);
  size_t n = emit_code_into_memory(m);

  const char* filename = "/tmp/jitout.bin";
  FILE* outfile = fopen(filename, "wb");
  if (outfile) {
    if (fwrite(m, 1, n, outfile) == n) {
      printf("Successfully emitted binary to %s\n", filename);
    }
    fclose(outfile);
  }

  JittedFunc func = m;
  int result = func(2);
  printf("result = %d\n", result);
}

// Allocates RW memory, emits the code into it and sets it to RX before
// executing.
void emit_to_rw_run_from_rx() {
  void* m = alloc_writable_memory(SIZE);
  emit_code_into_memory(m);
  make_memory_executable(m, SIZE);

  JittedFunc func = m;
  int result = func(2);
  printf("result = %d\n", result);
}

int main(int argc, char** argv) {
  run_from_rwx();
  emit_to_rw_run_from_rx();

  return 0;
}
