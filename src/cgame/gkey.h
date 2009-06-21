#ifndef _GKEY_H_
#define _GKEY_H_

/**
 *  We can't use standart AG_*GlobalKey* func, because we have own event processor,
 * so instead here are reimplemented func but named GK_*GlobalKey*.
 * 
 * You can use AG_*GlobalKey* but it will avail. for agar's windows only
 *
 * Use GK_Init() at begin.
 */

__BEGIN_DECLS
void GK_Init(void);
void GK_BindGlobalKey(SDLKey keysym, SDLMod keymod, void (*fn)(void));
void GK_BindGlobalKeyEv(SDLKey keysym, SDLMod keymod, void (*fn_ev)(AG_Event *));
int GK_UnbindGlobalKey(SDLKey keysym, SDLMod keymod);
void GK_ClearGlobalKeys(void);

/* return 0 if no global key */
int GK_ProcessGlobalKey(SDL_Event *ev);
__END_DECLS




#endif // _GKEY_H_
