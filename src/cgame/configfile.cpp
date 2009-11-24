#include "../celestia/celestiacore.h"
#include "configfile.h"
#include <libconfig.h>
#include <stdint.h>

// for some version of inttypes.h need to define it for macros PRIi32...
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

const char *mainConfigFile = "openspace.cfg";

/** Variable contaner

    Contain pointer to binded variable. When created from config file, and not
    been binded, have allocated pointer with spec. type that will be *free* when
    variable binded.

    Reset value is string for easy cast to other types.

    Flag type is table of struct of strings and masks (i32flags). All array
    of string loaded from config file will be stored into strArray
    for future setup loaded value when binded with flagtable (when we have
    info about mask).
 */
typedef struct cvar {
    enum cvar_type {
        Int32,
        Bool,
        Float,
        Int64,
        String,
        Flags
    };
    void *p;
    string resetp; // reset string
    cvar_type type;
    string name;
    i32flags *flagtbl;
    bool saveHex; // for intager save in hex format
    bool loaded; //is loaded from config file
    // when loaded from cfg and settings is array of str, copy of array is here
    vector <string> strArray;
    cvar(string _name, int32 *v): p(v), resetp(""), type(Int32), name(_name), flagtbl(0), saveHex(false), loaded(false) {};
    cvar(string _name, bool *v): p(v), resetp(""), type(Bool), name(_name), flagtbl(0), saveHex(false), loaded(false) {};
    cvar(string _name, double *v): p(v), resetp(""), type(Float), name(_name), flagtbl(0), saveHex(false), loaded(false) {};
    cvar(string _name, int64 *v): p(v), resetp(""), type(Int64), name(_name), flagtbl(0), saveHex(false), loaded(false) {};
    cvar(string _name, string *v): p(v), resetp(""), type(String), name(_name), flagtbl(0), saveHex(false), loaded(false) {};
    cvar(string _name, i32flags *_flagtbl, uint32 *v):
        p(v), type(Flags), name(_name), flagtbl(_flagtbl), saveHex(false), loaded(false) {};
    ~cvar()
        {
            freeVal();
        }
    void freeVal()
        {
            if (!loaded)
                return;
            flagtbl = NULL;
            switch(type)
            {
            case Int32:
                delete (int32 *) p;
                break;
            case Bool:
                delete (bool *) p;
                break;
            case Float:
                delete (float *) p;
                break;
            case Int64:
                delete (int64 *) p;
                break;
            case String:
                delete (string *) p;
                break;
            case Flags:
                delete (uint32 *) p;
                break;
            }
        }
    void reset()
        {
            set(resetp);
        }

    string get() {
        char buffer [64];
        switch(type)
        {
        case Int32:
            sprintf(buffer, "%" PRIi32, *(int32 *)p);
            break;
        case Bool:
        {
            int32 v = *(bool *) p;
            sprintf(buffer, "%" PRIi32, v);
            break;
        }
        case Float:
            snprintf(buffer, sizeof(buffer) - 3, "%.*g", 10, *(double *)p);
            break;
        case Int64:
            sprintf(buffer, "%" PRIi64, *(int64 *) p);
            break;
        case String:
            return *(string *) p;
        case Flags:
            sprintf(buffer, "%" PRIu32, *(int32 *) p);
            break;
        }
        return string(buffer);
    }

    void set(const string &val) {
        switch(type)
        {
        case Flags:
        {
            // first try to reset by int valuse in reset value
            *(uint32 *) p = atoi(val.c_str());
            // if have flag table try to set flags from reset value
            if (flagtbl)
            {
                vector <string> resetFlags;
                uint cutAt;
                string str = val;
                while( (cutAt = str.find_first_of(" ,-+|")) != str.npos )
                {
                    if(cutAt > 0)
                    {
                        resetFlags.push_back(str.substr(0, cutAt));
                    }
                    str = str.substr(cutAt+1);
                }
                resetFlags.push_back(str);
                // need init var with table's vars
                std::vector<string>::iterator it;
                for (it = resetFlags.begin();
                     it != resetFlags.end(); it++)
                {
                    int j = 0;
                    while(flagtbl[j].name != NULL)
                    {
                        string s = *it;
                        if(s == flagtbl[j].name )
                        {
                            *(uint32 *) p |= flagtbl[j].mask;
                            break;
                        }
                        j++;
                    }
                }
            }
            break;
        }
        case Int32:
            *(int32 *) p = atoi(val.c_str());
            break;
        case Bool:
            if (val == "true")
                *(bool *) p = true;
            else
                *(bool *) p = (bool) atoi(val.c_str());
            break;
        case Float:
            *(double *) p = atof(val.c_str());
            break;
        case String:
            *(string *) p = val;
            break;
        case Int64:
            sscanf(val.c_str(), "%" PRIi64, (int64 *)p);
            break;
        }
    }
};

