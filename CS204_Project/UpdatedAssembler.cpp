#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <set>
using namespace std;


enum Token
{
    tok_operator = -1,
    tok_label = -2,
    tok_register = -3,
    tok_immediate = -4,
    tok_EOF = -5,
    tok_directive = -6,
    tok_newline=-7,
};
ifstream asmFile;
string Operator;
string label;
string directive;
int registerValue = -1;
string immediateValue;
unordered_set<string> operators = {"add", "and", "or", "sll", "slt", "sra", "srl", "sub", "xor", "mul", "div", "rem", "addi", "andi", "ori", "xori", "slli", "srli", "srai", "slti", "sltiu", "lb", "ld", "lh", "lw", "jalr", "sb", "sw", "sd", "sh", "beq", "bne", "bge", "blt", "auipc", "lui", "jal"};
unordered_map<string, int> reg = {
    {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4}, {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"s1", 9}, {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24}, {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31}};
// Declare lastChar as a static variable so its state is preserved.
static int lastChar = ' ';

int lex() {
    // Skip whitespace and return a newline token immediately when encountered.
    while (isspace(lastChar)) {
        if (lastChar == '\n') {
            lastChar = asmFile.get(); // consume the newline
            return tok_newline;
        }
        lastChar = asmFile.get();
    }

    // Skip commas.
    if (lastChar == ',') {
        while (lastChar == ',')
            lastChar = asmFile.get();
    }

    if (lastChar == EOF)
        return tok_EOF;
    if (lastChar == '#') {
        while (lastChar != '\n' && lastChar != EOF)
            lastChar = asmFile.get();
        if (lastChar == EOF)
            return tok_EOF;
        return tok_newline;
    }
    if (lastChar == '.') {
        directive = "";
        lastChar = asmFile.get();
        while (isalnum(lastChar)) {
            directive += lastChar;
            lastChar = asmFile.get();
        }
        return tok_directive;
    }
    // Handle registers written as x<number>
    if (lastChar == 'x' && isdigit(asmFile.peek())) {
        lastChar = asmFile.get(); // consume 'x'
        string num = "";
        while (isdigit(lastChar)) {
            num += lastChar;
            lastChar = asmFile.get();
        }
        registerValue = stoi(num);
        if (registerValue >= 0 && registerValue <= 31)
            return tok_register;
    }
    // Handle identifiers (operator names, labels, or register names by mnemonic)
    if (isalpha(lastChar)) {
        Operator = "";
        while ((isalnum(lastChar) || lastChar == '_') && lastChar != EOF) {
            Operator += lastChar;
            lastChar = asmFile.get();
        }
        if (reg.find(Operator) != reg.end()) {
            registerValue = reg[Operator];
            return tok_register;
        }
        if (operators.find(Operator) == operators.end()) {
            label = Operator;
            return tok_label;
        }
        return tok_operator;
    }
    // Handle immediate values.
    if (isdigit(lastChar) || lastChar == '-') {
        string num = "";
        if (lastChar == '-') {
            num += lastChar;
            lastChar = asmFile.get();
        }
        if (lastChar == '0' && asmFile.peek() == 'x') {
            num += lastChar;
            lastChar = asmFile.get();
            num += lastChar;
            lastChar = asmFile.get();
        }
        while (isxdigit(lastChar)) {
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
    {"x0", "00000"}, {"x1", "00001"}, {"x2", "00010"}, {"x3", "00011"},
    {"x4", "00100"}, {"x5", "00101"}, {"x6", "00110"}, {"x7", "00111"},
    {"x8", "01000"}, {"x9", "01001"}, {"x10", "01010"}, {"x11", "01011"},
    {"x12", "01100"}, {"x13", "01101"}, {"x14", "01110"}, {"x15", "01111"},
    {"x16", "10000"}, {"x17", "10001"}, {"x18", "10010"}, {"x19", "10011"},
    {"x20", "10100"}, {"x21", "10101"}, {"x22", "10110"}, {"x23", "10111"},
    {"x24", "11000"}, {"x25", "11001"}, {"x26", "11010"}, {"x27", "11011"},
    {"x28", "11100"}, {"x29", "11101"}, {"x30", "11110"}, {"x31", "11111"}
};

// Mapping instructions to their binary opcode, func3, and func7 values
struct Instruction {
    string opcode, func3, func7;
};
unordered_map<string, Instruction> instMap = {
    {"add", {"0110011", "000", "0000000"}}, {"sub", {"0110011", "000", "0100000"}},
    {"and", {"0110011", "111", "0000000"}}, {"or", {"0110011", "110", "0000000"}},
    {"xor", {"0110011", "100", "0000000"}}, {"sll", {"0110011", "001", "0000000"}},
    {"srl", {"0110011", "101", "0000000"}}, {"sra", {"0110011", "101", "0100000"}},
    {"slt", {"0110011", "010", "0000000"}}, {"mul", {"0110011", "000", "0000001"}},
    {"div", {"0110011", "100", "0000001"}}, {"rem", {"0110011", "110", "0000001"}},

    {"addi", {"0010011", "000", ""}}, {"andi", {"0010011", "111", ""}}, {"ori", {"0010011", "110", ""}},
    {"lb", {"0000011", "000", ""}}, {"lh", {"0000011", "001", ""}}, {"lw", {"0000011", "010", ""}}, {"ld", {"0000011", "011", ""}},
    {"jalr", {"1100111", "000", ""}},

    {"sb", {"0100011", "000", ""}}, {"sh", {"0100011", "001", ""}}, {"sw", {"0100011", "010", ""}}, {"sd", {"0100011", "011", ""}},

    {"beq", {"1100011", "000", ""}}, {"bne", {"1100011", "001", ""}}, {"blt", {"1100011", "100", ""}}, {"bge", {"1100011", "101", ""}},

    {"auipc", {"0010111", "", ""}}, {"lui", {"0110111", "", ""}}, {"jal", {"1101111", "", ""}}
};

// Label mapping to addresses
unordered_map<string, int> labelMap;

// Convert immediate value to binary with sign extension
string immToBinary(int num, int bits) {
    string binary = "";
    for (int i = 0; i < bits; i++) {
        binary = ((num & (1 << i)) ? '1' : '0') + binary;
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
}machine_code;




void processInstruction(vector<string> &tokens, ofstream &mcFile, int &address) {
    if (tokens.empty()) return;  // No instruction to process

    string op = tokens[0];  // First token is the opcode
    vector<string> operands(tokens.begin() + 1, tokens.end());  // Remaining tokens are operands

    // Validate if the opcode exists
    if (instMap.find(op) == instMap.end()) {
        cerr << "Unknown instruction: " << op << endl;
        return;
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

    if (inst.opcode == "0110011") {  // R-type (e.g., add, sub)
        if (operands.size() < 3) {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "R";
        mc.rd = regMap[operands[0]];
        mc.rs1 = regMap[operands[1]];
        mc.rs2 = regMap[operands[2]];
    }
    else if (inst.opcode == "0010011") {  // I-type (e.g., addi)
        if (operands.size() < 3) {
            cerr << "Error: Not enough operands for " << op << endl;
            return;
        }
        mc.format_type = "I";
        mc.rd = regMap[operands[0]];
        mc.rs1 = regMap[operands[1]];
        mc.imm = immToBinary(stoi(operands[2]), 12);
    }
    
    // Write output
    mcFile << std::hex << address << " ";
    if (mc.format_type == "R") {
        mcFile << mc.funct7 << mc.rs2 << mc.rs1 << mc.funct3 << mc.rd << mc.opcode << "  ";
        for (const auto &t : tokens)
            mcFile << t << " ";
        mcFile << " # " << mc.funct7 << "-" << mc.rs2 << "-" << mc.rs1 
               << "-" << mc.funct3 << "-" << mc.rd << "-" << mc.opcode;
    }
    else if (mc.format_type == "I") {
        mcFile << mc.imm << mc.rs1 << mc.funct3 << mc.rd << mc.opcode << "  ";
        for (const auto &t : tokens)
            mcFile << t << " ";
        mcFile << " # " << mc.imm << "-" << mc.rs1 << "-" << mc.funct3 
               << "-" << mc.rd << "-" << mc.opcode;
    }
    mcFile << endl;
    
    address += 4;  // Increment address for next instruction
}



void assemble() {
    ofstream mcFile("output.mc");
    if (!mcFile) {
        cerr << "Error: Unable to create output.mc" << endl;
        return;
    }
    
    int address = 0; // starting address for instructions
    vector<string> tokens;  // Accumulate tokens for one instruction
    
    while (true) {
        int tok = lex();  // Read next token
        
        if (tok == tok_EOF) {
            // If any tokens remain for the last line, process them
            if (!tokens.empty()) {
                processInstruction(tokens, mcFile, address);
            }
            break;  // Stop reading
        }
        
        if (tok == tok_newline) {
            // Process the accumulated tokens when a newline is reached
            if (!tokens.empty()) {
                processInstruction(tokens, mcFile, address);
                tokens.clear();  // Clear tokens for next instruction
            }
        }
        else {
            // Store token as a string for processing
            if (tok == tok_operator) {
                tokens.push_back(Operator);
            } else if (tok == tok_register) {
                tokens.push_back("x" + to_string(registerValue));
            } else if (tok == tok_immediate) {
                tokens.push_back(immediateValue);
            } else if (tok == tok_directive) {
                tokens.push_back("." + directive);
            } else if (tok == tok_label) {
                tokens.push_back(label);
            }
        }
    }
    
    mcFile.close();
}




int main() {
    asmFile.open("input.asm");
    if (!asmFile)
    {
        cerr << "Error: Unable to open input.asm" << endl;
        return 1;
    }
    assemble();
    return 0;
}