#include <stdio.h>
#include <stdlib.h>
#include <jit/jit.h>


int my_multiplier(int a, int b) {
  return a * b;
}


jit_function_t build_foo(jit_context_t context) {
  jit_context_build_start(context);

  // Create function signature and object. int (*)(int, int)
  jit_type_t params[2] = {jit_type_int, jit_type_int};
  jit_type_t signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, params, 2, 1);
  jit_function_t F = jit_function_create(context, signature);

  // x, y are the parameters; t is a temporary
  jit_value_t x = jit_value_get_param(F, 0);
  jit_value_t y = jit_value_get_param(F, 1);
  jit_value_t t = jit_value_create(F, jit_type_int);

  // t = x + y
  jit_value_t sum = jit_insn_add(F, x, y);
  jit_insn_store(F, t, sum);

  // Prepare calling my_multiplier: create its signature
  jit_type_t mult_params[2] = {jit_type_int, jit_type_int};
  jit_type_t mult_signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, params, 2, 1);

  // x = my_multiplier(t, y)
  jit_value_t mult_args[] = {t, y};
  jit_value_t res = jit_insn_call_native(
      F, "my_multiplier", my_multiplier, mult_signature,
      mult_args, sizeof(mult_args) / sizeof(jit_value_t), JIT_CALL_NOTHROW);
  jit_insn_store(F, x, res);

  // return x
  jit_insn_return(F, x);

  jit_context_build_end(context);
  return F;
}


int main(int argc, char** argv) {
  jit_init();
  jit_context_t context = jit_context_create();
  jit_function_t foo = build_foo(context);

  // This will dump the uncompiled function, showing libjit opcodes
  jit_dump_function(stdout, foo, "foo [uncompiled]");

  // Compile (JIT) the function to machine code
  jit_context_build_start(context);
  jit_function_compile(foo);
  jit_context_build_end(context);

  // This will dump the disassembly of the machine code for the function
  jit_dump_function(stdout, foo, "foo [compiled]");

  // Run the function on argv input
  int u = atoi(argv[1]);
  int v = atoi(argv[2]);
  void* args[2] = {&u, &v};

  printf("foo(%d, %d) --> ", u, v);
  jit_int result;
  jit_function_apply(foo, args, &result);
  printf("%d\n", (int)result);

  jit_context_destroy(context);
  return 0;
}


