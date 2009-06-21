#include <agar/core.h>
#include <agar/gui.h>
#include "gkey.h"
#define Malloc malloc
#define Free free

struct ag_global_key {
    SDLKey keysym;
    SDLMod keymod;
    void (*fn)(void);
    void (*fn_ev)(AG_Event *);
    AG_SLIST_ENTRY(ag_global_key) gkeys;
};
static AG_SLIST_HEAD(,ag_global_key) agGlobalKeys =
    AG_SLIST_HEAD_INITIALIZER(&agGlobalKeys);

static AG_Mutex agGlobalKeysLock;

static int initedGlobals = 0;

void
GK_Init(void)
{
    if (initedGlobals) {
	return;
    }
    initedGlobals = 1;

    AG_MutexInitRecursive(&agGlobalKeysLock);
}

void
GK_BindGlobalKey(SDLKey keysym, SDLMod keymod, void (*fn)(void))
{
    struct ag_global_key *gk;

    gk = Malloc(sizeof(struct ag_global_key));
    gk->keysym = keysym;
    gk->keymod = keymod;
    gk->fn = fn;
    gk->fn_ev = NULL;

    AG_MutexLock(&agGlobalKeysLock);
    AG_SLIST_INSERT_HEAD(&agGlobalKeys, gk, gkeys);
    AG_MutexUnlock(&agGlobalKeysLock);
}

void
GK_BindGlobalKeyEv(SDLKey keysym, SDLMod keymod, void (*fn_ev)(AG_Event *))
{
    struct ag_global_key *gk;

    gk = Malloc(sizeof(struct ag_global_key));
    gk->keysym = keysym;
    gk->keymod = keymod;
    gk->fn = NULL;
    gk->fn_ev = fn_ev;
	
    AG_MutexLock(&agGlobalKeysLock);
    AG_SLIST_INSERT_HEAD(&agGlobalKeys, gk, gkeys);
    AG_MutexUnlock(&agGlobalKeysLock);
}

int
GK_UnbindGlobalKey(SDLKey keysym, SDLMod keymod)
{
    struct ag_global_key *gk;

    AG_MutexLock(&agGlobalKeysLock);
    AG_SLIST_FOREACH(gk, &agGlobalKeys, gkeys) {
	if (gk->keysym == keysym && gk->keymod == keymod) {
	    AG_SLIST_REMOVE(&agGlobalKeys, gk, ag_global_key, gkeys);
	    AG_MutexUnlock(&agGlobalKeysLock);
	    Free(gk);
	    return (0);
	}
    }
    AG_MutexUnlock(&agGlobalKeysLock);
    AG_SetError("No such key binding");
    return (-1);
}

void
GK_ClearGlobalKeys(void)
{
    struct ag_global_key *gk, *gkNext;

    AG_MutexLock(&agGlobalKeysLock);
    for (gk = AG_SLIST_FIRST(&agGlobalKeys);
	 gk != AG_SLIST_END(&agGlobalKeys);
	 gk = gkNext) {
	gkNext = AG_SLIST_NEXT(gk, gkeys);
	Free(gk);
    }
    AG_SLIST_INIT(&agGlobalKeys);
    AG_MutexUnlock(&agGlobalKeysLock);
}

int
GK_ProcessGlobalKey(SDL_Event *ev)
{
    struct ag_global_key *gk;
    int is_gk = 0;
    AG_MutexLock(&agGlobalKeysLock);
    AG_SLIST_FOREACH(gk, &agGlobalKeys, gkeys) {
	if (gk->keysym == ev->key.keysym.sym &&
	    (gk->keymod == KMOD_NONE ||
	     ev->key.keysym.mod & gk->keymod)) {
	    if (gk->fn != NULL) {
		gk->fn();
	    } else if (gk->fn_ev != NULL) {
		gk->fn_ev(NULL);
	    }
	    is_gk = 1;
	}
    }
    AG_MutexUnlock(&agGlobalKeysLock);
    return is_gk;
}
