# jcc: jellyfish's Small C Compiler

## Overview
Create Self-host C Compiler.

## Status
The following are supported

### Type
- 8bit  integer (char)
- 16bit integer (short)
- 32bit integer (int)
- 64bit integer (long int)
- pointer

### Sentence
- Function
- For
- While
- Continue, Break
- Return
- GNU Statement
- Initializer

### Operator
[C Operator](https://en.cppreference.com/w/c/language/operator_precedence)

#### Precedence 1
- Suffix increment and decrement
- Function call
- Array subscripting

#### Precedence 2
- Prefix increment and decrement
- Unary plus and minus
- Logical NOT and bitwise NOT
- Indirection
- Address-Of

#### Precedence 3
- Multiplication, division, and remainder

#### Precedence 4
- Addition and subtraction

#### Precedence 5
- Bitwise left shift and right shift

#### Precedence 6
- Comparsion (<, <=, >, >=)

#### Precedence 7
- Comparsion (==, !=)

#### Precedence 8
- Bitwise AND

#### Precedence 9
- Bitwise XOR

#### Precedence 10
- Bitwise OR

#### Precedence 11
- Logical AND

#### Precedence 12
- Logical OR

#### Precedence 13
- Ternary conditional

#### Precdence 14
- Simple assignment
- Assignment by sum and difference
- Assignment by product, quotient, and remainder
- Assignment by bitwise left shift and right shift
- Assignment by bitwise AND, XOR, and OR
