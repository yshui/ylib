ylib
====

A set of C header files, implements a variety of stuffs. The major aim is to bring cross-platform easy access to a variety of utils to C.

## Contents

* ylist.h: A linked list implementation, imported from Linux kernel.
* yskiplist.h: A skip list implementation.
* ythread.h: A C11 thread implementation, imported from [TinyCThread](https://tinycthread.github.io)
* yref.h: WIP
* ydef.h: Some useful, compiler-independent macros.
* yrnd.h: Some fast random number generators.
* compiler.h: Some compiler specific macros.

## exinc/

These are some external dependencies of ylib. I'll sync those headers with upstream regularly, but I'm not the maintainer of those headers.
