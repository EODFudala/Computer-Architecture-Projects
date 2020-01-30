# Project 1 - MIPS Instruction Interpreter

In this project, you will create an instruction interpreter for a subset of MIPS code. Starting with the assembled instructions in memory, you will fetch, disassemble, decode, and execute MIPS machine instructions, simulating each stage in the computation. You're creating what is effectively a miniature version of MARS! There is one important difference, though—MARS takes in assembly language source Hiles, not .dump Hiles, so it contains an assembler, too.

* [Full Specification](CSE140_Project1(3).pdf)

## Design Documentation

### Decode:
* **Correctness Constraint:** The “Decode” function should take in arguments unsigned int instr, DecodedInst object d, and RegVals object rVals. The function should first identify the instruction type using the opcode (first 6 bits) and then partition the remaining 26 bits respectively. At termination, the function should have stored strings in rVals and instr that represent the MIPS instruction.
* **Structure of Solution:** To create an encapsulated, and efficient “Decode” function we utilized the bitwise ‘&’ operator to create a “bitmask” which is used in the partitioning portion of the function. The first step is for the op code to be identified. By shifting the instruction 26 bits, we are left with the last 6 bits which represent opcode.
Next, we will have a switch that has cases R, J, and I which will represent the types of instructions. Once the instruction type is identified, we can use “bitmasks” to isolate the relevant bits for decoding. Our bitmasks work by isolating 5 bits, and then shifting them so that they are the left most bits. For rs, we use a bitmap (inst & 0x03e00000) and then shift it by 21 bits. We chose this number because it represents the largest possible value for a register in Rs (which is 31). The following is the conversion to hex from the binary representation of an R-type instruction with Rs = $31:
```
 	000000 11111 00000 00000 00000 000000
=	0x03e00000

```
It is important that we have all the other bytes set to 0, because when we use the bitwise ‘&’ operator, the returned value will only include the two bytes we care about. This is called a “bitmask” because it essentially “masks” the bits we are not interested in. This is particularly useful for “Decode” because it allows us to translate the specific registers. The following is an example of how the “bitmask” would work:
```
	0x12a01002
&	0x03e00000
	----------------------------
		000100 10101 00000 00010 00000 000010
	&	000000 11111 00000 00000 00000 000000
	=	000000 10101 00000 00000 00000 000000
	=	0x02a00000
```
Once we have applied the “bitmask”, we simply shift the bytes respectively. For Rs, we would have a shift of 21 bits because we only care about bits 25-21. For Rt we would shift 16, Rd would shift 11, and Shamt would shift 6. Funct would still require a mask, but would not need to be shifted for it is already the rightmost bit.




