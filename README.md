# JoCC (JoC Compiler)

C is an extremely useful programming language. It's small and simple, meaning
programmers have to write a lot of code to do basic things, but they're at least
focusing on the actual problems they're trying to solve instead of focusing on
how to fit their problems into the language idioms.

Indeed, any time I'm writing code and I find myself thinking more about the code
itself than the problem at hand or what I want the computer to do, it kills my
morale and productivity. I, of course, want to maximize morale and productivity.

C is also maybe the most important programming language we have today because
it's been around so long. It's the foundation of most of the tools we use and
its ABI is the de-facto standard by which other programming languages interact
with each other. If you want to write code that can be used anywhere and
everywhere, you write it in C.

JoC is an effort to further remove language-specific details from programmers'
cognitive loads so they can just focus on writing the code that solves their
problems, while still maintaining broad compatibility with existing C code.

## Translation Groups

By far, my least favorite thing about C compared to other, more modern
programming languages, is all the time I spend thinking about forward
declarations, headers, `#include` directives, and build systems. Standard C
necessitates these for two main reasons: single-pass and separate compilation.
Semantically, each translation unit is processed only once, top-to-bottom, and
completely independently of all other translation units.

Most modern languages don't have this problem. You don't have to write
forward-declarations separate from definitions; you can just reference code
that's defined later in the same source file or in a completely different file.
It's great! One less thing to worry about.

Separate translation also means how you organize your code between source files
ends up affecting how it gets optimized. I've seen several programs that
actually rely on this behavior to keep the compiler from making optimizations
that would break their program! I don't know about you, but this is another
programming language detail I don't want to waste time thinking about. I want
to be able to move my code around source files for purely organizational
purposes without worrying about how it affects the compiled result.

What I really want is to write all my global type, function, and variable
definitions in any order, split between source files however I see fit for
purely organizational purposes, and just tell my compiler to translate them all
together. In JoC, I call this a "translation group".

For example, the following translation group would compile just fine in JoC:

**a.joc:**
```c
void b(void)
{
    c(123);
    f();
}

void c(long x)
{
    // Do something.
}
```

**d.joc:**
```c
void e(void)
{
    f();
    c(456);
}

void f(void)
{
    // Do something.
}
```

In `a.joc`, `b()` can call `c()` and `f()` even though they weren't declared
before `b()` in the same file. The fact that they're declared anywhere else in
the same translation group is enough. In `d.joc`, `e()` can call `f()` and `c()`
for the same reason; the circular dependency between the two files isn't a
problem.

Of course, if you really like headers because they separate interfaces from
implementations, there's nothing stopping you from doing the same thing in a
translation group. Example:

**foo-interface.joc:**
```c
// This is the interface documentation for foo.
void foo(void);
```

**foo-implementation.joc:**
```c
// This is the implementation documentation for foo.
void foo(void)
{
    // Do something.
}
```

## Prelude Files

While order-independent global definitions are generally possible in the main
language, doing something similar for preprocessing would be much more
complicated. JoC doesn't even try. Instead, in JoC, the preprocessor is
single-pass, top-to-bottom, and independent for each source file just as it is
in Standard C.

Problem is we still might want preprocessor macros to be available in multiple
source files, particularly those that come from legacy C header files. JoC
solves this problem with "prelude files". Files with the `.jop` extension in a
translation group get preprocessed first, and all `.joc` files in the same
translation group begin with the macro definitions the `.jop` preprocessors
ended with. Example:

**prelude.jop:**
```c
#include <stdio.h>
#include <stdlib.h>

#define HELLO_WORLD "Hello, world!\n"
```

**main.joc:**
```c
int main(void)
{
    printf(HELLO_WORLD);
    return EXIT_SUCCESS;
}
```

I imagine most JoC projects will have one `prelude.jop` file containing all the
legacy header `#include` directives and project-wide macro definitions.

## Build Systems

The build system and library distribution ecosystem for C programs is fragmented
and messy. Any time I want to make a new C program or library, I have to spend
at least half an hour writing a build script or makefile or whatever. As with
headers and forward declarations, it's a serious morale killer when all I really
want to do is write code.

This isn't a problem in modern programming languages like Rust. Rust
distributions always come with [Cargo](https://doc.rust-lang.org/cargo/) and
creating a new Rust project is as simple as running `cargo new <project name>`.
I want JoC to have something as ubiquitous and easy to use as Cargo.

## Other Ideas

My next biggest problem with Standard C is implicit arithmetic conversions. They
can cause subtle bugs and make code hard to reason about at times. I much prefer
how Rust handles conversions, where every conversion is explicit. I'm still
thinking about how to cleanly extend C to get the semantics I want. Clang's
non-standard `_ExtInt` types might provide some guidance here.

I also want to make some common, non-standard C extensions part of JoC. See
https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html for examples.

And finally, as a game developer, I want JoC to play nice with the native window
system and graphics API's on all my target platforms. On Windows, that means
support for COM to use DirectX. On Apple platforms, that means support for
Objective-C to use Cocoa and Metal and countless other frameworks.

## Implementation Status

So far, I've only really implemented a lexer and the data structures for
representing diagnostics and the abstract syntax tree. This is just a side
project I work on max a few hours every day, so it's slow-going.

You may have noticed the implementation so far is just one `.c` file and a bunch
of `.h` files containing function definitions with internal linkage. This is how
I solve my single-pass, separate compilation complaints in Standard C without
translation groups. It lets me have fewer files, fewer forward declarations,
and lets me spend less time thinking about the build system. I know it's not
idiomatic, but it's worked well for me so far.

The goal is to write the compiler `jocc` itself in Standard C, and eventually
the accompanying tools in JoC. The `common` directory contains code that will be
shared between multiple tools (e.g. an automatic code formatter, a language
server, etc.) while the `jocc` directory contains code exclusive to the
compiler.

The implementation uses a lot of dynamic arrays with `malloc`/`realloc`. Not the
fastest thing in the world but it's portable and probably better than calling
`malloc` for each element individually. Many objects are referenced by 32-bit
indices into these arrays. Using 32-bit indices instead of full 64-bit indicies
or pointers provides the following advantages:

- Indices instead of pointers means the arrays can relocate in memory as they
  grow.
- 32-bit values instead of 64-bit values save space. `astman::data`
  in-particular would be twice as big.
- 64-bit data manipulation often requires an extra byte per-instruction in
  x86-64. Using 32-bit values saves on code size in-addition to data size.
- Smaller sizes doesn't just mean lower memory requirements; it also mean more
  data and instructions fit in CPU caches, resulting in speed boosts.
- Provides greater consistency when compiled on 32-bit targets. WebAssembly
  in-particular is a platform I'd like JoCC to work on.
