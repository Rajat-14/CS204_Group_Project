# RISC-V Assembler

## Overview

The RISC-V Assembler is a robust tool designed to convert RISC-V assembly language into corresponding machine code. It is ideal for both academic and professional environments, providing an accurate translation from human-readable assembly code to executable binary instructions.

## Group Members

- Rajat Bhasin
- Pradeep Singh
- Rohit Verma

## Project Description

This project implements a RISC-V assembler capable of converting `.asm` files into `.mc` files.

### Key Features

- **Instruction Set Support:**  
  Processes all basic R-type, I-type, UJ-type, SB-type, S-type, and U-type instructions.
  
- **Data Directives:**  
  Handles directives such as `.word`, `.byte`, `.half`, `.asciiz`, and `.dword` for defining and initializing data.
  
- **Label and Memory Address Resolution:**  
  Efficiently resolves labels and calculates accurate memory addresses.
  
- **Segment Handling:**  
  Differentiates between text (code) and data segments for organized output.

## Input and Output

- **Input File:** `input.asm`  
  Contains RISC-V assembly code, which may include both text (instructions) and data segments (directives).

- **Output File:** `output.mc`  
  - **Machine Code:** Binary representation of the instructions.
  - **Memory Contents:** Visualization of the data section, with memory addresses and corresponding data in hexadecimal.
  - **Address-Annotated Instructions:** Each instruction is labeled with its address, aiding in debugging and verification.

## Working Principle

The assembler is built on a two-pass architecture to ensure precise conversion and address resolution.

### First Pass (preParse)
- **Label Address Table:**  
  Scans the assembly code to build a table of labels and their corresponding addresses.
- **Data Directive Processing:**  
  Handles the processing of data directives, setting up the data segment.
- **Address Calculation:**  
  Computes the address of each instruction, which is essential for correctly linking labels during the second pass.

### Second Pass (assemble)
- **Machine Code Generation:**  
  Converts the assembly instructions into binary machine code.
- **Final Output Assembly:**  
  Compiles the processed data and instructions into a formatted output file, complete with addresses and comments.

## Detailed Component Descriptions

### Lexer Implementation

The `lex()` function is central to the assembler's operation. It processes the input file by:
- **Skipping Whitespace and Comments:**  
  Ignores irrelevant characters and comments to focus on the actual code.
- **Token Recognition:**  
  Accurately identifies:
  - **Registers:** Recognizes both numeric registers (`x0–x31`) and their aliases (e.g., `sp`, `ra`).
  - **Operators:** Detects operation codes such as `add`, `lw`, `jal`, etc.
  - **Labels:** Processes labels (e.g., `main:`) used for branching and jumping.
  - **Directives:** Identifies directives like `.text` and `.data` to distinguish between code and data.
  - **Immediate Values:** Supports immediate values in both decimal and hexadecimal formats.
- **Error Resilience:**  
  Uses recursive parsing to handle errors gracefully, ensuring that the assembler can manage unexpected tokens without crashing.

### Assembler Function

The `assemble()` function is responsible for the overall coordination of the conversion process:
- **Segment Management:**  
  Differentiates between text and data segments, ensuring that each is processed in its proper context.
- **Token Coordination:**  
  Works with the lexer to process tokens and translate them into machine code.
- **Output Formatting:**  
  Writes the output in a user-friendly format, including:
  - Hexadecimal addresses.
  - Binary machine code.
  - Original assembly comments.
  - A clear visualization of data stored in memory.

### processInstruction Function

This is the core function that translates individual instructions:
- **Instruction Type Identification:**  
  Determines whether an instruction is R-type, I-type, S-type, or U-type.
- **Operand Extraction:**  
  Extracts necessary operands (registers, immediates, etc.) from the tokenized input.
- **Binary Conversion:**
  - **Register Mapping (`regMap`):** Converts register identifiers to binary codes.
  - **Immediate Conversion (`immToBinary`):** Translates immediate values into binary.
  - **Instruction Mapping (`instMap`):** Uses predefined mappings to obtain opcode and function fields.
- **Machine Code Construction:**  
  Combines all parts to construct a complete 32-bit machine code word.

### Data Memory and Directives Handling

The assembler also provides comprehensive support for data handling:
- **Data Section Processing:**  
  Processes data directives such as `.word`, `.byte`, `.half`, and `.dword`. The values are stored in a memory map beginning at a base address (`0x10000000`).
- **Little-Endian Ordering:**  
  Ensures that multi-byte data is stored using little-endian format, which is standard on many architectures.

## Conclusion

The RISC-V Assembler is a comprehensive tool that lays a solid foundation for RISC-V assembly language processing. Its key strengths include:

- **Complete Instruction Set Support:**  
  Ensures compatibility with all basic RISC-V instructions.
- **Efficient Data Memory Management:**  
  Handles various data directives with clear memory mapping.
- **Human-Readable Output:**  
  Provides an output that is both machine-executable and easy to interpret for debugging purposes.
- **Modular and Extensible Architecture:**  
  Designed for ease of future enhancements and integration with other systems.

## Future Improvements

Potential areas for future development include:
- **Support for Pseudo-Instructions:**  
  Expanding the assembler to recognize and process pseudo-instructions.
- **Enhanced Error Reporting:**  
  Offering more detailed and user-friendly error messages.
- **ELF File Format Output:**  
  Supporting the output in the ELF format for better integration with modern toolchains.
- **Floating-Point Instruction Support:**  
  Adding support for floating-point operations to cover a wider range of applications.
