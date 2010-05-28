// Copyright (C) 2010 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SND_LOCAL_H_
#define _SND_LOCAL_H_

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif
#endif /* _WIN32 */

#include <context.h>
#include "samplemanager.h"

// clunk context
extern clunk::Context *context;

#endif // _SND_LOCAL_H_
