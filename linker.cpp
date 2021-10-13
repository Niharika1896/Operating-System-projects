#include <iostream>
#include <string>
#include <cstring>
#include <string.h>
#include <fstream>
#include <utility>
#include <vector>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <unordered_set>

using namespace std;

struct Symbol {
    int mod;
    int val;
    string name;
    int sym_address;
    bool defMulTimes = false;
    bool usedButNotDefined = false;
};
struct Module{
    int base_address = 0;
    int length;
    int number;
    vector <Symbol> mod_defList;
    vector <string> mod_useList;
    vector <string> mod_instrList;
    vector <string> mod_eTypeSymList;
    void insertUseListSym(string sym){
        mod_useList.push_back(sym);
    }
    void insertdefListSym(Symbol sym) { 
        mod_defList.push_back(sym);
    }
    void insertinstrList(string s) {
        mod_instrList.push_back(s);
    }
    void insertETypeSymList(string s) {
        mod_eTypeSymList.push_back(s);
    }
};
struct Token {
    char *token;
    int line , lineofs;
};
vector <Symbol> sym_vector;
vector <Module> mod_vector;
vector <string> memory_map;
vector <Symbol> symbol_table;

unordered_set<string> definedSymbols ;
unordered_set<string> symbolsDefinedMultipleTimes ;
unordered_set<string> usedSymbols ;
unordered_set<string> eListSymbols ;

int mod_count = 1;
int mod_badr = 0;
int global_codecount_sum=0;

string line;
int line_no = 0;
int line_offset=0;
ifstream input_file;
bool eof_reached = false;
int prev_tok_length=0;

bool isNumber(const string& str) {
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

void __parseerror(int errcode, int linenum, int lineoffset) {
    static string errstr[] = {
    "NUM_EXPECTED", 
    "SYM_EXPECTED",
    "ADDR_EXPECTED",
    "SYM_TOO_LONG",
    "TOO_MANY_DEF_IN_MODULE",
    "TOO_MANY_USE_IN_MODULE",
    "TOO_MANY_INSTR"
    };
    printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, errstr[errcode].c_str());
}

string getToken(){
    bool is_null = true;
    string str = "";
    char *ptr;
    bool input_file_end_reached = false;

    while(line.empty()){
        getline(input_file, line);
        line_no++;
        is_null = false;

        if(input_file.eof()){
            break;
        }
    }

    while(!line.empty()){

        if(!input_file.eof()){
            //cout<<"Input file not empty "<<endl;
            if(is_null){
                ptr = strtok(NULL, "\t ");
                if(ptr - line.c_str() + 1 > 0){

                    line_offset = ptr - line.c_str() + 1;
                }
                
            } else {
                line_offset = 0;
                ptr = strtok((char*)line.c_str(), "\t ");
                line_offset = ptr - line.c_str() + 1;
                
            }
        } else if(input_file.eof() && !line.empty()){
            //cout<<"Now here "<< endl;
            if(is_null){
                //cout<<"Inside is null set "<<endl;
                ptr = strtok(NULL, "\t ");
                //line_offset = ptr - line.c_str() + 1;
                if(ptr - line.c_str() + 1 > 0){

                    line_offset = ptr - line.c_str() + 1;
                }
                
            } else {
                //cout<<"Inside is null not set"<<endl;
                line_offset = 0;
                ptr = strtok((char*)line.c_str(), "\t ");
                if(ptr - line.c_str() + 1 > 0){
                    line_offset = ptr - line.c_str() + 1;
                } else {
                    line_offset ++;
                }
                //cout<<"Set line offset to "<<line_offset<<endl;
                
            }
        } else {
            break;
        }
        if(ptr==NULL){
            if(input_file.eof()){
                //cout<<"Reached eof breaking "<<endl;
                break;
            }
            getline(input_file, line);
            line_no++;
            //cout<<line_no<<" : Line is "<<line << endl;
            //cout<<" Is line empty ? "<<line.empty()<<endl;
            if(input_file.eof() && line.empty()){
                //cout<<" a1"<<endl;
                line_no--;
                if(line_offset < 0){
                    line_offset = 1;
                } else {
                    line_offset= line_offset + prev_tok_length;
                }
                
                break;
            }
            while(!input_file.eof() && line.empty()){
                //cout<<" a2"<<endl;
                getline(input_file, line);
                line_no++;

            }
            if(input_file.eof() && line.empty()){
                //cout<<" a3"<<endl;
                line_no--;
                line_offset=1;
                break;
                
            }
            //cout<<" a4"<<endl;
            is_null = false;
        } else {
            break;
        }
        
    }

    //cout <<"Came out of it "<<endl;
    if(input_file.eof() && line.empty()){
        //cout<<"Condition satisfied"<<endl;
        input_file_end_reached = true;
        return "endOfFile";
    }
    bool check = (ptr == NULL);
    //cout<<"Checking if ptr is NULL "<<check<<endl;
    if(ptr == NULL){
        str ="";
    } else {
        str = ptr;
    }
    //cout<<" token is "<<str<<endl;
    prev_tok_length = str.length();
    return str;
    
}