vector <cvar*> variables;

void freeCfg()
{
    std::vector<cvar*>::iterator it;
    for (it = variables.begin();
         it != variables.end(); it++)
    {
        (*it)->freeVal();
         delete *it;
    }
}

cvar *getCVar(string name)
{
    std::vector<cvar*>::iterator it;
    for (it = variables.begin();
         it != variables.end(); it++)
    {
        if((*it)->name == name)
            return (*it);
    }
    return NULL;
}

#define DECLARE_CVARBIND(FunType, VarType)                              \
    void cVarBind##FunType##S(string name, VarType *var, const char *resetval) \
    {                                                                   \
        if (!var)                                                       \
            return;                                                     \
        cvar *v = getCVar(name);                                        \
        if(!v)                                                          \
        {                                                               \
            v = new cvar(name, var);                                    \
            variables.push_back(v);                                     \
            v->resetp = resetval;                                       \
            /* reset with default value,  */                            \
            /* because not defined in cfg */                            \
            v->reset();                                                 \
        }                                                               \
        else                                                            \
        {                                                               \
            string loadedval;                                           \
            if (v->loaded)                                              \
            {                                                           \
                /* if loaded from configfile */                         \
                /* we must copy var's value if typea equal */           \
                loadedval = v->get();                                   \
                /* free old loaded var */                               \
                v->freeVal();                                           \
            }                                                           \
            v->type = cvar::FunType;                                    \
            v->p = var;                                                 \
            if (v->loaded)                                              \
                v->set(loadedval);                                      \
            v->loaded = false;                                          \
            v->flagtbl = NULL;                                          \
            v->resetp = resetval;                                       \
            v->saveHex = false;                                         \
        }                                                               \
    }

DECLARE_CVARBIND(Int32, int32)
DECLARE_CVARBIND(Float, double)
DECLARE_CVARBIND(Int64, int64)
DECLARE_CVARBIND(Bool, bool)
DECLARE_CVARBIND(String, string)

void cVarBindInt32Hex(string name, int32 *var, const int32 resetval)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, resetval);
    cVarBindInt32S(name, var, buffer);
    cvar *v = getCVar(name);
    if (v)
    {
        v->saveHex = true;;
    }
}

void cVarBindInt32(string name, int32 *var, const int32 resetval)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, resetval);
    cVarBindInt32S(name, var, buffer);
}


void cVarBindFloat(string name, double *var, const double resetval)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer) - 3, "%.*g", 10, resetval);
    cVarBindFloatS(name, var, buffer);
}

void cVarBindInt64Hex(string name, int64 *var, const int64 resetval)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi64, resetval);
    cVarBindInt64S(name, var, buffer);
    cvar *v = getCVar(name);
    if (v)
    {
        v->saveHex = true;;
    }
}

void cVarBindInt64(string name, int64 *var, const int64 resetval)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi64, resetval);
    cVarBindInt64S(name, var, buffer);
}

void cVarBindBool(string name, bool *var, const bool resetval)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, (int32) resetval);
    cVarBindBoolS(name, var, buffer);
}

void cVarBindFlags(string name, i32flags *flagtbl, uint32 *var, const uint32 resetval)
{
    char buffer[64];
    sprintf(buffer, "%" PRIu32, resetval);
    cVarBindFlagsS(name, flagtbl, var, buffer);
}

void cVarBindFlagsS(string name, i32flags *flagtbl, uint32 *var, const char *resetval)
{
    if (!var)
        return;
    cvar *v = getCVar(name);
    if (!v)
    {
        v = new cvar(name, flagtbl, var);
        variables.push_back(v);
        v->resetp = resetval;
        // reset with default value,
        //because not defined in cfg
        v->reset();
    }
    else
    {
        // flags table must be loaded before before
        if (v->loaded)
        {
            /* if loaded from configfile */
            /* we must copy var's value if typea equal */
            *var = atoi(v->get().c_str());
            if (flagtbl)
            {
                // need init var with table's vars
                std::vector<string>::iterator it;
                for (it = v->strArray.begin();
                     it != v->strArray.end(); it++)
                {
                    int j = 0;
                    while(flagtbl[j].name != NULL)
                    {
                        string s = *it;
                        if(s == flagtbl[j].name )
                        {
                            *var |= flagtbl[j].mask;
                            break;
                        }
                        j++;
                    }
                }
            }
            v->freeVal(); // free loaded old var
        } // loaded
        v->flagtbl = flagtbl;
        v->type = cvar::Flags;
        v->p = var;
        v->loaded = false;
        v->resetp = resetval;
        v->saveHex = false;
    }
}

