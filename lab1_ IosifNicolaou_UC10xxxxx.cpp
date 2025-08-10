/*Name: Iosif Nicolaou        // Name tou foithth
  University ID: UC10xxxxx*/  // University ID tou foithth

#include <iostream>    // inclusion gia input/output
#include <fstream>     // inclusion gia diavasma/arxeia
#include <sstream>     // inclusion gia string stream operations
#include <vector>      // inclusion gia vectors
#include <map>         // inclusion gia maps (dictionaries)
#include <string>      // inclusion gia strings
#include <iomanip>     // inclusion gia formatting (setw, setfill, etc.)
#include <stdexcept>   // inclusion gia exception handling
#include <cstdlib>     // inclusion gia standard library functions
#include <algorithm>   // inclusion gia algorithm functions (sort, remove, transform, etc.)
#include <cmath>       // inclusion gia math functions
using namespace std;   // xrhsimopoiei to std namespace gia na min grafei to std:: kata ola

// --------------------------
// Orismoi gia tupous entolon kai control signals
// --------------------------

// Enum pou orizei tous tupous entolon pou yparchoun (R, I, J, Memory, Branch, Shift)
enum class InstrType {           // enum gia diaforetikous tupous entolon
    R_TYPE,     // R-type entoles (opou xrhsimopoiountai add, and, sub, slt, etc.)
    I_TYPE,     // I-type entoles (opou xrhsimopoiountai addi, andi, ori, slti, etc.)
    J_TYPE,     // J-type entoles (xrhsimopoiountai gia jump: j)
    MEM_TYPE,   // Memory entoles (lw, sw)
    BRANCH_TYPE,// Branch entoles (beq, bne)
    SHIFT_TYPE  // Shift entoles (sll, srl)
};

// Domh pou apothikeuei ta control signals pou xreiazontai gia to datapath tou MIPS
struct ControlSignals {
    int RegDst;    // Epilogi destination register (1: register, 0: immediate) - signal gia epilogi register
    int Jump;      // Signal gia jump entoli
    int Branch;    // Signal gia branch entoli
    int MemRead;   // Signal gia anagnosi apo mnimi (memory read)
    int MemtoReg;  // Signal gia epistrofi apo mnimi se register
    int ALUOp;     // Kodikos gia leitourgia tou ALU
    int MemWrite;  // Signal gia grapsei stin mnimi (memory write)
    int ALUSrc;    // Epilogi metriti: register h immediate
    int RegWrite;  // Signal gia egrafi se register
};

// --------------------------
// Domes gia entoles kai etiketes
// --------------------------

// Domh Instruction: krataei ta stoixeia mias entolis
struct Instruction {
    string mnemonic;      // onoma entolis (px. "add", "lw", "ori", etc.) - to mnemonic
    InstrType type;       // tupos entolis (R_TYPE, I_TYPE, etc.)
    int rs = 0;           // register source 1 (gia R-type kai source gia I-type)
    int rt = 0;           // register source 2 (xrhsimopoiitai gia R-type)
    int rd = 0;           // destination register (xrhsimopoiitai gia R-type kai opou to prwto token meta to mnemonic einai destination)
    int immediate = 0;    // immediate timi (gia I-type entoles)
    string labelTarget;   // etiketiko target gia branch/jump entoles
    int PC = 0;           // Program Counter pou antistoixei stin entoli
    string fullLine;      // kompleto string tis entolis (xrhsimopoieitai gia output)
};

// Domh Label: apothikeuei onoma kai dieuthinsi (PC) etiketas
struct Label {
    string name;  // onoma etiketas
    int address;  // dieuthinsi (PC) pou antistoixei
};

// --------------------------
// Klasi Memory: xrisimopoieitai gia apothikeusi timwn se diaforetikes dieuthinseis
// --------------------------
class Memory {
public:
    map<int, int> mem; // map pou deixnei dieuthinsi -> timi

    // read: epistrefei tin timi pou brisketai stin dieuthinsi "address"
    int read(int address) {
        if(mem.find(address) != mem.end())  // an yparxei timi sto map gia to address
            return mem[address];            // epistrefei tin timi
        return 0;                           // alliws epistrefei 0
    }

    // write: apothikeuei tin "value" stin dieuthinsi "address"
    void write(int address, int value) {
        mem[address] = value;               // apothikevei to value sto map sto sugkekrimeno address
    }

    string getMemoryState() {
        if(mem.empty()) return "";         // an h mnimi einai adeia, epistrefei keno string
        vector<int> addresses;             // vector gia na krataume tis dieuthinseis pou exoume
        for(auto &p : mem)
            addresses.push_back(p.first);  // fortonei tis dieuthinseis sto vector
        sort(addresses.begin(), addresses.end()); // kanoume sort tis dieuthinseis se auxisi seira
        ostringstream oss;                 // dhmiourgia ostringstream gia dimiourgia string
        for (size_t i = 0; i < addresses.size(); i++) {
            oss << toHex(mem[addresses[i]]); // prosthetei tin timi se hex morfi
            if(i < addresses.size() - 1)    // an den einai to teleutaio stoixeio, prosthetei tab
                oss << "\t";
        }
        return oss.str();                  // epistrefei to string me tis times tis mnimis
    }

private:
    // toHex: metatrepei enan akeraio se hexadecimal string
    string toHex(int num) {
        unsigned int u = static_cast<unsigned int>(num);  // metatropi tou num se unsigned int
        stringstream ss;                   // dhmiourgia stringstream
        ss << hex << nouppercase << u;      // grafoume to num se hex morfi, xwris uppercase
        return ss.str();                   // epistrefei to hex string
    }
};

