#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <set>
#include <utility>
#include <map>
using namespace std;

enum Token
{
    tok_operator = -1,
    tok_label = -2,
    tok_label2 = -8,
    tok_register = -3,
    tok_immediate = -4,
    tok_EOF = -5,
    tok_directive = -6,
    tok_newline = -7,
    tok_string = -8,
};
ifstream asmFile;
string Operator;
string label;
string directive;
int registerValue = -1;
string immediateValue;
string literal;
unordered_set<string> operators = {"add", "and", "or", "sll", "slt", "sra", "srl", "sub", "xor", "mul", "div", "rem", "addi", "andi", "ori", "xori", "slli", "srli", "srai", "slti", "sltiu", "lb", "ld", "lh", "lw", "jalr", "sb", "sw", "sd", "sh", "beq", "bne", "bge", "blt", "auipc", "lui", "jal"};
unordered_map<string, int> reg = {
    {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4}, {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"s1", 9}, {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24}, {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31}};

static int lastChar = ' ';
bool istext = true;

int lex()
{
    // Skip whitespace and return a newline token immediately when encountered.
    while (isspace(lastChar))
    {
        if (lastChar == '\n')
        {
            lastChar = asmFile.get(); // consume the newline
            return tok_newline;
        }
        lastChar = asmFile.get();
    }

    if (lastChar == '"')
    {
        literal = "";
        lastChar = asmFile.get();
        while (lastChar != '"' && lastChar != EOF)
        {
            literal += lastChar;
            lastChar = asmFile.get();
        }
        if (lastChar == EOF)
            return tok_EOF;
        lastChar = asmFile.get();
        return tok_string;
    }

    // Skip commas.
    if (lastChar == ',')
    {
        while (lastChar == ',')
            lastChar = asmFile.get();
    }

    if (lastChar == EOF)
        return tok_EOF;
    if (lastChar == '#')
    {
        while (lastChar != '\n' && lastChar != EOF)
            lastChar = asmFile.get();
        if (lastChar == EOF)
            return tok_EOF;
        return tok_newline;
    }
    if (lastChar == '.')
    {
        directive = "";
        lastChar = asmFile.get();
        while (isalnum(lastChar))
        {
            directive += lastChar;
            lastChar = asmFile.get();
        }
        return tok_directive;
    }
    // Handle registers written as x<number>
    if (lastChar == 'x' && isdigit(asmFile.peek()))
    {
        lastChar = asmFile.get(); // consume 'x'
        string num = "";
        while (isdigit(lastChar))
        {
            num += lastChar;
            lastChar = asmFile.get();
        }
        registerValue = stoi(num);
        if (registerValue >= 0 && registerValue <= 31)
            return tok_register;
    }

    // Handle identifiers (operator names, labels, or register names by mnemonic)

    if (isalpha(lastChar))
    {
        Operator = "";
        while ((isalnum(lastChar) || lastChar == '_') && lastChar != EOF)
        {
            Operator += lastChar;
            lastChar = asmFile.get();
        }
        if (reg.find(Operator) != reg.end())
        {
            registerValue = reg[Operator];
            return tok_register;
        }

        if (operators.find(Operator) == operators.end())
        {
            label = Operator;
            if (lastChar == ':')
            {
                return tok_label;
            }
            else
            {
                return tok_label2;
            }
        }
        return tok_operator;
    }
    // Handle immediate values.
    if (isdigit(lastChar) || lastChar == '-')
    {
        string num = "";
        if (lastChar == '-')
        {
            num += lastChar;
            lastChar = asmFile.get();
        }
        if (lastChar == '0' && asmFile.peek() == 'x')
        {
            num += lastChar;
            lastChar = asmFile.get();
            num += lastChar;
            lastChar = asmFile.get();
        }
        while (isxdigit(lastChar))
        {
            num += lastChar;
            lastChar = asmFile.get();
        }
        immediateValue = num;
        return tok_immediate;
    }

    // If none of the above, consume the character and try again.
    lastChar = asmFile.get();
    return lex();
}

