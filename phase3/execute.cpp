#include "simulator.h"
#include "utils.h"
#include <sstream>

void DataHazard()
{
    // we have to identify producer consumer relationship
    // producer can be ex_mem or mem_wb 
    //consumer will be in id_ex stage
    if(id_ex.rs1 == ex_mem.rd || id_ex.rs2 == ex_mem.rd){
        if(!enableDataForwarding){
            // if data forwarding is not enabled, we need to stall the pipeline
            if_id.isValid = false; // stall the IF stage
            id_ex.valid = false; // stall the ID stage
            ex_mem.valid = true; // allow EX stage to proceed
            cout << "Stalling pipeline due to data hazard\n";
            stallCount++;
        }
        else{
            // data forwarding is enabled, we can forward the data from ex_mem to id_ex
            if_id.isValid = true; 
            if(id_ex.rs1 == ex_mem.rd){
                id_ex.operandA = ex_mem.aluResult; // forward data to operandA
            }else{
                id_ex.operandB = ex_mem.aluResult; // forward data to operandB
            }
        }
    }else if(id_ex.rs1 == mem_wb.rd || id_ex.rs2 == mem_wb.rd){
        // we have RAW data dependency with mem_wb stage
        if(!enableDataForwarding){
            // we need to stall the pipeline
            if_id.isValid = false; // stall the IF stage
            id_ex.valid = false; // stall the ID stage
            ex_mem.valid = true; // allow EX stage to proceed
            cout << "Stalling pipeline due to data hazard\n";
            stallCount++;
        }
        else{
            // data forwarding is enabled, we can forward the data from mem_wb to id_ex
            if(id_ex.rs1 == mem_wb.rd){
                id_ex.operandA = mem_wb.writeData; // forward data to operandA
            }else{
                id_ex.operandB = mem_wb.writeData; // forward data to operandB
            }
        }
    }
    else{
        // no data hazard detected, allow the pipeline to proceed
        if_id.isValid = true; 
        id_ex.valid = true; 
        ex_mem.valid = true; 
        mem_wb.valid = true;
    }
}



string IAG()
{
    // Convert current PC hex to integer.
    unsigned int currentPC = convert_hexstr2int(currentPC_str, 32);
    unsigned int nextPC = currentPC + 4; // Default next PC if no branch or jump instructions

    // Adjust nextPC for branch and jump instructions.
    if (ir.opcode == 0x63)
    { // Branch instructions.
        bool branchTaken = false;
        if (aluResult == 1)
        {
            branchTaken = true;
        }
        if (branchTaken)
        {
            nextPC = currentPC + ir.imm;
        }
    }
    else if (ir.opcode == 0x6F)
    { // JAL
        returnAddress = currentPC + 4;
        nextPC = currentPC + ir.imm;
    }
    else if (ir.opcode == 0x67)
    {                                       // JALR
        nextPC = (R[ir.rs1] + ir.imm) & ~1; // Ensure the target address is even.
    }

    return convert_PC2hex(nextPC);
}

void fetch()
{
    string pc = currentPC_str;
    instructionRegister = instruction_map[pc];
    cout << "FETCH: Instruction " << instructionRegister << "from PC = " << pc << endl;
    clockCycle++;
}

