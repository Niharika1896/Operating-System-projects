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


using namespace std;

int quantum = 10000;
int max_prios = 4;

enum process_state {created,ready,running,blocked,terminated};
enum transition{TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT, TRANS_TO_COMPLETE};

class Process{
  public:
    int pid;
    int arrival_time;
    int cpu_burst;
    int io_burst;
    int total_cpu_time;
    process_state state;
    int stat_prio = -99;
    int dyn_prio;
    int remaining_cpu_time;
    int total_time_spent_in_io = 0;
    int finish_time = -1;
    int readySince = -1;
    int state_ts;
    int remaining_cpu_burst = -99;
    bool dp_enabled = false;
    
};

class Event{
  public:
    int timestamp;
    Process* p;
    transition trans;

    void load(int time, Process* pro){
      this->timestamp= time;
      this->p = pro;
    }


};

class DES {
  public:
    deque <Event*> eventQ;
    Event* get_event(){
      if (eventQ.empty()) {
			   return NULL;
		  }
      
      Event* cur_event = eventQ.front();
      eventQ.pop_front();
		  return cur_event;
    }

    void put_event(Event* e) {
      int i;
      deque<Event*>::iterator it = eventQ.begin();
      for(i=0; i < eventQ.size(); i++){
        if(eventQ.at(i)->timestamp > e->timestamp){
          break;
        }
        ++it;
      }
      
	    eventQ.insert(it,e);
    }

    void remove_event(Process *pro) {
      deque<Event*>::iterator it;
      bool flag = false;
      for(it = eventQ.begin(); it != eventQ.end(); it++){
        if((*it)->p->pid == pro->pid){
          flag = true;
          break;
        }
      }
      if (flag)
        eventQ.erase(it);
      
    }



    int get_next_event_ts(Process *pro){
      
      deque<Event*>::iterator it = eventQ.begin();
      for(it = eventQ.begin(); it!= eventQ.end(); it++){
        if((*it)->p->pid == pro->pid)
        return (*it)->timestamp;
      }
      return 0;
    }

   

    void print_all_event(){
      cout<<"Current status of deque is: \n";
      deque<Event>::iterator it1;
      for(int i=0; i < eventQ.size(); i++){
          cout<<"Timestamp: "<<eventQ.at(i)->timestamp<<" Process: "<<eventQ.at(i)->p->pid<<endl;
      }
      cout<<endl;
        
    }
};

ifstream rand_file;
ifstream input_file;

vector <Process*> process_vector;
vector <int> random_nums;
vector <pair<int, int>> io_vector;

DES mydes;
int ofs = 0;
int tot_random_nums =0;

bool first_read_flag = true;
int system_arrival_time;
int current_time = 0;

bool v_verbose_flag;
bool t_eventQ_flag;
bool e_runQ_flag;
bool p_preemp_flag;

void printStats();
void checkGetEvent();
void printStats();
void simulation();

int io_min_bracket = 0;
int io_max_bracket = 0;
int io_burst_total = 0;
string scheduling_algorithm = "";

int myrandom(int burst) { 
  if(ofs == tot_random_nums){
    ofs = 0;
  }
  return 1 + (random_nums[ofs++] % burst); 
}

void checkGetEvent(){
  while (!mydes.eventQ.empty()){
    Event* temp = mydes.eventQ.front();
    Process *temp_proc = temp->p;
    cout<<"Event id: "<<temp_proc->pid<<" ; Event timestamp: "<<temp->timestamp<<endl;
    mydes.eventQ.pop_front();
  }
}

