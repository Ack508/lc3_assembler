/*
 * @Author       : Chivier Humber
 * @Date         : 2021-08-30 15:10:31
 * @LastEditors  : liuly
 * @LastEditTime : 2022-11-15 21:10:23
 * @Description  : content for samll assembler
 */

#include "assembler.h"
#include <string>
#include<math.h>
#define MAX 10
int ifhex=0;
int orig_address = -1;
int orig[MAX] = { 0 };
int End[MAX] = { 0 };
int gfy = 0;
int gfyb = 0;
int specialnum(std::string s)
{
    int num = 0;
    for (int i = 0; i < s.length(); i++)
        if (s[i] == '\\')
            num++;
    return num;
}
int placeinorig(int address)
{
    for (int i = 0; i < MAX; i++)
    {
        if (orig[i] == address)
            return i;
    }
    return -1;
}

// add label and its address to symbol table
void LabelMapType::AddLabel(const std::string &str, const unsigned address) {
    labels_.insert({str, address});
}

unsigned LabelMapType::GetAddress(const std::string &str) const {
    if (labels_.find(str) == labels_.end()) {
        // not found
        return -1;
    }
    return labels_.at(str);
}

std::string assembler::TranslateOprand(unsigned int current_address, std::string str,
                                       int opcode_length) {
    // Translate the oprand
    str = Trim(str);
    auto item = label_map.GetAddress(str);
    if (item != -1) {
        // str is a label
        int pcoffset = item - current_address - 1;
        if (opcode_length == 6)
        {
            if (pcoffset > 31 || pcoffset < -32)
            {
                std::cout << "label->PCoffset out of range:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
                exit(-20);
            }
        }
        if (opcode_length == 9)
        {
            if (pcoffset >255  || pcoffset < -256)
            {
                std::cout << "label->PCoffset out of range:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
                exit(-20);
            }
        }
        if (opcode_length == 11)
        {
            if (pcoffset > 1023 || pcoffset < -1024)
            {
                std::cout << "label->PCoffset out of range:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
                exit(-20);
            }
        }
        return NumberToAssemble(item-current_address-1).substr(16-opcode_length,opcode_length);
        
    }
    if (str[0] == 'R') {
        // str is a register
        if (str[1] == '0')
            return "000";
        else if (str[1] == '1')
            return "001";
        else if (str[1] == '2')
            return "010";
        else if (str[1] == '3')
            return "011";
        else if (str[1] == '4')
            return "100";
        else if (str[1] == '5')
            return "101";
        else if (str[1] == '6')
            return "110";
        else if (str[1] == '7')
            return "111";
        else {
            std::cout << "Not a register:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
            exit(-20);
        }
    } else {
        // str is an immediate number
        if (str.find('#') == std::string::npos && str.find('x') == std::string::npos)
        {
            std::cout << "Not a true number:" << current_address - orig[0]+ 1 << " " << str << std::endl;//error
            exit(-20);
        }
        int number2 = RecognizeNumberValue(str);
        if (opcode_length == 6)
        {
            if (number2 > 31 || number2 < -32)
            {
                std::cout << "number out of range:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
                exit(-20);
            }
        }
        if (opcode_length == 9)
        {
            if (number2 > 255 || number2 < -256)
            {
                std::cout << "number out of range:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
                exit(-20);
            }
        }
        if (opcode_length == 11)
        {
            if (number2 > 1023 || number2 < -1024)
            {
                std::cout << "number out of range:" << current_address - orig[0] + 1 << " " << str << std::endl;//error
                exit(-20);
            }
        }
        str = NumberToAssemble(str);
        str = str.substr(16-opcode_length,opcode_length);
        return str;
    }
    return "";
}

std::string assembler::LineLabelSplit(const std::string &line,
                                      int current_address) {
    // label?
    auto first_whitespace_position = line.find(' ');
    auto first_token = line.substr(0, first_whitespace_position);

    if (IsLC3Pseudo(first_token) == -1 && IsLC3Command(first_token) == -1 &&
        IsLC3TrapRoutine(first_token) == -1) {
        // * This is an label
        if (!(first_token[0] == '_' || first_token[0] >= 'A' && first_token[0] <= 'Z' || first_token[0] >= 'a' && first_token[0] <= 'z'))
        {
            std::cout << "illegal label:" << current_address - orig[0] + 1 << " " << line << std::endl;//error
            exit(-20);
        }
        label_map.AddLabel(first_token,current_address);
        // save it in label_map
        

        // remove label from the line
        if (first_whitespace_position == std::string::npos) {
            // nothing else in the line
            return "";
        }
        auto command = line.substr(first_whitespace_position + 1);
        return Trim(command);
    }
    return line;
}

