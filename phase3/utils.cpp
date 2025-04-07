#include "simulator.h"
#include <stdexcept>
#include <sstream>

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

