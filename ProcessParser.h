#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"
#include "util.h"


using namespace std;

class ProcessParser{

    public:

    static string getCmd(string pid);
    static vector<string> getPidList();
    static std::string getVmSize(string pid);
    static std::string getCpuPercent(string pid);
    static long int getSysUpTime();
    static std::string getProcUpTime(string pid);
    static string getProcUser(string pid);
    static std::vector<std::string> getSysCpuPercent(std::string coreNumber = "");
    static float getSysRamPercent();
    static string getSysKernelVersion();
    static int getNumberOfCores();
    static int getTotalThreads();
    static int getTotalNumberOfProcesses();
    static int getNumberOfRunningProcesses();
    static string getOSName();
    static std::string PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2);
    static bool isPidExisting(string pid);
};

// helper functions
float getSysIdleCpuTime(std::vector<std::string> values)
{
    return (stof(values[S_IDLE]) + stof(values[S_IOWAIT]));
}

float getSysActiveCpuTime(std::vector<std::string> values)
{
return (stof(values[S_USER]) +
        stof(values[S_NICE]) +
        stof(values[S_SYSTEM]) +
        stof(values[S_IRQ]) +
        stof(values[S_SOFTIRQ]) +
        stof(values[S_STEAL]) +
        stof(values[S_GUEST]) +
        stof(values[S_GUEST_NICE]) );
}

// TODO: Define all of the above functions below:
std::string ProcessParser::getCmd(std::string pid)
{
    std::string line;
    ifstream stream;
    std::string path(Path::basePath() + pid + Path::cmdPath());

    try {
        Util::getStream(path, stream);
        std::getline(stream, line);
    } catch(...) {}

    return line;
}

std::vector<std::string> ProcessParser::getPidList()
{
    // return a pid list from /proc
    /* get the directory listing and filter out directories
       that are not a pid (int) */

    DIR *dir;
    std::vector<std::string> pidList;

    if (!(dir = opendir("/proc"))) {
        throw std::runtime_error(std::strerror(errno));
         // very unlikely that this will fail on linux
    }
    while (dirent *dirp = readdir(dir)) {
        /* check each entry for being a directory type 
        and the name consisting of all digits */
        if (dirp->d_type == DT_DIR) {
            if (all_of(dirp->d_name, dirp->d_name + std::strlen(dirp->d_name),
             [](char c){return isdigit(c);})) {
               pidList.push_back(dirp->d_name); 
            }
        }
    }

    if (closedir(dir)) {
        throw std::runtime_error(std::strerror(errno));
    }

    return pidList;
}

std::string ProcessParser::getVmSize(std::string pid)
{
    // returns how much RAM a given process (pid) is using

    std::string line;
    std::string vmSize;
    std::string name = "VmData";
    std::string path = Path::basePath() + pid + Path::statusPath();
    float result;

    // open stream to the path
    std::ifstream stream;
    try {
    Util::getStream(path, stream);
    while (std::getline(stream, line)){
        /* find the line that starts with VmData */
        if (line.compare(0, name.size(), name) == 0) {
            // this is the right line; now process it
            std::istringstream buffer(line);

            // this way of parsing can only use space as a delimiter
            std::istream_iterator<std::string> bufBegin(buffer);
            std::istream_iterator<std::string> bufEnd;

            // use range constructor of vector to perform the split
            std::vector<std::string> values(bufBegin, bufEnd);

            // Use second element since that is the "number we are looking for"
            // and convert to GB
            result = (stof(values[1])/float(pow(1024,2)));
            vmSize = to_string(result);
            break;
        }
    } 
    } catch (...) {}

    return vmSize;
}

std::string ProcessParser::getCpuPercent(std::string pid)
{
    // Gather information to calculate cpu percent

    // get ready to open file and extract information
    // one line so no loop needed
    std::string line;
    std::string cpuPercent;
    std::string path = Path::basePath() + pid + "/" + Path::statPath();

    std::ifstream stream;
    try {
        Util::getStream(path, stream);
    
    std::getline(stream, line);
    std::istringstream buffer(line);

    // this way of parsing can only use space as a delimiter
    std::istream_iterator<std::string> bufBegin(buffer);
    std::istream_iterator<std::string> bufEnd;

    // use range constructor of vector to perform the split
    std::vector<std::string> values(bufBegin, bufEnd);

    // other needed values and their positions in the stat data
    float stime = stof(values.at(14)); // 14
    float cutime = stof(values.at(15)); // 15
    float cstime = stof(values.at(16)); // 16
    float starttime = stof(values.at(21)); // 21

    
    float utime = stof(ProcessParser::getProcUpTime(pid)); // process uptime
    float uptime = ProcessParser::getSysUpTime(); // system uptime
    float freq = sysconf(_SC_CLK_TCK); // returns number of clock ticks per second

    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime/freq);
    float result = 100.0 * ((total_time/freq) / seconds);
    cpuPercent = to_string(result);
} catch(...) {}

    return cpuPercent;
}

