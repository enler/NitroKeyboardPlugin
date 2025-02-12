// this file only used for linking nitrosdk compiled as thumb instructions
#include <nds/ndstypes.h>
#include "nitro/sdk_ver.h"
#include "nitro/fs.h"

#define IMPORT __attribute__((naked))

#if NITROSDK_VER >= MAKE_NITROSDK_VER(5, 0)
IMPORT u32 FS_GetLength(const FSFile *p_file) {}
#endif

IMPORT bool FS_LoadOverlay(u32 target, u32 id) {}

IMPORT void FS_Init(u32 default_dma_no ) {}

IMPORT void FS_InitFile(FSFile *p_file) {}

IMPORT bool FS_OpenFile(FSFile *p_file, const char *path) {}

IMPORT s32 FS_ReadFile(FSFile *p_file, void *dst, s32 len) {}

IMPORT bool FS_CloseFile(FSFile *p_file) {}

IMPORT bool FS_SeekFile( FSFile *p_file, s32 offset, s32 origin ) {}

IMPORT bool FS_LoadOverlayInfo(FSOverlayInfo *p_ovi, u32 target, u32 id) {}

IMPORT bool FS_LoadOverlayImage(FSOverlayInfo *p_ovi) {}

IMPORT void FS_StartOverlay(FSOverlayInfo *p_ovi) {}