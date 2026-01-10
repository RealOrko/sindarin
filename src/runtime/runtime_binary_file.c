#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW is POSIX-compatible */
    #include <sys/stat.h>
    #include <unistd.h>
    #else
    #include "../platform/compat_windows.h"
    #endif
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "runtime_file.h"
#include "runtime_arena.h"
#include "runtime_array.h"

/* ============================================================================
 * BinaryFile Operations Implementation
 * ============================================================================
 * This module provides binary file I/O operations for the Sn runtime:
 * - Opening and closing binary files
 * - Reading binary content (bytes, byte arrays)
 * - Writing binary content
 * - File state queries and manipulation
 * ============================================================================ */

/* ============================================================================
 * File Promotion Functions
 * ============================================================================ */

/* Promote a binary file handle to a destination arena */
RtBinaryFile *rt_binary_file_promote(RtArena *dest, RtArena *src_arena, RtBinaryFile *src)
{
    if (dest == NULL || src == NULL || !src->is_open) {
        return NULL;
    }

    /* Allocate new handle in destination arena */
    RtBinaryFile *promoted = rt_arena_alloc(dest, sizeof(RtBinaryFile));
    if (promoted == NULL) {
        return NULL;
    }

    /* Copy file state */
    promoted->fp = src->fp;
    promoted->path = rt_arena_promote_string(dest, src->path);
    promoted->is_open = src->is_open;

    /* Track in destination arena */
    promoted->handle = rt_arena_track_file(dest, src->fp, promoted->path, false);

    /* Untrack from source arena (file is no longer owned by source) */
    if (src->handle != NULL) {
        rt_arena_untrack_file(src_arena, src->handle);
    }

    /* Mark source as closed (ownership transferred) */
    src->is_open = false;
    src->fp = NULL;

    return promoted;
}

/* ============================================================================
 * BinaryFile Static Methods
 * ============================================================================ */

/* Open binary file for reading and writing (panics if file doesn't exist) */
RtBinaryFile *rt_binary_file_open(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.open: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.open: path is NULL\n");
        exit(1);
    }

    /* Try to open in r+b (read/write binary, must exist) */
    FILE *fp = fopen(path, "r+b");
    if (fp == NULL) {
        /* If file doesn't exist, create it with w+b */
        fp = fopen(path, "w+b");
        if (fp == NULL) {
            fprintf(stderr, "BinaryFile.open: failed to open file '%s': %s\n",
                    path, strerror(errno));
            exit(1);
        }
    }

    /* Allocate file handle */
    RtBinaryFile *file = rt_arena_alloc(arena, sizeof(RtBinaryFile));
    if (file == NULL) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.open: memory allocation failed\n");
        exit(1);
    }

    /* Copy path to arena */
    size_t path_len = strlen(path);
    char *path_copy = rt_arena_alloc(arena, path_len + 1);
    if (path_copy == NULL) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.open: memory allocation failed for path\n");
        exit(1);
    }
    memcpy(path_copy, path, path_len + 1);

    file->fp = fp;
    file->path = path_copy;
    file->is_open = true;

    /* Track file handle in arena for auto-close */
    RtFileHandle *handle = rt_arena_alloc(arena, sizeof(RtFileHandle));
    if (handle == NULL) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.open: memory allocation failed for handle\n");
        exit(1);
    }
    handle->fp = fp;
    handle->path = path_copy;
    handle->is_open = true;
    handle->next = arena->open_files;
    arena->open_files = handle;
    file->handle = handle;

    return file;
}