// Scan #1: save commands and labels with their addresses
int assembler::firstPass(std::string &input_filename) {
    std::string line;
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        std::cout << "Unable to open file" << std::endl;
        // @ Input file read error
        return -1;
    }

    
    int current_address = -1;
    int numpos=5;
    int charpos=0;
    int numlen;
    int addAddress=0;
    int firststr,secondstr;
    int i;
    while (std::getline(input_file, line)) {
        
        line = FormatLine(line);
        if (line.empty()) {
            continue;
        }

        auto command = LineLabelSplit(line, current_address);
        if (command.empty()) {
            continue;
        }

        // OPERATION or PSEUDO?
        auto first_whitespace_position = command.find(' ');
        auto first_token = command.substr(0, first_whitespace_position);

        // Special judge .ORIG and .END
        if (first_token == ".ORIG") {
            std::string orig_value =
                command.substr(first_whitespace_position + 1);
            orig_address = RecognizeNumberValue(orig_value);
            orig[gfy] = orig_address;
            if (orig_address == std::numeric_limits<int>::max()) {
                if(gIsErrorLogMode)
                    std::cout << "error address with orig" << std::endl;
                return -2;
            }
            current_address = orig_address;
            continue;
        }
        
        /*if (orig_address == -1) {
            if(gIsErrorLogMode)
                std::cout << "Error: Program begins before.ORIG" << std::endl;
            return -3;
        }*/

        if (first_token == ".END") {
            End[gfy++] = current_address;
            continue;
        }

        // For LC3 Operation
        if (IsLC3Command(first_token) != -1 ||
            IsLC3TrapRoutine(first_token) != -1) {
            commands.push_back(
                {current_address, command, CommandType::OPERATION});
            current_address += 1;
            continue;
        }

        // For Pseudo code
        commands.push_back({current_address, command, CommandType::PSEUDO});
        auto operand = command.substr(first_whitespace_position + 1);
        if (first_token == ".FILL") {
            auto num_temp = RecognizeNumberValue(operand);
            if (num_temp == std::numeric_limits<int>::max()) {
                if(gIsErrorLogMode)
                    std::cout << "Error: Invalid Number input @ FILL    " <<current_address-orig[0] + 1 << "  " << command << std::endl;
                return -4;
            }
            if (num_temp > 65535 || num_temp < -65536) {
                if (gIsErrorLogMode)
                    std::cout << "Error Too large or too small value  @ FILL    " <<  current_address-orig[0] + 1 << "  " << command << std::endl;
                return -5;
            }
            current_address += 1;
            continue;
        }
        if (first_token == ".BLKW") {
            // modify current_address
            std::string s;
            s = command.substr(command.find_first_of(' '));
            s=Trim(s);
            addAddress = RecognizeNumberValue(s);
            current_address += addAddress;
            continue;
        }
        if (first_token == ".STRINGZ") {
            // modify current_address
            firststr = command.find_first_of('"');
            secondstr = command.find_last_of('"');
            current_address += secondstr - firststr-specialnum(command);
            continue;
        }
        //unknown symbol
        if (gIsErrorLogMode) {
            std::cout << "Unknown symbol:" << current_address - orig[0] + 1 << "    " << command << std::endl;
        }
        exit(-30);
    }
    // OK flag
    return 0;
}

std::string assembler::TranslatePseudo(std::stringstream &command_stream) {
    std::string pseudo_opcode;
    std::string output_line;
    command_stream >> pseudo_opcode;
    if (pseudo_opcode == ".FILL") {
        std::string number_str;
        command_stream >> number_str;
        output_line = NumberToAssemble(number_str);
        if (ifhex==1)
            output_line = ConvertBin2Hex(output_line);
    } else if (pseudo_opcode == ".BLKW") {
        // Fill 0 here
        std::string number_str;
        std::string fk = "0000000000000000\n";
        std::string fk1 = "x0000\n";
        command_stream >> number_str;
        int num = RecognizeNumberValue(number_str);
        for (int i = 0; i < num - 1; i++) {
            if (ifhex)
                output_line.append(fk1);
            else
                output_line.append(fk);
        }
        if (ifhex)
            output_line.append("x0000");
        else
            output_line.append("0000000000000000");
    } else if (pseudo_opcode == ".STRINGZ") {
        // Fill string here
        int num;
        int place;
        std::string str;
        std::getline(command_stream,str,'$');
        str=Trim(str);
        str = str.substr(1, str.length() - 2);
        while (str.find('\\') != std::string::npos)
        {
            place = str.find_first_of("\\");
            if (str[place + 1] == 't')
                str[place] = '\t';
            if (str[place + 1] == 'n')
                str[place] = '\n';
            if (str[place + 1] == 'r')
                str[place] = '\r';
            if (str[place + 1] == 'v')
                str[place] = '\v';
            if (str[place + 1] == 'f')
                str[place] = '\f';
            str = str.erase(place + 1, 1);
        }
        for (int i = 0; i < str.length(); i++)
        {
            num = str[i];
            if (ifhex)
                output_line.append(ConvertBin2Hex(NumberToAssemble(num)));
            else
                output_line.append(NumberToAssemble(num));
            output_line.push_back('\n');
        }
        if (ifhex)
            output_line.append("x0000");
        else
            output_line.append("0000000000000000");
    }
    return output_line;
}

