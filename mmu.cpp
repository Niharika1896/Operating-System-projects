using namespace std;

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <iostream>
#include <deque>
#include <fstream>
#include <vector>
#include <algorithm>
#include <list>
#include <limits.h>


ifstream rand_file;
ifstream input_file;

int NUM_PROCESSES;
int _NUMFRAMES;
const int _NUMPAGES = 64;
string victim_selection_algo;

bool verbose = false, pagetableprint =false, frametableprint = false, statprint = false;

const long cost_of_maps = 300;
const long cost_of_unmaps = 400;
const long cost_of_ins = 3100;
const long cost_of_outs = 2700;
const long cost_of_fins = 2800;
const long cost_of_fouts = 2400;
const long cost_of_zeros = 140;
const long cost_of_segv = 340;
const long cost_of_segprot = 420;

const long cost_of_context_switch = 130;
const long cost_of_process_exits = 1250;

unsigned long long total_cost = 0;

unsigned long total_instructions = 0;
unsigned long total_context_switches = 0;
unsigned long total_process_exits = 0;
unsigned long total_read_write = 0;


class PTE {
public:
    unsigned int PRESENT : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;
    unsigned int FRAME_NO : 7;
    unsigned int FILE_MAPPED : 1;
    unsigned int ISHOLE : 1;
    unsigned int BUFFER : 18;
};

class VMA
{
public:
    int starting_virtual_page;
    int ending_virtual_page;
    bool write_protected;
    bool file_mapped;
    VMA(int start_page, int end_page, bool write_pro, bool file_map ) {
        this->starting_virtual_page = start_page;
        this->ending_virtual_page = end_page;
        this->write_protected = write_pro;
        this->file_mapped = file_map;
    }
};

class Process {
public:
    int process_id;
    int num_vma;
    vector<VMA *> vma_vector;
    PTE page_table[_NUMPAGES];

    unsigned long total_unmaps = 0;
    unsigned long total_maps = 0;
    unsigned long total_ins = 0;
    unsigned long total_outs = 0;
    unsigned long total_fins = 0;
    unsigned long total_fouts = 0;
    unsigned long total_zeros = 0;
    unsigned long total_segv = 0;
    unsigned long total_segprot = 0;

    Process(int pid) {
        this->process_id = pid;
        for (int i = 0; i < _NUMPAGES; i++) {
            page_table[i].PRESENT = 0;
            page_table[i].REFERENCED = 0;
            page_table[i].MODIFIED = 0;
            page_table[i].WRITE_PROTECT = 0;
            page_table[i].PAGEDOUT = 0;
            page_table[i].FRAME_NO = 0;
            page_table[i].FILE_MAPPED = 0;
            page_table[i].ISHOLE = 0;
            page_table[i].BUFFER = 0;
        }
    }
};

vector<Process *> process_vector;

class Frame {
    public:
        int frame_no;
        int process_id;
        PTE* pte;
        bool isUsed = false;
        uint32_t age = 0;
        int tau = 0;
        Frame(int frame_no) {
            this->frame_no = frame_no;
            this->process_id = -1;
            this->pte = NULL;
            this->isUsed = false;
        }
};

vector <Frame*> frametable;
deque<Frame*> freeframepool;

void init_frametable() {
    //frametable(_NUMFRAMES);
    for(int i =0 ;i < _NUMFRAMES; i++) {
        Frame *f = new Frame(i);
        frametable.push_back(f);
        freeframepool.push_back(f);
        //cout<<"Pushed frame "<<i<<" into frametable and free frame pool"<<endl;
    }
}

int tot_random_nums;
vector <int> random_nums;
int ofs = 0;

int myrandom(int burst) { 
  if(ofs == tot_random_nums){
    ofs = 0;
  }
  return (random_nums[ofs++] % burst); 
}

