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
