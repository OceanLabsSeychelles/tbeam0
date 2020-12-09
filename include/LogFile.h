#ifndef TBEAM0_LOGFILE_H
#define TBEAM0_LOGFILE_H

#include "LITTLEFS.h"
#define SPIFFS LITTLEFS

class LogFile{
public:
    String path;
    bool first_write = true;
    LogFile(const String &_path){
        path = _path;
        if(SPIFFS.exists(path)){
            SPIFFS.remove(path);
        }
    }
    void log(const String &line){
        if (first_write){
            first_write = false;
            File file = SPIFFS.open(path, FILE_WRITE);
            file.println(line);
            file.close();
        }
    }
};



#endif //TBEAM0_LOGFILE_H