std::string assembler::TranslateCommand(std::stringstream &command_stream,
                                        unsigned int current_address) {
    std::string opcode;
    command_stream >> opcode;
    auto command_tag = IsLC3Command(opcode);

    std::vector<std::string> operand_list;
    std::string operand;
    while (command_stream >> operand) {
        operand_list.push_back(operand);
    }
    auto operand_list_size = operand_list.size();

    std::string output_line;

    if (command_tag == -1) {
        // This is a trap routine
        command_tag = IsLC3TrapRoutine(opcode);
        if (command_tag == -1) {
            // Unknown opcode
            if (gIsErrorLogMode) {
                std::cout << "Unknown opcode:" << current_address - orig[0] + 1 << "    " << opcode;
                
                std::cout << std::endl;
            }
            exit(-30);
        }
        output_line = kLC3TrapMachineCode[command_tag];
    } else {
        std::string s;
        int i;
        // This is a LC3 command
        switch (command_tag) {
        case 0:
            // "ADD"
            output_line += "0001";
            if (operand_list_size != 3) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 3):" << current_address-orig[0] + 1 << "    " << opcode;
                    for (std::string s:operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1]);
            if (operand_list[2][0] == 'R') {
                // The third operand is a register
                output_line += "000";
                output_line +=
                    TranslateOprand(current_address, operand_list[2]);
            } else {
                // The third operand is an immediate number
                output_line += "1";
                output_line +=
                    TranslateOprand(current_address, operand_list[2], 5);
                if (RecognizeNumberValue(operand_list[2]) > 15 || RecognizeNumberValue(operand_list[2]) < -16)
                {
                    std::cout << "number out of range:" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                    exit(-30);
                }

            }
            break;
        case 1:
            // "AND"
            output_line += "0101";
            if (operand_list_size != 3) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 3):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1]);
            if (operand_list[2][0] == 'R') {
                // The third operand is a register
                output_line += "000";
                output_line +=
                    TranslateOprand(current_address, operand_list[2]);
            }
            else {
                // The third operand is an immediate number
                output_line += "1";
                output_line +=
                    TranslateOprand(current_address, operand_list[2], 5);
                if (RecognizeNumberValue(operand_list[2]) > 15 || RecognizeNumberValue(operand_list[2]) < -16)
                {
                    std::cout << "number out of range:" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                    exit(-30);
                }
            }
            break;
            
        case 2:
            // "BR"
            output_line += "0000111";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                exit(-30);
            }

            output_line += TranslateOprand(current_address,operand_list[0],9);
            break;
        case 3:
            // "BRN"
            output_line += "0000100";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 4:
            // "BRZ"
            output_line += "0000010";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 5:
            // "BRP"
            output_line += "0000001";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 6:
            // "BRNZ"
            output_line += "0000110";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 7:
            // "BRNP"
            output_line += "0000101";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 8:
            // "BRZP"
            output_line += "0000011";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 9:
            // "BRNZP"
            output_line += "0000111";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 9);
            break;
        case 10:
            // "JMP"
            output_line += "1100000";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address,operand_list[0]);
            output_line += "000000";
            break;
        case 11:
            // "JSR"
            output_line += "01001";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0], 11);
            break;
        case 12:
            // "JSRR"
            output_line += "0100000";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += "000000";
            break;
        case 13:
            // "LD"
            output_line += "0010";
            if (operand_list_size != 2) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 2):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1],9);
            break;
        case 14:
            // "LDI"
            output_line += "1010";
            if (operand_list_size != 2) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 2):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1], 9);
            break;
        case 15:
            // "LDR"
            output_line += "0110";
            if (operand_list_size != 3) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 3):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1]);
            output_line += TranslateOprand(current_address, operand_list[2],6);

            break;
        case 16:
            // "LEA"
            output_line += "1110";
            if (operand_list_size != 2) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 2):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1],9);

            break;
        case 17:
            // "NOT"
            output_line += "1001";
            if (operand_list_size != 2) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 2):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1]);
            output_line += "111111";
            break;
        case 18:
            // RET
            output_line += "1100000111000000";
            if (operand_list_size != 0) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 0):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            break;
        case 19:
            // RTI
            output_line += "1000000000000000";
            if (operand_list_size != 0) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 0):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            break;
        case 20:
            // ST
            output_line += "0011";
            if (operand_list_size != 2) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 2):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1], 9);
            break;
        case 21:
            // STI
            output_line += "1011";
            if (operand_list_size != 2) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 2):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1], 9);
            break;
        case 22:
            // STR
            output_line += "0111";
            if (operand_list_size != 3) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 3):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            output_line += TranslateOprand(current_address, operand_list[0]);
            output_line += TranslateOprand(current_address, operand_list[1]);
            output_line += TranslateOprand(current_address, operand_list[2],6);
            break;
        case 23:
            // TRAP
            output_line += "11110000";
            if (operand_list_size != 1) {
                if (gIsErrorLogMode) {
                    std::cout << "Error operand numbers(should be 1):" << current_address - orig[0] + 1 << "    " << opcode;
                    for (std::string s : operand_list)
                    {
                        std::cout << " " << s;
                    }
                    std::cout << std::endl;
                }
                // @ Error operand numbers
                exit(-30);
            }
            s = operand_list[0];
            if (s.length() != 3)
            {
                std::cout << "error trap number:" << current_address - orig[0] + 1 << "    " << opcode;
                for (std::string s : operand_list)
                {
                    std::cout << " " << s;
                }
                std::cout << std::endl;
                exit(-30);
            }
            for (i = 1; i < 3; i++)
            {
                if (s[i] == '0')
                    output_line.append("0000");
                else if (s[i] == '1')
                    output_line.append("0001");
                else if (s[i] == '2')
                    output_line.append("0010");
                else if (s[i] == '3')
                    output_line.append("0011");
                else if (s[i] == '4')
                    output_line.append("0100");
                else if (s[i] == '5')
                    output_line.append("0101");
                else if (s[i] == '6')
                    output_line.append("0110");
                else if (s[i] == '7')
                    output_line.append("0111");
                else if (s[i] == '8')
                    output_line.append("1000");
                else if (s[i] == '9')
                    output_line.append("1001");
                else if (s[i] == 'a' || s[i] == 'A')
                    output_line.append("1010");
                else if (s[i] == 'b' || s[i] == 'B')
                    output_line.append("1011");
                else if (s[i] == 'c' || s[i] == 'C')
                    output_line.append("1100");
                else if (s[i] == 'd' || s[i] == 'D')
                    output_line.append("1101");
                else if (s[i] == 'e' || s[i] == 'E')
                    output_line.append("1110");
                else
                    output_line.append("1111");
            }
            break;
        default:
            
            break;
        }
    }

    if (ifhex==1)
        output_line = ConvertBin2Hex(output_line);
    return output_line;
}

