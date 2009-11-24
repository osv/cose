#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

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
extern void loadCfg(const string &filename, const char *namesp);
extern void saveCfg(const string &filename, const char *namesp);

// free all variables (mem clr)
extern void freeCfg();

// flag name and his mask struct
struct i32flags {
    const char *name;
    uint32 mask;
};

// bind and save in hex format
extern void cVarBindInt32Hex(string name, int32 *var, const int32 resetval);
extern void cVarBindInt64Hex(string name, int64 *var, const int64 resetval);

//binds
extern void cVarBindInt32(string name, int32 *var, const int32 resetval);
extern void cVarBindInt64(string name, int64 *var, const int64 resetval);
extern void cVarBindBool(string name, bool *var, const bool resetval);
extern void cVarBindFloat(string name, double *var, const double resetval);
extern void cVarBindFlags(string name, i32flags *flagtbl,
                          uint32 *var, const uint32 resetval);

// binds with string reset
extern void cVarBindInt32S(string name, int32 *var, const char *resetval);
extern void cVarBindInt64S(string name, int64 *var, const char *resetval);
extern void cVarBindBoolS(string name, bool *var, const char *resetval);
extern void cVarBindFloatS(string name, double *var, const char *resetval);
extern void cVarBindFlagsS(string name, i32flags *flagtbl,
                           uint32 *var, const char *resetval);
// string bind
extern void cVarBindStringS(string name, string *var, const char *resetval);

extern int32 cVarGetInt32(string name);
extern int64 cVarGetInt64(string name);
extern bool cVarGetBool(string name);
extern double cVarGetFloat(string name);
extern string cVarGetString(string name);

extern void cVarReset(string name);
#endif // _CONFIGFILE_H_