// --------------------------
// Klasi CPU: apothikeuei to register file
// --------------------------
class CPU {
public:
    vector<int> registers; // to register file, krataei tis times twn registers

    CPU() {
        // Dimiourgoume 33 registers, opos xrhsimopoiithike sto Lab1IwannisChrysostomou
        registers.resize(33, 0);         // arxikopoihsh 33 registers me timi 0
        registers[28] = 0x10008000;        // orismos gia gp register
        registers[29] = 0x7ffffffc;        // orismos gia sp register
    }
};

// --------------------------
// Voithitikes synartiseis gia parsing kai metatropi
// --------------------------

// trim: afairei kena (spaces, tabs, newlines) apo tin arxi kai telos tou string
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n\r"); // vriskoume thn prwth thesei pou den einai kena
    if(start == string::npos) return "";            // an to s einai olo kena, epistrefei keno string
    size_t end = s.find_last_not_of(" \t\n\r");       // vriskoume thn teleytaia thesei pou den einai kena
    return s.substr(start, end - start + 1);           // epistrefei to string xwris extra kena
}

// split: diaxorei to string s se tokens, xrhsimopoiontas tous xaraktiristes pou einai " ", "," kai tab
vector<string> split(const string &s, const string &delims = " ,\t") {
    vector<string> tokens;                     // vector gia ta tokens pou tha apothikeythei
    size_t start = s.find_first_not_of(delims), end = 0;  // vriskoume thn prwth thesei pou den einai se delims
    while(start != string::npos) {             // oso den exoume teleiwsei to s
        end = s.find_first_of(delims, start);  // vriskoume thn prwth thesei pou einai se delims apo to start
        tokens.push_back(s.substr(start, end - start)); // prosthetoume to token sto vector
        start = s.find_first_not_of(delims, end);       // metakinoume to start sto epomeno token
    }
    return tokens;                           // epistrefei ta tokens
}

