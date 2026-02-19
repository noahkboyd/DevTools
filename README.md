This repo is for tools I make for my own convenience for developing but want to share.

Not polished and I believe has some bugs! Compile just as a single c file. clang -o simple simple_function_map.c

simple_function_map - attempts to find a simple expression map some u32 inputs to u32 outputs. This is useful when you want to avoid control flow overhead (a switch case block or series of if else's) to select a value and when the inputs aren't dense (like 0, 1, 2...) and thus impractical to use as an index to an array of the outputs. Avoiding control flow and memory accessing can have significant runtime performance improvements especially when a small expression is found. Additionally it only uses simple operations that each take 1 cpu cycle to perform (except multiply).

Notes for me:
restrict keyword?
