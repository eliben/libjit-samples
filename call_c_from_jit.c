//------------------------------------------------------------------------------
// call_c_from_jit libjit sample.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <jit/jit.h>


int native_mult(int a, int b) {
  return a * b;
}

// Builds this function, and returns an uncompiled jit_function_t:
//
// int jit_adder(int x, y) {
//    return x + y;
// }
jit_function_t build_jit_adder(jit_context_t context) {
  jit_context_build_start(context);

  // Create function signature and object. int (*)(int, int)
  jit_type_t params[2] = {jit_type_int, jit_type_int};
  jit_type_t signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, params, 2, 1);
  jit_function_t F = jit_function_create(context, signature);

  // x, y are the parameters; sum is a temporary
  jit_value_t x = jit_value_get_param(F, 0);
  jit_value_t y = jit_value_get_param(F, 1);
  jit_value_t sum = jit_value_create(F, jit_type_int);

  // sum = x + y
  jit_value_t temp_sum = jit_insn_add(F, x, y);
  jit_insn_store(F, sum, temp_sum);

  // return sum
  jit_insn_return(F, sum);
  jit_context_build_end(context);
  return F;
}

// Builds this function:
//
// void foo(int x, int y, int* result) {
//   int t = jit_adder(x, y);
//   *result = native_mult(t, y);
// }
//
// Returns an uncompiled jit_function_t
// Note that jit_adder is a jit_function_t that's passed into this builder.
jit_function_t build_foo(jit_context_t context, jit_function_t jit_adder) {
  jit_context_build_start(context);

  // Create function signature and object. void (*)(int, int, void*)
  // libjit treats all native pointers as void*.
  jit_type_t params[] = {jit_type_int, jit_type_int, jit_type_void_ptr};
  jit_type_t signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_void, params, 3, 1);
  jit_function_t F = jit_function_create(context, signature);

  // x, y, result are the parameters; t is a temporary
  jit_value_t x = jit_value_get_param(F, 0);
  jit_value_t y = jit_value_get_param(F, 1);
  jit_value_t result = jit_value_get_param(F, 2);
  jit_value_t t = jit_value_create(F, jit_type_int);

  // t = jit_adder(x, y)
  jit_value_t adder_args[] = {x, y};
  jit_value_t call_temp = jit_insn_call(
      F, "jit_adder", jit_adder, 0, adder_args, 2, 0);

  jit_insn_store(F, t, call_temp);

  // Prepare calling native_mult: create its signature
  jit_type_t mult_params[] = {jit_type_int, jit_type_int};
  jit_type_t mult_signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, params, 2, 1);

  // x = native_mult(t, y)
  jit_value_t mult_args[] = {t, y};
  jit_value_t res = jit_insn_call_native(
      F, "native_mult", native_mult, mult_signature,
      mult_args, sizeof(mult_args) / sizeof(jit_value_t), JIT_CALL_NOTHROW);
  jit_insn_store(F, x, res);

  // *result = x
  // Note that this creates a store of a value libjit considers to be a
  // jit_type_int, so the pointer must point to at least that size.
  jit_insn_store_relative(F, result, 0, x);

  jit_context_build_end(context);
  return F;
}


// Run foo with arguments and return its result
int run_foo(jit_function_t jit_foo, int x, int y) {
  int result, *presult = &result;
  void* args[] = {&x, &y, &presult};

  jit_function_apply(jit_foo, args, NULL);
  return result;
}


int main(int argc, char** argv) {
  jit_init();
  jit_context_t context = jit_context_create();
  jit_function_t jit_adder = build_jit_adder(context);
  jit_function_t foo = build_foo(context, jit_adder);

  // This will dump the uncompiled function, showing libjit opcodes
  jit_dump_function(stdout, jit_adder, "jit_adder [uncompiled]");
  jit_dump_function(stdout, foo, "foo [uncompiled]");

  // Compile (JIT) the functions to machine code
  jit_context_build_start(context);
  jit_function_compile(jit_adder);
  jit_function_compile(foo);
  jit_context_build_end(context);

  // This will dump the disassembly of the machine code for the function
  jit_dump_function(stdout, jit_adder, "jit_adder [compiled]");
  jit_dump_function(stdout, foo, "foo [compiled]");

  // Run the function on argv input
  if (argc > 2) {
    int u = atoi(argv[1]);
    int v = atoi(argv[2]);
    printf("foo(%d, %d) --> %d\n", u, v, run_foo(foo, u, v));
  }

  jit_context_destroy(context);
  return 0;
}


