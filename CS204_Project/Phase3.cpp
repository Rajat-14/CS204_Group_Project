#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

map<string, string> instruction_map;
map<string, string> data_map;
string instructionRegister;
string currentPC_str = "0x0";
unsigned int returnAddress = 0;
long long int RM;
int long long aluResult;
long long int RY;
int long long memoryData;
bool is_PCupdated_while_execution = false;
int R[32] = {0}; // Register file

long long int clockCycle = 0;

bool knob1 = true;
struct IF_ID_Register
{
    string instruction;
    string pc;
    bool isValid;
};
IF_ID_Register if_id = {"", "", false};

struct ID_EX_Register
{
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int funct3;
    int funct7;
    long long int imm;
    int operandA;
    int operandB;
    string operation;
    string pc;
    bool valid;
};
ID_EX_Register id_ex = {0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", false};

struct EX_MEM_Register
{
    int aluResult;
    int rd;
    int opcode;
    int funct3;
    string operation;
    string pc;
    bool valid;
};
EX_MEM_Register ex_mem = {0, 0, 0, 0, "", "", false};

struct MEM_WB_Register
{
    int writeData;
    int rd;
    int opcode;
    string operation;
    string pc;
    bool valid;
};
MEM_WB_Register mem_wb = {0, 0, 0, "", "", false};

struct instruction_register
{
    string operation;
    int opcode;
    int rd;
    int funct3;
    int funct7;
    int rs1;
    int rs2;
    long long int imm;
};

instruction_register ir;

void load_mc_file(const string &filename)
{
    ifstream file(filename);
    if (!file)
    {
        cerr << "Error: Unable to open file " << filename << "\n";
        return;
    }

    string line;
    bool foundTextSegment = true;

    while (getline(file, line))
    {
        if (line.find("Memory:") != string::npos)
        {
            foundTextSegment = false;
            break;
        }
    }

    // Process Data Segment lines (address-value pairs)
    while (getline(file, line))
    {
        if (line.find("Text Segment:") != string::npos)
        {
            foundTextSegment = true;
            break;
        }

        if (line.find("->") != string::npos)
        {
            istringstream iss(line);
            string address, arrow, value;
            if (iss >> address >> arrow >> value && arrow == "->")
            {
                data_map[address] = value;
            }
        }
    }

    // Process the Text Segment lines (PC and machine code)
    if (foundTextSegment)
    {
        while (getline(file, line))
        {
            if (line.empty())
                continue;
            istringstream iss(line);
            string pc, code;

            if (iss >> pc >> code)
            {

                if (code == "Terminate")
                {
                    instruction_map[pc] = code;
                    break;
                }
                instruction_map[pc] = code;
            }
        }
    }

    file.close();
}

std::string convert_int2hexstr(long long num, int numBits)
{
    if (numBits <= 0 || numBits > 64)
    {
        throw std::invalid_argument("numBits must be between 1 and 64.");
    }
    // Create a mask for the desired number of bits.
    uint64_t mask = (numBits == 64) ? ~0ULL : ((1ULL << numBits) - 1);
    // Apply the mask to get the two's complement representation.
    uint64_t val = static_cast<uint64_t>(num) & mask;

    int hexDigits = (numBits + 3) / 4;

    std::stringstream ss;
    ss << "0x"
       << std::uppercase
       << std::setw(hexDigits)
       << std::setfill('0')
       << std::hex << val;
    return ss.str();
}

long long convert_hexstr2int(const std::string &hexstr, int numBits)
{
    if (numBits <= 0 || numBits > 64)
    {
        throw std::invalid_argument("numBits must be between 1 and 64.");
    }

    uint64_t result = 0;
    int start = 0;

    if (hexstr.size() >= 2 && hexstr[0] == '0' &&
        (hexstr[1] == 'x' || hexstr[1] == 'X'))
    {
        start = 2;
    }

    // Process each character in the string.
    for (size_t i = start; i < hexstr.size(); ++i)
    {
        char c = hexstr[i];
        result *= 16;
        if (c >= '0' && c <= '9')
        {
            result += c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            result += c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            result += c - 'A' + 10;
        }
        else
        {
            throw std::invalid_argument("Invalid character in hexadecimal string");
        }
    }

    // Determine the sign bit. For numBits == 64, use 1ULL << 63.
    uint64_t signBit = (numBits == 64) ? (1ULL << 63) : (1ULL << (numBits - 1));

    if (result & signBit)
    {
        if (numBits == 64)
        {
            // For 64 bits, casting from uint64_t to int64_t yields the proper two's complement.
            return static_cast<int64_t>(result);
        }
        else
        {
            // For less than 64 bits, subtract 2^(numBits).
            uint64_t subtractVal = (1ULL << numBits);
            return static_cast<long long>(result - subtractVal);
        }
    }
    else
    {
        return static_cast<long long>(result);
    }
}

unsigned int convert_binstr2int(const std::string &binstr)
{
    unsigned int result = 0;
    for (char c : binstr)
    {
        result <<= 1; // multiply by 2
        if (c == '1')
        {
            result |= 1; // Set the least significant bit if the current character is '1'
        }
        else if (c != '0')
        {

            throw std::invalid_argument("Binary string contains invalid characters. Only '0' and '1' are allowed.");
        }
    }

    return result;
}

string convert_PC2hex(unsigned int num)
{
    // Convert PC to hex string.
    stringstream ss;
    ss << "0x" << hex << num;
    return ss.str();
}

string IAG()
{
    // Convert current PC hex to integer.
    unsigned int currentPC = convert_hexstr2int(currentPC_str, 32);
    unsigned int nextPC = currentPC + 4; // Default next PC if no branch or jump instructions

    // Adjust nextPC for branch and jump instructions.
    if (id_ex.opcode == 0x63)
    { // Branch instructions.
        bool branchTaken = false;
        if (aluResult == 1)
        {
            branchTaken = true;
        }
        if (branchTaken)
        {
            nextPC = currentPC + id_ex.imm;
        }
    }
    else if (id_ex.opcode == 0x6F)
    { // JAL
        returnAddress = currentPC + 4;
        nextPC = currentPC + id_ex.imm;
    }
    else if (id_ex.opcode == 0x67)
    {                                             // JALR
        nextPC = (R[id_ex.rs1] + id_ex.imm) & ~1; // Ensure the target address is even.
    }

    return convert_PC2hex(nextPC);
}

void fetch()
{
    string pc = currentPC_str;
    instructionRegister = instruction_map[pc];
    if_id.instruction = instructionRegister;
    if_id.pc = currentPC_str;
    if_id.isValid = true;
    cout << "FETCH: Instruction " << instructionRegister << "from PC = " << pc << endl;
    clockCycle++;
}

void decode()
{
    int instruction = convert_binstr2int(if_id.instruction);
    id_ex.opcode = instruction & 0x7F;
    id_ex.rd = (instruction >> 7) & 0x1F;
    id_ex.funct3 = (instruction >> 12) & 0x7;
    id_ex.rs1 = (instruction >> 15) & 0x1F;
    id_ex.rs2 = (instruction >> 20) & 0x1F;
    id_ex.funct7 = (instruction >> 25) & 0x7F;
    id_ex.imm = 0;

    switch (id_ex.opcode)
    {
    // R-type
    case 0x33:
        if (id_ex.funct3 == 0 && id_ex.funct7 == 0)
            id_ex.operation = "ADD";
        else if (id_ex.funct3 == 0 && id_ex.funct7 == 0x20)
            id_ex.operation = "SUB";
        else if (id_ex.funct3 == 7)
            id_ex.operation = "AND";
        else if (id_ex.funct3 == 0 && id_ex.funct7 == 0x01)
            id_ex.operation = "MUL";
        else if (id_ex.funct3 == 6 && id_ex.funct7 == 0)
            id_ex.operation = "OR";
        else if (id_ex.funct3 == 6 && id_ex.funct7 == 0x01)
            id_ex.operation = "REM";
        else if (id_ex.funct3 == 4 && id_ex.funct7 == 0)
            id_ex.operation = "XOR";
        else if (id_ex.funct3 == 4 && id_ex.funct7 == 0x01)
            id_ex.operation = "DIV";
        else if (id_ex.funct3 == 1)
            id_ex.operation = "SLL";
        else if (id_ex.funct3 == 5 && id_ex.funct7 == 0)
            id_ex.operation = "SRL";
        else if (id_ex.funct3 == 5 && id_ex.funct7 == 0x20)
            id_ex.operation = "SRA";
        else if (id_ex.funct3 == 2)
            id_ex.operation = "SLT";
        else
            id_ex.operation = "UNKNOWN R-TYPE";

        cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", second operand " << R[id_ex.rs2] << ", destination register " << id_ex.rd << endl;
        break;

    // I-type
    case 0x13:
        id_ex.imm = (instruction >> 20) & 0xFFF; // Extract 12-bit immediate.
        if (id_ex.imm & 0x800)
            id_ex.imm |= 0xFFFFF000;
        if (id_ex.funct3 == 0)
            id_ex.operation = "ADDI";
        else if (id_ex.funct3 == 7)
            id_ex.operation = "ANDI";
        else if (id_ex.funct3 == 6)
            id_ex.operation = "ORI";
        else if (id_ex.funct3 == 4)
            id_ex.operation = "XORI";
        else if (id_ex.funct3 == 2)
            id_ex.operation = "SLTI";
        else if (id_ex.funct3 == 1)
            id_ex.operation = "SLLI";
        else if (id_ex.funct3 == 5)
            id_ex.operation = "SRLI";
        else
            id_ex.operation = "UNKNOWN I-TYPE";
        cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", immediate value " << id_ex.imm << ", destination register " << id_ex.rd << endl;
        break;

    // I-type Load instructions
    case 0x03:
        id_ex.imm = (instruction >> 20) & 0xFFF;
        if (id_ex.imm & 0x800)
            id_ex.imm |= 0xFFFFF000;
        if (id_ex.funct3 == 0)
            id_ex.operation = "LB";
        else if (id_ex.funct3 == 1)
            id_ex.operation = "LH";
        else if (id_ex.funct3 == 2)
            id_ex.operation = "LW";
        else if (id_ex.funct3 == 3)
            id_ex.operation = "LD";
        else
            id_ex.operation = "UNKNOWN LOAD";
        cout << "DECODE: Operation is " << id_ex.operation << ", base register " << R[id_ex.rs1] << ", offset " << id_ex.imm << ", destination register " << id_ex.rd << endl;
        break;

    // JALR instruction (I-type)
    case 0x67:
        id_ex.imm = (instruction >> 20) & 0xFFF;
        if (id_ex.imm & 0x800)
            id_ex.imm |= 0xFFFFF000;
        id_ex.operation = "JALR";
        cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", immediate value " << id_ex.imm << ", destination register " << id_ex.rd << endl;
        break;

    // S-type
    case 0x23:
        // Immediate is split between bits [11:7] and [31:25]
        id_ex.imm = (((instruction >> 7) & 0x1F) | (((instruction >> 25) & 0x7F) << 5));
        if (id_ex.imm & 0x800)
            id_ex.imm |= 0xFFFFF000;
        if (id_ex.funct3 == 0)
            id_ex.operation = "SB";
        else if (id_ex.funct3 == 1)
            id_ex.operation = "SH";
        else if (id_ex.funct3 == 2)
            id_ex.operation = "SW";
        else if (id_ex.funct3 == 3)
            id_ex.operation = "SD";
        else
            id_ex.operation = "UNKNOWN S-TYPE";
        cout << "DECODE: Operation is " << id_ex.operation << ", base register " << R[id_ex.rs1] << ", second register " << R[id_ex.rs2] << ", offset " << id_ex.imm << endl;
        break;

    // SB-type
    case 0x63:
        cout << instruction << endl;

        id_ex.imm = (((instruction >> 7) & 0x1) << 11) |
                    (((instruction >> 8) & 0xF) << 1) |
                    (((instruction >> 25) & 0x3F) << 5) |
                    (((instruction >> 31) & 0x1) << 12);
        if (id_ex.imm & 0x1000)
            id_ex.imm |= 0xFFFFE000;
        if (id_ex.funct3 == 0)
            id_ex.operation = "BEQ";
        else if (id_ex.funct3 == 1)
            id_ex.operation = "BNE";
        else if (id_ex.funct3 == 4)
            id_ex.operation = "BLT";
        else if (id_ex.funct3 == 5)
            id_ex.operation = "BGE";
        else
            id_ex.operation = "UNKNOWN BRANCH";
        cout << "DECODE: Operation is " << id_ex.operation << ", first operand " << R[id_ex.rs1] << ", second operand " << R[id_ex.rs2] << ", branch offset " << id_ex.imm << endl;
        break;

    // U-type lui
    case 0x37:
        id_ex.imm = instruction & 0xFFFFF000;
        id_ex.operation = "LUI";
        cout << "DECODE: Operation is " << id_ex.operation << ", destination register " << id_ex.rd << ", immediate " << id_ex.imm << endl;
        break;

    // U-type auipc
    case 0x17:
        id_ex.imm = instruction & 0xFFFFF000;
        id_ex.operation = "AUIPC";
        cout << "DECODE: Operation is " << id_ex.operation << ", destination register " << id_ex.rd << ", immediate " << id_ex.imm << endl;
        break;

    // UJ-type
    case 0x6F:
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

void execute()
{
    is_PCupdated_while_execution = false;
    bool usePCForA = false;
    bool useImmediateForB = false;
    int operandB = selectoperandB(useImmediateForB);
    int operandA = selectoperandA(usePCForA);

    if (id_ex.opcode == 0x33)
    { // R-type
        if (id_ex.operation == "ADD")
            aluResult = operandA + operandB;
        else if (id_ex.operation == "SUB")
            aluResult = operandA - operandB;

        else if (id_ex.operation == "AND")
            aluResult = operandA & operandB;
        else if (id_ex.operation == "MUL")
            aluResult = operandA * operandB;
        else if (id_ex.operation == "DIV")
            aluResult = operandA / operandB;
        else if (id_ex.operation == "REM")
            aluResult = operandA % operandB;
        else if (id_ex.operation == "OR")
            aluResult = operandA | operandB;
        else if (id_ex.operation == "XOR")
            aluResult = operandA ^ operandB;
        else if (id_ex.operation == "SLL")
            aluResult = operandA << (operandB & 0x1F);
        else if (id_ex.operation == "SRL")
            aluResult = (unsigned)operandA >> (operandB & 0x1F);
        else if (id_ex.operation == "SRA")
            aluResult = operandA >> (operandB & 0x1F);
        else if (id_ex.operation == "SLT")
            aluResult = (operandA < operandB) ? 1 : 0;
        else
            cout << "EXECUTE: Unsupported R-type ir.operation." << endl;
        cout << "EXECUTE: R-type operation result is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x13)
    { // I-type
        useImmediateForB = true;
        operandB = selectoperandB(useImmediateForB);
        if (id_ex.funct3 == 0) // ADDI
            aluResult = operandA + operandB;
        else if (id_ex.funct3 == 7) // ANDI
            aluResult = operandA & operandB;
        else if (id_ex.funct3 == 6) // ORI
            aluResult = operandA | operandB;
        else if (id_ex.funct3 == 4) // XORI
            aluResult = operandA ^ operandB;
        else if (id_ex.funct3 == 2) // SLTI
            aluResult = (operandA < operandB) ? 1 : 0;
        else if (id_ex.funct3 == 1) // SLLI
            aluResult = operandA << (operandB & 0x1F);
        else if (id_ex.funct3 == 5) // SRLI
            aluResult = (unsigned)operandA >> (operandB & 0x1F);
        else
            cout << "EXECUTE: Unsupported I-type ALU ir.operation." << endl;
        cout << "EXECUTE: I-type operation result is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x03)
    { // Load instruction
        useImmediateForB = true;
        operandB = selectoperandB(useImmediateForB);
        aluResult = operandA + operandB; // EA
        cout << "EXECUTE: Effective address is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x23)
    {                  // Store instructions (S-type)
        RM = operandB; // here operandB=R[ir.rs2] that is value to be stored
        useImmediateForB = true;
        operandB = selectoperandB(useImmediateForB); // now operandB is offset
        aluResult = operandA + operandB;             // EA
        cout << "EXECUTE: Effective address is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x63)
    { // Branch instructions (SB-type)
        aluResult = 0;
        if (id_ex.funct3 == 0 && (R[id_ex.rs1] == R[id_ex.rs2])) // BEQ
            aluResult = 1;
        else if (id_ex.funct3 == 1 && (R[id_ex.rs1] != R[id_ex.rs2])) // BNE
            aluResult = 1;
        else if (id_ex.funct3 == 4 && (R[id_ex.rs1] < R[id_ex.rs2])) // BLT
            aluResult = 1;
        else if (id_ex.funct3 == 5 && (R[id_ex.rs1] >= R[id_ex.rs2])) // BGE
            aluResult = 1;
        else
            aluResult = 0;
        currentPC_str = IAG();
        is_PCupdated_while_execution = true;
        cout << "EXECUTE: Branch operation result is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x37)
    { // LUI
        aluResult = id_ex.imm;
        cout << "EXECUTE: LUI operation result is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x17)
    { // AUIPC
        // ir.imm is shifted left by 12 bits and added to PC.
        aluResult = id_ex.imm;

        aluResult = aluResult + convert_hexstr2int(currentPC_str, 32);
        cout << "EXECUTE: AUIPC operation result is " << aluResult << endl;
    }
    else if (id_ex.opcode == 0x6F)
    { // JAL
        // IAG povides add stores current PC + 4 in returnAddress
        currentPC_str = IAG();
        is_PCupdated_while_execution = true;
        cout << "EXECUTE: NO ALU operation required for JAL\n";
    }
    else if (id_ex.opcode == 0x67)
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
    ex_mem.aluResult = aluResult;
    ex_mem.operation = id_ex.operation;
    ex_mem.opcode = id_ex.opcode;
    ex_mem.funct3 = id_ex.funct3;
    ex_mem.pc = id_ex.pc;
    ex_mem.rd = id_ex.rd;
    ex_mem.valid = true;
    clockCycle++;
}

void memory_access()
{
    // Load instruction
    if (ex_mem.opcode == 0x03)
    {
        // we have to check lw,lb,lh,ld
        string addressStr;
        string res = "";
        if (ex_mem.funct3 == 0)
        { // LB
            addressStr = convert_int2hexstr(ex_mem.aluResult, 32);
            res += data_map[addressStr];
            memoryData = convert_hexstr2int(res, 8);
        }
        else if (ex_mem.funct3 == 1)
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
        else if (ex_mem.funct3 == 2)
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
        else if (ex_mem.funct3 == 3)
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
        cout << "MEM: Loaded data " << memoryData << " from memory address " << ex_mem.aluResult << endl;
    }
    // Store instruction
    else if (ex_mem.opcode == 0x23)
    {
        string addressStr;
        string data;
        long long int int_address = ex_mem.aluResult;
        if (ex_mem.funct3 == 0)
        { // SB
            data = convert_int2hexstr(RM, 8);
            addressStr = convert_int2hexstr(int_address, 32);
            data_map[addressStr] = data.substr(2, 2);
        }
        else if (ex_mem.funct3 == 1)
        { // SH
            data = convert_int2hexstr(RM, 16);
            for (int i = 1; i >= 0; --i)
            {
                addressStr = convert_int2hexstr(int_address, 32);
                int_address += 1;
                data_map[addressStr] = data.substr(2 * i + 2, 2);
            }
        }
        else if (ex_mem.funct3 == 2)
        { // SW
            data = convert_int2hexstr(RM, 32);
            for (int i = 3; i >= 0; --i)
            {
                addressStr = convert_int2hexstr(int_address, 32);
                int_address += 1;
                data_map[addressStr] = data.substr(2 * i + 2, 2);
            }
        }
        else if (ex_mem.funct3 == 3)
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
    if (ex_mem.opcode == 0x03)
    { // Load instruction
        RY = memoryData;
    }
    else if (ex_mem.opcode == 0x6F)
    { // JAL
        RY = returnAddress;
    }
    else if (ex_mem.opcode == 0x67)
    { // JALR
        RY = returnAddress;
    }
    else if (ex_mem.opcode == 0x23)
    { // Store instruction
        RY = 0;
    }
    else if (ex_mem.opcode == 0x63)
    {
        RY = 0;
    }
    else
    {
        RY = ex_mem.aluResult;
    }
    mem_wb.opcode = ex_mem.opcode;
    mem_wb.operation = ex_mem.operation;
    mem_wb.pc = ex_mem.pc;
    mem_wb.rd = ex_mem.rd;
    mem_wb.writeData = RY;
    clockCycle++;
    return;
}

void write_back()
{
    if (mem_wb.opcode == 0x23 || mem_wb.opcode == 0x63)
    {
        cout << "WB: No write back required\n";
    }
    else
    {
        R[mem_wb.rd] = RY;
        cout << "WB: Wrote " << RY << " to R[" << mem_wb.rd << "]\n";
    }
    R[0] = 0; // x0 is always 0
    clockCycle++;
    return;
}

int main()
{
    // Initializing the register file
    R[2] = 2147483612; // Stack pointer
    R[3] = 268435456;  // Frame pointer
    R[10] = 1;
    R[11] = 2147483612;
    load_mc_file("input.mc");

    cout << "Instruction Memory:\n";
    for (map<string, string>::const_iterator it = instruction_map.begin();
         it != instruction_map.end(); ++it)
    {
        cout << "PC: " << it->first << " -> " << it->second << "\n";
    }

    cout << "\nData Memory:\n";
    for (map<string, string>::const_iterator it = data_map.begin(); it != data_map.end(); ++it)
    {
        cout << "Address: " << it->first
             << " -> Value: " << it->second << "\n";
    }

    cout << "----------------------------------------------------------------------------------------------------\n";
    while (instruction_map[currentPC_str] != "Terminate")
    {
        fetch();
        decode();
        execute();
        memory_access();
        write_back();
        if (is_PCupdated_while_execution == false)
        {
            currentPC_str = IAG();
        }
        cout << "Clock: " << clockCycle << endl;
        for (auto it : R)
        {
            cout << it << " ";
        }
        cout << endl;
        cout << "----------------------------------------------------------------------------------------------------\n";
    }

    // write data memory in a file named data_memory.txt
    ofstream data_memory_file("data_memory.txt");
    if (!data_memory_file)
    {
        cerr << "Error: Unable to create data_memory.txt" << endl;
        return 1;
    }
    data_memory_file << "Data Memory:\n";
    for (map<string, string>::const_iterator it = data_map.begin();
         it != data_map.end(); ++it)
    {
        data_memory_file << "Address: " << it->first
                         << " -> Value: " << it->second << "\n";
    }

    return 0;
}