long int ProcessParser::getSysUpTime()
{
    std::string line;
    std::string path = Path::basePath() + Path::upTimePath();

    std::ifstream stream;
    Util::getStream(path, stream);
    std::getline(stream, line);

    std::istringstream buffer(line);

    // this way of parsing can only use space as a delimiter
    std::istream_iterator<std::string> bufBegin(buffer);
    std::istream_iterator<std::string> bufEnd;

    // use range constructor of vector to perform the split
    std::vector<std::string> values(bufBegin,bufEnd);

    int sysTime = stoi(values[0]);
    
    return sysTime;
}

std::string ProcessParser::getProcUpTime(std::string pid)
{
    std::string line;
    std::string procUpTime;
    std::string path = Path::basePath() + pid + "/" + Path::statPath();

    std::ifstream stream;

    try {
    Util::getStream(path, stream);
    std::getline(stream, line);

    std::istringstream buffer(line);

    // this way of parsing can only use space as a delimiter
    std::istream_iterator<std::string> bufBegin(buffer);
    std::istream_iterator<std::string> bufEnd;

    // use range constructor of vector to perform the split
    std::vector<std::string> values(bufBegin,bufEnd);

    procUpTime = values[13]; // 13
    float freq = sysconf(_SC_CLK_TCK); // returns number of clock ticks per second
    float value = stof(procUpTime)/freq;
    procUpTime = to_string(value);
    } catch (...) {}
    return procUpTime;
}

std::string ProcessParser::getProcUser(std::string pid)
{
    // get user ID from status file
    // use it to go to "etc/passwd" file
    // the delimiter in passwd is ":"
    // the first item is the user name 0
    // the Uid is in the third position 2.

    std::string line;
    std::string name = "Uid:";
    std::string uid;
    std::string userName = "";
    std::string path = Path::basePath() + pid + Path::statusPath();

    std::ifstream stream;

    // get Uid from the first file
    try {
    Util::getStream(path, stream);

    while (std::getline(stream, line)){
        /* find the line that starts with Uid: */
        if (line.compare(0, name.size(), name) == 0) {
            // this is the right line; now process it
            std::istringstream buffer(line);

            // this way of parsing can only use space as a delimiter
            std::istream_iterator<std::string> bufBegin(buffer);
            std::istream_iterator<std::string> bufEnd;

            // use range constructor of vector to perform the split
            std::vector<std::string> values(bufBegin, bufEnd);

            // grab the Uid
            uid = values[1]; // 1
            break;
        }
    }
    // reset variables
    stream.close();

    // now go to the /etc/passwd file
    path = "/etc/passwd";
    Util::getStream(path, stream);
    std::string uidstr("x:" + uid);
    // find the line with the matching Uid
    while (std::getline(stream, line)){
        /* find the line contains x:"Uid" */
        if (line.find(uidstr) != std::string::npos) {
            // this is the right line; now process it
            userName = line.substr(0, line.find(':'));
            break;
        }
    }
} catch (...) {}
    return userName;
}

std::vector<std::string> ProcessParser::getSysCpuPercent(std::string coreNumber)
{
    std::vector<std::string> values;
    std::string line;
    std::string name("cpu" + coreNumber);
    std::string path(Path::basePath() + Path::statPath());

    std::ifstream stream;
    Util::getStream(path, stream);

    while(std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begBuffer(buf);
            std::istream_iterator<std::string> endBuffer;
            std::vector<std::string> values(begBuffer, endBuffer);

            return values;
        }
    }

    return std::vector<std::string>();
}

float ProcessParser::getSysRamPercent()
{
    // lines that we are looking for 
    std::string name1("MemAvailable:");
    std::string name2("MemFree:");
    std::string name3("Buffers");

    std::string line;
    std::string path(Path::basePath() + Path::memInfoPath());
    std::ifstream stream;

    float totalMem = 0; // values[1]
    float freeMem = 0;  // values[1]
    float buffers = 0;  // values[1]

    Util::getStream(path, stream);

    int foundCount = 0;
    // get data from the file
    while (std::getline(stream, line))
    {
        if (line.compare(0, name1.size(), name1) == 0) {
            istringstream lineBuffer(line);
            istream_iterator<std::string> begBuffer(lineBuffer);
            istream_iterator<std::string> endBuffer;
            std::vector<std::string> values(begBuffer, endBuffer);
            totalMem = stof(values[1]);
            foundCount++;
        } else if (line.compare(0, name2.size(), name2) == 0) {
            istringstream lineBuffer(line);
            istream_iterator<std::string> begBuffer(lineBuffer);
            istream_iterator<std::string> endBuffer;
            std::vector<std::string> values(begBuffer, endBuffer);
            freeMem = stof(values[1]);
            foundCount++;
        } else if (line.compare(0, name3.size(), name3) == 0) {
            istringstream lineBuffer(line);
            istream_iterator<std::string> begBuffer(lineBuffer);
            istream_iterator<std::string> endBuffer;
            std::vector<std::string> values(begBuffer, endBuffer);
            buffers = stof(values[1]);
            foundCount++;
        }
        if (foundCount >= 3) {
            break;
        }
    }

    return float(100.0 * (1 - (freeMem/(totalMem - buffers))));
}

