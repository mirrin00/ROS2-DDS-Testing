#include <string>
#include <chrono>
#include <ctime>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "nlohmann/json.hpp"
#include <fstream>

class TestMiddlewarePub
{
public:
    explicit TestMiddlewarePub(std::vector<std::string> &topics,  int msgCount, int prior, int cpu_index,
            int min_msg_size, int max_msg_size, int step, int interval, int msgs_before_step) :
            _topic_names(topics),
    _msInterval(interval),
    _msgCount(msgCount),
    _priority(prior),
    _cpu_index(cpu_index),
    _byteSizeMin(min_msg_size),
    _byteSizeMax(max_msg_size),
    _step(step),
    _msg_count_befor_step(msgs_before_step)
    {
        pid_t id = getpid();
        if(prior >= 0){
            sched_param priority;
            priority.sched_priority = sched_get_priority_max(prior);
            int err = sched_setscheduler(id, SCHED_FIFO, &priority);
            if(err) {
                std::cout << "Erorr in setting priority: " << -err << std::endl;
                throw;
            }
        }
        if(cpu_index >= 0){
            std::ofstream f_task("/sys/fs/cgroup/cpuset/pub_cpuset/tasks", std::ios_base::out);
            if(!f_task.is_open()){
                std::cout << "Erorr in adding to cpuset"<< std::endl;
                throw;
            }
            else{                                                   // добавить изменения номера ядра для привязки
                auto s = std::to_string(id);
                f_task.write(s.c_str(),s.length());
            }
            f_task.close();
        }
    };
    int StartTest(){
        std::this_thread::sleep_for(std::chrono::seconds(4));
        int cur_size = _byteSizeMin;
        for (int i = 0; i < _msgCount; ++i) {
            if(i % (_msg_count_befor_step-1) == 0 && cur_size <= _byteSizeMax)
                cur_size += _step;
            publish(i, cur_size, _topic_names[0]);
            std::this_thread::sleep_for(std::chrono::milliseconds(_msInterval));
        }
        return 0;
    }

    virtual void publish(short id, unsigned size, std::string topic)=0;

    virtual void setQoS(std::string filename)=0;	//считывать наверное тоже из json, так как будут разные конфигурации QoS

protected:
    std::vector<std::string> _topic_names;
    int _msInterval;
    int _msgCount;
    int _priority; //not stated
    int _cpu_index; //not stated
    int _byteSizeMin;
    int _byteSizeMax;
    int _step;
    int _msg_count_befor_step;
};

