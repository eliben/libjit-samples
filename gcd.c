//------------------------------------------------------------------------------
// gcd.c Taken from the libjit tutorial, part 2.
//
// The libjit license applies to this file.
//------------------------------------------------------------------------------
/*
 * JITs this gcd function:
 * unsigned int gcd(unsigned int x, unsigned int y) {
 *  if (x == y)
 *    return x;
 *  else if (x < y)
 *    return gcd(x, y - x);
 *  else
 *    return gcd(x - y, y);
 * }
*/

#include <stdio.h>
#include <jit/jit.h>

int main(int argc, char **argv) {
  jit_context_t context;
  jit_type_t params[2];
  jit_type_t signature;
  jit_function_t function;
  jit_value_t x, y;
  jit_value_t temp1, temp2;
  jit_value_t temp3, temp4;
  jit_value_t temp_args[2];
  jit_label_t label1 = jit_label_undefined;
  jit_label_t label2 = jit_label_undefined;
  jit_uint arg1, arg2;
  void *args[2];
  jit_uint result;

  /* Create a context to hold the JIT's primary state */
  context = jit_context_create();

  /* Lock the context while we build and compile the function */
  jit_context_build_start(context);

  /* Build the function signature */
  params[0] = jit_type_uint;
  params[1] = jit_type_uint;
  signature = jit_type_create_signature
    (jit_abi_cdecl, jit_type_uint, params, 2, 1);

  /* Create the function object */
  function = jit_function_create(context, signature);
  jit_type_free(signature);

  /* Check the condition "if(x == y)" */
  x = jit_value_get_param(function, 0);
  y = jit_value_get_param(function, 1);
  temp1 = jit_insn_eq(function, x, y);
  jit_insn_branch_if_not(function, temp1, &label1);

  /* Implement "return x" */
  jit_insn_return(function, x);

  /* Set "label1" at this position */
  jit_insn_label(function, &label1);

  /* Check the condition "if(x < y)" */
  temp2 = jit_insn_lt(function, x, y);
  jit_insn_branch_if_not(function, temp2, &label2);

  /* Implement "return gcd(x, y - x)" */
  temp_args[0] = x;
  temp_args[1] = jit_insn_sub(function, y, x);
  temp3 = jit_insn_call
    (function, "gcd", function, 0, temp_args, 2, 0);
  jit_insn_return(function, temp3);

  /* Set "label2" at this position */
  jit_insn_label(function, &label2);

  /* Implement "return gcd(x - y, y)" */
  temp_args[0] = jit_insn_sub(function, x, y);
  temp_args[1] = y;
  temp4 = jit_insn_call
    (function, "gcd", function, 0, temp_args, 2, 0);
  jit_insn_return(function, temp4);

  /* Compile the function */
  jit_function_compile(function);

  /* Unlock the context */
  jit_context_build_end(context);

  /* Execute the function and print the result */
  arg1 = 27;
  arg2 = 14;
  args[0] = &arg1;
  args[1] = &arg2;
  jit_function_apply(function, args, &result);
  printf("gcd(27, 14) = %u\n", (unsigned int)result);

  /* Clean up */
  jit_context_destroy(context);

  /* Finished */
  return 0;
}