int readInt(bool modBegin = false){
    string snum = getToken();
    if(snum == ""){
        if(!modBegin){
            __parseerror(0, line_no, line_offset);
            exit(0);
        } else {
            eof_reached = true;
            return -1;
        }
        
    }
    if(snum == "endOfFile") {
        if(modBegin){
            eof_reached = true;
            return -1;
        } else {
            __parseerror(0, line_no, line_offset);
            exit(0);
            
        }
        
        
    }
    bool check = isNumber(snum);
    //cout<<"Checking if it is number : "<<check<<endl;
    if(check){
        stringstream geek(snum);
        int num = 0;
        geek >> num;
        return num;
    } else {
        __parseerror(0, line_no, line_offset);
        exit(0);
    } 
    
}

string readSym(){
    string symname = getToken();
    
    if(symname.empty()){
        __parseerror(1, line_no, line_offset);
        exit(0);
    }
    if(symname == "endOfFile") {
        __parseerror(1, line_no, line_offset);
        exit(0);
    }
    if (symname.length() > 16) {
        __parseerror(3, line_no, line_offset);
        exit(0);
    }
    const char *symname_char = symname.c_str();
    
    if(!isalpha(symname_char[0])){
        __parseerror(1, line_no, line_offset);
        exit(0);
    }
    if (symname.length() > 1) {
        for (int i = 1; i < strlen(symname_char); i++){
            if (!isalnum(symname_char[i])){
                __parseerror(1, line_no, line_offset);
                exit(0);
            }
        }
    }
    
    return symname;
}

string readIAER(){
    string instr = getToken();
    
    if(instr!= "I" && instr!= "A" && instr!= "E" && instr!="R"){
        __parseerror(2, line_no, line_offset);
        exit(0);
    }
    return instr;
}

void printSymbolTable(){
    cout<<"Symbol Table"<<endl;
    for(int i=0; i< sym_vector.size(); i++){
        cout<<sym_vector.at(i).name<<"="<<sym_vector.at(i).sym_address;
        if(sym_vector.at(i).defMulTimes){
            cout<<" Error: This variable is multiple times defined; first value used";
        }
        if(sym_vector.at(i).usedButNotDefined){
            cout<<" Error: "<<sym_vector.at(i).name<<" is not defined; zero used";
        }
        
        cout<<endl;
    }
}

void printPass1Errors(){
    unordered_set<string> :: iterator itr;       

    for(auto& sym: sym_vector){
        string sym_name = sym.name;
        int sym_val = sym.val;
        int module_num = sym.mod;
        int module_len = mod_vector.at(module_num-1).length;
        if(module_len < sym_val){
            printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", module_num, sym_name.c_str(), sym_val, module_len-1);
            
            sym.val = 0;
            sym.sym_address = sym.val + mod_vector.at(module_num-1).base_address;
        }

    }

    

}