// Mapping registers to their binary representations
unordered_map<string, string> regMap = {
    {"x0", "00000"}, {"x1", "00001"}, {"x2", "00010"}, {"x3", "00011"}, {"x4", "00100"}, {"x5", "00101"}, {"x6", "00110"}, {"x7", "00111"}, {"x8", "01000"}, {"x9", "01001"}, {"x10", "01010"}, {"x11", "01011"}, {"x12", "01100"}, {"x13", "01101"}, {"x14", "01110"}, {"x15", "01111"}, {"x16", "10000"}, {"x17", "10001"}, {"x18", "10010"}, {"x19", "10011"}, {"x20", "10100"}, {"x21", "10101"}, {"x22", "10110"}, {"x23", "10111"}, {"x24", "11000"}, {"x25", "11001"}, {"x26", "11010"}, {"x27", "11011"}, {"x28", "11100"}, {"x29", "11101"}, {"x30", "11110"}, {"x31", "11111"}, {"ra", "00001"}, {"sp", "00010"}, {"gp", "00011"}, {"tp", "00100"}, {"t0", "00101"}, {"t1", "00110"}, {"t2", "00111"}, {"s0", "01000"}, {"s1", "01001"}, {"a0", "01010"}, {"a1", "01011"}, {"a2", "01100"}, {"a3", "01101"}, {"a4", "01110"}, {"a5", "01111"}, {"a6", "10000"}, {"a7", "10001"}, {"s2", "10010"}, {"s3", "10011"}, {"s4", "10100"}, {"s5", "10101"}, {"s6", "10110"}, {"s7", "10111"}, {"s8", "11000"}, {"s9", "11001"}, {"s10", "11010"}, {"s11", "11011"}, {"t3", "11100"}, {"t4", "11101"}, {"t5", "11110"}, {"t6", "11111"}};

// Mapping instructions to their binary opcode, func3, and func7 values
struct Instruction
{
    string opcode, func3, func7;
};
unordered_map<string, Instruction> instMap = {
    {"add", {"0110011", "000", "0000000"}}, {"sub", {"0110011", "000", "0100000"}}, {"and", {"0110011", "111", "0000000"}}, {"or", {"0110011", "110", "0000000"}}, {"xor", {"0110011", "100", "0000000"}}, {"sll", {"0110011", "001", "0000000"}}, {"srl", {"0110011", "101", "0000000"}}, {"sra", {"0110011", "101", "0100000"}}, {"slt", {"0110011", "010", "0000000"}}, {"mul", {"0110011", "000", "0000001"}}, {"div", {"0110011", "100", "0000001"}}, {"rem", {"0110011", "110", "0000001"}},

    {"addi", {"0010011", "000", ""}},
    {"andi", {"0010011", "111", ""}},
    {"ori", {"0010011", "110", ""}},
    {"lb", {"0000011", "000", ""}},
    {"lh", {"0000011", "001", ""}},
    {"lw", {"0000011", "010", ""}},
    {"ld", {"0000011", "011", ""}},
    {"jalr", {"1100111", "000", ""}},

    {"sb", {"0100011", "000", ""}},
    {"sh", {"0100011", "001", ""}},
    {"sw", {"0100011", "010", ""}},
    {"sd", {"0100011", "011", ""}},

    {"beq", {"1100011", "000", ""}},
    {"bne", {"1100011", "001", ""}},
    {"blt", {"1100011", "100", ""}},
    {"bge", {"1100011", "101", ""}},

    {"auipc", {"0010111", "", ""}},
    {"lui", {"0110111", "", ""}},
    {"jal", {"1101111", "", ""}}};

// Label mapping to addresses
unordered_map<string, int> labelMap;

// Convert immediate value to binary with sign extension
string immToBinary(int num, int bits)
{
    string binary = "";
    for (int i = bits - 1; i >= 0; i--) // Start from MSB
    {
        binary += (num & (1 << i)) ? '1' : '0';
    }
    return binary;
}

typedef struct machine_code
{
    string format_type;
    string opcode;
    string funct3;
    string funct7;
    string rs1;
    string rs2;
    string rd;
    string imm;
} machine_code;

