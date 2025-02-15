#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
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
int lex()
{
    int lastChar = ' ';
    while (isspace(lastChar))
    {
        lastChar = asmFile.get();
    }

    if (lastChar == ',')
    {
        while (lastChar == ',')
        {
            lastChar = asmFile.get();
        }
    }

    if (lastChar == EOF || asmFile.eof())
        return tok_EOF;
    if (lastChar == '#')
    {
        while (lastChar != '\n' && lastChar != EOF)
        {
            lastChar = asmFile.get();
        }
        if (asmFile.eof() || lastChar == EOF)
        {
            return tok_EOF;
        }
        return lex();
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

    if (lastChar == 'x' && isdigit(asmFile.peek()))
    {
        lastChar = asmFile.get();
        string num = "";
        while (isdigit(lastChar) && lastChar != EOF)
        {
            num += lastChar;
            lastChar = asmFile.get();
        }
        registerValue = stoi(num);
        if (registerValue >= 0 && registerValue <= 31)
        {
            return tok_register;
        }
    }

    if (isalpha(lastChar))
    {
        Operator = "";
        while ((isalnum(lastChar) || (lastChar == '_')) && lastChar != EOF)
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
            return tok_label;
        }
        return tok_operator;
    }

    if (isdigit(lastChar) || lastChar == '-')
    {
        string num = "";
        bool isHex = false;
        if (lastChar == '-')
        {
            num += lastChar;
            lastChar = asmFile.get();
        }

        if (lastChar == '0' && asmFile.peek() == 'x')
        {
            isHex = true;
            num += lastChar;
            lastChar = asmFile.get();
            num += lastChar;
            lastChar = asmFile.get();
        }

        while (isxdigit(lastChar) && lastChar != EOF)
        {
            num += lastChar;
            if (isxdigit(asmFile.peek()))
            {
                lastChar = asmFile.get();
            }
            else
            {
                break;
            }
        }
        immediateValue = num;

        return tok_immediate;
    }

    lastChar = asmFile.get();
    return lex();
}

int main()
{
    asmFile.open("input.asm");
    if (!asmFile)
    {
        cerr << "Error: Unable to open input.asm" << endl;
        return 1;
    }

    while (true)
    {
        int token = lex();
        if (token == tok_EOF)
            break;

        switch (token)
        {
        case tok_operator:
            cout << "Operator: " << Operator << endl;
            break;
        case tok_label:
            cout << "Label: " << label << endl;
            break;
        case tok_register:
            cout << "Register: x" << registerValue << endl;
            break;
        case tok_immediate:
            cout << "Immediate: " << immediateValue << endl;
            break;
        case tok_directive:
            cout << "Directive: " << directive << endl;
            break;
        default:
            cout << "Unknown Token" << endl;
        }
    }
    return 0;
}