void read_randomfile(string randomfilename){ 
  rand_file.open(randomfilename.c_str());
  string line;
  char *ptr;
  getline(rand_file, line);
  int rand_num = stoi(line);
  tot_random_nums = rand_num;
  for(int i = 1; i <= rand_num; i++){
    getline(rand_file, line);
    int rand = stoi(line);
    random_nums.push_back(rand);
  }

}

void read_inputfile(string filename)
{
    //cout<<"Inside read input file"<<endl;
    input_file.open(filename.c_str());

    string line;
    char *ptr;
    string str = "";

    getline(input_file, line);
    while(line.empty() || (line.rfind("#", 0) == 0)) {
        getline(input_file, line);
    }
    
    NUM_PROCESSES = stoi(line);
    //cout<<"No of processes "<<NUM_PROCESSES<<endl;

    for(int i = 0; i < NUM_PROCESSES; i++) {

        Process *pro = new Process(i);
        getline(input_file, line);
        while(line.empty() || (line.rfind("#", 0) == 0)) {
            getline(input_file, line);
        }
        pro->num_vma = stoi(line);

        //cout<<"No of vma for process "<<i<<" = "<<pro->num_vma<<endl;
        for(int j=1; j <= pro->num_vma; j++) {
            
            getline(input_file, line);
            while(line.rfind("#", 0) == 0) 
                getline(input_file, line);

            ptr = strtok((char *)line.c_str(), " ");
            str = ptr;
            int start_page = stoi(str);
            //cout<<"SP:"<<start_page<<" ";

            ptr = strtok(NULL, " ");
            str = ptr;
            int end_page = stoi(str);
            //cout<<"EP:"<<end_page<<" ";

            ptr = strtok(NULL, " ");
            str = ptr;
            bool write_pro = stoi(str);
            //cout<<"WP:"<<write_pro<<" ";

            ptr = strtok(NULL, " ");
            str = ptr;
            bool file_map = stoi(str);
            //cout<<"FM:"<<file_map<<" ";

            VMA *vma = new VMA(start_page, end_page, write_pro, file_map);
            pro->vma_vector.push_back(vma);
        }
        
        //cout<<"Size of vma vector for process "<<i<<" - "<<pro->vma_vector.size()<<endl;
        process_vector.push_back(pro);
        //cout<<"Size of process vector "<<process_vector.size()<<endl;
    }
    
}

void printProcessVectorInformation() {
    for(int i = 0; i < process_vector.size(); i++) {
        cout<<"Process "<<process_vector[i]->process_id<<" :- "<<endl;
        cout<<"#VMA: "<<process_vector[i]->num_vma<<endl;
        vector<VMA *> temp = process_vector[i]->vma_vector;
        for(int j = 0; j < temp.size(); j++) {
             cout<<temp[j]->starting_virtual_page<<" "<<temp[j]->ending_virtual_page<<" "<<temp[j]->write_protected<<" "<<temp[j]->file_mapped<<endl;
        } 
        cout<<endl;
    }
}

class Pager {
    public:
        virtual Frame* select_victim_frame() = 0; // virtual base class
        virtual void init_age(int frame_num) = 0; // virtual base class
};

