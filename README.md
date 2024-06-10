# bC Programming Language and Compiler

## Table of Contents
1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Usage](#usage)
4. [Syntax and Examples](#syntax-and-examples)

## Introduction

bC is a small and easy to use C-like programming language. I wrote the compiler for the language for my CS445 Compiler Design class at the University of Idaho. The compiler is targeted for the Tiny Machine by Kenneth C. Louden in [Compiler Construction - Principles and Practice](http://www.cs.sjsu.edu/~louden/cmptext/). 

## Installation
1. Clone the repo:
    ```bash
    https://github.com/Lance-Town/bC-Compiler.git
    cd bC-Compiler
    ```
2. Build the compiler
    ```bash
    make
    ```

## Usage 

```bash
./bC yourProgram.bC > yourProgram.tm
```

## Syntax and Examples 

Some example programs in bC are:

### Variables and Arithmetic

```bC
// All variables must be declared at the top of the scope
main() {
    int a:5;
    int b:10;
    int sum;

    sum = a + b;
}
```

### Conditionals
```bC
main() {
    bool b: true;

    if b then {
        b = not b;
    } else {
        b = true;
    }
}
```

### Loops
```bC
main() {
    for i=3 to 3 by 2 do {}

    while (true) do {}
}
```

Find more examples in the testFiles folder


