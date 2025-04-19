#include "simulator.h"
#include "utils.h"
#include <sstream>

void controlHazard()
{
    // id_ex.signal == "100" for branch instructions
    // id_ex.signal == "111" for JAL or JALR instructions
    if (id_ex.signal == "100" || id_ex.signal == "111")
    {
        // flush pipeline only if branch taken
        // now check AluResult for branch taken or not
        if (aluResult == 1)
        {
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            cout << "Flushed pipeline due to branch taken\n";
            clockCycle++;
            write_back();
            memory_access();
            fetch();
            is_PCupdated_while_execution = false;
            // now in next cycle there should be no MA,no EXE.
            if (is_PCupdated_while_execution == false)
            {
                currentPC_str = IAG();
            }
            else
            {
                is_PCupdated_while_execution = false;
            }
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";

            clockCycle++;
            write_back();
            if (!pipelineEnd)
            { // if pc is at terminate so we will not fetch again and not decode it
                id_ex = {0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", 0, false, false, "", false, false, 0, false, 0};
                ex_mem = {0, 0, false, false, false, 0, false, 0};
                mem_wb = {0, 0, false, false};
                decode();
                fetch();
            }
            ex_mem.valid = false;
            mem_wb.valid = false;
            isFlushingDone = true;
        }
    }
}

void DataHazard()
{
    // we  identify producer consumer relationship
    // producer can be ex_mem or mem_wb
    // consumer will be in id_ex stage
    if (ex_mem.valid && ((id_ex.rs1 == ex_mem.rd && id_ex.rs1 != 0) || (id_ex.rs2 == ex_mem.rd && id_ex.rs1 != 0)))
    {
        if (!enableDataForwarding)
        {
            // if data forwarding is not enabled, we need to stall the pipeline
            numStallNeeded = 2;
            cout << "Stalling pipeline due to data hazard\n";
            stallCount += 2;
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
        }
        else
        {
            // data forwarding is enabled, we can forward the data from ex_mem to id_ex
            // we have to check for special case when rd is from a load instruction
            // in special case one stall will be still needed
            if_id.isValid = true;

            if (id_ex.rs1 == ex_mem.rd && id_ex.rs1 != 0)
            {
                // we have to check if ex_mem is a load instruction
                // if yes then we need to stall the pipeline for one cycle
                // check if ex_mem is a load instruction
                if (ex_mem.memoryAccess && ex_mem.rd != 0)
                {
                    // we have to stall the pipeline for one cycle
                    stallingWhileDataForwarding = true;
                    cout << "Stalling pipeline due to Data Forwarding\n";
                    clockCycle++;
                    write_back();
                    memory_access();
                    fetch();
                    cout << "Clock: " << clockCycle << endl;
                    for (auto it : R)
                    {
                        cout << it << " ";
                    }
                    cout << endl;
                    cout << "----------------------------------------------------------------------------------------------------\n";
                    if (is_PCupdated_while_execution == false)
                    {
                        currentPC_str = IAG();
                    }
                    else
                    {
                        is_PCupdated_while_execution = false;
                    }
                    // doing write back for next cycle in this one only and keeping flow same for next cycles
                    write_back();
                    // now our correct rd value is in mem_wb stage
                    // so forward this correct value to id_ex stage
                    id_ex.operandA = mem_wb.writeData;
                    ex_mem.valid = false;
                    mem_wb.valid = false;
                }
                else
                {
                    // we can forward the data directly
                    id_ex.operandA = ex_mem.aluResult; // forward data to operandA
                    cout << "Forwarding data from ex_mem to id_ex for rs1\n";
                }
            }
            else
            {
                if (ex_mem.memoryAccess && ex_mem.rd != 0)
                {
                    // we have to stall the pipeline for one cycle
                    stallingWhileDataForwarding = true;
                    cout << "Stalling pipeline due to Data Forwarding\n";
                    clockCycle++;
                    write_back();
                    memory_access();
                    fetch();
                    cout << "Clock: " << clockCycle << endl;
                    for (auto it : R)
                    {
                        cout << it << " ";
                    }
                    cout << endl;
                    cout << "----------------------------------------------------------------------------------------------------\n";
                    if (is_PCupdated_while_execution == false)
                    {
                        currentPC_str = IAG();
                    }
                    else
                    {
                        is_PCupdated_while_execution = false;
                    }
                    // doing write back for next cycle in this one only and keeping flow same for next cycles
                    write_back();
                    // now our correct rd value is in mem_wb stage
                    // so forward this correct value to id_ex stage
                    id_ex.operandB = mem_wb.writeData;
                    ex_mem.valid = false;
                    mem_wb.valid = false;
                }
                else
                {
                    // we can forward the data directly
                    id_ex.operandB = ex_mem.aluResult; // forward data to operandB
                    cout << "Forwarding data from ex_mem to id_ex for rs2\n";
                }
            }
        }
    }
    else if (mem_wb.valid && ((id_ex.rs1 == mem_wb.rd && id_ex.rs1 != 0) || (id_ex.rs2 == mem_wb.rd && id_ex.rs2 != 0)))
    {
        // we have RAW data dependency with mem_wb stage
        if (!enableDataForwarding)
        {
            // we need to stall the pipeline
            numStallNeeded = 1;

            cout << "Stalling pipeline due to data hazard\n";
            stallCount += 1;
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
        }
        else
        {
            // data forwarding is enabled, we can forward the data from mem_wb to id_ex
            if (id_ex.rs1 == mem_wb.rd)
            {
                id_ex.operandA = mem_wb.writeData; // forward data to operandA
                cout << "Forwarding data from mem_wb to id_ex for rs1\n";
            }
            else
            {
                id_ex.operandB = mem_wb.writeData; // forward data to operandB
                cout << "Forwarding data from mem_wb to id_ex for rs2\n";
            }
        }
    }
}

int ALU(int operand1, int operand2, int control)
{
    switch (control)
    {
    case 1: // ADD
        return operand1 + operand2;
        break;
    case 2: // SUB
        return operand1 - operand2;
        break;
    case 3: // AND
        return operand1 & operand2;
        break;
    case 4: // MUL
        return operand1 * operand2;
        break;
    case 5: // OR
        return operand1 | operand2;
        break;
    case 6: // REM
        return operand1 % operand2;
        break;
    case 7: // XOR
        return operand1 ^ operand2;
        break;
    case 8: // DIV
        return operand1 / operand2;
        break;
    case 9: // SLL
        return operand1 << (operand2 & 0x1F);
        break;
    case 10: // SRL
        return (unsigned)operand1 >> (operand2 & 0x1F);
        break;
    case 11: // SRA
        return operand1 >> (operand2 & 0x1F);
        break;
    case 12: // SLT
        return (operand1 < operand2) ? 1 : 0;
        break;
    case 13: // BEQ
        return (operand1 == operand2) ? 1 : 0;
        break;
    case 14: // BNE
        return (operand1 != operand2) ? 1 : 0;
        break;
    case 15: // BLT
        return (operand1 < operand2) ? 1 : 0;
        break;
    case 16: // BGE
        return (operand1 >= operand2) ? 1 : 0;
        break;
    }
    return -1;
}

string IAG()
{
    // Convert current PC hex to integer.
    unsigned int currentPC = convert_hexstr2int(currentPC_str, 32);
    unsigned int nextPC = currentPC + 4; // Default next PC if no branch or jump instructions

    // Adjust nextPC for branch and jump instructions.
    if (id_ex.signal == "100" && is_PCupdated_while_execution)
    { // Branch instructions.
        bool branchTaken = false;
        if (aluResult == 1)
        {
            branchTaken = true;
        }
        if (branchTaken)
        {
            nextPC = currentPC + id_ex.imm - 8;
        }
        else
        {
            nextPC = currentPC;
            is_PCupdated_while_execution = false;
            isFlushingDone = false;
        }
    }
    else if (id_ex.operation == "JAL" && is_PCupdated_while_execution)
    { // JAL
        returnAddress = currentPC - 4;
        nextPC = currentPC + id_ex.imm - 8;
    }
    else if (id_ex.operation == "JALR" && is_PCupdated_while_execution)
    {                                             // JALR
        nextPC = (R[id_ex.rs1] + id_ex.imm) & ~1; // Ensure the target address is even.
        returnAddress = currentPC - 4;
    }
    return convert_PC2hex(nextPC);
}

void fetch()
{
    string pc = currentPC_str;
    instructionRegister = instruction_map[pc];
    if (instructionRegister == "Terminate" || instructionRegister == "")
    {
        if_id.isValid = false;
        pipelineEnd = true;
        cout << "FETCH: Instruction " << instructionRegister << "from PC = " << pc << endl;
        return;
    }
    pipelineEnd = false;
    if_id.instruction = instructionRegister;
    if_id.pc = currentPC_str;
    if_id.isValid = true;
    cout << "FETCH: Instruction " << instructionRegister << "from PC = " << pc << endl;
    // clockCycle++;
}

void decode()
{

    if (if_id.isValid)
    {
        int instruction = convert_binstr2int(if_id.instruction);
        id_ex.opcode = instruction & 0x7F;
        id_ex.rd = (instruction >> 7) & 0x1F;
        id_ex.funct3 = (instruction >> 12) & 0x7;
        id_ex.rs1 = (instruction >> 15) & 0x1F;
        id_ex.rs2 = (instruction >> 20) & 0x1F;
        id_ex.funct7 = (instruction >> 25) & 0x7F;
        id_ex.imm = 0;
        id_ex.useImmediateForB = false;
        id_ex.memoryAccess = false;
        id_ex.memoryRequest = false;
        id_ex.writeBack = true;
        id_ex.controlMuxY = 0;
        switch (id_ex.opcode)
        {
        // R-type
        case 0x33:
            id_ex.signal = "000";
            id_ex.useImmediateForB = false;
            if (id_ex.funct3 == 0 && id_ex.funct7 == 0)
            {
                id_ex.operation = "ADD";
                id_ex.aluOperation = 1;
            }
            else if (id_ex.funct3 == 0 && id_ex.funct7 == 0x20)
            {
                id_ex.operation = "SUB";
                id_ex.aluOperation = 2;
            }
            else if (id_ex.funct3 == 7)
            {
                id_ex.operation = "AND";
                id_ex.aluOperation = 3;
            }
            else if (id_ex.funct3 == 0 && id_ex.funct7 == 0x01)
            {
                id_ex.operation = "MUL";
                id_ex.aluOperation = 4;
            }
            else if (id_ex.funct3 == 6 && id_ex.funct7 == 0)
            {
                id_ex.operation = "OR";
                id_ex.aluOperation = 5;
            }
            else if (id_ex.funct3 == 6 && id_ex.funct7 == 0x01)
            {
                id_ex.operation = "REM";
                id_ex.aluOperation = 6;
            }
            else if (id_ex.funct3 == 4 && id_ex.funct7 == 0)
            {
                id_ex.operation = "XOR";
                id_ex.aluOperation = 7;
            }
            else if (id_ex.funct3 == 4 && id_ex.funct7 == 0x01)
            {
                id_ex.operation = "DIV";
                id_ex.aluOperation = 8;
            }
            else if (id_ex.funct3 == 1)
            {
                id_ex.operation = "SLL";
                id_ex.aluOperation = 9;
            }
            else if (id_ex.funct3 == 5 && id_ex.funct7 == 0)
            {
                id_ex.operation = "SRL";
                id_ex.aluOperation = 10;
            }
            else if (id_ex.funct3 == 5 && id_ex.funct7 == 0x20)
            {
                id_ex.operation = "SRA";
                id_ex.aluOperation = 11;
            }
            else if (id_ex.funct3 == 2)
            {
                id_ex.operation = "SLT";
                id_ex.aluOperation = 12;
            }
            else
                id_ex.operation = "UNKNOWN R-TYPE";

            cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", second operand " << R[id_ex.rs2] << ", destination register " << id_ex.rd << endl;
            break;

        // I-type
        case 0x13:
            // here rs2 will contain garbage value
            id_ex.rs2 = 0;
            id_ex.signal = "001";
            id_ex.imm = (instruction >> 20) & 0xFFF; // Extract 12-bit immediate.
            id_ex.useImmediateForB = true;
            if (id_ex.imm & 0x800)
                id_ex.imm |= 0xFFFFF000;
            if (id_ex.funct3 == 0)
            {
                id_ex.operation = "ADDI";
                id_ex.aluOperation = 1;
            }
            else if (id_ex.funct3 == 7)
            {
                id_ex.operation = "ANDI";
                id_ex.aluOperation = 3;
            }
            else if (id_ex.funct3 == 6)
            {
                id_ex.operation = "ORI";
                id_ex.aluOperation = 5;
            }
            else if (id_ex.funct3 == 4)
            {
                id_ex.operation = "XORI";
                id_ex.aluOperation = 7;
            }
            else if (id_ex.funct3 == 2)
            {
                id_ex.operation = "SLTI";
                id_ex.aluOperation = 12;
            }
            else if (id_ex.funct3 == 1)
            {
                id_ex.operation = "SLLI";
                id_ex.aluOperation = 9;
            }
            else if (id_ex.funct3 == 5)
            {
                id_ex.operation = "SRLI";
                id_ex.aluOperation = 10;
            }
            else
                id_ex.operation = "UNKNOWN I-TYPE";
            cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", immediate value " << id_ex.imm << ", destination register " << id_ex.rd << endl;
            break;

        // I-type Load instructions
        case 0x03:
            // here rs2 will contain garbage value
            id_ex.rs2 = 0;
            id_ex.signal = "001";
            id_ex.memoryAccess = true;
            id_ex.imm = (instruction >> 20) & 0xFFF;
            id_ex.useImmediateForB = true;
            id_ex.controlMuxY = 1;
            if (id_ex.imm & 0x800)
                id_ex.imm |= 0xFFFFF000;
            id_ex.aluOperation = 1;
            if (id_ex.funct3 == 0)
            {
                id_ex.operation = "LB";
                id_ex.size = 1;
            }
            else if (id_ex.funct3 == 1)
            {
                id_ex.operation = "LH";
                id_ex.size = 2;
            }
            else if (id_ex.funct3 == 2)
            {
                id_ex.operation = "LW";
                id_ex.size = 4;
            }
            else if (id_ex.funct3 == 3)
            {
                id_ex.operation = "LD";
                id_ex.size = 8;
            }
            else
                id_ex.operation = "UNKNOWN LOAD";
            cout << "DECODE: Operation is " << id_ex.operation << ", base register " << R[id_ex.rs1] << ", offset " << id_ex.imm << ", destination register " << id_ex.rd << endl;
            break;

        // JALR instruction (I-type)
        case 0x67:
            // here rs2 will contain garbage value
            id_ex.rs2 = 0;
            id_ex.useImmediateForB = true;
            id_ex.controlMuxY = 2;
            id_ex.imm = (instruction >> 20) & 0xFFF;
            id_ex.signal = "111";

            if (id_ex.imm & 0x800)
                id_ex.imm |= 0xFFFFF000;
            id_ex.operation = "JALR";
            cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", immediate value " << id_ex.imm << ", destination register " << id_ex.rd << endl;
            break;

        // S-type
        case 0x23:
            // both rs1 and rs2 are used, no rd is there
            id_ex.rd = 0;
            id_ex.signal = "011";
            id_ex.useImmediateForB = false;
            id_ex.memoryRequest = true;
            id_ex.controlMuxY = 3;
            // Immediate is split between bits [11:7] and [31:25]
            id_ex.imm = (((instruction >> 7) & 0x1F) | (((instruction >> 25) & 0x7F) << 5));
            id_ex.aluOperation = 1;
            id_ex.useImmediateForB = true;
            id_ex.writeBack = false;
            if (id_ex.imm & 0x800)
                id_ex.imm |= 0xFFFFF000;
            if (id_ex.funct3 == 0)
            {
                id_ex.operation = "SB";
                id_ex.size = 1;
            }
            else if (id_ex.funct3 == 1)
            {
                id_ex.operation = "SH";
                id_ex.size = 2;
            }
            else if (id_ex.funct3 == 2)
            {
                id_ex.operation = "SW";
                id_ex.size = 4;
            }
            else if (id_ex.funct3 == 3)
            {
                id_ex.operation = "SD";
                id_ex.size = 8;
            }
            else
                id_ex.operation = "UNKNOWN S-TYPE";
            cout << "DECODE: Operation is " << id_ex.operation << ", base register " << R[id_ex.rs1] << ", second register " << R[id_ex.rs2] << ", offset " << id_ex.imm << endl;
            break;

        // SB-type
        case 0x63:
            // no rd is there
            id_ex.rd = 0;
            id_ex.useImmediateForB = false;
            id_ex.writeBack = false;
            id_ex.controlMuxY = 3;
            cout << instruction << endl;
            id_ex.signal = "100";
            id_ex.imm = (((instruction >> 7) & 0x1) << 11) |
                        (((instruction >> 8) & 0xF) << 1) |
                        (((instruction >> 25) & 0x3F) << 5) |
                        (((instruction >> 31) & 0x1) << 12);
            if (id_ex.imm & 0x1000)
                id_ex.imm |= 0xFFFFE000;
            if (id_ex.funct3 == 0)
            {
                id_ex.operation = "BEQ";
                id_ex.aluOperation = 13;
            }
            else if (id_ex.funct3 == 1)
            {
                id_ex.operation = "BNE";
                id_ex.aluOperation = 14;
            }
            else if (id_ex.funct3 == 4)
            {
                id_ex.operation = "BLT";
                id_ex.aluOperation = 15;
            }
            else if (id_ex.funct3 == 5)
            {
                id_ex.operation = "BGE";
                id_ex.aluOperation = 16;
            }
            else
                id_ex.operation = "UNKNOWN BRANCH";
            cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", second operand " << R[id_ex.rs2] << ", branch offset " << id_ex.imm << endl;
            break;

        // U-type lui
        case 0x37:
            id_ex.rs1 = 0;
            id_ex.rs2 = 0;
            id_ex.useImmediateForB = true;
            id_ex.signal = "101";
            id_ex.imm = instruction & 0xFFFFF000;
            id_ex.operation = "LUI";
            cout << "DECODE: Operation is " << id_ex.operation << ", destination register " << id_ex.rd << ", immediate " << id_ex.imm << endl;
            break;

        // U-type auipc
        case 0x17:
            id_ex.rs1 = 0;
            id_ex.rs2 = 0;
            id_ex.useImmediateForB = false;
            id_ex.signal = "110";
            id_ex.imm = instruction & 0xFFFFF000;
            id_ex.operation = "AUIPC";
            cout << "DECODE: Operation is " << id_ex.operation << ", destination register " << id_ex.rd << ", immediate " << id_ex.imm << endl;
            break;

        // UJ-type
        case 0x6F:
            // no rs1,rs2 are there
            id_ex.rs1 = 0;
            id_ex.rs2 = 0;
            id_ex.useImmediateForB = false;
            id_ex.signal = "111";
            id_ex.controlMuxY = 2;
            id_ex.imm = (((instruction >> 21) & 0x3FF) << 1) |
                        (((instruction >> 20) & 0x1) << 11) |
                        (((instruction >> 12) & 0xFF) << 12) |
                        (((instruction >> 31) & 0x1) << 20);
            if (id_ex.imm & 0x100000)
                id_ex.imm |= 0xFFE00000; // Sign extend to 32 bits

            id_ex.operation = "JAL";
            cout << "DECODE: Operation is " << id_ex.operation << ", destination register " << id_ex.rd << ", jump offset " << id_ex.imm << endl;
            break;

        default:
            cout << "DECODE: Unsupported ir.opcode 0x" << hex << id_ex.opcode << dec << endl;
            break;
        }
        id_ex.valid = true;
        if (numStallNeeded == 0)
            DataHazard();
    }
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
        operandA = R[id_ex.rs1];
    }
    return operandA;
}

