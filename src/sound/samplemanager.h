// Copyright (C) 2010 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SAMPLENAGAGER_H_
#define _SAMPLENAGAGER_H_

#include <celutil/resmanager.h>
#include <sample.h>

class SamplerInfo : public ResourceInfo<clunk::Sample>
{
 public:
    std::string source;
    std::string path;

    SamplerInfo(const std::string _source, const std::string _path = ""):
        source(_source),
        path(_path) {};

    virtual std::string resolve(const std::string&);
    virtual clunk::Sample* load(const std::string&);
};

inline bool operator<(const SamplerInfo& s0,
                      const SamplerInfo& s1)
{
    if (s0.source == s1.source)
        return s0.path < s1.path;
    else
        return s0.source < s1.source;
}

typedef ResourceManager<SamplerInfo> SampleManager;

extern SampleManager* GetSampleManager();


#endif // _SAMPLENAGAGER_H_
