#include "samplemanager.h"
#include "snd_local.h"

#include <celutil/debug.h>

#include <iostream>
#include <fstream>

using namespace std;

static SampleManager* sampleManager = NULL;

SampleManager* GetSampleManager()
{
    if (sampleManager == NULL)
        sampleManager = new SampleManager("snd");
    return sampleManager;
}

string SamplerInfo::resolve(const string& baseDir)
{
    if (!path.empty())
    {
        string filename = path + "/snd/" + source;
        ifstream in(filename.c_str());
        if (in.good())
            return filename;
    }

    return baseDir + "/" + source;
}

clunk::Sample* SamplerInfo::load(const string& filename)
{
    DPRINTF(1, "Loading sound: %s\n", filename.c_str());
    clunk::Sample *sample = context->create_sample();

    try {
        sample->load(filename);
    } catch (const std::exception &_e){
        cout << _e.what() << "\n";
        delete sample;
        sample = NULL;
    }
    return sample;
}