// Mux B selection: choose either register ir.rs2 or the immediate value.
int selectoperandB(bool useImmediateForB)
{
    int operandB;
    if (useImmediateForB)
    {
        operandB = id_ex.imm;
    }
    else
    {
        operandB = R[id_ex.rs2];
    }
    return operandB;
}

void MuxY(int control)
{
    switch (control)
    {
    case 0:
        RY = ex_mem.aluResult;
        break;
    case 1:
        RY = memoryData;
        break;
    case 2:
        RY = returnAddress;
        break;
    case 3:
        RY = 0;
        break;

    default:
        break;
    }
}
void execute()
{
    if (id_ex.valid)
    {
        is_PCupdated_while_execution = false;
        bool usePCForA = false;

        int operandA = selectoperandA(usePCForA);
        int operandB = selectoperandB(0);

        if (id_ex.signal == "000")
        { // R-type
            aluResult = ALU(operandA, operandB, id_ex.aluOperation);
            cout << "EXECUTE: R-type operation result is " << aluResult << endl;
        }
        else if (id_ex.signal == "001")
        { // I-type & Load
            // id_ex.useImmediateForB = true;
            operandB = selectoperandB(id_ex.useImmediateForB);
            aluResult = ALU(operandA, operandB, id_ex.aluOperation);
            cout << "EXECUTE: I-type operation result is " << aluResult << endl;
        }

        else if (id_ex.signal == "011")
        {                  // Store instructions (S-type)
            RM = operandB; // here operandB=R[ir.rs2] that is value to be stored
            // id_ex.useImmediateForB = true;
            operandB = selectoperandB(id_ex.useImmediateForB); // now operandB is offset
            aluResult = ALU(operandA, operandB, id_ex.aluOperation);
            cout << "EXECUTE: Effective address is " << aluResult << endl;
        }
        else if (id_ex.signal == "100")
        { // Branch instructions (SB-type)
            aluResult = ALU(operandA, operandB, id_ex.aluOperation);
            is_PCupdated_while_execution = true;
            currentPC_str = IAG();
            cout << "EXECUTE: Branch operation result is " << aluResult << endl;
        }
        else if (id_ex.signal == "101")
        { // LUI
            aluResult = id_ex.imm;
            cout << "EXECUTE: LUI operation result is " << aluResult << endl;
        }
        else if (id_ex.signal == "110")
        { // AUIPC
            // ir.imm is shifted left by 12 bits and added to PC.
            aluResult = id_ex.imm;
            aluResult = aluResult + convert_hexstr2int(currentPC_str, 32);
            cout << "EXECUTE: AUIPC operation result is " << aluResult << endl;
        }
        else if (id_ex.signal == "111")
        { // JAL & JALR
            // IAG povides add stores current PC + 4 in returnAddress
            aluResult = 1;
            is_PCupdated_while_execution = true;
            currentPC_str = IAG();
            cout << "EXECUTE: NO ALU operation required\n";
        }

        else
        {
            cout << "EXECUTE: Unsupported ir.opcode in ALU simulation." << endl;
        }
        ex_mem.aluResult = aluResult;

        ex_mem.rd = id_ex.rd;
        ex_mem.valid = true;
        ex_mem.memoryAccess = id_ex.memoryAccess;
        ex_mem.memoryRequest = id_ex.memoryRequest;
        ex_mem.size = id_ex.size;
        ex_mem.writeBack = id_ex.writeBack;
        ex_mem.controlMuxY = id_ex.controlMuxY;

        ex_mem.valid = true;
        if ((id_ex.operation == "JAL" || id_ex.operation == "JALR" || id_ex.signal == "100") && isFlushingDone == false)
        {
            controlHazard();
        }
        // clockCycle++;
    }
}

