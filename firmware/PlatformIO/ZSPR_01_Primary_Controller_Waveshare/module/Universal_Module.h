#pragma once

#include <cstring>
#include <cstdlib>
#include <math.h>

enum return_codes_t
{
    SUCCESS = 0,
    ERROR = 1,
    TIMEOUT = 2,
    INVALID_PARAM = 3,
    NOT_IMPLEMENTED = 4,
    NOT_SUPPORTED = 5
};
    
class Universal_Module
{
public:
    Universal_Module() {}
    // virtual ~Universal_Module() {}
    virtual return_codes_t setup() { return SUCCESS; }
    virtual return_codes_t update() { return SUCCESS; }
};