void printPass2Errors(){
    unordered_set<string> :: iterator itr;  

    for(int i =0; i < sym_vector.size(); i++){
        string name = sym_vector.at(i).name;
        bool isUsed = false;
        bool isInEList = false;
        if(usedSymbols.find(name) != usedSymbols.end()){
            isUsed = true;
        }
        if(eListSymbols.find(name) != eListSymbols.end()){
            isInEList = true;
        }
        if(!isInEList || !isUsed){
            int modu = sym_vector.at(i).mod;
            printf("Warning: Module %d: %s was defined but never used\n", modu, name.c_str());
        }
    }
}

void printMemoryMap(){

    
    int mm_counter = 0;
    cout<<endl<<"Memory Map"<<endl;
    string mm_index;
    
    for(int i= 0 ; i < mod_vector.size(); i++){
        
        int modlen = mod_vector.at(i).length;
        vector<string> temp_uselist = mod_vector.at(i).mod_useList;
        vector<bool> checkForUse ;
        for(int j =0; j < temp_uselist.size();j++){
            checkForUse.push_back(false);
        }
        vector<string> temp_Elist = mod_vector.at(i).mod_eTypeSymList;
        string warningmsg = "";
        bool isWarning = false;

        
        
        for(int j = 0 ; j < temp_Elist.size(); j++){ 
            string sym = temp_Elist.at(j);
            for(int k =0 ; k < temp_uselist.size(); k++){
                if(temp_uselist.at(k) == sym && checkForUse.at(k) == false){
                    checkForUse.at(k) = true;
                    break;
                }

            }
        }
        for(int j = 0 ; j < temp_uselist.size(); j++){
            string sym = temp_uselist.at(j);
            if(checkForUse.at(j) == false){
                warningmsg += "Warning: Module "+to_string(i+1)+": "+sym+" appeared in the uselist but was not actually used\n";
                isWarning = true;
            }
        }

        if(modlen==0){
            cout<<warningmsg;
        }
        for(int j = 1; j <= modlen; j++){
            if(mm_counter < 10){
                mm_index = "00"+to_string(mm_counter);
            } else if(mm_counter>=10 && mm_counter <100){
                mm_index = "0"+to_string(mm_counter);
            } else if (mm_counter >=100 && mm_counter <1000){
                mm_index = to_string(mm_counter);
            }
            cout<<mm_index<<": "<<memory_map.at(mm_counter)<<endl;
            if(isWarning && j == modlen){
                cout<<warningmsg;
            }
            mm_counter++;
        }
    }
    cout<<endl;
    
}

void pass1(string in_file){
    input_file.open(in_file.c_str());

    while(!input_file.eof()){
        
        if(input_file.eof()){
            break;
        }

        Module mod;
        int n = mod_vector.size();
        if (n>0){
            mod.base_address = mod_vector[n-1].base_address + mod_vector[n-1].length;
        } else {
            mod.base_address = 0;
        }
        mod.number = mod_count;
        
        
        int defcount = readInt(true);
        if(defcount == -1 && eof_reached){
            //cout<<"EOF reached cant read next defcount\n";
            break;
        }
        if(defcount > 16){
            __parseerror(4, line_no, line_offset);
            exit(0);
        }
        //<<"defcount : "<<defcount<<endl;
        for(int i=1; i <=defcount; i++){
            Symbol s;
            
            string name  = readSym();
            if(name == "endOfFile"){
                break;
            }

            //cout<<"symbol : "<<name<<endl;
            int value = readInt();
            if(value ==-1 && eof_reached){
                break;
            }
            if(definedSymbols.find(name) == definedSymbols.end()){
                s.name = name;
                s.val = value;
                //s.mod = mod_count;
                s.mod = mod.number;
                s.sym_address = s.val + mod.base_address;
                s.defMulTimes = false;
                s.usedButNotDefined = false;
                sym_vector.push_back(s);
                mod.insertdefListSym(s);
                definedSymbols.insert(name);
            } else {
                //cout<<name<<"Defined multiple times"<<endl;
                symbolsDefinedMultipleTimes.insert(name);
               
                for(auto& sym: sym_vector){
                    if(sym.name == name ){
                        sym.defMulTimes = true;
                    }
                }
                continue;
            }
        }

        if(eof_reached){
            break;
        }
      

        
        int usecount = readInt();
        if(usecount ==-1 && eof_reached){
            break;
        }
        if(usecount > 16){
            __parseerror(5, line_no, line_offset);
            exit(0);
        }
        //cout<<"Usecount : "<<usecount<<endl;
        for(int i=1; i <=usecount; i++){
           
            string name = readSym();
            if(name == "endOfFile"){
                break;
            }
            //cout<<"Symbol "<<name<<endl;
            
            mod.insertUseListSym(name);
            if(usedSymbols.find(name) == usedSymbols.end()){
                string usedSym_with_mod = name;
                usedSymbols.insert(usedSym_with_mod);
            } else {
                
            }
            
        }
        


        
        int codecount = readInt();
        if(codecount ==-1 && eof_reached){
            break;
        }
        
        global_codecount_sum += codecount;
        if(global_codecount_sum >= 512){
            __parseerror(6, line_no, line_offset);
            exit(0);
        }


        mod.length = codecount;
        //cout<<"codecount "<<codecount<<endl;
        for(int i = 1; i <=codecount; i++){
            string s1 = readIAER();
            string s2 = to_string(readInt());
            string s3 = s1 + s2;
            //cout<<"instr "<<s3<<endl;
            mod.insertinstrList(s3);
        }
        

        mod_count++;
        mod_vector.push_back(mod);


    }

    printPass1Errors();
    printSymbolTable();

}