class FIFOPager : public Pager {
    public:
    int hand = 0;
    void init_age(int frame_num) {}
    Frame* select_victim_frame() {
        Frame* f = frametable[hand];
        hand = (hand+1) % _NUMFRAMES;
        return f;
    }
};
class RandomPager : public Pager {
    public:
    void init_age(int frame_num) {}
    Frame* select_victim_frame() {
        int index = myrandom(_NUMFRAMES);
        return frametable.at(index);
    }
};
class ClockPager : public Pager {
    public:
    int hand = 0;
    void init_age(int frame_num) {}
    Frame* select_victim_frame() {
        while(true) {
            Frame *f = frametable[hand];
            PTE *pte = f->pte;
            if(pte->REFERENCED) {
                pte->REFERENCED = 0;
                hand = (hand+1)% _NUMFRAMES;
            } else {
                hand = (hand+1)% _NUMFRAMES;
                return f;
            }
        }
        return NULL;
    }
};
class NRUPager : public Pager {
    public:
    int victim_call_count = 0;
    bool reset = 0;
    int hand = 0;
    void init_age(int frame_num) {}
    Frame* select_victim_frame() {
        reset = 0;
        if(total_instructions - victim_call_count >= 50) {
            reset = 1;
            victim_call_count = total_instructions;
        }
        vector <vector<Frame *> > classvector;
        classvector.resize(4);
        
        for(int i=0; i < frametable.size(); i++) {
            PTE *pte = frametable.at(hand)->pte;
            int M = pte->MODIFIED;
            int R = pte->REFERENCED;
            int index = 2*R + M;
            classvector[index].push_back(frametable.at(hand));
            if(reset) {
                pte->REFERENCED = 0;
            }
            hand = (hand+1)%_NUMFRAMES;
        }
        for(int i =0 ;i< classvector.size(); i++) {
            if(!classvector.at(i).empty()) {
                hand = (classvector.at(i)[0]->frame_no+1)% _NUMFRAMES;
                return classvector.at(i)[0];
            }
        }
        return NULL;
    }
};
class AgingPager : public Pager {
    public:
    int hand = 0;
    void init_age(int frame_num) {
        frametable.at(frame_num)->age = 0;
    }

    Frame* select_victim_frame() {
        Frame *f = frametable.at(hand);
        for(int i =0; i < frametable.size(); i++) {
            frametable.at(hand)->age = frametable.at(hand)->age >> 1;
            PTE *pte = frametable.at(hand)->pte;
            if(pte->REFERENCED) {
                pte->REFERENCED = 0;
                frametable.at(hand)->age = frametable.at(hand)->age | 0x80000000;
            }
            if(f->age > frametable.at(hand)->age) {
                f = frametable.at(hand);
            }
            hand = (hand+1)%_NUMFRAMES;
            
        }
        hand = (f->frame_no + 1)%_NUMFRAMES;
        return f;
    }
};
class WorkingSetPager : public Pager {
    public:
    int hand = 0;
    
    void init_age(int frame_num) {}

    Frame* select_victim_frame() {
        int minTime = 0;
        Frame *f = frametable.at(hand);
        for(int i =0; i < frametable.size(); i++) {
            PTE *pte = frametable.at(hand)->pte;
            if(pte->REFERENCED) {
                pte->REFERENCED = 0;
                frametable.at(hand)->tau = total_instructions;
            } else if(total_instructions - frametable.at(hand)->tau >= 50) {
                f = frametable.at(hand);
                break;
            } else if(total_instructions - frametable.at(hand)->tau > minTime) {
                minTime = total_instructions - frametable.at(hand)->tau;
                f = frametable.at(hand);
            }
            
            hand = (hand+1)%_NUMFRAMES;
            
        }
        hand = (f->frame_no + 1)%_NUMFRAMES;
        return f;
    }
};

Pager *pager;

Frame *allocate_frame_from_free_list() {
    if(freeframepool.empty()) {
        return NULL;
    } else {
        Frame *f = freeframepool.front();
        freeframepool.pop_front();
        return f;
    }
}

Frame *get_frame() {
    Frame *f = allocate_frame_from_free_list();
    if (f == NULL) {
        f = pager->select_victim_frame();
    }
    return f;
}

