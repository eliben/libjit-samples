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
#include <sys/mman.h>


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

void emit_code_into_memory(unsigned char* m) {
  // mov %rdi, %rax
  *m++ = 0x48; *m++ = 0x89; *m++ = 0xf8;
  // add $4, %rax
  *m++ = 0x48; *m++ = 0x83; *m++ = 0xc0; *m++ = 0x28;
  // ret
  *m++ = 0xc3;
}

typedef long (*JittedFunc)(long);


int main(int argc, char** argv) {
  void* m = alloc_executable_memory(1024);
  emit_code_into_memory(m);

  // Finally, run the "JITed" code as a long (*)(long), passing it an argument
  JittedFunc func = m;

  int result = func(2);
  printf("result = %d\n", result);

  return 0;
}

