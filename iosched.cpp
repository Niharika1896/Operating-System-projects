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

ifstream input_file;

int tick = 0;

enum DiskHeadDirection {
    FORWARD,
    BACKWARD
};

class DiskHead {
public:
    int current_seek_position;
    int head_count;
    DiskHeadDirection direction;

    DiskHead(int val1, int val2) {
        this->current_seek_position = val1;
        this->head_count = val2;
        this->direction = FORWARD;
    }
};

class IO {
public:
    int disk_track;
    int arrival_time;
    int start_time;
    int end_time;
    int tat;
    int wait_time;
    bool isActive;

    IO(int arr_time, int io_disk_track)
    {
        this->disk_track = io_disk_track;
        this->arrival_time = arr_time;
    }
};

deque<IO *> io_queue;
deque<IO *> wait_queue;
deque<IO *> active_queue;
vector<IO *> io_vector;

double total_time;
double total_movement;
double avg_tat;
double avg_waittime;
double max_wttime;

bool verbose_flag = false;
bool queue_flag = false;
bool flook_flag = false;
string scheduling_algorithm = "";
DiskHead* disk_head;

class Strategy {
public:
    virtual IO *select_victim_io() = 0;
};
class FIFOStrategy : public Strategy
{
public:
    IO *select_victim_io()
    {
        if (wait_queue.empty())
        {
            return NULL;
        }
        IO *io = wait_queue.front();
        wait_queue.pop_front();
        return io;
    }
};
class SSTFStrategy : public Strategy
{
public:
    IO *select_victim_io()
    {
        if (wait_queue.empty()) {
            return NULL;
        }

        IO* res;

        int shortest_seek_track = INT_MAX;
        int closest_seek_index = -1;

        for(int i=0; i < wait_queue.size();i++) {
            IO *io = wait_queue.at(i);
            int local_min_diff = abs(io->disk_track - disk_head->current_seek_position);
            if(local_min_diff < shortest_seek_track) {
                shortest_seek_track = local_min_diff;
                closest_seek_index = i;
            }
        }
        res = wait_queue.at(closest_seek_index);

        deque<IO*>::iterator it;
        it = wait_queue.begin();
        if(closest_seek_index > 0) {
            for (int i = 0; i <= closest_seek_index-1; i++) {
                it++;
            }
        }
        wait_queue.erase(it);

        return res;
    }
};
class LOOKStrategy : public Strategy
{
public:
    IO *select_victim_io()
    {
        if (wait_queue.empty())
        {
            return NULL;
        }
        IO* res;
        int shortest_seek_track = INT_MAX;
        int closest_seek_index = -1;

        while (true) {
            for(int i =0 ;i < wait_queue.size(); i++) {
                IO* io = wait_queue.at(i);
                if(disk_head->direction == FORWARD && io->disk_track >= disk_head->current_seek_position) {
                    int local_min_diff = io->disk_track - disk_head->current_seek_position;
                    if(local_min_diff < shortest_seek_track) {
                        shortest_seek_track = local_min_diff;
                        closest_seek_index = i;
                    }
                } else if(disk_head->direction == BACKWARD && io->disk_track <= disk_head->current_seek_position) {
                    int local_min_diff = disk_head->current_seek_position - io->disk_track;
                    if(local_min_diff < shortest_seek_track) {
                        shortest_seek_track = local_min_diff;
                        closest_seek_index = i;
                    }
                }
            }
            if(closest_seek_index == -1) {
                if(disk_head->direction == FORWARD) {
                    disk_head->direction = BACKWARD;
                } else if(disk_head->direction == BACKWARD) {
                    disk_head->direction = FORWARD;
                }
            } else {
                break;
            }
        }
        res = wait_queue.at(closest_seek_index);

        deque<IO*>::iterator it;
        it = wait_queue.begin();
        if(closest_seek_index > 0) {
            for (int i = 0; i <= closest_seek_index-1; i++) {
                it++;
            }
        }
        wait_queue.erase(it);
        
        return res;

    }
};
class CLOOKStrategy : public Strategy {
public:
    IO *select_victim_io() {
        if (wait_queue.empty()) {
            return NULL;
        }
        IO* res;
        int shortest_seek_track;
        int largest_seek_time;
        int closest_seek_index;

        while (true) {
            shortest_seek_track = INT_MAX;
            largest_seek_time = INT_MIN;
            closest_seek_index = -1;
            for(int i =0 ;i < wait_queue.size(); i++) {
                IO* io = wait_queue.at(i);
                if(io->disk_track >= disk_head->current_seek_position) {
                    int local_min_diff = io->disk_track - disk_head->current_seek_position;
                    if(local_min_diff < shortest_seek_track) {
                        shortest_seek_track = local_min_diff;
                        closest_seek_index = i;
                    }
                }
            }
            if(closest_seek_index == -1) {
                for(int i =0 ;i < wait_queue.size(); i++) {
                    IO* io = wait_queue.at(i);
                    if(io->disk_track < disk_head->current_seek_position) {
                        int local_max_diff = disk_head->current_seek_position - io->disk_track;
                        if(local_max_diff > largest_seek_time) {
                            largest_seek_time = local_max_diff;
                            closest_seek_index = i;
                        }
                    }
                }
            } 
            break;
        }
        res = wait_queue.at(closest_seek_index);

        deque<IO*>::iterator it;
        it = wait_queue.begin();
        if(closest_seek_index > 0) {
            for (int i = 0; i <= closest_seek_index-1; i++) {
                it++;
            }
        }
        wait_queue.erase(it);
        
        return res;

    }
};
class FLOOKStrategy : public Strategy {
public:
    IO *select_victim_io()
    {
        if (wait_queue.empty() && active_queue.empty())
        {
            return NULL;
        }

        if(active_queue.empty()){
            active_queue.swap(wait_queue);
        }
        IO* res;
        int shortest_seek_track = INT_MAX;
        int closest_seek_index = -1;

        while (true) {
            for(int i =0 ;i < active_queue.size(); i++) {
                IO* io = active_queue.at(i);
                if(disk_head->direction == FORWARD && io->disk_track >= disk_head->current_seek_position) {
                    int local_min_diff = io->disk_track - disk_head->current_seek_position;
                    if(local_min_diff < shortest_seek_track) {
                        shortest_seek_track = local_min_diff;
                        closest_seek_index = i;
                    }
                } else if(disk_head->direction == BACKWARD && io->disk_track <= disk_head->current_seek_position) {
                    int local_min_diff = disk_head->current_seek_position - io->disk_track;
                    if(local_min_diff < shortest_seek_track) {
                        shortest_seek_track = local_min_diff;
                        closest_seek_index = i;
                    }
                }
            }
            if(closest_seek_index == -1) {
                if(disk_head->direction == FORWARD) {
                    disk_head->direction = BACKWARD;
                } else if(disk_head->direction == BACKWARD) {
                    disk_head->direction = FORWARD;
                }
            } else {
                break;
            }
        }
        res = active_queue.at(closest_seek_index);

        deque<IO*>::iterator it;
        it = active_queue.begin();
        if(closest_seek_index > 0) {
            for (int i = 0; i <= closest_seek_index-1; i++) {
                it++;
            }
        }
        active_queue.erase(it);
        
        return res;

    }
};