void processInstruction(vector<pair<string, int>> &tokens, ofstream &mcFile, int &address)
{
    if (tokens.empty())
        return; // No instruction to process

    string op;
    vector<string> operands;
    if (tokens[0].second == tok_operator)
    {
        op = tokens[0].first;
        // First token is the opcode

        for (int i = 1; i < tokens.size(); i++)
        {
            operands.push_back(tokens[i].first);
        }

        if (instMap.find(op) == instMap.end())
        {
            cerr << "Unknown instruction: " << op << endl;
            return;
        }
    }

    else if (tokens[0].second == tok_label)
    {
        if (tokens.size() == 1)
        {
            return;
        }
        op = tokens[1].first;

        for (int i = 2; i < tokens.size(); i++)
        {
            operands.push_back(tokens[i].first);
        }
        // Validate if the opcode exists
        if (instMap.find(op) == instMap.end())
        {
            cerr << "Unknown instruction: " << op << endl;
            return;
        }
    }
    Instruction inst = instMap[op];

    machine_code mc;
    mc.format_type = "";
    mc.opcode = inst.opcode;
    mc.funct3 = inst.func3;
    mc.funct7 = inst.func7;
    mc.rd = "";
    mc.rs1 = "";
    mc.rs2 = "";
    mc.imm = "";

    if (inst.opcode == "0110011")
    {
        // R-type
        if (operands.size() < 3)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "R";
        mc.rd = regMap[operands[0]];
        mc.rs1 = regMap[operands[1]];
        mc.rs2 = regMap[operands[2]];
    }
    else if (inst.opcode == "0010011" || inst.opcode == "1100111")
    { // I-type
        if (operands.size() < 3)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "I";
        mc.rd = regMap[operands[0]];
        mc.rs1 = regMap[operands[1]];
        int imm = stoi(operands[2]);
        if (operands[2][0] == '0' && (operands[2][1] == 'x' || operands[2][1] == 'X'))
        {
            imm = stoi(operands[2], nullptr, 16);
        }
        mc.imm = immToBinary(imm, 12);
    }
    else if (inst.opcode == "0100011")
    {
        // S-type
        if (operands.size() < 3)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "S";
        mc.rs2 = regMap[operands[0]];
        mc.rs1 = regMap[operands[2]];
        int imm = stoi(operands[1]);
        if (operands[1][0] == '0' && (operands[1][1] == 'x' || operands[1][1] == 'X'))
        {
            imm = stoi(operands[1], nullptr, 16);
        }
        mc.imm = immToBinary(imm, 12);
    }
    else if (inst.opcode == "0110111" || inst.opcode == "0010111")
    {
        // U-type
        if (operands.size() < 2)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "U";
        mc.rd = regMap[operands[0]];
        int imm = stoi(operands[1]);
        if (operands[1][0] == '0' && (operands[1][1] == 'x' || operands[1][1] == 'X'))
        {
            imm = stoi(operands[1], nullptr, 16);
        }
        mc.imm = immToBinary(imm, 20);
    }
    else if (inst.opcode == "1101111")
    {
        // UJ-type
        if (operands.size() < 2)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "UJ";
        mc.rd = regMap[operands[0]];
        mc.imm = immToBinary(labelMap[operands[1]] - address, 21);
    }
    else if (inst.opcode == "0000011")
    {
        // load
        if (operands.size() < 3)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "I";
        mc.rd = regMap[operands[0]];
        mc.rs1 = regMap[operands[2]];
        int imm = stoi(operands[1]);
        if (operands[1][0] == '0' && (operands[1][1] == 'x' || operands[1][1] == 'X'))
        {
            imm = stoi(operands[1], nullptr, 16);
        }
        mc.imm = immToBinary(imm, 12);
    }
    else if (inst.opcode == "1100011")
    {
        // SB-type
        if (operands.size() < 3)
        {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "SB";
        mc.rs1 = regMap[operands[0]];
        mc.rs2 = regMap[operands[1]];
        mc.imm = immToBinary(labelMap[operands[2]] - address, 13);
    }
    else
    {
        cerr << "Error: Unknown instruction format for " << op << endl;
        return;
    }

    // Write output
    mcFile << "0x" << std::hex << address << " ";
    if (mc.format_type == "R")
    {
        mcFile << mc.funct7 << mc.rs2 << mc.rs1 << mc.funct3 << mc.rd << mc.opcode << "  ";
        mcFile << op << " ";
        for (const auto &t : operands)
            mcFile << t << " ";
        mcFile << " # " << mc.opcode << "-" << mc.funct3 << "-" << mc.funct7
               << "-" << mc.rd << "-" << mc.rs1 << "-" << mc.rs2 << "-" << "NULL";
    }
    else if (mc.format_type == "I")
    {
        mcFile << mc.imm << mc.rs1 << mc.funct3 << mc.rd << mc.opcode << "  ";
        mcFile << op << " ";
        for (const auto &t : operands)
            mcFile << t << " ";
        mcFile << " # " << mc.opcode << "-" << mc.funct3 << "-" << "NULL"
               << "-" << mc.rd << "-" << mc.rs1 << "-" << mc.imm;
    }
    else if (mc.format_type == "S")
    {
        mcFile << mc.imm.substr(0, 7) << mc.rs2 << mc.rs1 << mc.funct3 << mc.imm.substr(7, 5) << mc.opcode << " ";
        mcFile << op << " ";
        for (const auto &t : operands)
            mcFile << t << " ";
        mcFile << " # " << mc.opcode << "-" << mc.funct3 << "-" << mc.funct7
               << "-" << mc.rs1 << "-" << mc.rs2 << "-" << mc.imm;
    }
    else if (mc.format_type == "U")
    {
        mcFile << mc.imm << mc.rd << mc.opcode << " ";
        mcFile << op << " ";
        for (const auto &t : operands)
            mcFile << t << " ";

        mcFile << " # " << mc.opcode << "-" << "NULL" << "-" << "NULL" << mc.rd << "-" << mc.imm;
    }

    else if (mc.format_type == "UJ")
    {

        mcFile << mc.imm.substr(0, 1)
               << mc.imm.substr(10, 10)
               << mc.imm.substr(9, 1)
               << mc.imm.substr(1, 8)
               << mc.rd
               << mc.opcode << " ";
        mcFile << op << " ";

        for (const auto &t : operands)
            mcFile << t << " ";
        mcFile << " # " << mc.opcode << "-" << "NULL" << "-" << "NULL" << mc.rd << "-" << mc.imm;
    }
    else if (mc.format_type == "SB")
    {

        mcFile << mc.imm.substr(0, 1)
               << mc.imm.substr(2, 6)
               << mc.rs2
               << mc.rs1
               << mc.funct3
               << mc.imm.substr(8, 4)
               << mc.imm.substr(1, 1)
               << mc.opcode << " ";
        mcFile << op << " ";
        for (const auto &t : operands)
            mcFile << t << " ";
        mcFile << " # " << mc.opcode << "-" << mc.funct3 << "-" << mc.funct7 << "-" << mc.rs1 << "-" << mc.rs2 << "-" << mc.imm;
    }

    mcFile << endl;

    address += 4; // Increment address for next instruction
}
bool isCode = true;
map<string, string> memory; //{address,value}
void assemble()
{
    ofstream mcFile("output.mc");
    if (!mcFile)
    {
        cerr << "Error: Unable to create output.mc" << endl;
        return;
    }
    mcFile << "Memory: " << endl;
    for (const auto &it : memory)
    {
        mcFile << it.first << " -> " << it.second << endl;
    }
    mcFile << "Text Segment:" << endl;
    int address = 0;
    vector<pair<string, int>> tokens;

    while (true)
    {
        int tok = lex();
        if (tok == tok_directive)
        {
            if (directive == "text")
            {
                isCode = true;
            }
            else if (directive == "data")
                isCode = false;
        }

        if (isCode)
        {
            if (tok == tok_EOF)
            {
                // If any tokens remain in the last line then process them
                if (!tokens.empty())
                {
                    processInstruction(tokens, mcFile, address);
                }
                break;
            }

            if (tok == tok_newline)
            {
                // Process the collectted tokens when a newline is reached
                if (!tokens.empty())
                {
                    processInstruction(tokens, mcFile, address);
                    tokens.clear();
                }
            }
            else
            {
                // Store token as a string for processing
                if (tok == tok_operator)
                {
                    tokens.push_back({Operator, tok_operator});
                }
                else if (tok == tok_register)
                {
                    tokens.push_back({"x" + to_string(registerValue), tok_register});
                }
                else if (tok == tok_immediate)
                {
                    tokens.push_back({immediateValue, tok_immediate});
                }
                else if (tok == tok_label)
                {
                    tokens.push_back({label, tok_label});
                }
                else if (tok == tok_label2)
                {
                    tokens.push_back({label, tok_label2});
                }
            }
        }
        if (tok == tok_EOF)
            break;
    }

    mcFile << "0x" << std::hex << address << " " << "Terminate";
    mcFile.close();
}

int data_address = 0x10000000;

string decimalToHex(int decimal)
{
    char hexStr[11];
    sprintf(hexStr, "0x%08X", decimal);
    return string(hexStr);
}

string decimalToHexDword(unsigned long long decimal)
{
    char hexStr[19];
    sprintf(hexStr, "0x%016llX", decimal);
    return string(hexStr);
}

void preParse()
{
    // Reset file state and pointer for pre-parsing.
    asmFile.clear();
    asmFile.seekg(0);

    int address = 0;
    vector<pair<string, int>> tokens;

    while (true)
    {
        int tok = lex();
        if (tok == tok_directive)
        {
            if (directive == "data")
            {
                istext = false;
            }
            else if (directive == "text")
            {
                istext = true;
            }
        }
        if (istext)
        {
            if (tok == tok_EOF)
            {
                // End-of-file reached: process any remaining tokens.
                if (!tokens.empty())
                {
                    // If the first token is a label, store it.
                    if (tokens[0].second == tok_label)
                    {
                        labelMap[tokens[0].first] = address;

                        tokens.erase(tokens.begin());
                    }

                    if (!tokens.empty())
                        address += 4;
                }
                break;
            }

            if (tok == tok_newline)
            {
                if (!tokens.empty())
                {
                    // Check if the line begins with a label.
                    if (tokens[0].second == tok_label)
                    {
                        labelMap[tokens[0].first] = address;

                        tokens.erase(tokens.begin());
                    }

                    if (!tokens.empty())
                        address += 4;
                }
                tokens.clear();
            }
            else
            {
                // Store tokens (only for operator, register, immediate, and label).
                if (tok == tok_operator)
                    tokens.push_back({Operator, tok_operator});
                else if (tok == tok_register)
                    tokens.push_back({"x" + to_string(registerValue), tok_register});
                else if (tok == tok_immediate)
                    tokens.push_back({immediateValue, tok_immediate});
                else if (tok == tok_label)
                    tokens.push_back({label, tok_label});
            }
        }
        else
        {
            if (tok == tok_label)
            {
                tok = lex();
                if (tok == tok_directive)
                {
                    tok = lex();
                    while (tok == tok_immediate)
                    {
                        if (directive == "word")
                        {

                            int imm = stoi(immediateValue, nullptr, 0);
                            immediateValue = decimalToHex(imm);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(8, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(6, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(4, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(2, 2);
                        }

                        else if (directive == "byte")
                        {

                            int imm = stoi(immediateValue, nullptr, 0);
                            immediateValue = decimalToHex(imm);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(8, 2);
                        }

                        else if (directive == "half")
                        {

                            int imm = stoi(immediateValue, nullptr, 0);
                            immediateValue = decimalToHex(imm);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(8, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(6, 2);
                        }

                        else if (directive == "dword")
                        {

                            int imm = stoi(immediateValue, nullptr, 0);
                            immediateValue = decimalToHexDword(imm);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(16, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(14, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(12, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(10, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(8, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(6, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(4, 2);

                            memory[decimalToHex(data_address++)] = immediateValue.substr(2, 2);
                        }

                        tok = lex();
                    }

                    if (directive == "asciiz")
                    {

                        string str = literal;
                        if (!str.empty() && str.front() == '"' && str.back() == '"')
                        {
                            str = str.substr(1, str.size() - 2);
                        }
                        for (int i = 0; i < str.size(); i++)
                        {
                            int conv = str[i];
                            string toHex = decimalToHex(conv).substr(8, 2);
                            memory[decimalToHex(data_address++)] = toHex;
                        }
                        memory[decimalToHex(data_address++)] = "00";
                        tok = lex();
                    }
                }
            }
        }
    }
}

int main()
{
    asmFile.open("input.asm");
    if (!asmFile)
    {
        cerr << "Error: Unable to open input.asm" << endl;
        return 1;
    }
    preParse();

    asmFile.clear();
    asmFile.seekg(0);
    lastChar = ' ';

    assemble();
    return 0;
}