void memory_access()
{
    // Load instruction
    if (ex_mem.valid)
    {
        if (ex_mem.memoryAccess)
        {
            // we have to check lw,lb,lh,ld
            string addressStr;
            string res = "";
            if (ex_mem.size == 1)
            { // LB
                addressStr = convert_int2hexstr(ex_mem.aluResult, 32);
                res += data_map[addressStr];
                memoryData = convert_hexstr2int(res, 8);
            }
            else if (ex_mem.size == 2)
            { // LH
                long long int x = ex_mem.aluResult + 1;
                while (x >= ex_mem.aluResult)
                {
                    addressStr = convert_int2hexstr(x, 32);
                    res += data_map[addressStr];
                    x--;
                }
                memoryData = convert_hexstr2int(res, 16);
            }
            else if (ex_mem.size == 4)
            { // LW
                long long int x = ex_mem.aluResult + 3;
                while (x >= ex_mem.aluResult)
                {
                    addressStr = convert_int2hexstr(x, 32);
                    res += data_map[addressStr];
                    x--;
                }
                memoryData = convert_hexstr2int(res, 32);
            }
            else if (ex_mem.size == 8)
            { // LD
                long long int x = ex_mem.aluResult + 7;
                while (x >= ex_mem.aluResult)
                {
                    addressStr = convert_int2hexstr(x, 32);
                    res += data_map[addressStr];
                    x--;
                }
                memoryData = convert_hexstr2int(res, 64);
            }
            ex_mem.memoryAccess = false;
            ex_mem.memoryRequest = false;
            cout << "MEM: Loaded data " << memoryData << " from memory address " << ex_mem.aluResult << endl;
        }
        // Store instruction
        else if (ex_mem.memoryRequest)
        {
            string addressStr;
            string data;
            long long int int_address = ex_mem.aluResult;
            if (ex_mem.size == 1)
            { // SB
                data = convert_int2hexstr(RM, 8);
                addressStr = convert_int2hexstr(int_address, 32);
                data_map[addressStr] = data.substr(2, 2);
            }
            else if (ex_mem.size == 2)
            { // SH
                data = convert_int2hexstr(RM, 16);
                for (int i = 1; i >= 0; --i)
                {
                    addressStr = convert_int2hexstr(int_address, 32);
                    int_address += 1;
                    data_map[addressStr] = data.substr(2 * i + 2, 2);
                }
            }
            else if (ex_mem.size == 4)
            { // SW
                data = convert_int2hexstr(RM, 32);
                for (int i = 3; i >= 0; --i)
                {
                    addressStr = convert_int2hexstr(int_address, 32);
                    int_address += 1;
                    data_map[addressStr] = data.substr(2 * i + 2, 2);
                }
            }
            else if (ex_mem.size == 8)
            { // SD
                data = convert_int2hexstr(RM, 64);
                for (int i = 7; i >= 0; --i)
                {
                    addressStr = convert_int2hexstr(int_address, 32);
                    int_address += 1;
                    data_map[addressStr] = data.substr(2 * i + 2, 2);
                }
            }
            cout << "MEM: Stored data " << RM << " at memory address " << ex_mem.aluResult << endl;
        }
        else
        {
            cout << "MEM: No memory access required\n";
        }

        // now we have mux Y with input as aluResult or memroyData or returnAddress
        MuxY(ex_mem.controlMuxY);

        mem_wb.rd = ex_mem.rd;
        mem_wb.writeData = RY;
        mem_wb.writeBack = ex_mem.writeBack;

        mem_wb.valid = true;
        ex_mem.memoryAccess = false;
        ex_mem.memoryRequest = false;
        // clockCycle++;
        return;
    }
}

void write_back()
{
    if (mem_wb.valid)
    {
        if (!mem_wb.writeBack || mem_wb.rd == 0)
        {
            cout << "WB: No write back required\n";
        }
        else
        {
            R[mem_wb.rd] = RY;
            mem_wb.writeBack = false;
            cout << "WB: Wrote " << RY << " to R[" << mem_wb.rd << "]\n";
        }
        R[0] = 0; // x0 is always 0
        // clockCycle++;
        return;
    }
}