Strategy *strategy;

void read_inputfile(string filename) {
    input_file.open(filename.c_str());

    string line;
    char *ptr;
    string str = "";

    while (!input_file.eof()) {
        getline(input_file, line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        ptr = strtok((char *)line.c_str(), " ");
        str = ptr;
        int arrival_time = stoi(str);

        ptr = strtok(NULL, " ");
        str = ptr;
        int io_disk_track = stoi(str);

        IO *io = new IO(arrival_time, io_disk_track);
        io_vector.push_back(io);
    }
}

void simulation() {

    IO * current_io = NULL;

    //copy from vector to queue
    for(int i = 0; i < io_vector.size(); i++) {
        IO* io = io_vector.at(i);
        io_queue.push_back(io);
    }

    //make DiskHead object 
    disk_head = new DiskHead(0, 0);

    while(!io_queue.empty() || !wait_queue.empty() || !active_queue.empty() || current_io != NULL) {
        
        //add a new IO that has arrived to the system to wait queue
        if(!io_queue.empty()) {
            IO* io = io_queue.front();
            if(io->arrival_time == tick) {
                wait_queue.push_back(io);
                io_queue.pop_front();
            }
        }
        
        //if the current IO is not null and completed at the time
        if(current_io != NULL && current_io->disk_track == disk_head->current_seek_position) {
            current_io->end_time = tick;
            current_io->isActive = false;
            current_io->wait_time = current_io->start_time - current_io->arrival_time;
            current_io->tat = current_io->end_time - current_io->arrival_time;
            current_io = NULL;
        }

        

        //if no IO request is active now, fetch next from startegy() and start new IO, else if all IO are processed exit
        if(current_io == NULL) {
            current_io = strategy->select_victim_io();
            if(current_io != NULL) {
                current_io->start_time = tick;
                current_io->isActive = true;
                int current_io_track = current_io->disk_track;
                if(current_io_track - disk_head->current_seek_position > 0 and disk_head->direction == FORWARD) {
                    disk_head->direction = FORWARD;
                } else if(current_io_track - disk_head->current_seek_position > 0 and disk_head->direction == BACKWARD) {
                    disk_head->direction = FORWARD;
                } else if(current_io_track - disk_head->current_seek_position < 0 and disk_head->direction == FORWARD) {
                    disk_head->direction = BACKWARD;
                } else if(current_io_track - disk_head->current_seek_position < 0 and disk_head->direction == BACKWARD) {
                    disk_head->direction = BACKWARD;
                } else if(current_io_track - disk_head->current_seek_position == 0) {
                    continue;
                }
            } else {
                //cout<<"victim selection returned null"<<endl;
            }
        }

        //if an IO is active, move head by one unit in the direction it is going. incremet time by 1
        if(current_io != NULL && current_io->disk_track != disk_head->current_seek_position) {
            if(disk_head->direction == FORWARD) {
                disk_head->current_seek_position++;
            } else {
                disk_head->current_seek_position--;
            }
            disk_head->head_count++;
        }

        
        tick++;

    }
        
    
}

void printSummary() {
    
    int total_time = tick-1;
    int tot_movement = disk_head->head_count;
    double tot_tat = 0 ;
    double tot_waittime = 0;
    int _NUMIO = io_vector.size();
    int max_waittime = INT_MIN;

    for(int i = 0; i < io_vector.size(); i++) {
        IO* iotemp = io_vector.at(i);
        tot_waittime += iotemp->wait_time;
        tot_tat += iotemp->tat;
        if(iotemp->wait_time > max_waittime) {
            max_waittime = iotemp->wait_time;
        }
    }

    double avg_turnaround = tot_tat/_NUMIO;
    double avg_waittime = tot_waittime/_NUMIO;

    for(int i = 0 ;i < io_vector.size(); i++) {
        IO* req = io_vector.at(i);
        printf("%5d: %5d %5d %5d\n",i, req->arrival_time, req->start_time, req->end_time);
    }
    printf("SUM: %d %d %.2lf %.2lf %d\n",total_time, tot_movement, avg_turnaround, avg_waittime, max_waittime);
}

int main(int argc, char **argv)
{
    int c;
    opterr = 0;
    string options = "";
    string sched_ch = "";
    
    while ((c = getopt(argc, argv, "vqfs:")) != -1)
    {
        switch (c)
        {
        case 'v':
            verbose_flag = true;
            break;
        case 'q':
            queue_flag = true;
            break;
        case 'f':
            flook_flag = true;
            break;
        case 's':
            sched_ch = string(optarg);
            if (sched_ch[0] == 'i' || sched_ch[0] == 'I')
            {
                strategy = new FIFOStrategy();
                scheduling_algorithm = "FIFO";
                //cout<<scheduling_algorithm<<endl;
            }
            else if (sched_ch[0] == 'j' || sched_ch[0] == 'J')
            {
                strategy = new SSTFStrategy();
                scheduling_algorithm = "SSTF";
                //cout<<scheduling_algorithm<<endl;
            }
            else if (sched_ch[0] == 's' || sched_ch[0] == 'S')
            {
                strategy = new LOOKStrategy();
                scheduling_algorithm = "LOOK";
                //cout<<scheduling_algorithm<<endl;
            }
            else if (sched_ch[0] == 'c' || sched_ch[0] == 'C')
            {
                strategy = new CLOOKStrategy();
                scheduling_algorithm = "CLOOK";
                //cout<<scheduling_algorithm<<endl;
            }
            else if (sched_ch[0] == 'f' || sched_ch[0] == 'F')
            {
                strategy = new FLOOKStrategy();
                scheduling_algorithm = "FLOOK";
                //cout<<scheduling_algorithm<<endl;
            }
            break;
        default:
            abort();
        }
    }

    string filename = string(argv[optind++]);

    read_inputfile(filename);

    simulation();

    printSummary();
}