void decode()
{
    int instruction = convert_binstr2int(instructionRegister);
    ir.opcode = instruction & 0x7F;
    ir.rd = (instruction >> 7) & 0x1F;
    ir.funct3 = (instruction >> 12) & 0x7;
    ir.rs1 = (instruction >> 15) & 0x1F;
    ir.rs2 = (instruction >> 20) & 0x1F;
    ir.funct7 = (instruction >> 25) & 0x7F;
    ir.imm = 0;

    switch (ir.opcode)
    {
    // R-type
    case 0x33:
        if (ir.funct3 == 0 && ir.funct7 == 0)
            ir.operation = "ADD";
        else if (ir.funct3 == 0 && ir.funct7 == 0x20)
            ir.operation = "SUB";
        else if (ir.funct3 == 7)
            ir.operation = "AND";
        else if (ir.funct3 == 0 && ir.funct7 == 0x01)
            ir.operation = "MUL";
        else if (ir.funct3 == 6 && ir.funct7 == 0)
            ir.operation = "OR";
        else if (ir.funct3 == 6 && ir.funct7 == 0x01)
            ir.operation = "REM";
        else if (ir.funct3 == 4 && ir.funct7 == 0)
            ir.operation = "XOR";
        else if (ir.funct3 == 4 && ir.funct7 == 0x01)
            ir.operation = "DIV";
        else if (ir.funct3 == 1)
            ir.operation = "SLL";
        else if (ir.funct3 == 5 && ir.funct7 == 0)
            ir.operation = "SRL";
        else if (ir.funct3 == 5 && ir.funct7 == 0x20)
            ir.operation = "SRA";
        else if (ir.funct3 == 2)
            ir.operation = "SLT";
        else
            ir.operation = "UNKNOWN R-TYPE";

        cout << "DECODE: Operation is " << ir.operation << ", first operand " << R[ir.rs1] << ", second operand " << R[ir.rs2] << ", destination register " << ir.rd << endl;
        break;

    // I-type
    case 0x13:
        ir.imm = (instruction >> 20) & 0xFFF; // Extract 12-bit immediate.
        if (ir.imm & 0x800)
            ir.imm |= 0xFFFFF000;
        if (ir.funct3 == 0)
            ir.operation = "ADDI";
        else if (ir.funct3 == 7)
            ir.operation = "ANDI";
        else if (ir.funct3 == 6)
            ir.operation = "ORI";
        else if (ir.funct3 == 4)
            ir.operation = "XORI";
        else if (ir.funct3 == 2)
            ir.operation = "SLTI";
        else if (ir.funct3 == 1)
            ir.operation = "SLLI";
        else if (ir.funct3 == 5)
            ir.operation = "SRLI";
        else
            ir.operation = "UNKNOWN I-TYPE";
        cout << "DECODE: Operation is " << ir.operation << ", first operand " << R[ir.rs1] << ", immediate value " << ir.imm << ", destination register " << ir.rd << endl;
        break;

    // I-type Load instructions
    case 0x03:
        ir.imm = (instruction >> 20) & 0xFFF;
        if (ir.imm & 0x800)
            ir.imm |= 0xFFFFF000;
        if (ir.funct3 == 0)
            ir.operation = "LB";
        else if (ir.funct3 == 1)
            ir.operation = "LH";
        else if (ir.funct3 == 2)
            ir.operation = "LW";
        else if (ir.funct3 == 3)
            ir.operation = "LD";
        else
            ir.operation = "UNKNOWN LOAD";
        cout << "DECODE: Operation is " << ir.operation << ", base register " << R[ir.rs1] << ", offset " << ir.imm << ", destination register " << ir.rd << endl;
        break;

    // JALR instruction (I-type)
    case 0x67:
        ir.imm = (instruction >> 20) & 0xFFF;
        if (ir.imm & 0x800)
            ir.imm |= 0xFFFFF000;
        ir.operation = "JALR";
        cout << "DECODE: Operation is " << ir.operation << ", first operand " << R[ir.rs1] << ", immediate value " << ir.imm << ", destination register " << ir.rd << endl;
        break;

    // S-type
    case 0x23:
        // Immediate is split between bits [11:7] and [31:25]
        ir.imm = (((instruction >> 7) & 0x1F) | (((instruction >> 25) & 0x7F) << 5));
        if (ir.imm & 0x800)
            ir.imm |= 0xFFFFF000;
        if (ir.funct3 == 0)
            ir.operation = "SB";
        else if (ir.funct3 == 1)
            ir.operation = "SH";
        else if (ir.funct3 == 2)
            ir.operation = "SW";
        else if (ir.funct3 == 3)
            ir.operation = "SD";
        else
            ir.operation = "UNKNOWN S-TYPE";
        cout << "DECODE: Operation is " << ir.operation << ", base register " << R[ir.rs1] << ", second register " << R[ir.rs2] << ", offset " << ir.imm << endl;
        break;

    // SB-type
    case 0x63:
        cout << instruction << endl;

        ir.imm = (((instruction >> 7) & 0x1) << 11) |
                 (((instruction >> 8) & 0xF) << 1) |
                 (((instruction >> 25) & 0x3F) << 5) |
                 (((instruction >> 31) & 0x1) << 12);
        if (ir.imm & 0x1000)
            ir.imm |= 0xFFFFE000;
        if (ir.funct3 == 0)
            ir.operation = "BEQ";
        else if (ir.funct3 == 1)
            ir.operation = "BNE";
        else if (ir.funct3 == 4)
            ir.operation = "BLT";
        else if (ir.funct3 == 5)
            ir.operation = "BGE";
        else
            ir.operation = "UNKNOWN BRANCH";
        cout << "DECODE: Operation is " << ir.operation << ", first operand " << R[ir.rs1] << ", second operand " << R[ir.rs2] << ", branch offset " << ir.imm << endl;
        break;

    // U-type lui
    case 0x37:
        ir.imm = instruction & 0xFFFFF000;
        ir.operation = "LUI";
        cout << "DECODE: Operation is " << ir.operation << ", destination register " << ir.rd << ", immediate " << ir.imm << endl;
        break;

    // U-type auipc
    case 0x17:
        ir.imm = instruction & 0xFFFFF000;
        ir.operation = "AUIPC";
        cout << "DECODE: Operation is " << ir.operation << ", destination register " << ir.rd << ", immediate " << ir.imm << endl;
        break;

    // UJ-type
    case 0x6F:
        ir.imm = (((instruction >> 21) & 0x3FF) << 1) |
                 (((instruction >> 20) & 0x1) << 11) |
                 (((instruction >> 12) & 0xFF) << 12) |
                 (((instruction >> 31) & 0x1) << 20);
        if (ir.imm & 0x100000)
            ir.imm |= 0xFFE00000; // Sign extend to 32 bits

        ir.operation = "JAL";
        cout << "DECODE: Operation is " << ir.operation << ", destination register " << ir.rd << ", jump offset " << ir.imm << endl;
        break;

    default:
        cout << "DECODE: Unsupported ir.opcode 0x" << hex << ir.opcode << dec << endl;
        break;
    }
    clockCycle++;
}