void simulation() {
    string line;
    char *ptr;
    string str = "";

    string operation;
    int vpage;

    Process * current_running_process = NULL;

    while(!input_file.eof()) {
        getline(input_file, line);
        while(!input_file.eof() && (line.empty() || (line.rfind("#", 0) == 0))) getline(input_file, line);
        if(input_file.eof()) break;
        ptr = strtok((char *)line.c_str(), " ");
        str = ptr;
        operation = str;
        
        ptr = strtok(NULL, " ");
        str = ptr;
        vpage = stoi(str);

        if(verbose) cout<<total_instructions<<": ==> "<<operation<<" "<<vpage<<endl;
        total_instructions++;

        if(operation[0] == 'c') {
            current_running_process = process_vector.at(vpage);
            total_context_switches++;

        } else if(operation[0] == 'e') {
            current_running_process = process_vector.at(vpage);
            Process *pro = current_running_process;
            if(verbose) cout<<"EXIT current process "<<pro->process_id<<endl;
            for(int i = 0; i < _NUMPAGES; i++) {
                PTE *pte = &pro->page_table[i];
                if(pte->PRESENT) {
                    if(verbose) cout<<" UNMAP "<<pro->process_id<<":"<<i<<endl;
                    pro->total_unmaps++;

                    int frame_entry = pte->FRAME_NO;

                    //unmap frame from frametable 
                    Frame *f = frametable.at(frame_entry);
                    f->pte = NULL;
                    f->isUsed = false;
                    f->process_id = -1;
                    freeframepool.push_back(f);
                
                    //free stuff from PTE 
                    pte->FRAME_NO = 0;
                    if(pte->FILE_MAPPED && pte->MODIFIED){
                        pro->total_fouts++;
                        if(verbose) cout<<" FOUT"<<endl;
                    }
                    
                    pte->PRESENT = 0;
                    pte->PAGEDOUT = 0;
                    pte->REFERENCED = 0;
                    pte->MODIFIED = 0;

                } else {
                    pte->PAGEDOUT = 0;
                }
                
                
            }
            current_running_process = NULL;
            total_process_exits++;

        } else {
            PTE *pte = &current_running_process->page_table[vpage];
            Process *pro = current_running_process;
            total_read_write++; //DANGER DANGER 

            if(!pte->PRESENT) {
                //raise page fault exception

                //check if this is a valid page 
                bool isVPageAHole = true;
                for(int i =0 ; i < pro->vma_vector.size(); i++) {
                    int cur_start_page = pro->vma_vector.at(i)->starting_virtual_page;
                    int cur_end_page = pro->vma_vector.at(i)->ending_virtual_page;
                    if(vpage>= cur_start_page && vpage <= cur_end_page) {
                        pte->FILE_MAPPED = pro->vma_vector.at(i)->file_mapped;
                        pte->WRITE_PROTECT = pro->vma_vector.at(i)->write_protected;
                        isVPageAHole = false;
                        break;
                    }
                }
                //No, it is not a valid page
                //raise segv error and continue;
                if(isVPageAHole) {
                    if(verbose) cout<<" SEGV"<<endl;
                    pro->total_segv++;
                    continue;
                } 
                //Yes, it is a valid page  
                else {
                    pte->PRESENT = 1;
                    Frame *f = get_frame();
                    if(f->isUsed) {
                        int old_process_id = f->process_id;
                        Process *old_pro = process_vector.at(old_process_id); 
                        PTE * old_pte = f->pte;
                        int old_vpage;

                        for(int k = 0; k < _NUMPAGES; k++){
                            if(&old_pro->page_table[k] == old_pte) {
                                old_vpage = k;
                            }
                        }
                        if(verbose) cout<<" UNMAP "<<old_process_id<<":"<<old_vpage<<endl;
                        old_pro->total_unmaps++;

                        //unmap the old frame
                        //f->pte = NULL;
                        //f->process_id = -1;
                    

                        //unmap the old pte
                        if(old_pte->FILE_MAPPED && old_pte->MODIFIED){
                            old_pro->total_fouts++;
                            if(verbose) cout<<" FOUT"<<endl;
                        } else if(!old_pte->FILE_MAPPED && old_pte->MODIFIED) {
                            old_pro->total_outs++;
                            if(verbose) cout<<" OUT"<<endl;  
                            old_pte->PAGEDOUT = 1;                             
                        }

                        old_pte->PRESENT = 0;
                        old_pte->REFERENCED = 0;
                        old_pte->MODIFIED = 0;
                        old_pte->FRAME_NO = 0;
                        
                    } 
                    f->isUsed = true;
                    // map the new frame 

                    f->process_id = pro->process_id;
                    f->pte = pte;
                    
                    pte->FRAME_NO = f->frame_no;
                    pte->PRESENT = 1;

                    if(pte->FILE_MAPPED) {
                        pro->total_fins++;
                        if(verbose) cout<<" FIN"<<endl;
                    } else if(pte->PAGEDOUT) {
                        pro->total_ins++;
                        if(verbose) cout<<" IN"<<endl;

                    } else {
                        pro->total_zeros++;
                        if(verbose) cout<<" ZERO"<<endl;
                    }
                    if(verbose) cout<<" MAP "<<f->frame_no<<endl;
                    pager->init_age(pte->FRAME_NO);
                    frametable.at(pte->FRAME_NO)->tau = total_instructions;
                    pro->total_maps++;

                    if(operation[0] == 'w') {
                        pte->REFERENCED = 1;
                        if(pte->WRITE_PROTECT) {
                            pte->MODIFIED = 0;
                            pro->total_segprot++;
                            if(verbose) cout<<" SEGPROT"<<endl;
                        } else if(!pte->WRITE_PROTECT){
                            pte->MODIFIED = 1;
                        }
                    } else if(operation[0] == 'r') {
                        pte->REFERENCED = 1;
                    }

                    
                }
                
            } else {
                if(operation[0] == 'w') {
                    pte->REFERENCED = 1;
                    if(pte->WRITE_PROTECT) {
                        pte->MODIFIED = 0;
                        pro->total_segprot++;
                        if(verbose) cout<<" SEGPROT"<<endl;
                    } else if(!pte->WRITE_PROTECT){
                        pte->MODIFIED = 1;
                    }
                } else if(operation[0] == 'r') {
                    pte->REFERENCED = 1;
                }
            }
        }
    }
    
}

