# pstream
stream benchmark pthread version

==============================================================
* Program: STREAM
* Parallel Library: pthread
* Orignial Algorithm by John D. McCalpin
* Programmer: Jian Fang (TU Delft)
* Copyright: Jian Fang(j.fang-1@tudelft.nl)
* License: You are free to use this program and modify
*          this program for your own use. If you publish
*          results obtained from this program, please
*          cite both this program and the orignal virginia
*          version.
==============================================================


==============================================================
                        Introduction
==============================================================
1. There are three arrays which is indicate as a, b, c. You can
   define you own type of data for the array. For default, it is
   int64_t, which means each element takes up 8B.

2. The total data access amount consider the cache write strategy
   in most of the current system that if a write is not writing a
   whole cacheline, it needs to read the cacheline first before 
   writing it back. So each write miss for L3 leads to an extra
   read. If the system you are testing is not using this strategy,
   please modify the calculation method or the 'amountFactor'.

3. This benchmark contain 9 operations:   (n is a constant)
    OPERATION TYPE    OPERATION CODE    DESCRIPTION    amountFactor
    READ                  0             sum += c[j]        1
    WRITE                 1             c[j] = n           2
    SELFINC               2             c[j] ++            2
    COPY                  3             c[j] = a[j]        3
    MUL                   4             c[j] = a[j]*n      3
    SELFADD               5             c[j] += a[j]       3
    ADD                   6             c[j] = a[j]+b[j]   4
    MULADD                7             c[j] = a[j]*n+b[j] 4
    SELFDADD              8             c[j] += a[j]+b[j]  4
    
4. This program is written in C/C++ and using the pthread library.
   You can change the mapping[] array in the source code to
   change the core binding. For example you have a 2 nodes numa
   system. For node 1, it has core 0-9, and node 2 has core
   10-19. If you only want to test the bandwidth of node 1,
   just place 0-9 in the mapping[] array.
   
5. There is no Makefile yet. We provide a 'test.sh' file example
   which contains instructoins and an example how to compile tihs
   program, as well as a way to run this test. You are welcome to
   help and make you own Makefile.
   
6. If you have any suggestions or comments, please feel free to
   contact the author:
   Jian Fang(j.fang-1@tudelft.nl)