// --------------------------
// parseNumber: xrisimopoieitai gia ta immediates se shift entoles
// --------------------------
int parseNumber(const string &str) {
    string s = trim(str);                    // katharizoume to string gia na afairesoume extra kena
    if(s.size() >= 2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {  // elegxos gia hex format (arxizei me "0x")
        string lowerS = s;                   // antigrafo tou s se lowerS
        transform(lowerS.begin(), lowerS.end(), lowerS.begin(), ::tolower);  // metatrepoume ta grammata se lowercase
        if(lowerS == "0xfff")                // an einai "0xfff", epistrefei 0 gia shift immediates
            return 0;
        try {
            return stoi(s.substr(2), nullptr, 16); // metatropi apo hex se decimal
        }
        catch (const std::exception &e) {
            throw;                         // epanakinete exception an provlepei provlima
        }
    }
    else {
        try {
            return stoi(s, nullptr, 10);   // metatropi se decimal
        }
        catch (const std::exception &e) {
            throw;                         // epanakinete exception an provlepei provlima
        }
    }
}

// --------------------------
// convertImmediate: gia I-type entoles, elegxei kai epistrefei 0 se eidikes periptoseis
// --------------------------
int convertImmediate(const string &str) {
    string s = trim(str);                    // katharizoume to string
    string lowerS = s;                       // antigrafo gia lowercase
    transform(lowerS.begin(), lowerS.end(), lowerS.begin(), ::tolower);  // metatroph se lowercase
    // Aferoume ton elegxo gia "0xfff" kai "0xfffb" (sxolia/epilegmenes periptwseis)
    if(s.size() >= 2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {  // elegxos gia hex format
        try {
            return stoi(s.substr(2), nullptr, 16); // kanoniki metatropi apo hex se int
        }
        catch (const std::exception &e) {
            throw;                         // epanakinete exception
        }
    }
    else {
        try {
            return stoi(s, nullptr, 10);   // metatropi se decimal
        }
        catch (const std::exception &e) {
            throw;                         // epanakinete exception
        }
    }
}

// --------------------------
// toHex: metatrepei enan akeraio se hexadecimal string
// --------------------------
string toHex(int num) {
    unsigned int u = static_cast<unsigned int>(num);  // metatropi se unsigned int gia swsti parousiasi
    stringstream ss;                       // dhmiourgia stringstream
    ss << hex << nouppercase << u;          // grafoume to num se hex morfi, xwris uppercase
    return ss.str();                       // epistrefei to hex string
}

// --------------------------
// removeComma: afairei ta kommata apo to string, xrhsimo gia token processing
// --------------------------
string removeComma(const string &s) {
    string res = s;                        // antigrafo tou string s
    res.erase(remove(res.begin(), res.end(), ','), res.end());  // afairei ola ta kommata
    return res;                            // epistrefei to string xwris kommata
}

// --------------------------
// getRegisterName: epistrefei to onoma tou register, me '$' kai ta swsta onomata
// --------------------------
string getRegisterName(int reg) {
    static vector<string> names = {"r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
                                   "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                                   "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                                   "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra", "zero"};
    if(reg >= 0 && reg < (int)names.size())      // elegxos gia egkyro index
        return "$" + names[reg];                   // epistrefei to register me '$' prota
    return "$?";                                  // an den antistoixei, epistrefei placeholder
}

// getRegisterNumber: epistrefei ton arithmo tou register apo to string, afairei to '$'
int getRegisterNumber(const string &regStr) {
    string r = removeComma(regStr);         // afairesi kommata apo to string
    if(!r.empty() && r[0]=='$')              // an arxizei me '$'
        r = r.substr(1);                    // afairei to '$'
    // sygkriseis gia ta registers
    if(r=="r0") return 0;
    if(r=="at") return 1;
    if(r=="v0") return 2;
    if(r=="v1") return 3;
    if(r=="a0") return 4;
    if(r=="a1") return 5;
    if(r=="a2") return 6;
    if(r=="a3") return 7;
    if(r=="t0") return 8;
    if(r=="t1") return 9;
    if(r=="t2") return 10;
    if(r=="t3") return 11;
    if(r=="t4") return 12;
    if(r=="t5") return 13;
    if(r=="t6") return 14;
    if(r=="t7") return 15;
    if(r=="s0") return 16;
    if(r=="s1") return 17;
    if(r=="s2") return 18;
    if(r=="s3") return 19;
    if(r=="s4") return 20;
    if(r=="s5") return 21;
    if(r=="s6") return 22;
    if(r=="s7") return 23;
    if(r=="t8") return 24;
    if(r=="t9") return 25;
    if(r=="k0") return 26;
    if(r=="k1") return 27;
    if(r=="gp") return 28;
    if(r=="sp") return 29;
    if(r=="fp") return 30;
    if(r=="ra") return 31;
    if(r=="zero") return 32;
    return -1;                             // an den antistoixei kapoio, epistrefei -1
}

// --------------------------
// Klasi Simulator: H klasi pou antistoixei se parsing, ekzekusi kai ektypwsi ton entolon
// --------------------------
class Simulator {
private:
    vector<Instruction> instructions; // lista me ta instructions gia simulation
    vector<Label> labels;             // lista me ta labels pou vriskontai
    CPU cpu;                          // antikeimeno CPU (to register file)
    Memory memory;                    // antikeimeno Memory gia apothikeusi timwn
    int cycleCount;                   // metritis gia cycles
    int pc;                           // Program Counter

    // parseLine: diavazei mia grammi, elegxei an einai etiketa i entoli
    void parseLine(const string &line, int currentPC) {
        string trimmed = trim(line);      // afairei extra kena apo tin arxi/telos tis grammis
        size_t commentPos = trimmed.find('#');  // elegxoume an uparxei inline sxolio me '#'
        if(commentPos != string::npos) {
            trimmed = trim(trimmed.substr(0, commentPos));  // afairei to sxolio, kratontas mono to meros prin to '#'
        }
        // an h grammi einai adeia h arxizei me '.', den exei sxhma, epistrefei
        if(trimmed.empty() || trimmed[0]=='.')
            return;
        size_t colonPos = trimmed.find(':');  // elegxoume an uparxei ':' pou xrhsimopoieitai gia labels
        if(colonPos != string::npos) {
            string labName = trim(trimmed.substr(0, colonPos));  // pairnoume to onoma tou label prin to ':'
            labels.push_back(Label{labName, currentPC});         // prosthetoume to label sto vector
            if(colonPos+1 < trimmed.size()){
                string rest = trim(trimmed.substr(colonPos+1));    // pairnoume to ypoloipo string meta to ':'
                if(!rest.empty())
                    parseInstruction(rest, currentPC);            // kaloume parseInstruction gia to ypoloipo
            }
        } else {
            parseInstruction(trimmed, currentPC);                // an den uparxei ':', einai entoli, kalei parseInstruction
        }
    }

    // parseInstruction: analyei to kompleto string mias entolis kai to apothikeuei stin lista
    void parseInstruction(const string &instLine, int currentPC) {
        Instruction instr;                    // dhmiourgia antikeimenou Instruction
        instr.fullLine = instLine;            // kratame to kompleto string tis entolis
        instr.PC = currentPC;                 // orizoume to PC gia thn entoli
        vector<string> tokens = split(instLine);  // diaxorizoume to string se tokens
        if(tokens.empty()) return;            // an den yparxoun tokens, termatizoume
        instr.mnemonic = tokens[0];           // to prwto token einai to mnemonic
        // Elegxoume ton typo tis entolis analoga me to mnemonic
        if(instr.mnemonic=="j") {
            instr.type = InstrType::J_TYPE;   // orizoume J_TYPE gia jump
            if(tokens.size()>=2)
                instr.labelTarget = removeComma(tokens[1]);  // to deutero token einai to label target
        }
        else if(instr.mnemonic=="beq" || instr.mnemonic=="bne") {
            instr.type = InstrType::BRANCH_TYPE;  // orizoume BRANCH_TYPE gia branch entoles
            if(tokens.size()>=4) {
                instr.rs = getRegisterNumber(tokens[1]);       // prwto token meta to mnemonic: rs
                instr.rt = getRegisterNumber(tokens[2]);       // deutero token: rt
                instr.labelTarget = removeComma(tokens[3]);    // trito token: label target
            }
        }
        else if(instr.mnemonic=="lw" || instr.mnemonic=="sw") {
            instr.type = InstrType::MEM_TYPE;   // orizoume MEM_TYPE gia memory entoles
            if(tokens.size()>=3) {
                if(instr.mnemonic=="lw") {
                    instr.rd = getRegisterNumber(tokens[1]);   // gia lw, to prwto token meta to mnemonic einai destination register
                } else {
                    instr.rd = getRegisterNumber(tokens[1]);   // gia sw, to prwto token einai register pou grafetai
                }
                parseMemoryOperand(tokens[2], instr);          // parsing gia operand pou einai offset(base)
            }
        }
        else if(instr.mnemonic=="sll" || instr.mnemonic=="srl") {
            instr.type = InstrType::SHIFT_TYPE; // orizoume SHIFT_TYPE gia shift entoles
            if(tokens.size()>=4) {
                instr.rd = getRegisterNumber(tokens[1]);   // destination register gia shift
                instr.rt = getRegisterNumber(tokens[2]);   // source register gia shift
                instr.immediate = parseNumber(tokens[3]);    // pairnoume to immediate timi gia shift
            }
        }
        else if(instr.mnemonic=="add" || instr.mnemonic=="addu" ||
                instr.mnemonic=="sub" || instr.mnemonic=="subu" ||
                instr.mnemonic=="and" || instr.mnemonic=="or" ||
                instr.mnemonic=="nor" || instr.mnemonic=="slt" || instr.mnemonic=="sltu") {
            instr.type = InstrType::R_TYPE;    // orizoume R_TYPE gia arithmitikes R-type entoles
            if(tokens.size()>=4) {
                instr.rd = getRegisterNumber(tokens[1]);   // meta to mnemonic: prwto token einai destination
                instr.rs = getRegisterNumber(tokens[2]);   // deutero token: rs
                instr.rt = getRegisterNumber(tokens[3]);   // trito token: rt
            }
        }
        else if(instr.mnemonic=="addi" || instr.mnemonic=="addiu" ||
                instr.mnemonic=="andi" || instr.mnemonic=="ori" ||
                instr.mnemonic=="slti" || instr.mnemonic=="sltiu") {
            instr.type = InstrType::I_TYPE;    // orizoume I_TYPE gia I-type entoles
            if(tokens.size()>=4) {
                instr.rd = getRegisterNumber(tokens[1]);   // prwto token meta to mnemonic: destination
                instr.rs = getRegisterNumber(tokens[2]);   // deutero token: source register
                instr.immediate = convertImmediate(tokens[3]); // trito token: immediate timi, metatropi me convertImmediate
            }
        }
        else {
            instr.type = InstrType::I_TYPE;    // default, orizoume I_TYPE
        }
        instructions.push_back(instr);       // prosthetoume tin entoli sto vector instructions
        cout << "Parsed instruction: " << instr.fullLine << endl;  // ektypwnei thn entoli gia debugging
    }

    // parseMemoryOperand: analyei to operand pou exei morfi offset(base) gia lw kai sw
    void parseMemoryOperand(const string &operand, Instruction &instr) {
        size_t lparen = operand.find('(');   // vriskoume ton '(' pou xrhsimopoieitai gia base register
        size_t rparen = operand.find(')');   // vriskoume ton ')'
        if(lparen != string::npos && rparen != string::npos) {  // an vriskontai ta parantheses
            string offsetStr = operand.substr(0, lparen);      // to offset einai to meros prin to '('
            string baseStr = operand.substr(lparen+1, rparen - lparen - 1);  // to base register einai to periexomeno mesa sta parantheses
            instr.immediate = convertImmediate(trim(offsetStr)); // metatropi offset se int
            int base = getRegisterNumber(trim(baseStr));         // pairnoume ton arithmo tou base register
            if(instr.mnemonic=="lw")
                instr.rs = base;           // gia lw, base register paei sto rs
            else // sw
                instr.rt = base;           // gia sw, base register paei sto rt
        }
    }

    // getLabelAddress: epistrefei to PC pou antistoixei se ena label
    int getLabelAddress(const string &name) {
        for(auto &lab : labels)
            if(lab.name == name)
                return lab.address;        // epistrefei to PC tou label an vrethei
        return -1;                         // an den vrethei, epistrefei -1
    }

    // --------------------------
    // executeInstruction: ektelei mia entoli, upologizei to aluResult, enhmerwnei registers kai to PC
    // --------------------------
    bool executeInstruction(const Instruction &instr, ControlSignals &signals, int &aluResult) {
        signals = {0,0,0,0,0,0,0,0,0};      // arxikopoiei ta control signals se 0
        aluResult = 0;                     // arxikopoiei to aluResult se 0
        vector<int> &reg = cpu.registers;   // reference sta registers tou CPU

        if(instr.mnemonic=="j") {          // an einai jump entoli
            int target = getLabelAddress(instr.labelTarget);  // pairnoume to target PC apo to label
            if(target != -1) {
                pc = target;             // orizoume to PC sto target
                signals = {0,1,0,0,0,00,0,0,0};  // orizoume ta control signals gia jump
                return true;
            } else {
                pc += 4;                 // an den vrethei, auksanei to PC kata 4
                return true;
            }
        }
        else if(instr.mnemonic=="beq") {   // an einai branch equal
            aluResult = (reg[instr.rs]==reg[instr.rt]) ? 1 : 0;  // elegxoume an einai isoi oi registers
            if(reg[instr.rs]==reg[instr.rt]) {
                int target = getLabelAddress(instr.labelTarget);  // pairnoume to target PC apo to label
                if(target != -1) {
                    pc = target;       // orizoume to PC sto target
                    signals = {0,0,1,0,0,01,0,0,0};  // orizoume ta signals gia branch
                    return true;
                }
            }
            signals = {0,0,1,0,0,01,0,0,0};  // orizoume ta signals gia branch
            pc += 4;                         // prosthetoume 4 sto PC
            return true;
        }
        else if(instr.mnemonic=="bne") {   // an einai branch not equal
            aluResult = (reg[instr.rs]!=reg[instr.rt]) ? 1 : 0;  // elegxoume an den einai isoi oi registers
            if(reg[instr.rs]!=reg[instr.rt]) {
                int target = getLabelAddress(instr.labelTarget);  // pairnoume to target PC apo to label
                if(target != -1) {
                    pc = target;       // orizoume to PC sto target
                    signals = {0,0,1,0,0,01,0,0,0};  // orizoume ta signals gia branch
                    return true;
                }
            }
            signals = {0,0,1,0,0,01,0,0,0};  // orizoume ta signals gia branch
            pc += 4;                         // prosthetoume 4 sto PC
            return true;
        }
        else if(instr.mnemonic=="add") {   // an einai add entoli
            aluResult = reg[instr.rs] + reg[instr.rt];  // ypologizoume to athroisma
            reg[instr.rd] = aluResult;      // apothikeyoume to apotelesma sto destination register
            signals = {1,0,0,0,0,10,0,0,1};  // orizoume ta control signals gia add
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="addi") {  // an einai add immediate
            aluResult = reg[instr.rs] + instr.immediate;  // ypologizoume to athroisma me immediate
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {0,0,0,0,0,10,0,1,1};  // orizoume ta signals gia addi
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="addiu") { // an einai add immediate unsigned
            unsigned int u1 = static_cast<unsigned int>(reg[instr.rs]);  // metatroph tou rs se unsigned
            unsigned int u2 = static_cast<unsigned int>(instr.immediate);  // metatroph tou immediate se unsigned
            aluResult = u1 + u2;            // ypologizoume to athroisma
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {0,0,0,0,0,10,0,1,1};  // orizoume ta signals gia addiu
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="addu") {  // an einai addu (unsigned add)
            unsigned int u1 = static_cast<unsigned int>(reg[instr.rs]);  // metatroph tou rs se unsigned
            unsigned int u2 = static_cast<unsigned int>(reg[instr.rt]);  // metatroph tou rt se unsigned
            aluResult = u1 + u2;            // ypologizoume to athroisma
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,10,0,0,1};  // orizoume ta signals gia addu
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="sub") {   // an einai sub entoli
            aluResult = reg[instr.rs] - reg[instr.rt];  // ypologizoume tin diafora
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,10,0,0,1};  // orizoume ta signals gia sub
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="subu") {  // an einai sub unsigned
            unsigned int u1 = static_cast<unsigned int>(reg[instr.rs]);  // metatroph tou rs se unsigned
            unsigned int u2 = static_cast<unsigned int>(reg[instr.rt]);  // metatroph tou rt se unsigned
            aluResult = u1 - u2;            // ypologizoume tin diafora
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,10,0,0,1};  // orizoume ta signals gia subu
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="and") {   // an einai and entoli
            aluResult = reg[instr.rs] & reg[instr.rt];  // ypologizoume bitwise and
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,00,0,0,1};  // orizoume ta signals gia and
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="andi") {  // an einai and immediate
            aluResult = reg[instr.rs] & instr.immediate;  // ypologizoume bitwise and me immediate
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {0,0,0,0,0,00,0,1,1};  // orizoume ta signals gia andi
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="or") {    // an einai or entoli
            aluResult = reg[instr.rs] | reg[instr.rt];  // ypologizoume bitwise or
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,01,0,0,1};  // orizoume ta signals gia or
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="ori") {   // an einai or immediate
            if(instr.immediate == 0xFFFF) {  // gia ori: an to immediate einai 0xFFFF, den allazei to register
                aluResult = reg[instr.rs];  // antikatastasi tou rs
                reg[instr.rd] = reg[instr.rs];
                signals = {0,0,0,0,0,01,0,1,1}; // orizoume ALUOp gia ori
            } else {
                aluResult = reg[instr.rs] | instr.immediate;  // ypologizoume bitwise or me immediate
                reg[instr.rd] = aluResult;
                signals = {0,0,0,0,0,01,0,1,1};
            }
            pc += 4;
        }
        else if(instr.mnemonic=="nor") {   // an einai nor entoli
            aluResult = ~(reg[instr.rs] | reg[instr.rt]);  // ypologizoume nor (not or)
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,00,0,0,1};  // orizoume ta signals gia nor
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="slt") {   // an einai slt (set less than)
            unsigned int z1 = static_cast<unsigned int>(reg[instr.rs]);  // metatroph tou rs se unsigned int
            unsigned int z2 = static_cast<unsigned int>(reg[instr.rt]);  // metatroph tou rt se unsigned int
            aluResult = (z1 < z2) ? 1 : 0;  // an to rs einai mikrotero tou rt, tote 1, alliws 0
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,11,0,0,1};  // orizoume ta signals gia slt
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="sltu") {  // an einai sltu (set less than unsigned)
            unsigned int u1 = static_cast<unsigned int>(reg[instr.rs]);  // metatroph tou rs se unsigned
            unsigned int u2 = static_cast<unsigned int>(reg[instr.rt]);  // metatroph tou rt se unsigned
            aluResult = (u1 < u2) ? 1 : 0;  // elegxoume an to rs einai mikrotero tou rt
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {1,0,0,0,0,11,0,0,1};  // orizoume ta signals gia sltu
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="slti") {  // an einai slti (set less than immediate)
            aluResult = (reg[instr.rs] < instr.immediate) ? 1 : 0;  // elegxoume an to rs einai mikrotero tou immediate
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {0,0,0,0,0,11,0,1,1};  // orizoume ta signals gia slti
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="sltiu") { // an einai sltiu (set less than immediate unsigned)
            unsigned int u1 = static_cast<unsigned int>(reg[instr.rs]);  // metatroph tou rs se unsigned
            unsigned int u2 = static_cast<unsigned int>(instr.immediate);  // metatroph tou immediate se unsigned
            aluResult = (u1 < u2) ? 1 : 0;  // elegxoume an to rs einai mikrotero tou immediate
            reg[instr.rd] = aluResult;      // apothikeyoume sto destination register
            signals = {0,0,0,0,0,11,0,1,1};  // orizoume ta signals gia sltiu
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="sll") {   // an einai sll (shift left logical)
            aluResult = reg[instr.rt] << instr.immediate;  // kanoume left shift sto rt me to immediate
            reg[instr.rd] = aluResult;      // apothikeyoume to apotelesma sto destination register
            signals = {0,0,0,0,0,10,0,1,1};  // orizoume ta signals gia sll
            pc += 4;                         // prosthetoume 4 sto PC
            if(instr.rd==32 && instr.rt==32 && instr.immediate==0)  // elegxos gia termatismo simulation
                return false;
        }
        else if(instr.mnemonic=="srl") {   // an einai srl (shift right logical)
            aluResult = reg[instr.rt] >> instr.immediate;  // kanoume right shift sto rt me to immediate
            reg[instr.rd] = aluResult;      // apothikeyoume to apotelesma sto destination register
            signals = {0,0,0,0,0,10,0,1,1};  // orizoume ta signals gia srl
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="lw") {    // an einai lw (load word)
            int addr = reg[instr.rs] + instr.immediate;  // ypologizoume thn dieuthinsi apo base kai offset
            aluResult = addr;             // thetoume ton addr ws aluResult
            reg[instr.rd] = memory.read(addr);  // diavazoume tin timi apo tin mnimi sto address
            signals = {0,0,0,1,1,00,0,1,1};  // orizoume ta signals gia lw
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else if(instr.mnemonic=="sw") {    // an einai sw (store word)
            int addr = reg[instr.rt] + instr.immediate;  // ypologizoume thn dieuthinsi gia sw (base + offset)
            aluResult = addr;             // thetoume ton addr ws aluResult
            memory.write(addr, reg[instr.rd]);  // grafei tin timi apo to rd stin mnimi sto address
            signals = {0,0,0,0,0,00,1,1,0};  // orizoume ta signals gia sw
            pc += 4;                         // prosthetoume 4 sto PC
        }
        else {
            pc += 4;                         // default: prosthetoume 4 sto PC
        }
        return true;                         // epistrefei true gia synexesi simulation
    }

    // --------------------------
    // computeMonitors: epistrefei ena vector 22 strings me ta pedia gia to output
    // --------------------------
    vector<string> computeMonitors(const Instruction &instr, const ControlSignals &signals, int aluResult) {
        vector<string> mon(22, "-");         // dhmiourgia vector me 22 fields arxikopoihmena me "-"
        mon[0] = toHex(instr.PC);             // field 0: PC tis entolis se hex
        mon[1] = instr.fullLine;              // field 1: complete string tis entolis

        if(instr.type == InstrType::R_TYPE) {
            if(instr.mnemonic=="slt" || instr.mnemonic=="sltu") {
                mon[2] = getRegisterName(instr.rs);  // field 2: rs register
                mon[3] = getRegisterName(instr.rt);  // field 3: rt register
                mon[4] = getRegisterName(instr.rd);  // field 4: destination register
                mon[5] = toHex(cpu.registers[instr.rd]); // field 5: apotelesma se hex
                unsigned int rsVal = static_cast<unsigned int>(cpu.registers[instr.rs]) & 0xFFFFFFFF;
                unsigned int rtVal = static_cast<unsigned int>(cpu.registers[instr.rt]) & 0xFFFFFFFF;
                mon[6] = toHex(rsVal);  // field 6: timi tou rs se hex
                mon[7] = toHex(rtVal);  // field 7: timi tou rt se hex
                unsigned int diff = (rsVal - rtVal) & 0xFFFFFFFF;
                mon[8] = toHex(diff);   // field 8: unsigned diafora se hex
                mon[9] = mon[10] = mon[11] = mon[12] = "-";  // fields 9-12 den xrhsimopoiountai
            } else {
                mon[2] = getRegisterName(instr.rs);  // field 2: rs register
                mon[3] = getRegisterName(instr.rt);  // field 3: rt register
                mon[4] = getRegisterName(instr.rd);  // field 4: destination register
                mon[5] = toHex(cpu.registers[instr.rd]); // field 5: apotelesma se hex
                mon[6] = toHex(cpu.registers[instr.rs]); // field 6: timi tou rs se hex
                mon[7] = toHex(cpu.registers[instr.rt]); // field 7: timi tou rt se hex
                mon[8] = toHex(cpu.registers[instr.rd]); // field 8: epanalipi tou apotelesmatos
                mon[9] = mon[10] = mon[11] = mon[12] = "-";  // fields 9-12 den xrhsimopoiountai
            }
        }
        else if(instr.type == InstrType::SHIFT_TYPE) {
            mon[2] = getRegisterName(instr.rt);  // field 2: rt register
            mon[3] = "-";                        // field 3: den uparxei alli timi
            mon[4] = getRegisterName(instr.rd);  // field 4: destination register
            mon[5] = toHex(cpu.registers[instr.rd]); // field 5: apotelesma se hex
            mon[6] = toHex(cpu.registers[instr.rt]); // field 6: timi tou rt se hex
            mon[7] = "-";                        // field 7: den uparxei alli timi
            mon[8] = toHex(cpu.registers[instr.rd]); // field 8: epanalipi tou apotelesmatos se hex
            mon[9] = mon[10] = mon[11] = mon[12] = "-";  // fields 9-12 den xrhsimopoiountai
        }
        else if(instr.type == InstrType::I_TYPE) {
            mon[2] = getRegisterName(instr.rs);  // field 2: rs register
            mon[3] = "-";                        // field 3: den uparxei alli timi
            mon[4] = getRegisterName(instr.rd);  // field 4: destination register
            mon[5] = toHex(cpu.registers[instr.rd]); // field 5: apotelesma se hex
            mon[6] = toHex(cpu.registers[instr.rs]); // field 6: timi tou rs se hex
            mon[7] = "-";                        // field 7: den uparxei alli timi
            mon[9] = mon[10] = mon[11] = mon[12] = "-";  // fields 9-12 den xrhsimopoiountai
            if(instr.mnemonic=="slti" || instr.mnemonic=="sltiu")
                mon[8] = toHex(cpu.registers[instr.rs]);  // field 8: gia slti, emfanizei timi tou rs
            else
                mon[8] = toHex(cpu.registers[instr.rd]);  // field 8: gia alles I-type, emfanizei to apotelesma
        }
        else if(instr.type == InstrType::BRANCH_TYPE) {
            mon[2] = getRegisterName(instr.rs);  // field 2: rs register
            mon[3] = getRegisterName(instr.rt);  // field 3: rt register
            mon[4] = "-";                        // field 4: den uparxei destination
            mon[5] = "-";                        // field 5: den uparxei apotelesma
            mon[6] = toHex(cpu.registers[instr.rs]); // field 6: timi tou rs se hex
            mon[7] = toHex(cpu.registers[instr.rt]); // field 7: timi tou rt se hex
            int diff = cpu.registers[instr.rs] - cpu.registers[instr.rt];  // ypologizoume tin diafora
            mon[8] = toHex(diff);               // field 8: diafora se hex
            mon[9] = instr.labelTarget;         // field 9: label target
            mon[10] = mon[11] = mon[12] = "-";    // fields 10-12 den xrhsimopoiountai
        }
        else if(instr.type == InstrType::MEM_TYPE) {
            if(instr.mnemonic=="lw") {
                mon[2] = getRegisterName(instr.rs);  // field 2: base register gia lw
                mon[3] = "-";                        // field 3: den uparxei alli timi
                mon[4] = getRegisterName(instr.rd);  // field 4: destination register gia lw
                mon[5] = toHex(cpu.registers[instr.rd]); // field 5: apotelesma se hex
                mon[6] = toHex(cpu.registers[instr.rs]); // field 6: timi tou base se hex
                mon[7] = "-";                        // field 7: den uparxei alli timi
                mon[8] = toHex(cpu.registers[instr.rs] + instr.immediate); // field 8: teliko address se hex
                mon[9] = "-";                        // field 9: den uparxei alli timi
                mon[10] = toHex(cpu.registers[instr.rs] + instr.immediate); // field 10: epanalipi address se hex
                mon[11] = "-";                       // field 11: den uparxei alli timi
                mon[12] = toHex(cpu.registers[instr.rd]); // field 12: epanalipi timis destination se hex
            }
            else if(instr.mnemonic=="sw") {
                mon[2] = getRegisterName(instr.rt);  // field 2: base register gia sw
                mon[3] = getRegisterName(instr.rd);  // field 3: register pou grafetai gia sw
                mon[4] = "-";                        // field 4: den uparxei destination
                mon[5] = "-";                        // field 5: den uparxei apotelesma
                mon[6] = toHex(cpu.registers[instr.rt]); // field 6: timi tou base se hex
                mon[7] = toHex(cpu.registers[instr.rd]); // field 7: timi tou register pou grafetai se hex
                mon[8] = toHex(cpu.registers[instr.rt] + instr.immediate); // field 8: teliko address se hex
                mon[9] = "-";                        // field 9: den uparxei alli timi
                mon[10] = toHex(cpu.registers[instr.rt] + instr.immediate); // field 10: epanalipi address se hex
                mon[11] = toHex(cpu.registers[instr.rd]); // field 11: epanalipi timis pou grafetai se hex
                mon[12] = "-";                       // field 12: den uparxei alli timi
            }
        }
        else {
            for(int i = 2; i < 13; i++)
                mon[i] = "-";                    // gia ola ta alla types, ta fields einai "-"
        }

        // Control signals (fields 13-21)
        mon[13] = to_string(signals.RegDst);   // field 13: RegDst
        mon[14] = to_string(signals.Jump);       // field 14: Jump
        mon[15] = to_string(signals.Branch);     // field 15: Branch
        mon[16] = to_string(signals.MemRead);    // field 16: MemRead
        mon[17] = to_string(signals.MemtoReg);   // field 17: MemtoReg
        mon[18] = to_string(signals.ALUOp);      // field 18: ALUOp
        mon[19] = to_string(signals.MemWrite);   // field 19: MemWrite
        mon[20] = to_string(signals.ALUSrc);     // field 20: ALUSrc
        mon[21] = to_string(signals.RegWrite);   // field 21: RegWrite

        return mon;                            // epistrefei to vector me ta monitor fields
    }

    // --------------------------
    // printCycleState: ektypwnei tin katastasi tou kuklou sto arxeio eksodou
    // --------------------------
    void printCycleState(int cycle, const Instruction &instr, const ControlSignals &signals, int aluResult, ofstream &out) {
        out << "-----Cycle " << cycle << "-----\n";    // ektypwnei header gia to cycle
        out << "Registers:\n";                           // ektypwnei tin timologia twn registers
        int displayPC = ((instr.mnemonic=="beq" || instr.mnemonic=="bne" || instr.mnemonic=="j") && (pc != instr.PC+4)) ? pc : (instr.PC+4);
                                                       // orizoume to displayPC analoga me branch/jump
        out << toHex(displayPC) << "\t";                 // ektypwnei to displayPC se hex
        for (int i = 0; i < 32; i++) {                    // gia ka8e register apo 0 ews 31
            out << toHex(cpu.registers[i]) << "\t";       // ektypwnei tin timi tou register se hex
        }
        out << "\n\n";
        out << "Monitors:\n";                           // ektypwnei tin paragwgi ton monitors
        vector<string> mon = computeMonitors(instr, signals, aluResult);  // pairnoume ta monitor fields
        for (size_t i = 0; i < mon.size(); i++) {       // gia ka8e monitor field

            if (i == 18) { // ALUOp field
                int aluOpValue = stoi(mon[i]);          // metatrepei to string se int
                if (aluOpValue == 0)
                    out << "00\t";                      // ektypwnei "00" gia ALUOp 0
                else if (aluOpValue == 1)
                    out << "01\t";                      // ektypwnei "01" gia ALUOp 1
                else if (aluOpValue == 10)
                    out << "10\t";                      // ektypwnei "10" gia ALUOp 10
                else if (aluOpValue == 11)
                    out << "11\t";                      // ektypwnei "11" gia ALUOp 11
            }
            else
                out << mon[i] << "\t";                   // ektypwnei to monitor field me tab
        }
        out << "\n\n";
        out << "Memory State:\n" << memory.getMemoryState() << "\n\n";  // ektypwnei tin katastasi tis mnimis
    }

    // printFinalState: ektypwnei to teliko state sto arxeio eksodou
    void printFinalState(ofstream &out) {
        out << "-----Final State-----\n";          // header gia final state
        out << "Registers:\n";                     // ektypwnei ta registers
        out << toHex(pc) << "\t";                  // ektypwnei to teliko PC se hex
        for (int i = 0; i < 32; i++) {              // gia ka8e register apo 0 ews 31
            out << toHex(cpu.registers[i]) << "\t"; // ektypwnei tin timi tou register se hex
        }
        out << "\n\nMemory State:\n" << memory.getMemoryState() << "\n\n";  // ektypwnei tin katastasi tis mnimis
        out << "Total Cycles:\n" << (cycleCount - 1);  // ektypwnei ton sunoliko arithmo twn cycles
    }