int64 cVarGetInt64(string name)
{
    cvar *v = getCVar(name);
    if(v)
    {
        int64 r;
        sscanf(v->get().c_str(), "%" PRIi64, &r);
        return r;
    }
    else
        return 0;
}

double cVarGetFloat(string name)
{
    cvar *v = getCVar(name);
    if(v)
        return atof(v->get().c_str());
    else
        return 0;
}

int32 cVarGetInt32(string name)
{
    cvar *v = getCVar(name);
    if(v)
        return atoi(v->get().c_str());
    else
        return 0;
}

bool cVarGetBool(string name)
{
    return (bool) cVarGetInt32(name);
}

string cVarGetString(string name)
{
    cvar *v = getCVar(name);
    if(v)
        return v->get();
    else
        return "";
}
void cVarReset(string name)
{
    cvar *v = getCVar(name);
    if (v)
    {
        v->reset();
    }
}

void loadRecursive(config_t *cfg, config_setting_t *setting, string name, const char *validnamesp)
{
    if (!name.empty())
        name = name + '.';
    for (int i = 0; i < config_setting_length(setting); i++)
    {
        config_setting_t *set = config_setting_get_elem(setting, i);
        string setname = name + config_setting_name(set);

        cvar *v = NULL;
        switch (config_setting_type(set))
        {
        case CONFIG_TYPE_GROUP:
            loadRecursive(cfg, set, setname, validnamesp);
            break;
        case CONFIG_TYPE_INT:
        {
            if (setname.find(validnamesp) != 0) break;
            int32 *p = new int32;
            *p = (int32) config_setting_get_int(set);
            v = new cvar(setname, p);
            break;
        }
        case CONFIG_TYPE_INT64:
        {
            if (setname.find(validnamesp) != 0) break;
            int64 *p = new int64;
            *p = (int64) config_setting_get_int64(set);
            v = new cvar(setname, p);
            break;
        }
        case CONFIG_TYPE_FLOAT:
        {
            if (setname.find(validnamesp) != 0) break;
            double *p = new double;
            *p = (double) config_setting_get_float(set);
            v = new cvar(setname, p);
            break;
        }
        case CONFIG_TYPE_BOOL:
        {
            if (setname.find(validnamesp) != 0) break;
            bool *p = new bool;
            *p = (bool) config_setting_get_int(set);
            v = new cvar(setname, p);
            break;
        }
        case CONFIG_TYPE_STRING:
        {
            if (setname.find(validnamesp) != 0) break;
            string *p = new string(config_setting_get_string(set));
            v = new cvar (setname, p);
            break;
        }
        case CONFIG_TYPE_ARRAY:
        {
            if (setname.find(validnamesp) != 0) break;
            uint32 *p = new uint32;
            *p = 0;
            v = new cvar(setname, NULL, p);
            for (int ari = 0; ari< config_setting_length(set); ari++)
            {
                string s(config_setting_get_string_elem(set, ari));
                v->strArray.push_back(s);
            }
            break;
        }
        default:
            v = NULL;
        }
        if (v)
        {
            cvar *oldv = getCVar(setname);
            if (!oldv) // if var not prev loaded from cfg
            {
                v->loaded = true;
                variables.push_back(v);
            }
            else
            {
                switch(oldv->type)
                {
                case cvar::Int32:
                {
                    *(int32 *) oldv->p = atoi(v->get().c_str());
                    break;
                }
                case cvar::Int64:
                {
                    int64 r;
                    sscanf(v->get().c_str(), "%" PRIi64, &r);
                    *(int64 *) oldv->p = r;
                    break;
                }
                case cvar::Bool:
                {
                    *(bool *) oldv->p = (bool) atoi(v->get().c_str());
                    break;
                }
                case cvar::String:
                {
                    *(string *) oldv->p = v->get();
                    break;
                }
                case cvar::Float:
                {
                    *(double *) oldv->p = (double)atof(v->get().c_str());
                    break;
                }
                case cvar::Flags:
                {
                    *(uint32 *) oldv->p = 0;
                    if (oldv->flagtbl)
                    {
                        std::vector<string>::iterator it;
                        for (it = v->strArray.begin();
                             it != v->strArray.end(); it++)
                        {
                            int j = 0;
                            while(oldv->flagtbl[j].name != NULL)
                            {
                                string s = *it;
                                if(s == oldv->flagtbl[j].name )
                                {
                                    *(uint32 *)oldv->p |= oldv->flagtbl[j].mask;
                                    break;
                                }
                                j++;
                            }
                        }
                    }
                    break;
                }
                } // switch
                // free unneed var
                v->freeVal();
                delete v;
            } // if oldv
        }
    }
}