// Mux A selection: choose either register ir.rs1 or the current PC.
int selectoperandA(bool usePCForA)
{
    int operandA;
    if (usePCForA)
    {
        operandA = stoul(currentPC_str, nullptr, 16);
    }
    else
    {
        operandA = R[ir.rs1];
    }
    return operandA;
}

// Mux B selection: choose either register ir.rs2 or the immediate value.
int selectoperandB(bool useImmediateForB)
{
    int operandB;
    if (useImmediateForB)
    {
        operandB = ir.imm;
    }
    else
    {
        operandB = R[ir.rs2];
    }
    return operandB;
}

void execute()
{
    is_PCupdated_while_execution = false;
    bool usePCForA = false;
    bool useImmediateForB = false;
    int operandB = selectoperandB(useImmediateForB);
    int operandA = selectoperandA(usePCForA);

    if (ir.opcode == 0x33)
    { // R-type
        if (ir.operation == "ADD")
            aluResult = operandA + operandB;
        else if (ir.operation == "SUB")
            aluResult = operandA - operandB;

        else if (ir.operation == "AND")
            aluResult = operandA & operandB;
        else if (ir.operation == "MUL")
            aluResult = operandA * operandB;
        else if (ir.operation == "DIV")
            aluResult = operandA / operandB;
        else if (ir.operation == "REM")
            aluResult = operandA % operandB;
        else if (ir.operation == "OR")
            aluResult = operandA | operandB;
        else if (ir.operation == "XOR")
            aluResult = operandA ^ operandB;
        else if (ir.operation == "SLL")
            aluResult = operandA << (operandB & 0x1F);
        else if (ir.operation == "SRL")
            aluResult = (unsigned)operandA >> (operandB & 0x1F);
        else if (ir.operation == "SRA")
            aluResult = operandA >> (operandB & 0x1F);
        else if (ir.operation == "SLT")
            aluResult = (operandA < operandB) ? 1 : 0;
        else
            cout << "EXECUTE: Unsupported R-type ir.operation." << endl;
        cout << "EXECUTE: R-type operation result is " << aluResult << endl;
    }
    else if (ir.opcode == 0x13)
    { // I-type
        useImmediateForB = true;
        operandB = selectoperandB(useImmediateForB);
        if (ir.funct3 == 0) // ADDI
            aluResult = operandA + operandB;
        else if (ir.funct3 == 7) // ANDI
            aluResult = operandA & operandB;
        else if (ir.funct3 == 6) // ORI
            aluResult = operandA | operandB;
        else if (ir.funct3 == 4) // XORI
            aluResult = operandA ^ operandB;
        else if (ir.funct3 == 2) // SLTI
            aluResult = (operandA < operandB) ? 1 : 0;
        else if (ir.funct3 == 1) // SLLI
            aluResult = operandA << (operandB & 0x1F);
        else if (ir.funct3 == 5) // SRLI
            aluResult = (unsigned)operandA >> (operandB & 0x1F);
        else
            cout << "EXECUTE: Unsupported I-type ALU ir.operation." << endl;
        cout << "EXECUTE: I-type operation result is " << aluResult << endl;
    }
    else if (ir.opcode == 0x03)
    { // Load instruction
        useImmediateForB = true;
        operandB = selectoperandB(useImmediateForB);
        aluResult = operandA + operandB; // EA
        cout << "EXECUTE: Effective address is " << aluResult << endl;
    }
    else if (ir.opcode == 0x23)
    {                  // Store instructions (S-type)
        RM = operandB; // here operandB=R[ir.rs2] that is value to be stored
        useImmediateForB = true;
        operandB = selectoperandB(useImmediateForB); // now operandB is offset
        aluResult = operandA + operandB;             // EA
        cout << "EXECUTE: Effective address is " << aluResult << endl;
    }
    else if (ir.opcode == 0x63)
    { // Branch instructions (SB-type)
        aluResult = 0;
        if (ir.funct3 == 0 && (R[ir.rs1] == R[ir.rs2])) // BEQ
            aluResult = 1;
        else if (ir.funct3 == 1 && (R[ir.rs1] != R[ir.rs2])) // BNE
            aluResult = 1;
        else if (ir.funct3 == 4 && (R[ir.rs1] < R[ir.rs2])) // BLT
            aluResult = 1;
        else if (ir.funct3 == 5 && (R[ir.rs1] >= R[ir.rs2])) // BGE
            aluResult = 1;
        else
            aluResult = 0;
        currentPC_str = IAG();
        is_PCupdated_while_execution = true;
        cout << "EXECUTE: Branch operation result is " << aluResult << endl;
    }
    else if (ir.opcode == 0x37)
    { // LUI
        aluResult = ir.imm;
        cout << "EXECUTE: LUI operation result is " << aluResult << endl;
    }
    else if (ir.opcode == 0x17)
    { // AUIPC
        // ir.imm is shifted left by 12 bits and added to PC.
        aluResult = ir.imm;

        aluResult = aluResult + convert_hexstr2int(currentPC_str, 32);
        cout << "EXECUTE: AUIPC operation result is " << aluResult << endl;
    }
    else if (ir.opcode == 0x6F)
    { // JAL
        // IAG povides add stores current PC + 4 in returnAddress
        currentPC_str = IAG();
        is_PCupdated_while_execution = true;
        cout << "EXECUTE: NO ALU operation required for JAL\n";
    }
    else if (ir.opcode == 0x67)
    { // JALR
        // IAG update PC as per JALR instruction
        currentPC_str = IAG();
        is_PCupdated_while_execution = true;
        cout << "EXECUTE: NO ALU operation required for JALR\n";
    }
    else
    {
        cout << "EXECUTE: Unsupported ir.opcode in ALU simulation." << endl;
    }
    clockCycle++;
}