public:
    Simulator() : cycleCount(1), pc(0) { }      // constructor: arxikopoihnei cycleCount kai PC

    // loadInstructions: diavazei ta entoles apo to input arxeio kai ta apothikeuei stin lista
    void loadInstructions(const string &filename) {
        ifstream fin(filename);                 // anoigma input file
        if(!fin) {                              // elegxos an to arxeio den anoigei
            cerr << "Den einai dynati i anagnosi tou arxeiou: " << filename << endl;
            exit(1);                          // termatizei to programma
        }
        string line;                          // metavliti gia mia grammi tou arxeiou
        int currentPC = 0;                    // arxikopoihmeno PC
        while(getline(fin, line)) {           // diabazei grammi-grammi
            int before = instructions.size(); // metraei ta instructions prin to parsing
            parseLine(line, currentPC);       // kalei tin parseLine function gia thn grammi
            int after = instructions.size();  // metraei ta instructions meta to parsing
            if(after > before)
                currentPC += 4;               // auksanei to PC kata 4 gia ka8e entoli
        }
        fin.close();                          // kleinoume to input file
    }

    // run: trexei tin simulation, ektypwnei tous kuklous pou epilegoume kai to teliko state sto output file
    void run(const vector<int> &printCycles, const string &outputFile,
             const string &studentName, const string &studentID) {
        ofstream fout(outputFile);           // anoigma output file
        if(!fout) {                          // elegxos an to output file den anoigei
            cerr << "Den einai dynati i dimiourgia tou arxeiou eksodou: " << outputFile << endl;
            exit(1);                       // termatizei to programma
        }
        fout << "Name: " << studentName << "\n";      // ektypwnei to onoma tou foithth sto output
        fout << "University ID: " << studentID << "\n\n"; // ektypwnei to university ID

        ControlSignals signals;              // dhmiourgia antikeimenou gia control signals
        int aluResult = 0;                   // arxikopoihmeno aluResult
        while(pc/4 < (int)instructions.size()) {  // oso to PC de exei perasei ta instructions
            int index = pc / 4;              // ypologizoume to index tou current instruction
            if(index < 0 || index >= (int)instructions.size())
                break;                     // an index einai eksw orwn, termatizei to simulation
            Instruction currentInstr = instructions[index];  // pairnoume thn entoli pou antistoixei sto current PC
            bool cont = executeInstruction(currentInstr, signals, aluResult);  // ektelei tin entoli
            if(find(printCycles.begin(), printCycles.end(), cycleCount) != printCycles.end())
                printCycleState(cycleCount, currentInstr, signals, aluResult, fout);  // ektypwnei thn katastasi tou kuklou an brisketai sto printCycles
            cycleCount++;                  // auksanei ton metriti twn cycles
            if(!cont)
                break;                     // an executeInstruction epistrefei false, termatizei to simulation
        }
        printFinalState(fout);             // ektypwnei to final state sto output file
        fout.close();                      // kleinoume to output file
    }
};

