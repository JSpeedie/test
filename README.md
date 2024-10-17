# cmp-tree

### Project Description

This repo contains multiple different implementations of the same program:
`cmp-tree`. The program functions as a sort of combination between `cmp` and
`tree`, allowing you to compare the full contents of two directories and find
any disparities.

### Motivation

The impetus for this project came from my need to compare one backup of mine
that had suffered from a filesystem error which I attempted to recover from to
another backup that had not suffered the error. I wanted to make sure that
every single file contained on the recovered backup was byte-for-byte identical
with the corresponding file on the healthy backup.

### Why are There Multiple Implementations?

The long story short is that `cmp-tree` has served as a project for
self-directed exploration and learning. What started out as the need for a
simple utility expanded into an opportunity to learn something about:

1. The performance differences between Bash and C/C++/Rust
2. How certain objects (like `std::string` and `std::vector`) are implemented
   in C++
3. What it's like developing in C compared to more modern systems-level
   languages like C++/Rust
4. What it's like writing in a very modern language like Rust compared to C++
5. How to profile a program.

If you came here looking only for an executable that will allow you to compare
all the files in two directory trees, then simply build the Rust version of
`cmp-tree` and use that. If you want to hear a little more about what I learned
from this project, then please feel free to read the Project Report below.

### Project Report

Originally I thought this project could be a simple bash script. Run `find` on
the first directory, then `find` on the second directory, combine the results
into one giant list, `sort` it, filter it so it only contains unique paths
(using `uniq`), and then run `cmp` on `[first_directory]/[unique_find_path[i]]`
and `[second_directory]/[unique_find_path[i]]`.

It didn't take me long before I had a working Bash implementation, but it did
seem sort of slow. I looked online and found that `diff -qr [first_directory]
[second_directory]` already accomplished the behaviour I was trying to code in
`cmp-tree`. I compared the speeds between my Bash implementation and `diff
-qr`, and `diff -qr` was significantly faster (by about 3 times). I could have
simply stopped there and used `diff -qr` for my purpose, but I wondered if I
could make a program that could accomplish the task faster than `diff -qr`.
After all, `diff` is a tool for finding differences between two files and
telling you *how* they differ. `cmp-tree`, on the other hand, only needs to
find *if* the files differ.

My next thought was that I should be able to get dramatic time savings by
simply implementing `cmp-tree` in a more performant language like C++. It took
me about twice as long to implement the C++ version, but it was significantly
more enjoyable to write it and there was room for big improvements (such as
multithreading) that might not have been possible or easy in Bash. When I ran
some speed tests, however, I found that my C++ implementation was essentially
just as slow as the Bash implementation! I was shocked. My first thought was
that perhaps this absence of improved performance might come from some of the
"luxury" classes I was using in my C++ implementation (such as
`std::filesystem::path` and `std::vector`). I'd heard in passing that
`std::vector` was not super fast and `std::filesystem::path` seemed to
implement a lot of behaviour I perhaps didn't need for my program. To see if
this was really the case, I set out to implement `cmp-tree` in C.

The C implementation took me easily 4-6 times as long as it took me to write
the C++ implementation. This was mostly because I wrote my own implementations
of: (1) a C++ vector I called `DynamicArray`, (2) a C++ string I called
`String`, (3) basic functions that would allow me to work with my `String`s as
if they were paths, and (4) a hand implementation of merge sort that worked on
`DynamicArray`. This was a great learning opportunity in many ways. I got to
experience first hand some of the frustrations with C that might've motivated
C++ such as:

1. Having to hand-write implementations for things most programmers feel
   (perhaps rightfully so) entitled to such as vectors/dynamic arrays, and
   decent strings.
2. Proper support for generics so you can write one implementation of your
   dynamic array instead of one for each type of data your dynamic array needs
   to serve as a container for.
3. Proper support for generics so you can specify the type your dynamic array
   serves as a container for so that your compiler can warn you if you passed a
   dynamic array of ints to a function that only takes a dynamic array of
   strings, for instance.

I learned a lot about how strings and vectors might be implemented in C++, but
I also realized how much C can slow down development simply due to its lack of
basic utilities such as decent strings and vectors. I also saw the benefits of
C such as the fact that while you might have to implement something from
scratch, that implementation is likely bespoke and very well-suited to the
specific use case. Eventually though (and after lots of debugging), I finally
completed my C implementation. When I ran the speed tests I once again found
that it was on par with the Bash implementation in terms of speed!

I quickly realized that this lack of performance from the C implementation must
mean that there was something fundamentally off about the design of my
implementation that must be slowing things down. At this point, both my C++ and
C implementation would use `opendir()` and `readdir()` to generate a list of
relative paths representing every file within a directory, including all the
files in subdirectories. The implementations would then do this again with the
second directory, combine the two file path lists, sort them, filter them to
contain only unique entries and then `fork()` -> `exec()` a `cmp` command on
`[first_directory]/[unique_file_path[i]]` and
`[second_directory]/[unique_file_path[i]]`. Remembering this, I thought that it
was possible that calling `fork()` so many times could be a big part of
problem. After all, it's not free to just duplicate a process. Before making
any changes though, I also used `perf` to profile the C implementation and I
found that it was getting a lot of L2 and L3 cache misses. I remembered hearing
recently that sometimes caches are specific to processes, and so perhaps all
this forking is slowing things down in more ways than one.

My solution was to implement a quick-and-dirty function that would replace a
`fork()` -> `exec()` call pair by simply comparing the contents of two files,
and if at any point it found a difference, exiting and returning a status that
indicates that the two files are different. My first-draft of this function
yielded no significant performance gain which surprised me given that there
should be substantially less overhead - no forking, no commandline argument or
flag parsing, none of the extra functionality a more complete program like
`cmp` might have. My first-draft file-comparison function worked by reading a
set number of bytes from both files, and then looping through those bytes 1 by
1 and if the byte from one file mismatched the corresponding byte from the
other file, it exited early. After some quick research, I found that using
`memcmp()` to compare two sets of bytes was possibly faster. I gave it a go and
found my program was *substantially* faster. It turns out that on a lot of
architectures, `memcmp()` has an implementation written efficiently in
assembly. For example, it could be vectorized for your processor architecture,
giving it huge speed gains over whatever the compiler was able to do to my
C/C++ code for comparing bytes. At the time of writing, this switch from
manually checking each byte to using `memcmp()` represents the biggest speed
improvement I've found so far.

Before making the switch from manually comparing bytes to using `memcmp()`, I
also tried multithreading the C implementation to see if that would help. It
led to modest speed gains, but it also came at the cost of using significantly
more cores and with cache-miss downsides of its own. Using `memcmp()`
represented a solution that simply made better use of the underlying hardware.
If vectorized, the code will make use of hardware capability that was just
underutilized in other implementations.



