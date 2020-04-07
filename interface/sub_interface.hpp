#include <string>
#include <chrono>
#include <ctime>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "../nlohmann/json.hpp"
#include <fstream>
#include <cmath>
#include "test_errors.hpp"

#define TIMEOUT 2 * pow(10, 10)

class TestMiddlewareSub
{
public:
    explicit TestMiddlewareSub(std::vector<std::string> &topics, int msgCount, int prior, int cpu_index, std::vector<std::string> &filenames) :
            _topic_names(topics),
            rec_time(topics.size()),
            msgs(topics.size()),
            _msgCount(msgCount),
            _priority(prior),
            _cpu_index(cpu_index),
            _filenames(filenames)
    {
        for(unsigned i = 0; i < topics.size(); ++i) {
            rec_time[i].resize(msgCount);
            msgs[i].resize(msgCount);
        }
        pid_t id = getpid();
        if(prior >= 0){
            sched_param priority;
            priority.sched_priority = _priority;
            int err = sched_setscheduler(id, SCHED_FIFO, &priority);
            if(err) {
                throw test_exception("Error in setting priority: " + std::to_string(err), THREAD_PRIOR_ERROR);
            }
        }
        if(cpu_index >= 0){
            std::ofstream f_task("/sys/fs/cgroup/cpuset/sub_cpuset/tasks", std::ios_base::out);
            if(!f_task.is_open()){
                throw test_exception("Error in adding to cpuset!", CPUSET_ERROR);
            }
            else{                                                   // добавить изменения номера ядра для привязки
                auto s = std::to_string(id);
                f_task.write(s.c_str(),s.length());
            }
            f_task.close();
        }
    };

    virtual int receive(int topic_id)=0;  //записывает вектор принятых сообщений

    int StartTest(){
        for(unsigned i=0; i < _topic_names.size(); ++i){
            int count = 0;
            unsigned long start_timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            while (count < _msgCount) {
                int pre_rec_count = count;
                count += receive(i);
                unsigned long end_timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                if (pre_rec_count == count && count != 0) {
                    if (end_timeout - start_timeout > TIMEOUT)
                        throw test_exception("Timeout exceeded!", TIMEOUT_ERROR);
                } else {
                    start_timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                }
            }
        }
        to_Json();
        return 0;
    }

    void to_Json(){
        for (unsigned k = 0; k < _topic_names.size(); ++k) {
            auto json = nlohmann::json::array();
            for (int i = 0; i < _msgCount; ++i) {
                nlohmann::json msg;
                auto id = msgs[k][i].first;
                auto sent_time = msgs[k][i].second;
                msg["msg"] = {{"id", id}, {"sent_time", sent_time}, {"rec_time", rec_time[k][i]}, {"delay", rec_time[k][i] - sent_time}};
                json.push_back(msg);
            }
            std::ofstream file(_filenames[k]);
            file << json;
        }
    }

protected:
    std::vector<std::string> _topic_names;
    std::vector<std::vector<unsigned long>> rec_time;
    std::vector<std::vector<std::pair<short, unsigned long>>> msgs;
    int _msgCount;
    int _priority; //def not stated
    int _cpu_index; //def not stated
    std::vector<std::string> _filenames;
};