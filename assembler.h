/*
 * @Author       : Chivier Humber
 * @Date         : 2021-08-30 14:36:39
 * @LastEditors  : liuly
 * @LastEditTime : 2022-11-15 21:12:51
 * @Description  : header file for small assembler
 */

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <cmath>

const int kLC3LineLength = 16;

extern bool gIsErrorLogMode;
extern bool gIsHexMode;

const std::vector<std::string> kLC3Pseudos({
    ".ORIG",
    ".END",
    ".STRINGZ",
    ".FILL",
    ".BLKW",
});

const std::vector<std::string> kLC3Commands({
    "ADD",    // 00: "0001" + reg(line[1]) + reg(line[2]) + op(line[3])
    "AND",    // 01: "0101" + reg(line[1]) + reg(line[2]) + op(line[3])
    "BR",     // 02: "0000000" + pcoffset(line[1],9)  有误！！！，应为0000111
    "BRN",    // 03: "0000100" + pcoffset(line[1],9)
    "BRZ",    // 04: "0000010" + pcoffset(line[1],9)
    "BRP",    // 05: "0000001" + pcoffset(line[1],9)
    "BRNZ",   // 06: "0000110" + pcoffset(line[1],9)
    "BRNP",   // 07: "0000101" + pcoffset(line[1],9)
    "BRZP",   // 08: "0000011" + pcoffset(line[1],9)
    "BRNZP",  // 09: "0000111" + pcoffset(line[1],9)
    "JMP",    // 10: "1100000" + reg(line[1]) + "000000"
    "JSR",    // 11: "01001" + pcoffset(line[1],11)
    "JSRR",   // 12: "0100000"+reg(line[1])+"000000"
    "LD",     // 13: "0010" + reg(line[1]) + pcoffset(line[2],9)
    "LDI",    // 14: "1010" + reg(line[1]) + pcoffset(line[2],9)
    "LDR",    // 15: "0110" + reg(line[1]) + reg(line[2]) + offset(line[3])
    "LEA",    // 16: "1110" + reg(line[1]) + pcoffset(line[2],9)
    "NOT",    // 17: "1001" + reg(line[1]) + reg(line[2]) + "111111"
    "RET",    // 18: "1100000111000000"
    "RTI",    // 19: "1000000000000000"
    "ST",     // 20: "0011" + reg(line[1]) + pcoffset(line[2],9)
    "STI",    // 21: "1011" + reg(line[1]) + pcoffset(line[2],9)
    "STR",    // 22: "0111" + reg(line[1]) + reg(line[2]) + offset(line[3])
    "TRAP"    // 23: "11110000" + h2b(line[1],8)
});

const std::vector<std::string> kLC3TrapRoutine({
    "GETC",   // x20
    "OUT",    // x21
    "PUTS",   // x22
    "IN",     // x23
    "PUTSP",  // x24
    "HALT"    // x25
});

const std::vector<std::string> kLC3TrapMachineCode({
    "1111000000100000",
    "1111000000100001",
    "1111000000100010",
    "1111000000100011",
    "1111000000100100",
    "1111000000100101"
});

enum CommandType { OPERATION, PSEUDO };

static inline void SetErrorLogMode(bool error) {
    gIsErrorLogMode = error;
}

static inline void SetHexMode(bool hex) {
    gIsHexMode = hex;
}

// A warpper class for std::unorderd_map in order to map label to its address
class LabelMapType {
private:
    std::unordered_map<std::string, unsigned> labels_;

public:
    void AddLabel(const std::string &str, unsigned address);
    unsigned GetAddress(const std::string &str) const;
};

static inline int IsLC3Pseudo(const std::string &str) {
    int index = 0;
    for (const auto &command : kLC3Pseudos) {
        if (str == command) {
            return index;
        }
        ++index;
    }
    return -1;
}

static inline int IsLC3Command(const std::string &str) {
    int index = 0;
    for (const auto &command : kLC3Commands) {
        if (str == command) {
            return index;
        }
        ++index;
    }
    return -1;
}

static inline int IsLC3TrapRoutine(const std::string &str) {
    int index = 0;
    for (const auto &trap : kLC3TrapRoutine) {
        if (str == trap) {
            return index;
        }
        ++index;
    }
    return -1;
}

