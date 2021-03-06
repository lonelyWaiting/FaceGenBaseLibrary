//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors: Andrew Beatty
//

#include "stdafx.h"
#include "FgTime.hpp"
#include "FgString.hpp"
#include "FgOut.hpp"
#include "FgStdString.hpp"

using namespace std;

std::string
fgDateTime(time_t rawTime)
{
    struct tm   *fmtTime = gmtime(&rawTime);
    ostringstream   oss;
    oss << fmtTime->tm_year+1900 << "."
        << setw(2) << setfill('0') 
        << fmtTime->tm_mon+1 << "."
        << setw(2) << setfill('0') 
        << fmtTime->tm_mday << " "
        << setw(2) << setfill('0') 
        << fmtTime->tm_hour << ":"
        << setw(2) << setfill('0') 
        << fmtTime->tm_min << ":"
        << setw(2) << setfill('0') 
        << fmtTime->tm_sec;
    return oss.str();
}

std::string
fgDateTime()
{
    time_t          rawTime;
    time(&rawTime);
    return fgDateTime(rawTime);
}

std::string
fgDate(time_t rawTime)
{
    struct tm   *fmtTime = gmtime(&rawTime);
    ostringstream   oss;
    oss << setw(2) << setfill('0') 
        << fmtTime->tm_year-100 << "."
        << setw(2) << setfill('0') 
        << fmtTime->tm_mon+1 << "."
        << setw(2) << setfill('0') 
        << fmtTime->tm_mday << " ";
    return oss.str();
}

std::ostream &
operator<<(std::ostream & os,const FgTimer & t)
{
    double      et = t.read();
    return os << "Elapsed time: " << fgToStringPrecision(et,4) << " s";
}

bool
fgTick()
{
    static FgTimer  timer;
    if (timer.read() > 1.0) {
        timer.start();
        return true;
    }
    return false;
}