void saveVariables(config_t *cfg, const char *validnamesp)
{
    std::vector<cvar*>::iterator it;
    for (it = variables.begin();
         it != variables.end(); it++)
    {
        config_setting_t *setting = config_root_setting(cfg);
        cvar *cfgVar = *it;

        // create recursive subgroups if have delimiter '.'
        string str = cfgVar->name;
        if (str.find(validnamesp) != 0) continue;

        uint cutAt;
        while( (cutAt = str.find_first_of(".")) != str.npos )
        {
            if(cutAt > 0)
            {
                config_setting_t *s = config_setting_get_member(setting, str.substr(0, cutAt).c_str());
                if (s == NULL) // if not exist yet
                    setting = config_setting_add(setting, str.substr(0, cutAt).c_str(),
                                                 CONFIG_TYPE_GROUP);
                else
                    setting = s;
            }
            str = str.substr(cutAt+1);
        }
        string settingname = str;

        if (setting)
        {
            switch (cfgVar->type) {
            case cvar::Int32:
            {
                setting = config_setting_add(setting, settingname.c_str(),
                                             CONFIG_TYPE_INT);
                if (setting)
                {
                    if (cfgVar->saveHex)
                        config_setting_set_format(setting, CONFIG_FORMAT_HEX);
                    config_setting_set_int(setting,
                                           *(int32 *)cfgVar->p);
                }
                break;
            }
            case cvar::Int64:
            {
                setting = config_setting_add(setting, settingname.c_str(),
                                             CONFIG_TYPE_INT);
                if (setting)
                {
                    if (cfgVar->saveHex)
                        config_setting_set_format(setting, CONFIG_FORMAT_HEX);
                    config_setting_set_int64(setting,
                                             *(int64 *)cfgVar->p);
                }
                break;
            }
            case cvar::Bool:
            {
                bool v = *(bool *)cfgVar->p;
                setting = config_setting_add(setting, settingname.c_str(),
                                             CONFIG_TYPE_BOOL);
                if (setting)
                    config_setting_set_bool(setting, v);
                break;
            }
            case cvar::Float:
            {
                double v = *(double *)cfgVar->p;
                setting = config_setting_add(setting, settingname.c_str(),
                                             CONFIG_TYPE_FLOAT);
                config_setting_set_float(setting, v);
                break;
            }
            case cvar::String:
            {
                setting = config_setting_add(setting, settingname.c_str(),
                                             CONFIG_TYPE_STRING);
                string p = *(string *)cfgVar->p;
                config_setting_set_string(setting, p.c_str());
                break;
            }
            case cvar::Flags:
            {
                i32flags *flags = cfgVar->flagtbl;

                setting = config_setting_add(setting, settingname.c_str(),
                                             CONFIG_TYPE_ARRAY);
                if (!flags)
                    break;
                if (setting)
                {
                    int j = 0;
                    while(flags[j].name != NULL)
                    {
                        if (*(uint32 *)cfgVar->p & flags[j].mask)
                        {
                            config_setting_set_string_elem(setting, -1,
                                                           flags[j].name);
                        }
                        j++;
                    }
                }
                break;
            }
            }
        } // if setting
    }
}

void loadCfg(const string &filename, const char *namesp)
{
    struct config_t cfg;
    config_init(&cfg);
    if (!config_read_file(&cfg, filename.c_str()))
    {
        cout << "file not found" << filename << "\n";
        return;
    }
    cout << "Loaded config file '" << filename << "'\n";
    loadRecursive(&cfg, config_root_setting(&cfg), "", namesp);

    /* Free the configuration */
    config_destroy(&cfg);
}

void loadCfg()
{
    loadCfg(mainConfigFile, "");

#ifdef DEBUG
    std::vector<cvar*>::iterator it;
    for (it = variables.begin();
         it != variables.end(); it++)
    {
        cvar *c = *it;
        cout << "dumping var: " << c->name << "\t" << c->get()  << "\n";
    }
#endif
}

void saveCfg(const string &filename, const char *namesp)
{
    cout << "Saving cfg with name space '" << namesp << "' to file '"
         << filename << "'\n";
    struct config_t cfg;
    config_init(&cfg);
    saveVariables(&cfg, namesp);

    config_write_file(&cfg, filename.c_str());

    config_destroy(&cfg);
}

void saveCfg()
{
    struct config_t cfg;
    config_init(&cfg);
    cout << "Saving cfg: " << mainConfigFile << "\n";
    saveVariables(&cfg, "");

    config_write_file(&cfg, mainConfigFile);

    /* Free the configuration */
    config_destroy(&cfg);
}
