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
  jit_type_t signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_void, NULL, 0, 1);
  jit_function_t F = jit_function_create(context, signature);

  // char* type
  jit_type_t type_str = jit_type_create_pointer(jit_type_sys_char, 1);
  jit_value_t bufptr = jit_value_create(F, type_str);
#define CONST_BYTE(v) (jit_value_create_nint_constant(F, jit_type_ubyte, v))
  // char bufptr[2] = "a\x00";
  jit_value_t bufsize = CONST_BYTE(5);
  jit_value_t alloca_result = jit_insn_alloca(F, bufsize);
  jit_insn_store(F, bufptr, alloca_result);
  jit_insn_store_relative(F, bufptr, 0, CONST_BYTE('a'));
  jit_insn_store_relative(F, bufptr, 1, CONST_BYTE('b'));
  jit_insn_store_relative(F, bufptr, 2, CONST_BYTE('c'));
  jit_insn_store_relative(F, bufptr, 3, CONST_BYTE('\n'));
  jit_insn_store_relative(F, bufptr, 4, CONST_BYTE('\x00'));

  // Prepare calling puts: create its signature
  jit_type_t puts_params[] = {type_str};
  jit_type_t puts_signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, puts_params, 1, 1);

  // puts(bufptr);
  jit_value_t puts_args[] = {bufptr};
  jit_insn_call_native(
      F, "puts", puts, puts_signature, puts_args, 1, JIT_CALL_NOTHROW);

  jit_dump_function(stdout, F, "F [uncompiled]");
  jit_function_compile(F);
  jit_dump_function(stdout, F, "F [compiled]");

  jit_function_apply(F, NULL, NULL);

  jit_context_destroy(context);
  return 0;
}


