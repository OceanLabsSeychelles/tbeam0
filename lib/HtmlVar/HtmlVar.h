//
// Created by brett on 10/30/2020.
//
#include <SPIFFS.h>
#include <ESPFlash.h>
#include <ESPFlashString.h>

#ifndef TBEAM0_HTMLVAR_H
#define TBEAM0_HTMLVAR_H

class HtmlVar{
public:
    HtmlVar(const String &_varname, const String &_value, const String &_type);
    void flashWrite(const String &_value);
    void flashRead();
    void setLimits(const String &_min,const String &_max);
    void checkExists();
    String name;
    String value;
    String filename;
    String type;
    bool minmax = false;
    bool persistent = false;
    String min = "0";
    String max = "0";
};

HtmlVar::HtmlVar(const String &_varname, const String &_value, const String &_type){
    name=_varname;
    filename ="/"+_varname;
    type= _type;
    value= _value;
}

void HtmlVar::flashWrite(const String &_value) {
    if (persistent) {
        ESPFlashString flashString(filename.c_str());
        flashString.set(_value);
        value = _value;
    }
}

void HtmlVar::flashRead() {
    if (persistent) {
        ESPFlashString flashString(filename.c_str());
        value = flashString.get();
    }
}

void HtmlVar::setLimits(const String &_min,const String &_max){
    minmax = true;
    min = _min;
    max = _max;
}

void HtmlVar::checkExists() {
    if (persistent) {
        if (!SPIFFS.exists(filename)) {
            Serial.print(filename);
            Serial.println(" not found!");
            ESPFlashString flashManager(filename.c_str());
            flashManager.set(value);
        }else{
            Serial.print(filename);
            Serial.println(" found!");
            flashRead();
        }
    }
}

#endif //TBEAM0_HTMLVAR_H
