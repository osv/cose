// Copyright (C) 2010-2011 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// geekconsole is free software; you can redistribute it and/or modify
// it  under the terms  of the  GNU Lesser  General Public  License as
// published by the Free Software  Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// Alternatively, you  can redistribute it and/or modify  it under the
// terms of  the GNU General Public  License as published  by the Free
// Software Foundation; either  version 2 of the License,  or (at your
// option) any later version.
//
// geekconsole is distributed in the  hope that it will be useful, but
// WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
// MERCHANTABILITY or  FITNESS FOR A  PARTICULAR PURPOSE. See  the GNU
// Lesser General Public License or the GNU General Public License for
// more details.
//
// You should  have received a copy  of the GNU  Lesser General Public
// License. If not, see <http://www.gnu.org/licenses/>.

#ifndef _GVAR_H_
#define _GVAR_H_

#include <string>
#include <map>
#include <vector>
#include <celutil/basictypes.h>

/*
  GeekVar manager of  variables that are used to  hold scalar, string,
  flag, enum color or celestia body  that can be binded (only once) in
  C++ source for fast access or used by set/get.

  For enum and flag you need null terminated table of flags32_s struc.

  
*/
class GeekVar {
public:
    enum gvar_type {
        Int32,
        Bool,
        Double,
        Float,
        Int64,
        String,
        Flags32,
        Enum32,
        Color, // uint32 color
        Celbody, // same as string - celestia body name
        Unknown,
    };

    /* Flag name and his mask
       Used for flags and enum types of variable
       Enum type require default mask with 0 value

       Name of flag must _NOT_ start from digit
     */
    struct flags32_s {
        const char *name;
        uint32 mask;
        const char *helptip;
    };

    typedef void (* HookFunc)(std::string);

    typedef struct gvar {
        void *p; // pointer to binded var
        gvar_type type;
        std::string stringVal; // not binded value
        std::string lastVal; // last set value
        std::string resetString; // reset string
        std::string doc; // documentation of this var
        std::string flagDelim; // delimiters for flag
        flags32_s *flagtbl;
        bool saveAsHex; // for intager save in hex format
        HookFunc getHook;
        HookFunc setHook;
        ~gvar();
        gvar(): p(0), type(Unknown), flagtbl(0), saveAsHex(false), getHook(NULL), setHook(NULL) {};
        /* other helpfull constr.*/
        gvar(int32 *v, const std::string &resetstr, const std::string &_doc): p(v), type(Int32),
                                                                              resetString(resetstr), doc(_doc), flagtbl(0), saveAsHex(false),
                                                                              getHook(NULL), setHook(NULL){};
        gvar(bool *v, const std::string &resetstr, const std::string &_doc): p(v), type(Bool),
                                                                             resetString(resetstr), doc(_doc), flagtbl(0), saveAsHex(false),
                                                                             getHook(NULL), setHook(NULL){};
        gvar(double *v, const std::string &resetstr, const std::string &_doc): p(v), type(Double),
                                                                               resetString(resetstr), doc(_doc), flagtbl(0), saveAsHex(false),
                                                                               getHook(NULL), setHook(NULL) {};
        gvar(float *v, const std::string &resetstr, const std::string &_doc): p(v), type(Float),
                                                                              resetString(resetstr), doc(_doc), flagtbl(0), saveAsHex(false),
                                                                              getHook(NULL), setHook(NULL){};
        gvar(int64 *v, const std::string &resetstr, const std::string &_doc): p(v), type(Int64),
                                                                              resetString(resetstr), doc(_doc), flagtbl(0), saveAsHex(false),
                                                                              getHook(NULL), setHook(NULL){};
        gvar(std::string *v, const std::string &resetstr, const std::string &_doc): p(v), type(String),
                                                                                    resetString(resetstr), doc(_doc), flagtbl(0), saveAsHex(false),
                                                                                    getHook(NULL), setHook(NULL){};
        gvar(flags32_s *_flagtbl, const std::string &_flagDelim, uint32 *v, const std::string &resetstr, const std::string &_doc):
            p(v), type(Flags32), resetString(resetstr), doc(_doc), flagDelim(_flagDelim), flagtbl(_flagtbl), saveAsHex(false),
            getHook(NULL), setHook(NULL){};
        gvar(flags32_s *_flagtbl, uint32 *v, const std::string &resetstr, const std::string &_doc):
            p(v), type(Enum32), resetString(resetstr), doc(_doc), flagtbl(_flagtbl), saveAsHex(false),
            getHook(NULL), setHook(NULL){};

        // new var but not binded
        gvar(gvar_type _type, const std::string &resetstr, const std::string &_doc): p(0), type(_type),
                                                                                     resetString(resetstr), doc(_doc),
                                                                                     flagtbl(0), saveAsHex(false),
                                                                                     getHook(NULL), setHook(NULL){};
        void reset();
        std::string get();
        void set(const std::string &val);
    };

    GeekVar ();

    // Unbind var that will become to Unknown.
    // Use it in your deconst if you bind.
    void Unbind(std::string name);

    /* ==============================================================
       Bind
       return false if already binded or created by New*
       ==============================================================
    */

