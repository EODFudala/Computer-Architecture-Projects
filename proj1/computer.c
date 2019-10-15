#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */

    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }
    
    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
	/*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;
    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;
    
    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;
    while (1) {
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);
            if (s[0] == 'q') {
                return;
            }
        }

        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);

        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        /* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d);

        /* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
	 */
        val = Execute(&d, &rVals);

	UpdatePC(&d,val);

        /* 
	 * Perform memory load or store. Place the
	 * address of any updated memory in *changedMem, 
	 * otherwise put -1 in *changedMem. 
	 * Return any memory value that is read, otherwise return -1.
         */
        val = Mem(&d, val, &changedMem);

        /* 
	 * Write back to register. If the instruction modified a register--
	 * (including jal, which modifies $ra) --
         * put the index of the modified register in *changedReg,
         * otherwise put -1 in *changedReg.
         */
        RegWrite(&d, val, &changedReg);

        PrintInfo (changedReg, changedMem);
    }
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
    int k, addr;
    printf ("New pc = %8.8x\n", mips.pc);
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
        changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
        changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
             addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
             addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {
    d->op = instr >> 26;
    
    /* creates bitmasks using the bitwise & operator. Allows us to isolate relevant
     bits for each register */
    int rsMask = (instr & 0x03e00000);
    int rtMask = (instr & 0x001f0000);
    int rdMask = (instr & 0x0000f800);
    int shamtMask = (instr & 0x000007c0);
    int functMask = (instr & 0x0000003f);
    int addMask = (instr & 0x03ffffff);
    int immMask = (instr & 0x0000ffff);

    switch (d->op) {
          case 0x00: // R-type
            d->type = R;
            /* shifts allow us to move isolated bits to rightmost position*/
            rVals->R_rs = mips.registers[d->regs.r.rs = rsMask >> 21];
            rVals->R_rt = mips.registers[d->regs.r.rt = rtMask >> 16];
            rVals->R_rd = mips.registers[d->regs.r.rd = rdMask >> 11];
            d->regs.r.shamt = shamtMask >> 6;
            d->regs.r.funct = functMask;
            break;
          case 0x02:
          case 0x03: // J-type
              d->type = J;
              d->regs.j.target = addMask;
              break;
          default: // I-type
              d->type = I;
              rVals->R_rs = mips.registers[d->regs.i.rs = rsMask >> 21];
              rVals->R_rt = mips.registers[d->regs.i.rt = rtMask >> 16];
              d->regs.i.addr_or_immed = immMask;
      }
}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
    char* i;
    switch (d->op) {
        case 0x00: //R Type
            switch (d->regs.r.funct) {
                case 0x20://add
                    i = "add";
                    break;
                case 0x21://add unsigned
                    i = "addu";
                    break;
                case 0x24://and
                    i = "and";
                    break;
                case 0x08://jump register
                    i = "jr";
                    break;
                case 0x27://nor
                    i = "nor";
                    break;
                case 0x25://or
                    i = "or";
                    break;
                case 0x2a://set less than
                    i = "slt";
                    break;
                case 0x2b://set less than unsigned
                    i = "sltu";
                    break;
                case 0x00://shift left logical
                    i = "sll";
                    break;
                case 0x02://shift right logical
                    i = "srl";
                    break;
                case 0x22://subtract
                    i = "sub";
                    break;
                case 0x23://subtract unsigned
                    i = "subu";
                    break;
                default:
                    exit(1);
            }
            break;
        case 0x02://jump
            i = "j";
            break;
        case 0x03://jump and link
            i = "jal";
            break;
        case 0x08://add immediate
            i = "addi";
            break;
        case 0x09://add immediate unsigned
            i = "addiu";
            break;
        case 0x0c://and immediate
            i = "andi";
            break;
        case 0x04://branch on equal
            i = "beq";
            break;
        case 0x05://branch on not equal
            i = "bne";
            break;
        case 0x24://load byte unsigned
            i = "lbu";
            break;
        case 0x25://load halfword unsigned
            i = "lhu";
            break;
        case 0x30://load linked
            i = "ll";
            break;
        case 0x0f://load upper immediate
            i = "lui";
            break;
        case 0x23://load word
            i = "lw";
            break;
        case 0x0d://or immediate
            i = "ori";
            break;
        case 0x0a://set less than immediate
            i = "slti";
            break;
        case 0x0b://set less than immediate unsigned
            i = "sltiu";
            break;
        case 0x28://store byte
            i = "sb";
            break;
        case 0x38://store conditional
            i = "sc";
            break;
        case 0x29://store halfword
            i = "sh";
            break;
        case 0x2b://store word
            i = "sw";
            break;
        default:
            exit(1);
          }
        
          int rd = d->regs.r.rd;
          int rs = d->regs.r.rs;
          int rt = d->regs.r.rt;
          short imm = d->regs.i.addr_or_immed;
        
          switch (d->type) {
              case R:
                  switch (d->regs.r.funct) {
                      case 0x00://shift left logical
                      case 0x02://shift right logical
                          printf("%s\t$%d, $%d, %d\n", i, rd, rt, d->regs.r.shamt); return;
                      case 0x08://jump register
                          printf("%s\t$%d\n", i, rs);
                          return;
                      default:
                          printf("%s\t$%d, $%d, $%d\n", i, rd, rs, rt);
                          return;
                  }
              case J:
                  printf("%s\t0x%.8x\n", i, (mips.pc & 0xf0000000) | ((d->regs.j.target << 2) & 0x0fffffff));
                  return;
              case I:
                  switch (d->op) {
                      case 0x04://branch on equal
                      case 0x05://branch on not equal
                          printf("%s\t$%d, $%d, 0x%.8x\n", i, rs, rt, mips.pc + 4 + (imm << 2));
                          return;
                      case 0x09://add immediate unsigned
                          printf("%s\t$%d, $%d, %d\n", i, rt, rs, imm);
                          return;
                      case 0x0c://and immediate
                      case 0x0d://or immediate
                      case 0x0f://load upper immediate
                          printf("%s\t$%d, $%d, 0x%x\n", i, rt, rs, (unsigned short)imm);
                          return;
                      case 0x23://load word
                      case 0x2b://store word
                          printf("%s\t$%d, %d($%d)\n", i, rt, imm, rs);
                          return;
                      }
              }
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
  switch (d->op) {
      case 0x00://R type
              switch (d->regs.r.funct) {
                  case 0x20://add
                  case 0x21://add unsigned
                      return rVals->R_rs + rVals->R_rt;
                  case 0x08://jump register
                      return rVals->R_rs;
                  case 0x00://shift left logical
                      return rVals->R_rt << d->regs.r.shamt;
                  case 0x02://shift right logical
                      return (unsigned int)rVals->R_rt >> d->regs.r.shamt;
                 case 0x24://and
                      return rVals->R_rs & rVals->R_rt;
                  case 0x25://or
                      return rVals->R_rs | rVals->R_rt;
                  case 0x2a://set less than
                      return rVals->R_rs < rVals->R_rt;
                  case 0x22://subtract
                  case 0x23://subtract unsigned
                      return rVals->R_rs - rVals->R_rt;
            }
            break;
        case 0x08://add immediate
        case 0x09://add immediate unsigned
            return rVals->R_rs + d->regs.i.addr_or_immed;
        case 0x02://jump
            break;
        case 0x03://jump and link
            return mips.pc + 4;
        case 0x04://branch on equal
            return rVals->R_rs == rVals->R_rt;
        case 0x05://branch on not equal
            return rVals->R_rs != rVals->R_rt;
        case 0x0c://and immediate
            return rVals->R_rs & d->regs.i.addr_or_immed;
        case 0x0d://or immediate
            return rVals->R_rs | d->regs.i.addr_or_immed;
        case 0x0f://load upper immediate
            return d->regs.i.addr_or_immed << 16;
        case 0x23://load word
        case 0x2b://store word
            return rVals->R_rs + (short)d->regs.i.addr_or_immed;
      }
    return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
     mips.pc+=4;
    
    //Conditional statement to see if the program counter is within the approrpiate bounds. If not, program will exit
     if(mips.pc < 0x00400000 || mips.pc > 0x00401000 || mips.pc % 4 != 0) {
          exit(3);
    }
    //check if it is a jump register instruction
    if (d->type == R && d->regs.r.funct == 0x08)/*funct value for jump register*/ {
         mips.pc = mips.registers[d->regs.r.rs];
    }
    //if it isnt a jump register instruction, check if it is a beq or bne instructoin
    else if (d->op == 0x04/*opcode for beq*/ || d->op == 0x05/*opcode for bne*/) {
        //if the output of beq or bne is true increment pc counter by address
        if (val) {
           mips.pc += (short)d->regs.i.addr_or_immed;
         }
    }
    //if it is not a jr, bne, or bew instruction, then it must be J type
    else if (d->type == J) {
        //using two bitmasks and the bitwise "or" operand, we can find the new value for the program counter after jump or jump and link
        mips.pc = (mips.pc & 0xf0000000) | ((d->regs.j.target << 2) & 0x0fffffff);
    }
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
        *changedMem = -1;
        
        if ((d->op == 0x23 || d->op == 0x2b) && (val < 0x00401000 || val > 0x00403fff) || val % 4 != 0) { //cheak if accessing invalid memory address or not word aligned
          printf("Memory Access Exception at 0x%.8x: address 0x%.8x", mips.pc, val); //run test case to see if %x needs to be changed to %.8x
          exit(0);
        }
        
        switch (d->op){
          case 0x23: //lw
            return mips.memory[(val - 0x00400000) / 2];
            
          case 0x2b: //sw
            *changedMem = val;
            mips.memory[(val - 0x00400000) / 2] = mips.registers[d->regs.i.rt];
            break;
        }
        
        return val;
      return 0;
    }

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    switch (d->type) { //type should be saved but need to run test to make sure, if not will change switch and cases
          case R:
        *changedReg = d->regs.r.rd;
              mips.registers[d->regs.r.rd] = val;
              return;
        
      case J:
        *changedReg = 31;
              mips.registers[31] = val;
              return;
        
      case I:
        *changedReg = d->regs.i.rt;
              mips.registers[d->regs.i.rt] = val;
              return;
    }
}