/* Check if binary file exists without opening */
int rt_binary_file_exists(const char *path) {
    if (path == NULL) {
        return 0;
    }
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/* Delete binary file (panics if fails) */
void rt_binary_file_delete(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.delete: path is NULL\n");
        exit(1);
    }

    if (remove(path) != 0) {
        fprintf(stderr, "BinaryFile.delete: failed to delete file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }
}

/* Read entire binary file contents as byte array */
unsigned char *rt_binary_file_read_all(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.readAll: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "BinaryFile.readAll: failed to open file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.readAll: failed to seek in file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.readAll: failed to get size of file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.readAll: failed to rewind file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    /* Create byte array with metadata */
    unsigned char *data = rt_array_create_byte_uninit(arena, (size_t)size);

    /* Read file contents */
    if (size > 0) {
        size_t bytes_read = fread(data, 1, (size_t)size, fp);
        if (bytes_read != (size_t)size) {
            fclose(fp);
            fprintf(stderr, "BinaryFile.readAll: failed to read file '%s': %s\n",
                    path, strerror(errno));
            exit(1);
        }
    }

    fclose(fp);
    return data;
}

/* Write byte array to binary file (creates or overwrites) */
void rt_binary_file_write_all(const char *path, unsigned char *data) {
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.writeAll: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "BinaryFile.writeAll: failed to create file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    if (data != NULL) {
        size_t len = rt_array_length(data);
        if (len > 0) {
            size_t written = fwrite(data, 1, len, fp);
            if (written != len) {
                fclose(fp);
                fprintf(stderr, "BinaryFile.writeAll: failed to write file '%s': %s\n",
                        path, strerror(errno));
                exit(1);
            }
        }
    }

    fclose(fp);
}

/* Copy binary file to new location */
void rt_binary_file_copy(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "BinaryFile.copy: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "BinaryFile.copy: destination path is NULL\n");
        exit(1);
    }

    FILE *src_fp = fopen(src, "rb");
    if (src_fp == NULL) {
        fprintf(stderr, "BinaryFile.copy: failed to open source file '%s': %s\n",
                src, strerror(errno));
        exit(1);
    }

    FILE *dst_fp = fopen(dst, "wb");
    if (dst_fp == NULL) {
        fclose(src_fp);
        fprintf(stderr, "BinaryFile.copy: failed to create destination file '%s': %s\n",
                dst, strerror(errno));
        exit(1);
    }

    /* Copy in chunks */
    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_read, dst_fp);
        if (bytes_written != bytes_read) {
            fclose(src_fp);
            fclose(dst_fp);
            fprintf(stderr, "BinaryFile.copy: failed to write to '%s': %s\n",
                    dst, strerror(errno));
            exit(1);
        }
    }

    if (ferror(src_fp)) {
        fclose(src_fp);
        fclose(dst_fp);
        fprintf(stderr, "BinaryFile.copy: failed to read from '%s': %s\n",
                src, strerror(errno));
        exit(1);
    }

    fclose(src_fp);
    fclose(dst_fp);
}

/* Move/rename binary file */
void rt_binary_file_move(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "BinaryFile.move: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "BinaryFile.move: destination path is NULL\n");
        exit(1);
    }

    /* Try rename first (efficient, same filesystem) */
    if (rename(src, dst) == 0) {
        return;
    }

    /* If rename fails, try copy + delete (cross-filesystem) */
    rt_binary_file_copy(src, dst);
    if (remove(src) != 0) {
        fprintf(stderr, "BinaryFile.move: failed to remove source file '%s': %s\n",
                src, strerror(errno));
        exit(1);
    }
}

/* Close an open binary file */
void rt_binary_file_close(RtBinaryFile *file) {
    if (file == NULL) {
        return;
    }
    if (file->is_open && file->fp != NULL) {
        fclose((FILE *)file->fp);
        file->is_open = false;
        file->fp = NULL;
        /* Mark handle as closed too */
        if (file->handle != NULL) {
            file->handle->is_open = false;
        }
    }
}

/* ============================================================================
 * BinaryFile Instance Reading Methods
 * ============================================================================ */

/* Read single byte, returns -1 on EOF */
long rt_binary_file_read_byte(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readByte: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readByte: file is not open\n");
        exit(1);
    }

    int c = fgetc((FILE *)file->fp);
    if (c == EOF) {
        if (ferror((FILE *)file->fp)) {
            fprintf(stderr, "BinaryFile.readByte: read error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        return -1;  /* EOF */
    }
    return (long)(unsigned char)c;
}

/* Read N bytes into new array */
unsigned char *rt_binary_file_read_bytes(RtArena *arena, RtBinaryFile *file, long count) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.readBytes: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readBytes: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readBytes: file is not open\n");
        exit(1);
    }
    if (count < 0) {
        fprintf(stderr, "BinaryFile.readBytes: count cannot be negative\n");
        exit(1);
    }

    /* Create byte array */
    unsigned char *data = rt_array_create_byte_uninit(arena, (size_t)count);

    /* Read bytes */
    if (count > 0) {
        size_t bytes_read = fread(data, 1, (size_t)count, (FILE *)file->fp);
        /* If we read fewer bytes, we need to update the array length */
        if (bytes_read < (size_t)count) {
            /* Update the metadata in the RtArrayMetadata struct */
            RtArrayMetadata *meta = ((RtArrayMetadata *)data) - 1;
            meta->size = bytes_read;
        }
    }

    return data;
}