std::string ProcessParser::getSysKernelVersion()
{
    std::string line;
    std::string kernelVersion("");
    std::string name("Linux version");
    std::string path(Path::basePath() + Path::versionPath());

    std::ifstream stream;
    Util::getStream(path, stream);

    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            std::istringstream lineBuffer(line);
            istream_iterator<std::string> begBuffer(lineBuffer);
            istream_iterator<std::string> endBuffer;
            std::vector<std::string> values(begBuffer, endBuffer);
            kernelVersion = values[2];
            break;
        }
    }
    
    return kernelVersion;
}

int ProcessParser::getNumberOfCores()
{
    std::string line;
    std::string name = "cpu cores";
    std::string path(Path::basePath() + "cpuinfo");
    int numCores = 0;

    std::ifstream stream;

    // get Uid from the first file
    Util::getStream(path, stream);

    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(),name) == 0) {
            istringstream lineBuffer(line);
            istream_iterator<std::string> beginBuffer(lineBuffer);
            istream_iterator<std::string> endBuffer;

            std::vector<std::string> values(beginBuffer, endBuffer);
            numCores = stoi(values[3]);
        }
    }
    
    return numCores;
}

int ProcessParser::getTotalThreads()
{
    // get every process and read their number of threads
    int numThreads = 0; // position 1
    std::string path;
    std::string line;
    std::string name("Threads:");
    std::vector<std::string> pidList = getPidList();
    istream_iterator<std::string> endBuffer;


    for(std::string pid : pidList) {
        path = Path::basePath() + pid + Path::statusPath();
        std::ifstream stream;
        Util::getStream(path, stream);

        while (std::getline(stream, line)) {
            if (line.compare(0, name.size(), name) == 0) {
                istringstream lineBuffer(line);
                istream_iterator<std::string> beginBuffer(lineBuffer);
                std::vector<std::string> values(beginBuffer, endBuffer);
                numThreads += stoi(values[1]);
                break;
            }
        }
    }

    return numThreads;
}

int ProcessParser::getTotalNumberOfProcesses()
{
    int result = 0;
    std::string path(Path::basePath() + Path::statPath());
    std::string name("processes");
    std::string line;
    istream_iterator<std::string> endBuffer;

    std::ifstream stream;
    Util::getStream(path, stream);

       while (std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            istringstream lineBuffer(line);
            istream_iterator<std::string> beginBuffer(lineBuffer);
            std::vector<std::string> values(beginBuffer, endBuffer);
            result= stoi(values[1]);
            break;
        }
    }

    return result;
}

int ProcessParser::getNumberOfRunningProcesses()
{
    int result = 0;
    std::string path(Path::basePath() + Path::statPath());
    std::string name("procs_running");
    std::string line;
    istream_iterator<std::string> endBuffer;

    std::ifstream stream;
    Util::getStream(path, stream);

       while (std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            istringstream lineBuffer(line);
            istream_iterator<std::string> beginBuffer(lineBuffer);
            std::vector<std::string> values(beginBuffer, endBuffer);
            result= stoi(values[1]);
            break;
        }
    }

    return result;
}

std::string ProcessParser::getOSName()
{
    std::string line;
    std::string osName("");
    std::string name("PRETTY_NAME=");
    std::string path("/etc/os-release");

    std::ifstream stream;
    Util::getStream(path, stream);

    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(), name) == 0) {
            // get everything after = except ""
            auto pos1 = line.find("\"") +1;
            auto pos2 = line.find_last_of("\"");
            osName = line.substr(pos1, pos2-pos1);
            break;
        }
    }
    
    return osName;
}

std::string ProcessParser::PrintCpuStats(std::vector<std::string> values1,
 std::vector<std::string> values2)
 {
     std::string s;

    float activeTime = getSysActiveCpuTime(values2) - getSysActiveCpuTime(values1);
    float idleTime = getSysIdleCpuTime(values2) - getSysIdleCpuTime(values1);
    float totalTime = activeTime + idleTime;
    float result = 100.0 * (activeTime / totalTime);
    s = std::to_string(result);

     return s;
 }

bool ProcessParser::isPidExisting(std::string pid)
{
    DIR *dir;

    bool set = false;
    if (!(dir = opendir("/proc"))) {
        throw std::runtime_error(std::strerror(errno));
    }

    while (dirent* dirp = readdir(dir)) {
        // is this a directory
        if (dirp->d_type == DT_DIR) {
            // see if it is a match
            if (strcmp(dirp->d_name, pid.c_str()) == 0) {
                set = true;
                break;
            }
        }
    }
    
    if (closedir(dir)) {
        throw std::runtime_error(std::strerror(errno));
    }

    return set;
}