    // bind and save in hex format
    bool BindHex(std::string name, int32 *var,
                 const int32 resetval, std::string doc = "");
    bool BindHex(std::string name, int64 *var,
                 const int64 resetval, std::string doc = "");
    //binds
    bool Bind(std::string name, int32 *var,
              const int32 resetval, std::string doc = "");
    bool Bind(std::string name, int64 *var,
              const int64 resetval, std::string doc = "");
    bool Bind(std::string name, bool *var,
              const bool resetval, std::string doc = "");
    bool Bind(std::string name, double *var,
              const double resetval, std::string doc = "");
    bool Bind(std::string name, float *var,
              const float resetval, std::string doc = "");
    bool BindFlag(std::string name, flags32_s *flagtbl, std::string delim, uint32 *var,
                  const uint32 resetval, std::string doc = "");
    bool BindEnum(std::string name, flags32_s *flagtbl, uint32 *var,
                  const uint32 resetval, std::string doc = "");
    bool BindColor(std::string name, uint32 *color,
                  const uint32 resetval, std::string doc = "");

    // binds with string reset
    bool Bind(std::string name, int32 *var,
              const char *resetval, std::string doc = "");
    bool Bind(std::string name, int64 *var,
              const char *resetval, std::string doc = "");
    bool Bind(std::string name, bool *var,
              const char *resetval, std::string doc = "");
    bool Bind(std::string name, float *var,
              const char *resetval, std::string doc = "");
    bool Bind(std::string name, double *var,
              const char *resetval, std::string doc = "");
    bool BindFlag(std::string name, flags32_s *flagtbl, std::string delim, uint32 *var,
                  const char *resetval, std::string doc = "");
    bool BindEnum(std::string name, flags32_s *flagtbl, uint32 *var,
                  const char *resetval, std::string doc = "");
    bool BindColor(std::string name, uint32 *color,
                  const char *resetval, std::string doc = "");

    // string bind
    bool Bind(std::string name, std::string *var,
              const char *resetval, std::string doc = "");
    // string bind, celestia body
    bool BindCelBodyName(std::string name, std::string *var,
              const char *resetval, std::string doc = "");

    /* ==============================================================
       New var
       ==============================================================
    */

    bool NewHex(std::string name, const int32 resetval, std::string doc);
    bool NewHex(std::string name, const int64 resetval, std::string doc);

    bool New(std::string name, const int32 resetval, std::string doc = "");
    bool New(std::string name, const int64 resetval, std::string doc = "");
    bool New(std::string name, const bool resetval, std::string doc = "");
    bool New(std::string name, const double resetval, std::string doc = "");
    bool New(std::string name, const float resetval, std::string doc = "");
    bool NewFlag(std::string name, flags32_s *flagtbl, std::string delim,
                 const uint32 resetval, std::string doc = "");
    bool NewEnum(std::string name, flags32_s *flagtbl,
                 const uint32 resetval, std::string doc = "");
    bool NewColor(std::string name,
                  const uint32 resetval, std::string doc = "");

    // new var with string reset
    bool New(std::string name, gvar_type type, const char *resetval,
             std::string doc = "");
    bool NewFlag(std::string name, flags32_s *flagtbl, std::string delim,
                 const char *resetval, std::string doc = "");
    bool NewEnum(std::string name, flags32_s *flagtbl,
                 const char *resetval, std::string doc = "");
    bool NewColor(std::string name,
                  const char *resetval, std::string doc = "");
    bool NewCelBodyName(std::string name,
                        const char *resetval, std::string doc = "");

    /* ==============================================================
       Set
       ==============================================================
    */
    // set string (or flag list if var's type is flag)
    void Set(std::string name, const char *val);
    void Set(std::string name, std::string val);
    // other
    void Set(std::string name, int32 val);
    void Set(std::string name, int64 val);
    void Set(std::string name, bool val);
    void Set(std::string name, double val);
    void Set(std::string name, float val);

    /* ==============================================================
       Get
       ==============================================================
    */

    int32 GetI32(std::string name);
    /* Return as  uint32. Instead of  other Get* work with  not binded
       vars that have flag tbl*/
    uint32 GetUI32(std::string name);
    int64 GetI64(std::string name);
    bool GetBool(std::string name);
    double GetDouble(std::string name);
    float GetFloat(std::string name);
    std::string GetString(std::string name);

    /* ==============================================================
       Misc
       ==============================================================
    */

    void Reset(std::string name);
    std::string GetResetValue(std::string name);
    // return flags with delimiters or string enum or color name if var is color
    std::string GetFlagString(std::string name);
    // flags, enum or color name of reset value
    std::string GetFlagResetString(std::string name);
    // return last set value
    std::string GetLastValue(std::string name);
    // flags, enum or color name of last set value
    std::string GetFlagLastString(std::string name);
    gvar_type GetType(std::string name);
    std::string GetTypeName(gvar_type type);
    // get flag table
    flags32_s *GetFlagTbl(std::string name);
    // get flag's string delimiters
    std::string GetFlagDelim(std::string name);
    // get documentation
    std::string GetDoc(std::string name);
    // get var names with path
    std::vector<std::string> GetVarNames(std::string path);
    // copy all vars from one group to other
    void CopyVars(std::string srcgroup, std::string dstgroup);
    // copy types of variables. Only copy types, doc, etc, but *not* value
    void CopyVarsTypes(std::string srcgroup, std::string dstgroup);

    // return bind status for variable
    bool IsBinded(std::string name);

    // set gethook for var
    void SetGetHook(std::string name, HookFunc hook);
    void SetSetHook(std::string name, HookFunc hook);
    // hook for creation variable that has been matched @groupname
    void AddCreateHook(std::string groupname, HookFunc hook);

private:
    gvar *getGvar(std::string name);
    typedef std::map<std::string, gvar> vars_t;
    vars_t  vars;

    bool lock_set_get; // prevent infin. set/get in hook
    typedef std::multimap<std::string, HookFunc> createHook_t;
    createHook_t createHooks;
    void callCreateHook(std::string var);
};

extern GeekVar gVar;
extern void initGCVarInteractivsFunctions();

#endif // _GVAR_H_