int assembler::secondPass(std::string &output_filename) {
    // Scan #2:
    // Translate
    std::ofstream output_file;
    // Create the output file
    output_file.open(output_filename);
    if (!output_file) {
        if (gIsErrorLogMode)
            std::cout << " Error at output file" << std::endl;
        // @ Error at output file
        return -20;
    }

    for (const auto &command : commands) {
        const unsigned address = std::get<0>(command);
        const std::string command_content = std::get<1>(command);
        const CommandType command_type = std::get<2>(command);
        auto command_stream = std::stringstream(command_content);

        if (placeinorig(address) != -1 && placeinorig(address) != 0)
        {
            gfyb++;
            for (int i = 0; i < orig[placeinorig(address)] - End[placeinorig(address) - 1]; i++)
                output_file << "0000000000000000" << std::endl;
        }
        if (command_type == CommandType::PSEUDO) {
            // Pseudo
            output_file << TranslatePseudo(command_stream) << std::endl;
        } else {
            // LC3 command
            output_file << TranslateCommand(command_stream, address) << std::endl;
        }
        
    }

    // Close the output file
    output_file.close();
    // OK flag
    return 0;
}

// assemble main function
int assembler::assemble(std::string &input_filename, std::string &output_filename) {
    auto first_scan_status = firstPass(input_filename);
    if (first_scan_status != 0) {
        return first_scan_status;
    }
    std::cout << "bin->hex?(Yes:1  No:0)" << std::endl;
    std::cin >> ifhex;
    auto second_scan_status = secondPass(output_filename);
    if (second_scan_status != 0) {
        return second_scan_status;
    }
    // OK flag
    return 0;
}
