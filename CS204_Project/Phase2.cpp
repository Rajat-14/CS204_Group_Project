#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

map<string, string> instruction_map;
map<string, string> data_map;
string instructionRegister;
int R[32] = {0}; // Register file

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
                    break;
                instruction_map[pc] = code;
            }
        }
    }

    file.close();
}

void fetch(string pc)
{
    instructionRegister = instruction_map[pc];
    cout << "FETCH: Instruction " << instructionRegister << "from PC = " << pc << endl;
}

void decode()
{
    int instruction = stoul(instructionRegister, nullptr, 2);

    int opcode = instruction & 0x7F;
    int rd = (instruction >> 7) & 0x1F;
    int funct3 = (instruction >> 12) & 0x7;
    int rs1 = (instruction >> 15) & 0x1F;
    int rs2 = (instruction >> 20) & 0x1F;
    int funct7 = (instruction >> 25) & 0x7F;
    int imm = 0;
    string operation;

    switch (opcode)
    {
    // R-type
    case 0x33:
        if (funct3 == 0 && funct7 == 0)
            operation = "ADD";
        else if (funct3 == 0 && funct7 == 0x20)
            operation = "SUB";
        else if (funct3 == 7)
            operation = "AND";
        else if (funct3 == 0 && funct7 == 0x01)
            operation = "MUL";
        else if (funct3 == 6 && funct7 == 0)
            operation = "OR";
        else if (funct3 == 6 && funct7 == 0x01)
            operation = "REM";
        else if (funct3 == 4 && funct7 == 0)
            operation = "XOR";
        else if (funct3 == 4 && funct7 == 0x01)
            operation = "DIV";
        else if (funct3 == 1)
            operation = "SLL";
        else if (funct3 == 5 && funct7 == 0)
            operation = "SRL";
        else if (funct3 == 5 && funct7 == 0x20)
            operation = "SRA";
        else if (funct3 == 2)
            operation = "SLT";
        else
            operation = "UNKNOWN R-TYPE";

        cout << "DECODE: Operation is " << operation << ", first operand " << R[rs1] << ", second operand " << R[rs2] << ", destination register " << rd << endl;
        break;

    // I-type
    case 0x13:
        imm = (instruction >> 20) & 0xFFF; // Extract 12-bit immediate.
        if (imm & 0x800)
            imm |= 0xFFFFF000;
        if (funct3 == 0)
            operation = "ADDI";
        else if (funct3 == 7)
            operation = "ANDI";
        else if (funct3 == 6)
            operation = "ORI";
        else if (funct3 == 4)
            operation = "XORI";
        else if (funct3 == 2)
            operation = "SLTI";
        else if (funct3 == 1)
            operation = "SLLI";
        else if (funct3 == 5)
            operation = "SRLI";
        else
            operation = "UNKNOWN I-TYPE";
        cout << "DECODE: Operation is " << operation << ", first operand " << R[rs1] << ", immediate value " << imm << ", destination register " << rd << endl;
        break;

    // I-type Load instructions
    case 0x03:
        imm = (instruction >> 20) & 0xFFF;
        if (imm & 0x800)
            imm |= 0xFFFFF000;
        if (funct3 == 0)
            operation = "LB";
        else if (funct3 == 1)
            operation = "LH";
        else if (funct3 == 2)
            operation = "LW";
        else if (funct3 == 3)
            operation = "LD";
        else
            operation = "UNKNOWN LOAD";
        cout << "DECODE: Operation is " << operation << ", base register " << R[rs1] << ", offset " << imm << ", destination register " << rd << endl;
        break;

    // JALR instruction (I-type)
    case 0x67:
        imm = (instruction >> 20) & 0xFFF;
        if (imm & 0x800)
            imm |= 0xFFFFF000;
        operation = "JALR";
        cout << "DECODE: Operation is " << operation << ", first operand " << R[rs1] << ", immediate value " << imm << ", destination register " << rd << endl;
        break;

    // S-type
    case 0x23:
        // Immediate is split between bits [11:7] and [31:25]
        imm = (((instruction >> 7) & 0x1F) | (((instruction >> 25) & 0x7F) << 5));
        if (imm & 0x800)
            imm |= 0xFFFFF000;
        if (funct3 == 0)
            operation = "SB";
        else if (funct3 == 1)
            operation = "SH";
        else if (funct3 == 2)
            operation = "SW";
        else if (funct3 == 3)
            operation = "SD";
        else
            operation = "UNKNOWN S-TYPE";
        cout << "DECODE: Operation is " << operation << ", base register " << R[rs1] << ", second register " << R[rs2] << ", offset " << imm << endl;
        break;

    // SB-type
    case 0x63:
        cout << instruction << endl;
        // Construct branch immediate from scattered bits.
        imm = (((instruction >> 7) & 0x1) << 11) |
              (((instruction >> 8) & 0xF) << 1) |
              (((instruction >> 25) & 0x3F) << 5) |
              (((instruction >> 31) & 0x1) << 12);
        if (imm & 0x1000)
            imm |= 0xFFFFE000;
        if (funct3 == 0)
            operation = "BEQ";
        else if (funct3 == 1)
            operation = "BNE";
        else if (funct3 == 4)
            operation = "BLT";
        else if (funct3 == 5)
            operation = "BGE";
        else
            operation = "UNKNOWN BRANCH";
        cout << "DECODE: Operation is " << operation << ", first operand " << R[rs1] << ", second operand " << R[rs2] << ", branch offset " << imm << endl;
        break;

    // U-type lui
    case 0x37:
        imm = instruction & 0xFFFFF000;
        operation = "LUI";
        cout << "DECODE: Operation is " << operation << ", destination register " << rd << ", immediate " << imm << endl;
        break;

    // U-type auipc
    case 0x17:
        imm = instruction & 0xFFFFF000;
        operation = "AUIPC";
        cout << "DECODE: Operation is " << operation << ", destination register " << rd << ", immediate " << imm << endl;
        break;

    // UJ-type
    case 0x6F:

        imm = (((instruction >> 21) & 0x3FF) << 1) |
              (((instruction >> 20) & 0x1) << 11) |
              (((instruction >> 12) & 0xFF) << 12) |
              (((instruction >> 31) & 0x1) << 20);
        if (imm & 0x100000)
            imm |= 0xFFE00000; // Sign extend to 32 bits

        operation = "JAL";
        cout << "DECODE: Operation is " << operation << ", destination register " << rd << ", jump offset " << imm << endl;
        break;

    default:
        cout << "DECODE: Unsupported opcode 0x" << hex << opcode << dec << endl;
        break;
    }
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
    for (map<string, string>::const_iterator it = data_map.begin();
         it != data_map.end(); ++it)
    {
        cout << "Address: " << it->first
             << " -> Value: " << it->second << "\n";
    }
    string pc = "";

    cout << "\nEnter PC value to test (e.g., 00000000): ";
    cin >> pc;

    // Fetch the instruction at the provided PC and decode it.
    fetch(pc);
    decode();

    return 0;
}