void setQuantumMaxPrio(string sched_ch){
  
  if(sched_ch.length() > 1){
    if(sched_ch.find(':') != std::string::npos){
      quantum = stoi(sched_ch.substr(1, sched_ch.find(':')+1));
      max_prios = stoi(sched_ch.substr(sched_ch.find(':')+1));
    } else {
      quantum = stoi(sched_ch.substr(1));
    }
  } 
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

void read_inputfile(string filename){
  //cout<<"Inside read input file"<<endl;
  input_file.open(filename.c_str());

  string line;
  char *ptr;
  string str = "";
  int id = 0;
  while(!input_file.eof()){
    getline(input_file, line);
    if(line.empty()){
      continue;
    }

    Process *pro = new Process();

    pro->pid = id++;

    ptr = strtok((char*)line.c_str(), " ");
    str = ptr;
    pro->arrival_time = stoi(str);

    if(first_read_flag){
      system_arrival_time = stoi(str);
      first_read_flag = false;
    }

    ptr = strtok(NULL, " ");
    str = ptr;
    pro->total_cpu_time = stoi(str);
    pro->remaining_cpu_time = stoi(str);

    ptr = strtok(NULL, " ");
    str = ptr;
    pro->cpu_burst = stoi(str);

    ptr = strtok(NULL, " ");
    str = ptr;
    pro->io_burst = stoi(str);

    pro->state = created;
    pro->stat_prio = myrandom(max_prios);
    pro->dyn_prio = pro->stat_prio - 1;

    process_vector.push_back(pro);

    Event *e = new Event(); 
    e->p = pro;
    e->timestamp = pro->arrival_time;
    e->trans = TRANS_TO_READY;
    mydes.put_event(e);

  }
  

}

class Scheduler {
public:
	deque<Process *> runQ;
	virtual void add_process(Process * p){};
	virtual Process * get_next_process(){return nullptr;};
  virtual void change_dyn_prio(Process *){};
  virtual void check_preemption(Process *, Process *, int ){};
};

class FCFSScheduler : public Scheduler {
  public:
    void add_process(Process *p) {
		  runQ.push_back(p);
	  }
    Process * get_next_process(){
      if(runQ.empty()){
        return NULL;
      }
      Process * proc = runQ.front();
      runQ.pop_front();
      return proc;

    }
    void change_dyn_prio(Process *p){

    }
};

class LCFSScheduler : public Scheduler {
  public:
    void add_process(Process *p) {
		  runQ.push_back(p);
	  }
    Process * get_next_process(){
      if(runQ.empty()){
        return NULL;
      }
      Process * proc = runQ.back();
      runQ.pop_back();
      return proc;

    }
    void change_dyn_prio(Process *p){

    }
};

class SRTFScheduler : public Scheduler {
  public:
    void add_process(Process *p) {
		  runQ.push_back(p);
	  }
    Process * get_next_process(){
      if(runQ.empty()){
        return NULL;
      }
      
      int min_remaining_time = INT_MAX;
      deque<Process*>::iterator it = runQ.begin();
      deque<Process*>::iterator min_it;
      Process *proc;

      for(int i=0; i < runQ.size(); i++){
        if(runQ.at(i)->remaining_cpu_time < min_remaining_time){
          min_remaining_time = runQ.at(i)->remaining_cpu_time;
          min_it = it;
          proc = runQ.at(i);
        }
        
        ++it;
      }

      runQ.erase(min_it);
      return proc;

    }
    void change_dyn_prio(Process *p){

    }
};

class RRScheduler : public Scheduler {
  public:
    void add_process(Process *p) {
		  runQ.push_back(p);
	  }
    Process * get_next_process(){
      if(runQ.empty()){
        return NULL;
      }
      Process * proc = runQ.front();
      runQ.pop_front();
      return proc;

    }
    void change_dyn_prio(Process *p){

    }
};

class PRIOScheduler : public Scheduler {
  public:
    vector<deque<Process *>> activeQ;
    vector<deque<Process *>> expiredQ;

    PRIOScheduler() {
      activeQ = vector<deque<Process *>>(max_prios);
      expiredQ =  vector<deque<Process *>>(max_prios);
    }

    void add_process(Process *p) {
      
      int dyn_prio_index = p->dyn_prio;

      if (!p->dp_enabled){
        activeQ[dyn_prio_index].push_back(p);
      } else {
        expiredQ[dyn_prio_index].push_back(p);
      }
      p->dp_enabled = false;
		  
	  }

    void change_dyn_prio(Process *p) {
      p->dyn_prio --;
      if (p->dyn_prio == -1) {
        p->dyn_prio = p->stat_prio - 1;
        p->dp_enabled = true;
      } else {
        p->dp_enabled = false;
      }
    }

    void swapQ(){
      deque<Process *> temp;
      for(int i =0; i< max_prios; i++){
        temp = activeQ[i];
        activeQ[i] = expiredQ[i];
        expiredQ[i] = temp;
      }
    }

    bool isActiveQEmpty(){
      for(int i = 0 ; i < max_prios ; i++){
        if(!activeQ[i].empty()){
          return false;
        }
      }
      return true;
    }
    bool isExpiredQEmpty(){
      for(int i = 0 ; i < max_prios ; i++){
        if(!expiredQ[i].empty()){
          return false;
        }
      }
      return true;
    }
    Process * get_next_process(){
      if(isActiveQEmpty() && isExpiredQEmpty()){
        return NULL;
      }
      if(isActiveQEmpty()){
        swapQ();
      }
    
      Process * proc;

      for(int i= max_prios-1; i >=0; i--){
        if(!activeQ.at(i).empty()){
          proc = activeQ.at(i).front();
          activeQ.at(i).pop_front();
          break;
        }
      }
      
      return proc;

    }
};

class PREPRIOScheduler : public Scheduler {
  public:
    vector<deque<Process *>> activeQ;
    vector<deque<Process *>> expiredQ;

    PREPRIOScheduler() {
      activeQ = vector<deque<Process *>>(max_prios);
      expiredQ =  vector<deque<Process *>>(max_prios);
    }

    void add_process(Process *p) {
      
      int dyn_prio_index = p->dyn_prio;

      if (!p->dp_enabled){
        activeQ[dyn_prio_index].push_back(p);
      } else {
        expiredQ[dyn_prio_index].push_back(p);
      }
      p->dp_enabled = false;
		  
	  }

    void change_dyn_prio(Process *p) {
      p->dyn_prio = p->dyn_prio - 1 ;
      if (p->dyn_prio == -1) {
        p->dyn_prio = p->stat_prio - 1;
        p->dp_enabled = true;
      } else {
        p->dp_enabled = false;
      }
    }

    void swapQ(){
      deque<Process *> temp;
      for(int i =0; i< max_prios; i++){
        temp = activeQ[i];
        activeQ[i] = expiredQ[i];
        expiredQ[i] = temp;
      }
    }

    bool isActiveQEmpty(){
      for(int i = 0 ; i < max_prios ; i++){
        if(!activeQ[i].empty()){
          return false;
        }
      }
      return true;
    }
    bool isExpiredQEmpty(){
      for(int i = 0 ; i < max_prios ; i++){
        if(!expiredQ[i].empty()){
          return false;
        }
      }
      return true;
    }
    Process * get_next_process(){
      if(isActiveQEmpty() && isExpiredQEmpty()){
        return NULL;
      }
      if(isActiveQEmpty()){
        swapQ();
      }
    
      Process * proc;

      for(int i= max_prios-1; i >=0; i--){
        if(!activeQ.at(i).empty()){
          proc = activeQ.at(i).front();
          activeQ.at(i).pop_front();
          break;
        }
      }
      
      return proc;

    }

    void check_preemption(Process *cur_running_pro, Process *currentProcess, int cur_time) {
    int pro_next_ts = 0;
    if (cur_running_pro != nullptr) {
      pro_next_ts = mydes.get_next_event_ts(cur_running_pro);
      if (currentProcess->dyn_prio > cur_running_pro->dyn_prio) {
        if (pro_next_ts > cur_time) {

          mydes.remove_event(cur_running_pro);
          Event *e = new Event();
          e->p = cur_running_pro;
          e->timestamp = cur_time;
          e->trans = TRANS_TO_PREEMPT;
          mydes.put_event(e);

          cur_running_pro->remaining_cpu_time += (pro_next_ts - cur_time);
          if (cur_running_pro->remaining_cpu_burst == -99) {
            cur_running_pro->remaining_cpu_burst = (pro_next_ts - cur_time);
          }
          else {
            cur_running_pro->remaining_cpu_burst += (pro_next_ts - cur_time);
          }
          
        }
      }
    }
  }
};

void showRandomNums(){
  cout<<"Total no of random nos is ";
  cout<<tot_random_nums<<endl;
}

double get_system_IO() {
    double res = 0;
    int index = 0;
    sort(io_vector.begin(), io_vector.end());
    if (io_vector.size() == 0) {
        return res;
    }
    else {
        for (int i = 0; i < io_vector.size(); i++) {
            if (io_vector[index].second >= io_vector[i].first) {
                io_vector[index].second = max(io_vector[index].second, io_vector[i].second);
                io_vector[index].first = min(io_vector[index].first, io_vector[i].first);
            }
            else {
                index++;
                io_vector[index] = io_vector[i];
            }
        }

        for (int i = 0; i <= index; i++) {
            res += (io_vector[i].second - io_vector[i].first);
        }
    }
    return res;
}

Scheduler *scheduler;

int main (int argc, char **argv)
{
  int c;
  string sched_ch = "";
  string sched_algo = "";

  opterr = 0;

  int index;
/*
  optarg indicates an optional parameter to a command line option
  opterr can be set to 0 to prevent getopt() from printing error messages
  optind is the index of the next element of the argument list to be process 
  optopt is the command line option last matched. 
*/
  while ((c = getopt (argc, argv, "vteps:")) != -1)
    switch (c)
      {
      case 'v':
        v_verbose_flag = true;
        break;
      case 't':
        t_eventQ_flag = true;
        break;
      case 'e':
        e_runQ_flag = true;
        break;
      case 'p':
        p_preemp_flag = true;
        break;
      case 's':
        sched_ch = string(optarg);
        setQuantumMaxPrio(sched_ch);
        if(sched_ch[0] == 'F'){
          
            scheduler = new FCFSScheduler();
            scheduling_algorithm = "FCFS";
            cout<<scheduling_algorithm<<endl;

        } else if (sched_ch[0] == 'L') {
          
            scheduler = new LCFSScheduler();
            scheduling_algorithm = "LCFS";
            cout<<scheduling_algorithm<<endl;

        } else if (sched_ch[0] == 'S') {
          
            scheduler = new SRTFScheduler();
            scheduling_algorithm = "SRTF";
            cout<<scheduling_algorithm<<endl;

        } else if (sched_ch[0] == 'R') {
          
            scheduler = new RRScheduler();
            scheduling_algorithm = "RR";
            cout<<scheduling_algorithm<<" "<<quantum<<endl;

        } else if (sched_ch[0] == 'P') {
          
            scheduler = new PRIOScheduler();
            scheduling_algorithm = "PRIO";
            cout<<scheduling_algorithm<<" "<<quantum<<endl;

        } else if (sched_ch[0] == 'E') {
            //cout<<"1 "<<endl;
            scheduler = new PREPRIOScheduler();
            //cout<<"2 "<<endl;
            scheduling_algorithm = "PREPRIO";
            /*cout<<"3"<<endl;
            cout<<quantum<<endl;
            cout<<max_prios<<endl;*/
            cout<<scheduling_algorithm<<" "<<quantum<<endl;

        }
        
        break;
      case '?':
        if (optopt == 's')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
        return 1;
      default:
        abort ();
      }

    string filename = string(argv[optind++]);
    string randomfilename = string(argv[optind]);

    read_randomfile(randomfilename);
    read_inputfile(filename);

    simulation();
    printStats();

    

}

void printStats(){
  int last_event_finish_time = -1;
  int overall_total_cpu_time = 0;
  int overall_total_io_time = 0;
  int overall_total_tat = 0;
  int overall_cpu_wait_time = 0;
  int count = 0;
  for(int i = 0; i < process_vector.size(); i++){
      Process *pro = process_vector[i];
      int tat = pro->finish_time - pro->arrival_time;
      int cpu_waiting_time = tat - pro->total_cpu_time - pro->total_time_spent_in_io;

      if(pro->finish_time > last_event_finish_time){
        last_event_finish_time = pro->finish_time;
      }
      overall_total_cpu_time += pro->total_cpu_time;
      overall_total_io_time += pro->total_time_spent_in_io;
      overall_total_tat += tat;
      overall_cpu_wait_time += cpu_waiting_time;
      count++;
      
      printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", pro->pid, pro->arrival_time, pro->total_cpu_time, pro->cpu_burst, pro->io_burst, pro->stat_prio, pro->finish_time, tat, pro->total_time_spent_in_io, cpu_waiting_time);
  }

  double cpu_utilization = (overall_total_cpu_time/(double)last_event_finish_time)*100;
  double overall_io = get_system_IO();
  double io_utilization = (overall_io/(double)last_event_finish_time)*100;
  double avg_tat = overall_total_tat / (double)count;
  double avg_cpu_wait_time = overall_cpu_wait_time / (double)count;
  double throughput = (count/(double)last_event_finish_time) * 100;
  printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",last_event_finish_time,cpu_utilization,io_utilization,avg_tat,avg_cpu_wait_time,throughput);

}

void simulation(){
  Event *event;
  bool call_sched = false;
  Process * current_running_process = NULL;
  //cout<<"Inside simulation"<<endl;
  int timeInPrevState;

  while(event = mydes.get_event()){
    Process *pro = event->p;
    current_time = event->timestamp;
    //timeInPrevState = current_time - pro->state_ts;
    //cout<<"\n\ngetting event from event Q event process id: "<<pro->pid<<endl;
    //cout<<"get event trans is : "<<event->trans<<endl;

    switch(event->trans){
      case TRANS_TO_READY: 
        {
          if(pro->state == created){
            if(v_verbose_flag){
              cout<<current_time<<" "<<pro->pid<<" 0: CREATED -> READY"<<endl;
            }
          } else {
            if(v_verbose_flag){
              int time_spent_in_ready = current_time - pro->readySince;
              cout<<current_time<<" "<<pro->pid<<time_spent_in_ready<<" : READY -> RUNNING"<<endl;
            }
          }
          pro->state = ready;
          scheduler->check_preemption(current_running_process, pro,  current_time);

          call_sched = true;
          scheduler->add_process(pro);
          break;
        }
      case TRANS_TO_RUN:
        {
          int current_process_cpu_burst = pro->cpu_burst;
          int time_run;
          if(pro->remaining_cpu_burst > 0){
            time_run = pro->remaining_cpu_burst;
          } else {
            time_run = myrandom(current_process_cpu_burst);
          }
          int process_remaining_cpu_time = pro->remaining_cpu_time;
          //cout<<"cb: "<<time_run<<endl;
          if(process_remaining_cpu_time <= time_run && process_remaining_cpu_time <= quantum){
          //if(process_remaining_cpu_time <= time_run)  {
            int new_time = current_time + process_remaining_cpu_time;
            pro->remaining_cpu_time = 0;
            Event *e = new Event();
            e->p = pro;
            e->timestamp = new_time;
            e->trans = TRANS_TO_COMPLETE;
            mydes.put_event(e);
            if(v_verbose_flag){
              cout<<"cb = "<<time_run<<" rem = "<<process_remaining_cpu_time<<" prio = "<<pro->dyn_prio<<endl;
              int time_spent_in_current_state = new_time - current_time;
              cout<<new_time<<" "<<pro->pid<<" "<<time_spent_in_current_state<<" : RUNNING -> TERMINATED"<<endl;
            }
            
          } else if(time_run <= process_remaining_cpu_time && time_run <= quantum){
          //else{  
            int new_time = current_time + time_run;
            pro->remaining_cpu_time -= time_run;
            pro->remaining_cpu_burst = -99;
            Event * e = new Event();
            e->p = pro;
            e->timestamp = new_time;
            e->trans = TRANS_TO_BLOCK;
            mydes.put_event(e);
            if(v_verbose_flag){
              cout<<"cb = "<<time_run<<" rem = "<<process_remaining_cpu_time<<" prio = "<<pro->stat_prio<<endl;
              int time_spent_in_current_state = new_time - current_time;
              cout<<new_time<<" "<<pro->pid<<" "<<time_spent_in_current_state<<" : RUNNING -> BLOCKED"<<endl;
            }

          } else if(quantum < time_run && quantum < process_remaining_cpu_time){
            //cout<<"here here here"<<endl;
            int new_time = current_time+quantum;
            pro->remaining_cpu_time -= quantum;
            pro->remaining_cpu_burst =time_run- quantum;
            //cout<<"remaining cpu burst for "<<pro->pid<<" "<<pro->remaining_cpu_burst<<endl;
            Event * e = new Event();
            e->p = pro;
            e->timestamp = new_time;
            e->trans = TRANS_TO_PREEMPT;
            mydes.put_event(e);
            if(v_verbose_flag){
              cout<<"cb = "<<time_run<<" rem = "<<process_remaining_cpu_time<<" prio = "<<pro->stat_prio<<endl;
              int time_spent_in_current_state = new_time - current_time;
              cout<<new_time<<" "<<pro->pid<<" "<<time_spent_in_current_state<<" : RUNNING -> READY"<<endl;
            pro->readySince = new_time;
            }

          } 
          break;
        }
      case TRANS_TO_PREEMPT:
        {
          scheduler->change_dyn_prio(pro);
          call_sched = true;
          scheduler->add_process(pro);
          current_running_process = NULL;
          pro->state = ready;
          break;
        }
      case TRANS_TO_BLOCK:
        {
          
          int current_process_io_burst = pro->io_burst;
          int actual_io_burst = myrandom(current_process_io_burst);
          pro->total_time_spent_in_io += actual_io_burst;
          int new_time = current_time + actual_io_burst;

          pair <int, int>  iop;
          iop.second = new_time;
          iop.first = current_time;
          io_vector.push_back(iop);

          pro->dyn_prio = pro->stat_prio - 1;
          //cout<<"max io burst: "<<current_process_io_burst<<" ; actual io burst: "<<actual_io_burst<<endl;
          //cout<<"current time: "<<current_time<<" ; new time: "<<new_time<<endl;
          Event *e1 = new Event();
          e1->timestamp = new_time;
          e1->trans = TRANS_TO_READY;
          e1->p = pro;
          mydes.put_event(e1);

          if(v_verbose_flag){
            int time_spent_in_current_state = new_time - current_time;
            cout<<new_time<<" "<<pro->pid<<" "<<time_spent_in_current_state<<" : BLOCKED -> READY"<<endl;
            //pro->readySince = new_time;
          }

          current_running_process = NULL;
          call_sched = true;
          pro->state = blocked;

          pro->dyn_prio = pro->stat_prio - 1;
          break;
        }
      case TRANS_TO_COMPLETE:
        {
          pro->finish_time = current_time;
          current_running_process = NULL;
          call_sched = true;
          pro->state = terminated;
        } 

    }

    delete event;
    event = NULL;
    //cout<<"Scheduler runQ size : "<<scheduler->runQ.size()<<endl;
    if(call_sched){
      if(!mydes.eventQ.empty() && mydes.eventQ.front()->timestamp == current_time){
        continue;
      }
      call_sched = false;
      if(current_running_process == nullptr){
        current_running_process = scheduler->get_next_process();
        if(current_running_process == nullptr) 
          continue;
        //cout<<"current running process pid is:"<<current_running_process->pid<<endl;
        Event *e = new Event();
        e->p = current_running_process;
        e->timestamp = current_time;
        e->trans = TRANS_TO_RUN;
        mydes.put_event(e);
        current_running_process->state = running;
          
      } 
    }
  }
}