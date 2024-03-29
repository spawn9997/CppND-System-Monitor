#include "Process.h"
#include <vector>
class ProcessContainer{

private:
    std::vector<Process>_list;
public:
    ProcessContainer(){
        this->refreshList();
    }
    void refreshList();
    std::string printList();
    std::vector<std::string> getList();
};

void ProcessContainer::refreshList(){
    std::vector<std::string> pidList = ProcessParser::getPidList();
    this->_list.clear();
    for(int i=0;i<pidList.size();i++){
        /* *ADDENDUM* Check the pid to make sure it has an
        associated command cmdline. If it does not have one,
        do not add it to the list */
        std::string pid = pidList.at(i);
        if (!ProcessParser::getCmd(pid).empty()) {
            Process proc(pid);
            this->_list.push_back(proc);
        }
    }
}
std::string ProcessContainer::printList(){
    std::string result="";
    for(int i=0;i<this->_list.size();i++){
        result += this->_list[i].getProcess();
    }
    return result;
}
std::vector<std::string> ProcessContainer::getList(){
    std::vector<std::string> values;

    for(int i=(this->_list.size() -10); i < this->_list.size(); i++){
        values.push_back(this->_list[i].getProcess());
    }

    return values;
}
