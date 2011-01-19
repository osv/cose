// Copyright (C) 2010-2011 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include <string>
#include <celutil/basictypes.h>

/*
  Simple wrapper to libconfig for binding poiters of int32(64), float,
  bool, double, flag (uint32), with reset(default) value.

  You can load config file in any time and binded variables will be setupped.
*/

// save load base config file
extern void saveCfg();
extern void loadCfg();

// Load and save vars from/to file with
// specified name space (variable must match with namespace at begin)
extern void loadCfg(const std::string &filename, const char *namesp);
extern void saveCfg(const std::string &filename, const char *namesp);

// free all variables (mem clr)
extern void freeCfg();

// flag name and his mask struct
struct i32flags {
    const char *name;
    uint32 mask;
};

// bind and save in hex format
extern void cVarBindInt32Hex(std::string name, int32 *var, const int32 resetval);
extern void cVarBindInt64Hex(std::string name, int64 *var, const int64 resetval);

//binds
extern void cVarBindInt32(std::string name, int32 *var, const int32 resetval);
extern void cVarBindInt64(std::string name, int64 *var, const int64 resetval);
extern void cVarBindBool(std::string name, bool *var, const bool resetval);
extern void cVarBindFloat(std::string name, double *var, const double resetval);
extern void cVarBindFlags(std::string name, i32flags *flagtbl,
                          uint32 *var, const uint32 resetval);

// binds with string reset
extern void cVarBindInt32S(std::string name, int32 *var, const char *resetval);
extern void cVarBindInt64S(std::string name, int64 *var, const char *resetval);
extern void cVarBindBoolS(std::string name, bool *var, const char *resetval);
extern void cVarBindFloatS(std::string name, double *var, const char *resetval);
extern void cVarBindFlagsS(std::string name, i32flags *flagtbl,
                           uint32 *var, const char *resetval);
// string bind
extern void cVarBindStringS(std::string name, std::string *var, const char *resetval);

extern int32 cVarGetInt32(std::string name);
extern int64 cVarGetInt64(std::string name);
extern bool cVarGetBool(std::string name);
extern double cVarGetFloat(std::string name);
extern std::string cVarGetString(std::string name);

extern void cVarReset(std::string name);
#endif // _CONFIGFILE_H_
