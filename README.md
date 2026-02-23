This repo is for tools I make for my own convenience for developing but want to share.

Not polished and I believe has some bugs!
Compile just as a single c file. clang -o a expr_map.c

expr_map - attempts to convert a map (u32 : u32) to a simple expression. This is useful when you want significantly faster lookups and smaller binary size. This is a solution to avoid control flow overhead, avoid array indexing when inputs are sparse. Additionally it only uses simple operations that each take 1 cpu cycle to perform (except multiply).

Notes for me:
restrict keyword?