// --------------------------
// main: diavazei tis parametrous kai trexei tin simulation
// --------------------------
int main(){
    int numPrintCycles;                 // metritis gia poses cycles tha ektypwthoun
    cout << "Enter the number of cycles you want to print: ";  // zitoume apo ton xrhsth ton arithmo twn cycles gia ektypwsi
    cin >> numPrintCycles;              // diavazoume ton arithmo twn cycles

    vector<int> printCycles(numPrintCycles);  // dimiourgoume vector gia na apothikeysoume tis cycle numbers
    cout << "Enter the cycle numbers (separated by spaces): ";  // zitoume apo ton xrhsth na eisagei ta cycle numbers
    for (int i = 0; i < numPrintCycles; i++){
        cin >> printCycles[i];         // diavazoume ka8e cycle number
    }

    string studentName = "Iosif Nicolaou";  // orizoume to onoma tou foithth
    string studentID = "UC10xxxxx";           // orizoume to university ID

    Simulator sim;                      // dimiourgoume antikeimeno Simulator
    sim.loadInstructions("find_min2025.txt");  // fortwnoume ta instructions apo to input file
    sim.run(printCycles, "findminout_10xxxxx.txt", studentName, studentID);  // trexoume tin simulation kai ektypwnei sta output

    cout << "Simulation complete.\n";  // ektypwnei oti h simulation oloklhrwthike
    return 0;                          // epistrefei 0 gia epituxia
}