void costing() {
    total_cost += total_read_write;
    total_cost += (total_context_switches*cost_of_context_switch);
    total_cost += (total_process_exits)*cost_of_process_exits;
    for(int i =0 ;i < process_vector.size(); i++) {
        total_cost += (process_vector.at(i)->total_maps * cost_of_maps);
        total_cost += (process_vector.at(i)->total_unmaps * cost_of_unmaps);
        total_cost += (process_vector.at(i)->total_ins * cost_of_ins);
        total_cost += (process_vector.at(i)->total_fins * cost_of_fins);
        total_cost += (process_vector.at(i)->total_outs * cost_of_outs);
        total_cost += (process_vector.at(i)->total_fouts * cost_of_fouts);
        total_cost += (process_vector.at(i)->total_zeros * cost_of_zeros);
        total_cost += (process_vector.at(i)->total_segv * cost_of_segv);
        total_cost += (process_vector.at(i)->total_segprot * cost_of_segprot);
    }
    //cout<<total_instructions<<" "<<total_context_switches<<" "<<total_process_exits<<" "<<total_cost<<" "<<endl;
}

void printsummary() {
    if(pagetableprint) {
        for(int i = 0; i < process_vector.size(); i++) {
            Process *pro = process_vector.at(i);
            cout<<"PT["<<i<<"]:";
            for(int j =0; j < _NUMPAGES; j++) {
                if(pro->page_table[j].PRESENT) {
                    cout<<" "<< j << ":";
                    if (pro->page_table[j].REFERENCED)
                        cout << "R";
                    else
                        cout << "-";
                    if (pro->page_table[j].MODIFIED)
                        cout << "M";
                    else
                        cout << "-";
                    if (pro->page_table[j].PAGEDOUT)
                        cout << "S";
                    else
                        cout << "-";
                } else {
                    if(pro->page_table[j].PAGEDOUT) {
                        cout << " #";
                    } else {
                        cout << " *";
                    }
                }
            }
            cout<<endl;
        }
    }

    if(frametableprint) {
        cout << "FT:";
        for (int i = 0; i < frametable.size(); i++) {
            if (frametable.at(i)->process_id != -1) {
                PTE *pte = frametable.at(i)->pte;
                Process* pro = process_vector.at(frametable.at(i)->process_id);
                int vpage;
                for(int k = 0; k < _NUMPAGES; k++){
                    if(&(pro->page_table[k]) == pte) {
                        vpage = k;
                    }
                }
                cout << " "<<frametable[i]->process_id << ":" << vpage ;
            } else {
                cout << " *";
            }
        }
        cout << endl;
    }

    if(statprint) {
        for(int i = 0; i < process_vector.size(); i++) {
            Process *pro = process_vector.at(i);
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n", 
            pro->process_id, pro->total_unmaps, pro->total_maps, pro->total_ins, pro->total_outs, 
            pro->total_fins, pro->total_fouts, pro->total_zeros, pro->total_segv, pro->total_segprot);
        }

        printf("TOTALCOST %lu %lu %lu %llu %lu\n",total_instructions, total_context_switches, total_process_exits, total_cost, sizeof(PTE));
    }
}