void memory_access()
{
    // Load instruction
    if (ir.opcode == 0x03)
    {
        // we have to check lw,lb,lh,ld
        string addressStr;
        string res = "";
        if (ir.funct3 == 0)
        { // LB
            addressStr = convert_int2hexstr(aluResult, 32);
            res += data_map[addressStr];
            memoryData = convert_hexstr2int(res, 8);
        }
        else if (ir.funct3 == 1)
        { // LH
            long long int x = aluResult + 1;
            while (x >= aluResult)
            {
                addressStr = convert_int2hexstr(x, 32);
                res += data_map[addressStr];
                x--;
            }
            memoryData = convert_hexstr2int(res, 16);
        }
        else if (ir.funct3 == 2)
        { // LW
            long long int x = aluResult + 3;
            while (x >= aluResult)
            {
                addressStr = convert_int2hexstr(x, 32);
                res += data_map[addressStr];
                x--;
            }
            memoryData = convert_hexstr2int(res, 32);
        }
        else if (ir.funct3 == 3)
        { // LD
            long long int x = aluResult + 7;
            while (x >= aluResult)
            {
                addressStr = convert_int2hexstr(x, 32);
                res += data_map[addressStr];
                x--;
            }
            memoryData = convert_hexstr2int(res, 64);
        }
        cout << "MEM: Loaded data " << memoryData << " from memory address " << aluResult << endl;
    }
    // Store instruction
    else if (ir.opcode == 0x23)
    {
        string addressStr;
        string data;
        long long int int_address = aluResult;
        if (ir.funct3 == 0)
        { // SB
            data = convert_int2hexstr(RM, 8);
            addressStr = convert_int2hexstr(int_address, 32);
            data_map[addressStr] = data.substr(2, 2);
        }
        else if (ir.funct3 == 1)
        { // SH
            data = convert_int2hexstr(RM, 16);
            for (int i = 1; i >= 0; --i)
            {
                addressStr = convert_int2hexstr(int_address, 32);
                int_address += 1;
                data_map[addressStr] = data.substr(2 * i + 2, 2);
            }
        }
        else if (ir.funct3 == 2)
        { // SW
            data = convert_int2hexstr(RM, 32);
            for (int i = 3; i >= 0; --i)
            {
                addressStr = convert_int2hexstr(int_address, 32);
                int_address += 1;
                data_map[addressStr] = data.substr(2 * i + 2, 2);
            }
        }
        else if (ir.funct3 == 3)
        { // SD
            data = convert_int2hexstr(RM, 64);
            for (int i = 7; i >= 0; --i)
            {
                addressStr = convert_int2hexstr(int_address, 32);
                int_address += 1;
                data_map[addressStr] = data.substr(2 * i + 2, 2);
            }
        }
        cout << "MEM: Stored data " << RM << " at memory address " << aluResult << endl;
    }
    else
    {
        cout << "MEM: No memory access required\n";
    }

    // now we have mux Y with input as aluResult or memroyData or returnAddress
    if (ir.opcode == 0x03)
    { // Load instruction
        RY = memoryData;
    }
    else if (ir.opcode == 0x6F)
    { // JAL
        RY = returnAddress;
    }
    else if (ir.opcode == 0x67)
    { // JALR
        RY = returnAddress;
    }
    else if (ir.opcode == 0x23)
    { // Store instruction
        RY = 0;
    }
    else if (ir.opcode == 0x63)
    {
        RY = 0;
    }
    else
    {
        RY = aluResult;
    }
    clockCycle++;
    return;
}

void write_back()
{
    if (ir.opcode == 0x23 || ir.opcode == 0x63)
    {
        cout << "WB: No write back required\n";
    }
    else
    {
        R[ir.rd] = RY;
        cout << "WB: Wrote " << RY << " to R[" << ir.rd << "]\n";
    }
    R[0] = 0; // x0 is always 0
    clockCycle++;
    return;
}