static inline int CharToDec(const char &ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

static inline char DecToChar(const int &num) {
    if (num <= 9) {
        return num + '0';
    }
    return num - 10 + 'A';
}

// trim string from both left & right
static inline std::string &Trim(std::string &s) {
    int i = 0;
    while (s[i] == ' ')
    {
        i++;
    }
    s = s.substr(i);
    std::reverse(s.begin(), s.end());
    i = 0;
    while (s[i] == ' ')
    {
        i++;
    }
    s = s.substr(i);
    std::reverse(s.begin(), s.end());
    return s;
    
}

// Format one line from asm file, do the following:
// 1. remove comments
// 2. convert the line into uppercase
// 3. replace all commas with whitespace (for splitting)
// 4. replace all "\t\n\r\f\v" with whitespace
// 5. remove the leading and trailing whitespace chars
// Note: please implement function Trim first
static std::string FormatLine(const std::string &line) {
    std::string s,t;
    s = line;
    int flag=0;//是否有.STRINGZ
    int head;
    int first,second;//记录第一个引号和第二个引号的位置
    //有.STRINGZ的要单独处理，将引号部分分开
    if (s.find(".STRINGZ") != std::string::npos)
    {
        flag = 1;
        first = s.find_first_of('"');
        second = s.find_last_of('"');
        t = s.substr(first,second-first+1);
        s = s.erase(first);
    }
    //remove comments
    if (s.find(';') == 0)
        return "\0";
    if (s.find(';') != std::string::npos)
    {
        head = s.find(';');
        s=s.erase(head);
    }
    //小写->大写
    int length = s.length();
    for (int i = 0; i < length; i++)
    {
        if (s[i]<= 'z' && s[i]>='a')
            s[i] = s[i] -32;
    }
    //逗号->空格
    for (int i = 0; i < length; i++)
    {
        if (s[i] ==',')
            s[i] = ' ';
    }
    //特殊字符->空格
    for (int i = 0; i < length; i++)
    {
        if (s[i] == '\t'||s[i]=='\n'||s[i]=='\r'||s[i]=='\f'||s[i]=='\v')
            s[i] = ' ';
    }
    //Trim
    s = Trim(s);
    if (flag)
        return s + ' ' + t;
    else
        return s;
}

static int RecognizeNumberValue(const std::string &str) //注意：#和x的区别，以及开头可能有空格
{
    // Convert string `str` into a number and return it
    int i;
    int num = 0;
    std::string s;
    
    if (str.find('#') != std::string::npos) //十进制，分正负讨论
    {
        s = str.substr(str.find('#')+1,str.length()-str.find('#')-1);
        if (s.find('-') == std::string::npos) {
            for (i = 0; i < s.length(); i++)
                num += (s[i] - '0') * pow(10, s.length() - i - 1);
            return num;
        }
        else
        {
            for (i = 1; i < s.length(); i++)
                num += (s[i] - '0') * pow(10, s.length() - i - 1);
            return -num;
        }
    }
    else//十六进制，以x开头
    {
        s = str.substr(str.find('x') + 1, str.length() - str.find('x') - 1);
        for (int i = 0; i < s.length(); i++)
        {
            if(s[i]>='a'&&s[i]<='f')
                num+=(s[i]-'a'+10)* pow(16, s.length() - i - 1);
            if (s[i] >= 'A' && s[i] <= 'F')
                num += (s[i] - 'A'+10) * pow(16, s.length() - i - 1);
            if (s[i] >= '0' && s[i] <= '9')
                num += (s[i] - '0') * pow(16, s.length() - i - 1);
        }
        return num;
    }
    return 0;
}

static std::string NumberToAssemble(const int &number) {
    // Convert `number` into a 16 bit binary string
    int i = 0;
    int num = number;
    int sub;
    std::string s,s1;
    if (num == 0)
        return "0000000000000000";
    if (num < 0)
        num = 65536 + num;
    while (num > 0)
    {
        i++;
        sub = num % 2;
        num = num / 2;
        s.push_back('0' + sub);
    }
    std::reverse(s.begin(), s.end());
    if(number>0)
        for (int j = 0; j <16-i;j++ )
            s1.push_back('0');
    else
        for (int j = 0; j < 16 - i; j++)
            s1.push_back('1');
    return s1 + s;
}

static std::string NumberToAssemble(const std::string &number) {
    // Convert `number` into a 16 bit binary string
    // You might use `RecognizeNumberValue` in this function
    return NumberToAssemble(RecognizeNumberValue(number));
}

static std::string ConvertBin2Hex(const std::string &bin) {
    // Convert the binary string `bin` into a hex string
    std::string s;
    int i = 0;
    for (i = 0; i < 13;i = i + 4)
    {
        if (bin.substr(i, 4) == "0000")
            s.push_back('0');
        else if (bin.substr(i, 4) == "0001")
            s.push_back('1');
        else if (bin.substr(i, 4) == "0010")
            s.push_back('2');
        else if (bin.substr(i, 4) == "0011")
            s.push_back('3');
        else if (bin.substr(i, 4) == "0100")
            s.push_back('4');
        else if (bin.substr(i, 4) == "0101")
            s.push_back('5');
        else if (bin.substr(i, 4) == "0110")
            s.push_back('6');
        else if (bin.substr(i, 4) == "0111")
            s.push_back('7');
        else if (bin.substr(i, 4) == "1000")
            s.push_back('8');
        else if (bin.substr(i, 4) == "1001")
            s.push_back('9');
        else if (bin.substr(i, 4) == "1010")
            s.push_back('A');
        else if (bin.substr(i, 4) == "1011")
            s.push_back('B');
        else if (bin.substr(i, 4) == "1100")
            s.push_back('C');
        else if (bin.substr(i, 4) == "1101")
            s.push_back('D');
        else if (bin.substr(i, 4) == "1110")
            s.push_back('E');
        else 
            s.push_back('F');
    }
    s = 'x' + s;
    return s;
}

class assembler {
    using Commands = std::vector<std::tuple<unsigned, std::string, CommandType>>;

private:
    LabelMapType label_map;
    Commands commands;

    static std::string TranslatePseudo(std::stringstream &command_stream);
    std::string TranslateCommand(std::stringstream &command_stream,
                                 unsigned int current_address);
    std::string TranslateOprand(unsigned int current_address, std::string str,
                                int opcode_length = 3);
    std::string LineLabelSplit(const std::string &line, int current_address);
    int firstPass(std::string &input_filename);
    int secondPass(std::string &output_filename);

public:
    int assemble(std::string &input_filename, std::string &output_filename);
};