int main(int argc, char **argv) {
    int c;
    opterr = 0;
    string options ="";
    string sched_ch = "";
/*
  optarg indicates an optional parameter to a command line option
  opterr can be set to 0 to prevent getopt() from printing error messages
  optind is the index of the next element of the argument list to be process 
  optopt is the command line option last matched. 
*/
    while ((c = getopt (argc, argv, "f:o:a:")) != -1) {
        switch(c) {
            case 'f':
                _NUMFRAMES = stoi(string(optarg));
                break;
            case 'a':
                sched_ch = string(optarg);
                if(sched_ch[0] == 'f' || sched_ch[0] == 'F'){
                    pager = new FIFOPager();
                    victim_selection_algo = "FIFO";
                    //cout<<victim_selection_algo<<endl;
                } else if(sched_ch[0] == 'r' || sched_ch[0] == 'R'){
                    pager = new RandomPager();
                    victim_selection_algo = "Random";
                    //cout<<victim_selection_algo<<endl;
                } else if(sched_ch[0] == 'c' || sched_ch[0] == 'C'){
                    pager = new ClockPager();
                    victim_selection_algo = "Clock";
                    //cout<<victim_selection_algo<<endl;
                } else if(sched_ch[0] == 'e' || sched_ch[0] == 'E'){
                    pager = new NRUPager();
                    victim_selection_algo = "NRU";
                    //cout<<victim_selection_algo<<endl;
                } else if(sched_ch[0] == 'a' || sched_ch[0] == 'A'){
                    pager = new AgingPager();
                    victim_selection_algo = "Aging";
                    //cout<<victim_selection_algo<<endl;
                } else if(sched_ch[0] == 'w' || sched_ch[0] == 'W'){
                    pager = new WorkingSetPager();
                    victim_selection_algo = "Working Set";
                    //cout<<victim_selection_algo<<endl;
                } 
                break;
            case 'o':
                options = string(optarg);
                for(int i =0; i < options.length(); i++){
                    if(options[i] == 'O') 
                        verbose = true;
                    if(options[i] == 'P') 
                        pagetableprint = true;
                    if(options[i] == 'F') 
                        frametableprint = true;
                    if(options[i] == 'S') 
                        statprint = true;    
                }
                break;
            default:
                abort ();

        }
    }
    
    string filename = string(argv[optind++]);
    string randomfilename = string(argv[optind]);

    read_inputfile(filename);
    init_frametable();
    //cout<<"READ INPUT FILE"<<endl;
    read_randomfile(randomfilename);
    //cout<<"READ RANDOM FILE"<<endl;
    //printProcessVectorInformation();
    //cout<<"BEFORE SIMULATION"<<endl;
    simulation();
    //cout<<"END OF SIMULATION"<<endl;
    costing();
    printsummary();
    //cout<<"Hello"<<endl;
}



