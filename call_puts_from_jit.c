//------------------------------------------------------------------------------
// call_puts_from_jit libjit sample.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <jit/jit.h>


int main(int argc, char** argv) {
  jit_init();
  jit_context_t context = jit_context_create();

  // void foo()
  jit_function_t F = jit_function_create(context,
      jit_type_create_signature(jit_abi_cdecl, jit_type_void, NULL, 0, 1));

  // Approach #1: allocate the string buffer on stack inside the jit-ed
  // function and store the desired characters into it.

  // char* bufptr
#define CONST_BYTE(v) (jit_value_create_nint_constant(F, jit_type_ubyte, v))
  jit_type_t type_cstring = jit_type_create_pointer(jit_type_sys_char, 1);
  jit_value_t bufptr = jit_value_create(F, type_cstring);

  // Make bufptr point to a 4-byte buffer allocated on the stack
  jit_insn_store(F, bufptr, jit_insn_alloca(F, CONST_BYTE(4)));

  // Store "abc" (with explicit terminating zero) into bufptr
  jit_insn_store_relative(F, bufptr, 0, CONST_BYTE('a'));
  jit_insn_store_relative(F, bufptr, 1, CONST_BYTE('b'));
  jit_insn_store_relative(F, bufptr, 2, CONST_BYTE('c'));
  jit_insn_store_relative(F, bufptr, 3, CONST_BYTE('\x00'));

  // Create the signature of puts: int (*)(char*)
  jit_type_t puts_signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, &type_cstring, 1, 1);

  // puts(bufptr);
  jit_insn_call_native(
      F, "puts", puts, puts_signature, &bufptr, 1, JIT_CALL_NOTHROW);

  // Approach #2: use the address of a string literal in the host code directly,
  // storing it into a constant. Note that this has to explicitly specify that
  // host pointers are 64-bit.

  jit_value_t hostmemptr = jit_value_create_long_constant(
      F, type_cstring, (long)"foobar");

  jit_insn_call_native(
      F, "puts", puts, puts_signature, &hostmemptr, 1, JIT_CALL_NOTHROW);

  jit_dump_function(stdout, F, "F [uncompiled]");
  jit_function_compile(F);
  jit_dump_function(stdout, F, "F [compiled]");

  // Run
  jit_function_apply(F, NULL, NULL);

  jit_context_destroy(context);
  return 0;
}