string correctPadding(string s){
    int len = s.length();
    string res = "";
    if(len==4){
        return s;
    } else if(len == 3){
        res = "0"+s;
    } else if(len ==2){
        res = "00" + s;
    } else if(len ==1){
        res = "000" + s;
    } else if(len ==0){
        res = "0000";
    }
    return res;
}

void pass2(string in_file){
    input_file.open(in_file.c_str());
    
    while(!input_file.eof()){
        
        if(input_file.eof()){
            break;
        }
        
        Module mod;
        int n = mod_vector.size();
        if (n>0){
            mod.base_address = mod_vector[n-1].base_address + mod_vector[n-1].length;
        } else {
            mod.base_address = 0;
        }
        mod.number = mod_count;
        
        //cout<<"Module "<<mod_count;
        
        int defcount = readInt(true);
        if(defcount == -1 && eof_reached){
            break;
        }
        //cout<<"defcount : "<<defcount<<endl;
        for(int i=1; i <=defcount; i++){
            Symbol s;
            string name  = readSym();
            if(name == "endOfFile"){
                break;
            }
            //cout<<"symbol : "<<name<<endl;
            int value = readInt();
            if(value ==-1 && eof_reached){
                break;
            }
            if(definedSymbols.find(name) == definedSymbols.end()){
                s.name = name;
                s.val = value;
                s.mod = mod.number;
                s.sym_address = s.val + mod.base_address;
                s.defMulTimes = false;
                s.usedButNotDefined = false;
                sym_vector.push_back(s);
                mod.insertdefListSym(s);
                definedSymbols.insert(name);
            } else {
                //cout<<name<<"Defined multiple times"<<endl;
                symbolsDefinedMultipleTimes.insert(name);
               
                for(auto& sym: sym_vector){
                    if(sym.name == name ){
                        sym.defMulTimes = true;
                    }
                }
                continue;
            }
        }
        //cout<<"Got defcount "<<defcount<<endl;

        if(eof_reached){
            break;
        }
      
        int usecount = readInt();
        if(usecount ==-1 && eof_reached){
            break;
        }
        //cout<<"Usecount : "<<usecount<<endl;
        for(int i=1; i <=usecount; i++){
            
            string name = readSym();
            if(name == "endOfFile"){
                break;
            }
            //cout<<"Symbol "<<name<<endl;
           
            mod.insertUseListSym(name);
            if(usedSymbols.find(name) == usedSymbols.end()){
                string usedSym_with_mod = name;
                usedSymbols.insert(usedSym_with_mod);
            } else {
                
            }
            
        }
        
        //cout<<"Got usecount "<<usecount<<endl;

        
        int codecount = readInt();
        if(codecount ==-1 && eof_reached){
            break;
        }
        //cout<<"got codecount "<<codecount<<endl;
        mod.length = codecount;
        for(int i = 1; i <=codecount; i++){
            string s1 = readIAER();
            int op = readInt();
            int opcode = op/1000;
            int operand = op%1000;
            
            if(s1 == "I"){
                if(op >=10000){
                    string new_val = "9999 Error: Illegal immediate value; treated as 9999";
                    memory_map.push_back(new_val);
                } else {
                    string new_val = to_string(op);
                    new_val = correctPadding(new_val);
                    memory_map.push_back(new_val);
                }
            } else if(s1 == "A"){
                if(opcode >= 10 ){
                    string new_val ="9999 Error: Illegal opcode; treated as 9999";
                    memory_map.push_back(new_val);
                } else {
                    if(operand >= 512){
                        int new_addr = opcode*1000;
                        string new_val = correctPadding(to_string(new_addr)) + " Error: Absolute address exceeds machine size; zero used";
                        memory_map.push_back(new_val);
                    } else {
                        memory_map.push_back(correctPadding(to_string(op)));
                    }
                }
                    
                
            } else if (s1 == "E"){
                vector<string> uselist = mod.mod_useList;
                if(opcode >= 10 ){
                    string new_val ="9999 Error: Illegal opcode; treated as 9999";
                    memory_map.push_back(new_val);
                } else if(operand >= uselist.size()){
                    string new_val = correctPadding(to_string(op)) + " Error: External address exceeds length of uselist; treated as immediate";
                    memory_map.push_back(new_val);
                } else {
                    string sym_name = uselist.at(operand);
                    mod.insertETypeSymList(sym_name);
                    if(eListSymbols.find(sym_name) == eListSymbols.end()){
                        eListSymbols.insert(sym_name);
                    }
                    int addr;
                    bool found = false;
                    for (int i=0; i<symbol_table.size(); i++){
                        string sname = symbol_table.at(i).name;
                    
                        if(sname==sym_name){
                            found = true;
                            addr = symbol_table.at(i).sym_address;
                        }
                    }
                    if(found == false){
                        addr = 0;
                        int new_val = (opcode*1000) + addr;
                        string temp = correctPadding(to_string(new_val)) + " Error: "+sym_name+" is not defined; zero used";
                        memory_map.push_back(temp);
                        
                    }else {
                        int new_val = (opcode*1000) + addr;
                        memory_map.push_back(correctPadding(to_string(new_val)));
                        
                    }
                        
                }
                
            } else if (s1 == "R"){
                int size_of_module = codecount;
                if(opcode >= 10 ){
                    string new_val ="9999 Error: Illegal opcode; treated as 9999";
                    memory_map.push_back(new_val);
                } else if(operand > size_of_module){
                    operand = 0;
                    int new_val = mod.base_address + (opcode*1000);
                    string temp = correctPadding(to_string(new_val))+" Error: Relative address exceeds module size; zero used";
                    memory_map.push_back(temp);
                } else {
                    int new_val = mod.base_address+op;
                    memory_map.push_back(correctPadding(to_string(new_val)));
                }
                    
            }
            
            
            string s2 = to_string(op);
            string s3 = s1 + s2;
            //cout<<"instr "<<s3<<endl;
            mod.insertinstrList(s3);
        }
        

        mod_count++;
        mod_vector.push_back(mod);


    }

    printMemoryMap();
    printPass2Errors();

}

int main(int argc, char* argv[]){
    if(argc != 2){
        cout<<"Incorrect no of arguments!"<<endl;
        exit(0);
    }
    std::string in_file = argv[1];

    pass1(in_file);


    //Parsing again in pass2
    
    for (int i=0; i<sym_vector.size(); i++){
        symbol_table.push_back(sym_vector[i]);
    }
        

    sym_vector.clear();
    mod_vector.clear();

    definedSymbols.clear();
    symbolsDefinedMultipleTimes.clear();
    usedSymbols.clear();

    mod_count = 1;
    mod_badr = 0;
    line="";
    line_no = 0;
    line_offset=0;
    global_codecount_sum = 0;
    input_file.close();
    eof_reached = false;


    pass2(in_file);
    
    return 0;
}