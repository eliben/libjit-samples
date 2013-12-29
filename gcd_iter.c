//------------------------------------------------------------------------------
// gcd_iter libjit sample. Iterative version of gcd.c
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <jit/jit.h>

// Native version for benchmarking
int __attribute__ ((noinline)) gcd_iter_native(int u, int v) {
  int t;
  while (v) {
    t = u;
    u = v;
    v = t % v;
  }
  return u < 0 ? -u : u; /* abs(u) */
}

// Compute difference between two timespecs, as a timespec.
struct timespec timespec_diff(struct timespec start, struct timespec end) {
  const long SEC = 1000 * 1000 * 1000;  // 1 sec in ns
  struct timespec diff;
  if (end.tv_nsec < start.tv_nsec) {
    diff.tv_sec = end.tv_sec - start.tv_sec - 1;
    diff.tv_nsec = SEC + end.tv_nsec - start.tv_nsec;
  } else {
    diff.tv_sec = end.tv_sec - start.tv_sec;
    diff.tv_nsec = end.tv_nsec - start.tv_nsec;
  }

  return diff;
}

void print_diff(struct timespec start, struct timespec end) {
  struct timespec diff = timespec_diff(start, end);
  printf("Elapsed %ld . %ld\n", diff.tv_sec, diff.tv_nsec);
}

// Run a benchmark
void benchmark(jit_function_t jit_f) {
  int n1 = 49979687;
  int n2 = 982451653;
  const int N = 5 * 1000 * 1000;
  struct timespec t1, t2, d;
  typedef int (*FF)(int, int);
  int i, g;

  // Arguments for the JIT invocation
  void* args[] = {&n1, &n2};
  jit_int result;

  printf("Native\n");
  clock_gettime(CLOCK_REALTIME, &t1);
  for (i = 0; i < N; ++i) {
    g = gcd_iter_native(n1, n2);
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  print_diff(t1, t2);

  printf("\nJIT apply\n");
  clock_gettime(CLOCK_REALTIME, &t1);
  for (i = 0; i < N; ++i) {
    jit_function_apply(jit_f, args, &result);
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  print_diff(t1, t2);

  printf("\nJIT closure\n");
  FF clos = jit_function_to_closure(jit_f);
  clock_gettime(CLOCK_REALTIME, &t1);
  for (i = 0; i < N; ++i) {
    g = clos(n1, n2);
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  print_diff(t1, t2);
}


// Builds a function that performs the iterative GCD computation, implementing
// the following C code:
//
// int gcd_iter(int u, int v) {
//   int t;
//   while (v) {
//     t = u;
//     u = v;
//     v = t % v;
//   }
//   return u < 0 ? -u : u; /* abs(u) */
// }
//
// Returns an uncompiled jit_function_t.
jit_function_t build_gcd_func(jit_context_t context) {
  jit_context_build_start(context);

  // Create function signature and object. int (*)(int, int)
  jit_type_t params[2] = {jit_type_int, jit_type_int};
  jit_type_t signature = jit_type_create_signature(
      jit_abi_cdecl, jit_type_int, params, 2, 1);
  jit_function_t F = jit_function_create(context, signature);

  // u, v are function parameters; t is a temporary value.
  jit_value_t u, v, t;
  u = jit_value_get_param(F, 0);
  v = jit_value_get_param(F, 1);
  t = jit_value_create(F, jit_type_int);

  // Create the while (v) condition with a label that allows to loop back.
  //
  // label_while:
  //   if (v == 0) goto label_after_while
  //   .. contents of while loop
  //
  // label_after_while is created as undefined at this point, so that
  // instructions can have forward references to it. It will be placed later.
  jit_label_t label_while = jit_label_undefined;
  jit_label_t label_after_while = jit_label_undefined;
  jit_value_t const0 = jit_value_create_nint_constant(F, jit_type_int, 0);

  jit_insn_label(F, &label_while);
  jit_value_t cmp_v_0 = jit_insn_eq(F, v, const0);
  jit_insn_branch_if(F, cmp_v_0, &label_after_while);

  // t = u
  jit_insn_store(F, t, u);
  // u = v
  jit_insn_store(F, u, v);

  // v = t % v
  jit_value_t rem = jit_insn_rem(F, t, v);
  jit_insn_store(F, v, rem);

  //   goto label_while
  // label_after_while:
  //   ...
  jit_insn_branch(F, &label_while);
  jit_insn_label(F, &label_after_while);

  //   if (u >= 0) goto label_positive
  //   return -u
  // label_pos:
  //   return u
  jit_label_t label_positive = jit_label_undefined;
  jit_value_t cmp_u_0 = jit_insn_ge(F, u, const0);
  jit_insn_branch_if(F, cmp_u_0, &label_positive);

  jit_value_t minus_u = jit_insn_neg(F, u);
  jit_insn_return(F, minus_u);
  jit_insn_label(F, &label_positive);
  jit_insn_return(F, u);

  jit_context_build_end(context);
  return F;
}


int main(int argc, char** argv) {
  jit_init();
  jit_context_t context = jit_context_create();
  jit_function_t gcd = build_gcd_func(context);

  // This will dump the uncompiled function, showing libjit opcodes
  jit_dump_function(stdout, gcd, "gcd [uncompiled]");

  printf("Optimization level: %u\n", jit_function_get_optimization_level(gcd));
  // Compile (JIT) the function to machine code
  jit_context_build_start(context);
  jit_function_compile(gcd);
  jit_context_build_end(context);

  // This will dump the disassembly of the machine code for the function
  jit_dump_function(stdout, gcd, "gcd [compiled]");

  // Run the function on argv input
  if (argc > 2) {
    int u = atoi(argv[1]);
    int v = atoi(argv[2]);
    void* args[2] = {&u, &v};

    printf("gcd(%d, %d) --> ", u, v);
    jit_int result;
    jit_function_apply(gcd, args, &result);
    /*typedef int (*FF)(int, int);*/
    /*FF gcd_f = jit_function_to_closure(gcd);*/
    /*int result = gcd_f(u, v);*/
    printf("%d\n", (int)result);
  }

  /*benchmark(gcd);*/

  jit_context_destroy(context);
  return 0;
}

