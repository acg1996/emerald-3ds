#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "3ds_nuz_store.h"
#include "3ds_data.h"
#include "3ds_pak.h"
#include "3ds_platform.h"

#define NUZ_FILE "emerald3ds.nuz"
#define NUZ_TEMP "emerald3ds.nuz.tmp"
#define NUZ_BACKUP "emerald3ds.nuz.bak"
/* "EM3DNUZ1", the save's CRC, the blob's size, the blob, the CRC of all that. */
#define NUZ_HEADER 16u
#define NUZ_MAX_BLOB 4096u

static void WriteNumber(unsigned char *bytes, uint32_t value)
{
    for (unsigned index = 0; index < 4; ++index)
        bytes[index] = (unsigned char)(value >> (index * 8u));
}

static uint32_t ReadNumber(const unsigned char *bytes)
{
    uint32_t value = 0;

    for (unsigned index = 0; index < 4; ++index)
        value |= (uint32_t)bytes[index] << (index * 8u);
    return value;
}

bool CtrNuzStore_Load(const void *save, size_t size, void *blob, size_t blobSize)
{
    unsigned char bytes[NUZ_HEADER + NUZ_MAX_BLOB + 4];
    const char *const names[] = {NUZ_FILE, NUZ_TEMP, NUZ_BACKUP};
    size_t total = NUZ_HEADER + blobSize + 4;

    if (save == NULL || size == 0 || blob == NULL || blobSize == 0 || blobSize > NUZ_MAX_BLOB)
        return false;
    /* The temp file too: a power cut between the two renames of Save leaves
     * the newest state only there. Each must name this save's CRC. */
    for (unsigned index = 0; index < sizeof(names) / sizeof(names[0]); ++index)
    {
        FILE *file = CtrFs_OpenData(names[index], "rb");
        bool valid;

        if (file == NULL)
            continue;
        valid = fread(bytes, 1, total, file) == total && fgetc(file) == EOF && !ferror(file);
        if (fclose(file) != 0)
            valid = false;
        if (!valid || memcmp(bytes, "EM3DNUZ1", 8) != 0
         || ReadNumber(bytes + 8) != CtrPak_Crc32(0, save, size)
         || ReadNumber(bytes + 12) != blobSize
         || ReadNumber(bytes + total - 4) != CtrPak_Crc32(0, bytes, total - 4))
            continue;
        memcpy(blob, bytes + NUZ_HEADER, blobSize);
        return true;
    }
    return false;
}

bool CtrNuzStore_Save(const void *save, size_t size, const void *blob, size_t blobSize)
{
    unsigned char bytes[NUZ_HEADER + NUZ_MAX_BLOB + 4];
    size_t total = NUZ_HEADER + blobSize + 4;
    FILE *file;
    bool valid;

    /* A scenario being played saves into memory only: the player's own file
     * stays the one that matches the player's save. */
    if (!CtrScenario_SaveWrites())
        return true;
    if (save == NULL || size == 0 || blob == NULL || blobSize == 0 || blobSize > NUZ_MAX_BLOB)
        return false;
    memcpy(bytes, "EM3DNUZ1", 8);
    WriteNumber(bytes + 8, CtrPak_Crc32(0, save, size));
    WriteNumber(bytes + 12, (uint32_t)blobSize);
    memcpy(bytes + NUZ_HEADER, blob, blobSize);
    WriteNumber(bytes + total - 4, CtrPak_Crc32(0, bytes, total - 4));
    file = CtrFs_OpenData(NUZ_TEMP, "wb");
    if (file == NULL)
        goto fail;
    valid = fwrite(bytes, 1, total, file) == total && fflush(file) == 0;
    if (fclose(file) != 0)
        valid = false;
    if (valid && rename(CTR_DATA_DIR NUZ_TEMP, CTR_DATA_DIR NUZ_FILE) == 0)
        return true;
    if (valid)
    {
        file = CtrFs_OpenData(NUZ_FILE, "rb");
        if (file != NULL)
        {
            fclose(file);
            remove(CTR_DATA_DIR NUZ_BACKUP);
            if (rename(CTR_DATA_DIR NUZ_FILE, CTR_DATA_DIR NUZ_BACKUP) == 0)
            {
                if (rename(CTR_DATA_DIR NUZ_TEMP, CTR_DATA_DIR NUZ_FILE) == 0)
                    return true;
                rename(CTR_DATA_DIR NUZ_BACKUP, CTR_DATA_DIR NUZ_FILE);
            }
        }
    }
    remove(CTR_DATA_DIR NUZ_TEMP);
fail:
    CtrLog_Write(CTR_LOG_ERROR, "nuzlocke: sidecar write failed; game save is unchanged");
    return false;
}
