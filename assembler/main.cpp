#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <bitset>

using namespace std;

// ALU opcode mapping
map<string, uint8_t> alu_opcodes = {
    {"MOV", 0b0000},
    {"ADD", 0b0001},
    {"SUB", 0b0010},
    {"OR",  0b0011},
    {"AND", 0b0100},
    {"XOR", 0b0101},
    {"JZ",  0b0110},
    {"JZA", 0b0111},
    {"JZS", 0b1000},
    {"JMP", 0b1111}
};

// Argument type mapping
map<char, pair<string, bool>> arg_types = {
    {'#', {"000", false}}, // Immediate (constant)
    {'A', {"011", true}},  // Register A
    {'a', {"011", true}},  // Register A (alternative)
    {'B', {"101", true}},  // Register B
    {'b', {"101", true}},  // Register B (alternative)
    {'$', {"001", false}}, // Memory (RAM)
    {'%', {"111", false}}, // Display (screen)
    {'[', {"000", false}}  // Default for dynamic addressing
};

map<string, int> labels = {};

struct Instruction {
    string opcode;
    string arg1;
    string arg2;
    string dest;
    bool dynamic_addr = false;
    char dyn_reg = 'A'; // Default to register A
};

string trim(const string& s) {
    auto start = s.find_first_not_of(" \t");
    auto end = s.find_last_not_of(" \t");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Function to reverse bits in a string
string reverse_bits(const string& bits) {
    string reversed;
    for (int i = bits.length() - 1; i >= 0; i--) {
        reversed += bits[i];
    }
    return reversed;
}

int command = 0;
int atLine = 0;
bool parsing = true;

vector<Instruction> parse_program(const string& program) {
    vector<Instruction> instructions;
    istringstream iss(program);
    string line;
	
	command = 0;
	atLine = 0;
	parsing = true;
    
    while (getline(iss, line)) {
        // Remove comments
        line = line.substr(0, line.find(';'));
        
        // Trim whitespace
        line = trim(line);
		
		atLine++;
        
        if (line.empty()) continue;
		
		if (line[0] == '~') {
			labels.emplace(line.substr(1), command);
			continue;
		}
        
        Instruction inst;
        size_t space_pos = line.find(' ');
        if (space_pos == string::npos) {
            inst.opcode = line;
            instructions.push_back(inst);
            continue;
        }
        
        inst.opcode = line.substr(0, space_pos);
        string args_str = line.substr(space_pos + 1);
        
        // Parse destination (if -> exists)
        size_t arrow_pos = args_str.find("->");
        if (arrow_pos != string::npos) {
            inst.dest = trim(args_str.substr(arrow_pos + 2));
            args_str = trim(args_str.substr(0, arrow_pos));
        }
        
        // Split arguments
        vector<string> args;
        size_t start = 0;
        bool in_brackets = false;
        for (size_t i = 0; i < args_str.length(); i++) {
            if (args_str[i] == '[') {
                in_brackets = true;
                continue;
            }
            if (args_str[i] == ']') {
                in_brackets = false;
                continue;
            }
            
            if (args_str[i] == ',' && !in_brackets) {
                args.push_back(trim(args_str.substr(start, i - start)));
                start = i + 1;
            }
        }
        args.push_back(trim(args_str.substr(start)));
        
        if (args.size() >= 1) inst.arg1 = args[0];
        if (args.size() >= 2) inst.arg2 = args[1];
        
        // Check for dynamic addressing
        if (!inst.dest.empty() && (inst.dest[0] == '[' || (inst.dest[0] == '%' && inst.dest[1] == '['))) {
            inst.dynamic_addr = true;
            size_t reg_pos = inst.dest.find_first_of("ABab");
            if (reg_pos != string::npos) {
                inst.dyn_reg = toupper(inst.dest[reg_pos]);
            }
        }
        
        instructions.push_back(inst);
		command++;
    }
    
    return instructions;
}

pair<string, string> parse_arg(const string& arg) {
    if (arg.empty()) return {"000", "00000000"};
	
	if (arg[0] == '~') return {"000", to_string(labels[arg.substr(1)])};
	
	if (arg.size() >= 3 && arg[0] == '%' && arg[1] == '[') {
        size_t close_bracket = arg.find(']');
        if (close_bracket != string::npos) {
            string reg = arg.substr(2, close_bracket - 2);
            if (reg == "A" || reg == "a") return {"111", "00000000"}; // Screen with A
            if (reg == "B" || reg == "b") return {"111", "00000000"}; // Screen with B
        }
    }
    
    char type = arg[0];
    string value;
    
    if (type == '[') {
        // Dynamic address - extract the value inside brackets
        size_t end = arg.find(']');
        if (end != string::npos) {
            value = arg.substr(1, end - 1);
        }
        return {"000", value}; // Type will be overridden by mode bit
    }
    
    if (arg_types.find(type) == arg_types.end()) {
        // Assume immediate value if no type prefix
        return {"000", arg};
    }
    
    if (arg_types[type].second) {
        // Register - no value needed
        return {arg_types[type].first, "00000000"};
    } else {
        // Memory/immediate - extract value
        value = arg.substr(1);
        return {arg_types[type].first, value};
    }
}

string parse_dest(const string& dest) {
    if (dest.empty()) return "00"; // Default to no destination
	
	// Handle dynamic screen destination (%[B])
    if (dest.size() >= 3 && dest[0] == '%' && dest[1] == '[') {
        return "11"; // Screen destination
    }
    
    if (dest[0] == '[') return "00"; // Dynamic address handled separately
    
    char type = dest[0];
    if (type == 'A' || type == 'a') return "01";
    if (type == 'B' || type == 'b') return "10";
    if (type == '$') return "00";
    if (type == '%') return "11";
    
    return "00"; // Default to RAM
}

string to_binary_string(uint8_t value, int bits) {
    return bitset<8>(value).to_string().substr(8 - bits);
}

void compile_program(const vector<Instruction>& instructions, const string& output_prefix) {
    string rom1, rom2, rom3;
	parsing = false;
	atLine = 0;
	command = 0;
    
    for (const auto& inst : instructions) {
        // Parse arguments
        auto arg1 = parse_arg(inst.arg1);
        auto arg2 = parse_arg(inst.arg2);
        
        // ROM1: Control bits (16 bits total)
        string rom1_bits;
		
		rom1_bits += "000000";
        rom1_bits += (inst.dyn_reg == 'A') ? "0" : "1";
        rom1_bits += inst.dynamic_addr ? "1" : "0";
        rom1_bits += parse_dest(inst.dest);
        rom1_bits += arg2.first;
        rom1_bits += arg1.first;
        
        // Reverse the entire 16-bit string
        rom1 += rom1_bits + "\n";
        
        // ROM2: Destination address + opcode (16 bits total)
        string rom2_bits;
        
        // First byte: destination address (for RAM/screen)
        if (!inst.dynamic_addr && !inst.dest.empty() && (inst.dest[0] == '$' || inst.dest[0] == '%')) {
			string addr = inst.dest.substr(1);
            rom2_bits += bitset<8>(stoi(addr)).to_string();
        } else {
            rom2_bits += "00000000";
        }
        
        // Second byte: opcode (only first 4 bits used) + padding
		rom2_bits += "0000"; // Padding
        rom2_bits += to_binary_string(alu_opcodes[inst.opcode], 4);
        
        // Reverse the entire 16-bit string
        rom2 += rom2_bits + "\n";
        
        // ROM3: Arguments (16 bits total)
        string rom3_bits;
		
		// Second argument
        if (!arg2.second.empty()) {
            rom3_bits += bitset<8>(stoi(arg2.second)).to_string();
        } else {
            rom3_bits += "00000000";
        }
        
        // First argument
        if (!arg1.second.empty()) {
            rom3_bits += bitset<8>(stoi(arg1.second)).to_string();
        } else {
            rom3_bits += "00000000";
        }
        
        // Reverse the entire 16-bit string
        rom3 += rom3_bits + "\n";
		command++;
    }
    
    // Write output files
    ofstream rom1_file(output_prefix + "_rom1.bin");
    ofstream rom2_file(output_prefix + "_rom2.bin");
    ofstream rom3_file(output_prefix + "_rom3.bin");
    
    rom1_file << rom1;
    rom2_file << rom2;
    rom3_file << rom3;
    
    rom1_file.close();
    rom2_file.close();
    rom3_file.close();
    
    cout << "Compilation successful. Output files:\n";
    cout << output_prefix + "_rom1.bin\n";
    cout << output_prefix + "_rom2.bin\n";
    cout << output_prefix + "_rom3.bin\n";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_prefix>\n";
        return 1;
    }
    
    ifstream input_file(argv[1]);
    if (!input_file) {
        cerr << "Error: Could not open input file " << argv[1] << "\n";
        return 1;
    }
    
    string program((istreambuf_iterator<char>(input_file)), 
                   istreambuf_iterator<char>());
    
    try {
        auto instructions = parse_program(program);
        compile_program(instructions, argv[2]);
    } catch (const exception& e) {
        cerr << "Compilation error: " << e.what() << "\nOn command: " << command << "\nAt line: " << atLine << "\nWhile parsing: " << (parsing ? "true" : "false");
        return 1;
    }
    
    return 0;
}