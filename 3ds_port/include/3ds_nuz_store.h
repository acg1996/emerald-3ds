#ifndef CTR_NUZ_STORE_H
#define CTR_NUZ_STORE_H

/*
 * The Nuzlocke run's state kept next to the game save (like the clock's,
 * 3ds_clock_store.h): a file on the SD card that names the CRC of the save it
 * belongs to, so another save never reads it. SDK-free for the game units that
 * call it from the save and load hooks.
 */
#include <stdbool.h>
#include <stddef.h>

/* Fills blob (blobSize bytes) from the file that matches this save; false if none does. */
bool CtrNuzStore_Load(const void *save, size_t size, void *blob, size_t blobSize);
/* Writes blob next to this save. */
bool CtrNuzStore_Save(const void *save, size_t size, const void *blob, size_t blobSize);

#endif // CTR_NUZ_STORE_H
