#ifndef _INFOSHARED_H_
#define _INFOSHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Callers generally only want the node itself.  This structure is used
   to pass node information around.  None of the information in this
   structure should ever be directly freed.  The structure itself can
   be passed to free ().  Note that NODE->parent is non-null if this
   node's file is a subfile.  In that case, NODE->parent is the logical
   name of the file containing this node.  Both names are given as full
   paths, so you might have: node->filename = "/usr/gnu/info/emacs-1",
   with node->parent = "/usr/gnu/info/emacs". */
typedef struct {
  char *filename;               /* The physical file containing this node. */
  char *parent;                 /* Non-null is the logical file name. */
  char *nodename;               /* The name of this node. */
  char *contents;               /* Characters appearing in this node. */
  long nodelen;                 /* The length of the CONTENTS member. */
  unsigned long display_pos;    /* Where to display at, if nonzero.  */
  int flags;                    /* See immediately below. */
} NODE;

/* Defines that can appear in NODE->flags.  All informative. */
#define N_HasTagsTable 0x01     /* This node was found through a tags table. */
#define N_TagsIndirect 0x02     /* The tags table was an indirect one. */
#define N_UpdateTags   0x04     /* The tags table is out of date. */
#define N_IsCompressed 0x08     /* The file is compressed on disk. */
#define N_IsInternal   0x10     /* This node was made by Info. */
#define N_CannotGC     0x20     /* File buffer cannot be gc'ed. */
#define N_IsManPage    0x40     /* This node is a manpage. */
#define N_FromAnchor   0x80     /* Synthesized for an anchor reference. */

// infointer.cpp
extern NODE *getDynamicNode(char *filename, char *nodename);
extern char *getInfoFullPath(char *filename);

#ifdef __cplusplus
}
#endif

#endif // _INFOSHARED_H_
