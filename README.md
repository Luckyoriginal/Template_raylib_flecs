# Headerless-C

C with incremental compilation and **zero `.h` files**.  
Creating a new module takes about 10 seconds.

---

## Module template

```c
/* mymod.c */
#ifndef MYMOD_C
#define MYMOD_C

/* ── declarations (always visible when #included) ── */
int  my_func(int x);
void my_other(const char *s);

/* ── definitions (only compiled for this TU) ─────── */
#ifdef IMPL_mymod

#include <stdio.h>   /* system includes go here, inside the IMPL block */

int my_func(int x) {
    return x * 2;
}

void my_other(const char *s) {
    printf("%s\n", s);
}

#endif  /* IMPL_mymod */
#endif  /* MYMOD_C */
```

## Makefile — one line per module

```makefile
$(eval $(call MODULE,mymod))   # ← that's it, name must match filename and #ifdef
```

## Using a module

```c
#include "mymod.c"   /* acts as a header — only declarations flow in */

#ifdef IMPL_caller
int main(void) {
    my_func(21);
    return 0;
}
#endif
```

---

## How it works

The Makefile compiles each `.c` file with `-DIMPL_<name>` defined.

| Context | `#ifdef IMPL_mymod` | result |
|---|---|---|
| Compiling `mymod.c` directly | **defined** | bodies compiled |
| `#include "mymod.c"` from elsewhere | not defined | only declarations seen |

The include guard (`#ifndef MYMOD_C`) prevents double inclusion.

## Checklist for a new module

- [ ] `src/mymod.c` — filename sets the name
- [ ] `#ifndef MYMOD_C` / `#define MYMOD_C` — include guard at top
- [ ] Declarations **above** `#ifdef IMPL_mymod`
- [ ] Everything that needs `#include` goes **inside** `#ifdef IMPL_mymod`
- [ ] `$(eval $(call MODULE,mymod))` in Makefile
- [ ] `#include "mymod.c"` wherever you use it

## Build

```sh
make        # incremental — only changed files recompile
make run    # build + run
make clean
```

## Rules

- Module name = filename stem = `IMPL_` suffix. Always the same string.
- No numbers to track. No `module.h` needed. No per-function macros.
- System `#include`s go inside the `#ifdef IMPL_` block (they're only
  needed when compiling bodies, and this avoids polluting includers).