/* Read into byte buffer, returns number of bytes read */
long rt_binary_file_read_into(RtBinaryFile *file, unsigned char *buffer) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readInto: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readInto: file is not open\n");
        exit(1);
    }
    if (buffer == NULL) {
        fprintf(stderr, "BinaryFile.readInto: buffer is NULL\n");
        exit(1);
    }

    /* Get the buffer's length from its metadata */
    size_t buf_len = rt_array_length(buffer);
    if (buf_len == 0) {
        return 0;  /* Empty buffer, nothing to read */
    }

    FILE *fp = (FILE *)file->fp;

    /* Read up to buf_len bytes */
    size_t bytes_read = fread(buffer, 1, buf_len, fp);
    if (ferror(fp)) {
        fprintf(stderr, "BinaryFile.readInto: read error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return (long)bytes_read;
}

/* Read all remaining bytes from open file */
unsigned char *rt_binary_file_instance_read_all(RtArena *arena, RtBinaryFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readAll: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readAll: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Get current position and file size */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long end_pos = ftell(fp);
    if (end_pos < 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek back to current position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Calculate remaining bytes */
    size_t remaining = (size_t)(end_pos - current_pos);

    /* Create byte array */
    unsigned char *data = rt_array_create_byte_uninit(arena, remaining);

    /* Read remaining content */
    if (remaining > 0) {
        size_t bytes_read = fread(data, 1, remaining, fp);
        if (ferror(fp)) {
            fprintf(stderr, "BinaryFile.readAll: failed to read file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        /* Update length if fewer bytes read */
        if (bytes_read < remaining) {
            RtArrayMetadata *meta = ((RtArrayMetadata *)data) - 1;
            meta->size = bytes_read;
        }
    }

    return data;
}

/* ============================================================================
 * BinaryFile Instance Writing Methods
 * ============================================================================ */

/* Write single byte to file */
void rt_binary_file_write_byte(RtBinaryFile *file, long b) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.writeByte: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.writeByte: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fputc((unsigned char)b, fp) == EOF) {
        fprintf(stderr, "BinaryFile.writeByte: write error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Write byte array to file */
void rt_binary_file_write_bytes(RtBinaryFile *file, unsigned char *data) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.writeBytes: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.writeBytes: file is not open\n");
        exit(1);
    }
    if (data == NULL) {
        return;  /* Nothing to write */
    }

    FILE *fp = (FILE *)file->fp;
    size_t len = rt_array_length(data);
    if (len > 0) {
        size_t written = fwrite(data, 1, len, fp);
        if (written != len) {
            fprintf(stderr, "BinaryFile.writeBytes: write error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
    }
}

/* ============================================================================
 * BinaryFile State Methods
 * ============================================================================ */

/* Check if more bytes are available to read */
int rt_binary_file_has_bytes(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.hasBytes: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.hasBytes: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 0;  /* No more bytes */
    }
    ungetc(c, fp);
    return 1;  /* More bytes available */
}

/* Check if at end of file */
int rt_binary_file_is_eof(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.isEof: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.isEof: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 1;  /* At EOF */
    }
    ungetc(c, fp);
    return 0;  /* Not at EOF */
}

/* Get current byte position in file */
long rt_binary_file_position(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.position: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.position: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    long pos = ftell(fp);
    if (pos < 0) {
        fprintf(stderr, "BinaryFile.position: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
    return pos;
}

/* Seek to byte position */
void rt_binary_file_seek(RtBinaryFile *file, long pos) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.seek: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.seek: file is not open\n");
        exit(1);
    }
    if (pos < 0) {
        fprintf(stderr, "BinaryFile.seek: invalid position %ld\n", pos);
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fseek(fp, pos, SEEK_SET) != 0) {
        fprintf(stderr, "BinaryFile.seek: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Return to beginning of file */
void rt_binary_file_rewind(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.rewind: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.rewind: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    rewind(fp);
}

/* Force buffered data to disk */
void rt_binary_file_flush(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.flush: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.flush: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fflush(fp) != 0) {
        fprintf(stderr, "BinaryFile.flush: failed to flush file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* ============================================================================
 * BinaryFile Properties
 * ============================================================================ */

/* Get full file path */
char *rt_binary_file_get_path(RtArena *arena, RtBinaryFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.path: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.path: file is NULL\n");
        exit(1);
    }

    if (file->path == NULL) {
        /* Return empty string if path is not set */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Copy path string to arena */
    size_t len = strlen(file->path);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, file->path, len + 1);
    return result;
}

/* Get filename only (without directory) */
char *rt_binary_file_get_name(RtArena *arena, RtBinaryFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.name: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.name: file is NULL\n");
        exit(1);
    }

    if (file->path == NULL) {
        /* Return empty string if path is not set */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Find last path separator */
    const char *path = file->path;
    const char *last_sep = strrchr(path, '/');
    #ifdef _WIN32
    const char *last_backslash = strrchr(path, '\\');
    if (last_backslash != NULL && (last_sep == NULL || last_backslash > last_sep)) {
        last_sep = last_backslash;
    }
    #endif

    const char *name = (last_sep != NULL) ? last_sep + 1 : path;
    size_t len = strlen(name);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, name, len + 1);
    return result;
}

/* Get file size in bytes */
long rt_binary_file_get_size(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.size: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.size: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Save current position */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "BinaryFile.size: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek to end to get size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "BinaryFile.size: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fprintf(stderr, "BinaryFile.size: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Restore original position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "BinaryFile.size: failed to restore position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return size;
}
