#ifndef MINI_ZIP_H
#define MINI_ZIP_H

/* Minimal header-only ZIP writer. Stores one file without compression. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t mz_crc32_update(uint32_t crc, const unsigned char *data, size_t len) {
    size_t i;
    int bit;
    crc = ~crc;
    for (i = 0; i < len; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int)(crc & 1u));
        }
    }
    return ~crc;
}

static int mz_put16(FILE *f, uint16_t v) {
    unsigned char b[2] = {(unsigned char)(v & 0xffu), (unsigned char)((v >> 8) & 0xffu)};
    return fwrite(b, 1, 2, f) == 2;
}

static int mz_put32(FILE *f, uint32_t v) {
    unsigned char b[4] = {
        (unsigned char)(v & 0xffu), (unsigned char)((v >> 8) & 0xffu),
        (unsigned char)((v >> 16) & 0xffu), (unsigned char)((v >> 24) & 0xffu)
    };
    return fwrite(b, 1, 4, f) == 4;
}

static int mini_zip_store_file(const char *zip_path, const char *source_path,
                               const char *entry_name) {
    FILE *in = NULL, *out = NULL;
    unsigned char *buf = NULL;
    long size_long;
    uint32_t size, crc = 0;
    uint16_t name_len;
    int ok = 0;

    if (!zip_path || !source_path || !entry_name) return 0;
    if (strlen(entry_name) > 65535u) return 0;
    name_len = (uint16_t)strlen(entry_name);

    in = fopen(source_path, "rb");
    if (!in) goto cleanup;
    if (fseek(in, 0, SEEK_END) != 0) goto cleanup;
    size_long = ftell(in);
    if (size_long < 0 || (unsigned long)size_long > 0xffffffffUL) goto cleanup;
    size = (uint32_t)size_long;
    rewind(in);

    if (size > 0) {
        buf = (unsigned char *)malloc(size);
        if (!buf) goto cleanup;
        if (fread(buf, 1, size, in) != size) goto cleanup;
        crc = mz_crc32_update(0, buf, size);
    }
    fclose(in); in = NULL;

    out = fopen(zip_path, "wb");
    if (!out) goto cleanup;

    /* Local file header */
    if (!mz_put32(out, 0x04034b50u) || !mz_put16(out, 20) || !mz_put16(out, 0) ||
        !mz_put16(out, 0) || !mz_put16(out, 0) || !mz_put16(out, 0) ||
        !mz_put32(out, crc) || !mz_put32(out, size) || !mz_put32(out, size) ||
        !mz_put16(out, name_len) || !mz_put16(out, 0) ||
        fwrite(entry_name, 1, name_len, out) != name_len) goto cleanup;
    if (size > 0 && fwrite(buf, 1, size, out) != size) goto cleanup;

    /* Central directory entry */
    if (!mz_put32(out, 0x02014b50u) || !mz_put16(out, 20) || !mz_put16(out, 20) ||
        !mz_put16(out, 0) || !mz_put16(out, 0) || !mz_put16(out, 0) ||
        !mz_put16(out, 0) || !mz_put32(out, crc) || !mz_put32(out, size) ||
        !mz_put32(out, size) || !mz_put16(out, name_len) || !mz_put16(out, 0) ||
        !mz_put16(out, 0) || !mz_put16(out, 0) || !mz_put16(out, 0) ||
        !mz_put32(out, 0) || !mz_put32(out, 0) ||
        fwrite(entry_name, 1, name_len, out) != name_len) goto cleanup;

    /* End of central directory */
    if (!mz_put32(out, 0x06054b50u) || !mz_put16(out, 0) || !mz_put16(out, 0) ||
        !mz_put16(out, 1) || !mz_put16(out, 1) ||
        !mz_put32(out, (uint32_t)(46u + name_len)) ||
        !mz_put32(out, (uint32_t)(30u + name_len + size)) ||
        !mz_put16(out, 0)) goto cleanup;

    ok = fflush(out) == 0;

cleanup:
    if (in) fclose(in);
    if (out) fclose(out);
    free(buf);
    if (!ok) remove(zip_path);
    return ok;
}

#endif
