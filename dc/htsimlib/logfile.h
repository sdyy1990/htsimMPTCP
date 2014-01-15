#ifndef LOGFILE_H
#define LOGFILE_H

/*
 * Logfile is a class for specifying the log file format.
 * The loggers (loggers.h) face both
 *  1. the log file, using the base class Logger (defined here)
 *  2. the simulator, using the base classes in loggertypes.h
 */

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "config.h"
#include "network.h"
#include "eventlist.h"

class Logfile;

class Logger {
    friend class Logfile;
protected:
    void setLogfile(Logfile& logfile) {
        _logfile=&logfile;
    }
    Logfile* _logfile;
};

class Logfile {
public:
    Logfile(const string& filename, EventList& eventlist);
    ~Logfile();
    void setStartTime(simtime_picosec starttime);
    void write(const string& msg);
    void writeName(Logged& logged);
    void writeRecord(uint32_t type, uint32_t id, uint32_t ev, double val1, double val2, double val3); // prepend uint64_t time
    void addLogger(Logger& logger);
    simtime_picosec _starttime;
    void shutdown();
private:
    EventList& _eventlist;
    vector<Logger*> _loggers;
    // managing the files for writing
    void transposeLog();
    stringstream _preamble;
    string _logfilename;
    FILE* _logfile;
    bool _startedTrace;
    long int _numRecords;
};

#endif
