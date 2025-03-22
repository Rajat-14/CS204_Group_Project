#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

using namespace std;

map<string, string> instruction_map;
map<string, string> data_map;
string instructionRegister;
string currentPC_str = "0x0";
unsigned int returnAddress = 0;
unsigned int RM;
int long long aluResult;
unsigned int RY;
int long long memoryData;
bool is_PCupdated_while_execution = false;
int R[32] = {0}; // Register file
long long int clock=0;

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

string convert_int2hexstr(unsigned int num)
{
    stringstream ss;
    ss << "0x" << hex << num;
    return ss.str();
}

long long convert_hexstr2int(const std::string &hexstr)
{
    long long result = 0;
    int start = 0;

    // Check for optional "0x" or "0X" prefix.
    if (hexstr.size() >= 2 && hexstr[0] == '0' && (hexstr[1] == 'x' || hexstr[1] == 'X'))
    {
        start = 2;
    }

    // Process each character in the string.
    for (int i = start; i < hexstr.size(); ++i)
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

    return result;
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
            // If the character is neither '0' nor '1', throw an error.
            throw std::invalid_argument("Binary string contains invalid characters. Only '0' and '1' are allowed.");
        }
    }

    return result;
}

string IAG()
{
    // Convert current PC hex to integer.
    unsigned int currentPC = convert_hexstr2int(currentPC_str);
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

    return convert_int2hexstr(nextPC);
}

void fetch()
{
    string pc = currentPC_str;
    instructionRegister = instruction_map[pc];
    std::cout << "FETCH: Instruction " << instructionRegister << "from PC = " << pc << endl;
    clock++;
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
        // Construct branch immediate from scattered bits.
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
    clock++;
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
        else if (ir.operation=="MUL")
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
        // now we add this value to current PC
        aluResult = aluResult + convert_hexstr2int(currentPC_str);
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
    clock++;
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
            addressStr=convert_int2hexstr(aluResult);
            res += data_map[addressStr];
            memoryData = convert_hexstr2int(res);
        }
        else if (ir.funct3 == 1)
        { // LH
            long long int x = aluResult + 1;
            while (x >= aluResult)
            {
                addressStr = convert_int2hexstr(x);
                res += data_map[addressStr];
                x--;
            }
            memoryData = convert_hexstr2int(res);
        }
        else if (ir.funct3 == 2)
        { // LW
            long long int x = aluResult + 3;
            while (x >= aluResult)
            {
                addressStr = convert_int2hexstr(x);
                res += data_map[addressStr];
                x--;
            }
            memoryData = convert_hexstr2int(res);
        }
        else if (ir.funct3 == 3)
        { // LD
            long long int x = aluResult + 7;
            while (x >= aluResult)
            {
                addressStr = convert_int2hexstr(x);
                res += data_map[addressStr];
                x--;
            }
            memoryData = convert_hexstr2int(res);
        }
        cout << "MEM: Loaded data " << memoryData << " from memory address " << aluResult << endl;
    }
    // Store instruction
    else if (ir.opcode == 0x23)
    {
        string addressStr;
        string data = convert_int2hexstr(RM);
        reverse(data.begin(), data.end());
        
        if (data.size() < 16)
        {
            string s = "";
            for (int i = 0; i < 16 - data.size(); ++i)
            {
                s += "0";
            }
            data = s + data.substr(2);
        }
        

        if (ir.funct3 == 0)
        { // SB
            addressStr = convert_int2hexstr(aluResult);
            data_map[addressStr] = data.substr(0,2);
        }
        else if (ir.funct3 == 1)
        { // SH
            for (int i = 0; i < 2; ++i)
            {
                addressStr = convert_int2hexstr(aluResult + i);
                data_map[addressStr] = data.substr(2 * i, 2);
            }
        }
        else if (ir.funct3 == 2)
        { // SW
            for (int i = 0; i < 4; ++i)
            {
                addressStr = convert_int2hexstr(aluResult + i);
                data_map[addressStr] = data.substr(2 * i, 2);
            }
        }
        else if (ir.funct3 == 3)
        { // SD
            for (int i = 0; i < 8; ++i)
            {
                addressStr = convert_int2hexstr(aluResult + i);
                data_map[addressStr] = data.substr(2 * i, 2);
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
    }else if(ir.opcode==0x23){ // Store instruction
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
    clock++;
    return ;
}

void write_back()
{
    if(ir.opcode==0x23 || ir.opcode==0x63){
        cout << "WB: No write back required\n";
    }else{
        R[ir.rd] = RY;
        cout << "WB: Wrote " << RY << " to R[" << ir.rd << "]\n";
    }
    clock++;
    return;
}

int main()
{
    load_mc_file("input.mc");

    cout << "Instruction Memory:\n";
    for (map<string, string>::const_iterator it = instruction_map.begin();
         it != instruction_map.end(); ++it)
    {
        cout << "PC: " << it->first << " -> " << it->second << "\n";
    }

    cout << "\nData Memory:\n";
    for (map<string, string>::const_iterator it = data_map.begin();it != data_map.end(); ++it)
    {
        cout << "Address: " << it->first
             << " -> Value: " << it->second << "\n";
    }

    while (instruction_map[currentPC_str] != "Terminate")
    {
        fetch();
        decode();
        execute();
        memory_access();
        write_back();
        if(is_PCupdated_while_execution==false){
            currentPC_str = IAG();
        }
        cout<<"Clock: "<<clock<<endl;
        cout << "----------------------------------------------------------------\n";
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
