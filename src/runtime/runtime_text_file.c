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
 * TextFile Operations Implementation
 * ============================================================================
 * This module provides text file I/O operations for the Sn runtime:
 * - Opening and closing text files
 * - Reading text content (characters, words, lines)
 * - Writing text content
 * - File state queries and manipulation
 * ============================================================================ */

/* ============================================================================
 * File Promotion Functions
 * ============================================================================ */

/* Promote a text file handle to a destination arena */
RtTextFile *rt_text_file_promote(RtArena *dest, RtArena *src_arena, RtTextFile *src)
{
    if (dest == NULL || src == NULL || !src->is_open) {
        return NULL;
    }

    /* Allocate new handle in destination arena */
    RtTextFile *promoted = rt_arena_alloc(dest, sizeof(RtTextFile));
    if (promoted == NULL) {
        return NULL;
    }

    /* Copy file state */
    promoted->fp = src->fp;
    promoted->path = rt_arena_promote_string(dest, src->path);
    promoted->is_open = src->is_open;

    /* Track in destination arena */
    promoted->handle = rt_arena_track_file(dest, src->fp, promoted->path, true);

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
 * TextFile Static Methods
 * ============================================================================ */

/* Open file for reading and writing (panics if file doesn't exist) */
RtTextFile *rt_text_file_open(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.open: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "TextFile.open: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "r+b");  /* Binary mode for cross-platform consistency */
    if (fp == NULL) {
        /* Try to create if doesn't exist for write */
        fp = fopen(path, "w+b");
        if (fp == NULL) {
            fprintf(stderr, "TextFile.open: failed to open file '%s': %s\n", path, strerror(errno));
            exit(1);
        }
    }

    /* Allocate TextFile struct from arena */
    RtTextFile *file = rt_arena_alloc(arena, sizeof(RtTextFile));
    if (file == NULL) {
        fclose(fp);
        fprintf(stderr, "TextFile.open: memory allocation failed\n");
        exit(1);
    }

    file->fp = fp;
    file->path = rt_arena_strdup(arena, path);
    file->is_open = true;

    /* Track in arena for auto-close */
    file->handle = rt_arena_track_file(arena, fp, path, true);

    return file;
}

/* Check if file exists without opening */
int rt_text_file_exists(const char *path) {
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

/* Delete file (panics if fails) */
void rt_text_file_delete(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "TextFile.delete: path is NULL\n");
        exit(1);
    }

    if (remove(path) != 0) {
        fprintf(stderr, "TextFile.delete: failed to delete file '%s': %s\n", path, strerror(errno));
        exit(1);
    }
}

/* Read entire file contents as string */
char *rt_text_file_read_all(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "TextFile.readAll: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "TextFile.readAll: failed to open file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to get size of file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    /* Allocate buffer from arena */
    char *content = rt_arena_alloc(arena, (size_t)size + 1);
    if (content == NULL) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: memory allocation failed\n");
        exit(1);
    }

    /* Read file contents */
    size_t bytes_read = fread(content, 1, (size_t)size, fp);
    if (ferror(fp)) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to read file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    content[bytes_read] = '\0';
    fclose(fp);

    return content;
}

/* Write string to file (creates or overwrites) */
void rt_text_file_write_all(const char *path, const char *content) {
    if (path == NULL) {
        fprintf(stderr, "TextFile.writeAll: path is NULL\n");
        exit(1);
    }
    if (content == NULL) {
        content = "";
    }

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "TextFile.writeAll: failed to open file '%s' for writing: %s\n", path, strerror(errno));
        exit(1);
    }

    size_t len = strlen(content);
    if (len > 0) {
        size_t written = fwrite(content, 1, len, fp);
        if (written != len) {
            fclose(fp);
            fprintf(stderr, "TextFile.writeAll: failed to write to file '%s': %s\n", path, strerror(errno));
            exit(1);
        }
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "TextFile.writeAll: failed to close file '%s': %s\n", path, strerror(errno));
        exit(1);
    }
}

/* Copy file to new location */
void rt_text_file_copy(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "TextFile.copy: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "TextFile.copy: destination path is NULL\n");
        exit(1);
    }

    FILE *src_fp = fopen(src, "rb");
    if (src_fp == NULL) {
        fprintf(stderr, "TextFile.copy: failed to open source file '%s': %s\n", src, strerror(errno));
        exit(1);
    }

    FILE *dst_fp = fopen(dst, "wb");
    if (dst_fp == NULL) {
        fclose(src_fp);
        fprintf(stderr, "TextFile.copy: failed to open destination file '%s': %s\n", dst, strerror(errno));
        exit(1);
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        if (fwrite(buffer, 1, bytes, dst_fp) != bytes) {
            fclose(src_fp);
            fclose(dst_fp);
            fprintf(stderr, "TextFile.copy: failed to write to destination file '%s': %s\n", dst, strerror(errno));
            exit(1);
        }
    }

    if (ferror(src_fp)) {
        fclose(src_fp);
        fclose(dst_fp);
        fprintf(stderr, "TextFile.copy: failed to read from source file '%s': %s\n", src, strerror(errno));
        exit(1);
    }

    fclose(src_fp);
    if (fclose(dst_fp) != 0) {
        fprintf(stderr, "TextFile.copy: failed to close destination file '%s': %s\n", dst, strerror(errno));
        exit(1);
    }
}

/* Move/rename file */
void rt_text_file_move(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "TextFile.move: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "TextFile.move: destination path is NULL\n");
        exit(1);
    }

    if (rename(src, dst) != 0) {
        /* rename() may fail across filesystems, try copy+delete instead */
        rt_text_file_copy(src, dst);
        if (remove(src) != 0) {
            fprintf(stderr, "TextFile.move: failed to remove source file '%s' after copy: %s\n", src, strerror(errno));
            exit(1);
        }
    }
}

/* ============================================================================
 * TextFile Instance Reading Methods
 * ============================================================================ */

/* Read single character, returns -1 on EOF */
long rt_text_file_read_char(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.readChar: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readChar: file is not open\n");
        exit(1);
    }

    int c = fgetc((FILE *)file->fp);
    if (c == EOF) {
        if (ferror((FILE *)file->fp)) {
            fprintf(stderr, "TextFile.readChar: read error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        return -1;  /* EOF */
    }
    return (long)c;
}

/* Read whitespace-delimited word, returns empty string on EOF */
char *rt_text_file_read_word(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readWord: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readWord: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readWord: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Skip leading whitespace */
    int c;
    while ((c = fgetc(fp)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
        /* Skip whitespace */
    }

    if (c == EOF) {
        /* Return empty string on EOF */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Read word into a temporary buffer, then copy to arena */
    size_t capacity = 64;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);

    buffer[length++] = (char)c;

    while ((c = fgetc(fp)) != EOF && c != ' ' && c != '\t' && c != '\n' && c != '\r') {
        if (length >= capacity - 1) {
            /* Need more space - allocate new buffer and copy */
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }
        buffer[length++] = (char)c;
    }

    /* Put back the whitespace character if not EOF */
    if (c != EOF) {
        ungetc(c, fp);
    }

    buffer[length] = '\0';
    return buffer;
}

/* Read single line (strips trailing newline), returns NULL on EOF */
char *rt_text_file_read_line(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readLine: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readLine: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readLine: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Check for immediate EOF */
    int c = fgetc(fp);
    if (c == EOF) {
        if (ferror(fp)) {
            fprintf(stderr, "TextFile.readLine: read error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        return NULL;  /* EOF - return NULL */
    }
    ungetc(c, fp);

    /* Read line into buffer */
    size_t capacity = 256;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);

    while ((c = fgetc(fp)) != EOF && c != '\n') {
        if (length >= capacity - 1) {
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }
        buffer[length++] = (char)c;
    }

    /* Strip trailing \r if present (for Windows line endings) */
    if (length > 0 && buffer[length - 1] == '\r') {
        length--;
    }

    buffer[length] = '\0';
    return buffer;
}

/* Read all remaining content from open file */
char *rt_text_file_instance_read_all(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readAll: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readAll: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Get current position and file size */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "TextFile.readAll: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long end_pos = ftell(fp);
    if (end_pos < 0) {
        fprintf(stderr, "TextFile.readAll: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek back to current position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Calculate remaining bytes */
    size_t remaining = (size_t)(end_pos - current_pos);

    /* Allocate buffer from arena */
    char *content = rt_arena_alloc(arena, remaining + 1);
    if (content == NULL) {
        fprintf(stderr, "TextFile.readAll: memory allocation failed\n");
        exit(1);
    }

    /* Read remaining content */
    size_t bytes_read = fread(content, 1, remaining, fp);
    if (ferror(fp)) {
        fprintf(stderr, "TextFile.readAll: failed to read file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    content[bytes_read] = '\0';
    return content;
}

/* Read all remaining lines as array of strings */
char **rt_text_file_read_lines(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readLines: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readLines: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readLines: file is not open\n");
        exit(1);
    }

    /* Start with empty array */
    char **lines = rt_array_create_string(arena, 0, NULL);

    /* Read lines until EOF */
    char *line;
    while ((line = rt_text_file_read_line(arena, file)) != NULL) {
        lines = rt_array_push_string(arena, lines, line);
    }

    return lines;
}

/* Read into character buffer, returns number of chars read */
long rt_text_file_read_into(RtTextFile *file, char *buffer) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.readInto: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readInto: file is not open\n");
        exit(1);
    }
    if (buffer == NULL) {
        fprintf(stderr, "TextFile.readInto: buffer is NULL\n");
        exit(1);
    }

    /* Get the buffer's length from its metadata */
    size_t buf_len = rt_array_length(buffer);
    if (buf_len == 0) {
        return 0;  /* Empty buffer, nothing to read */
    }

    FILE *fp = (FILE *)file->fp;

    /* Read up to buf_len characters */
    size_t bytes_read = fread(buffer, 1, buf_len, fp);
    if (ferror(fp)) {
        fprintf(stderr, "TextFile.readInto: read error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return (long)bytes_read;
}

/* Close an open text file */
void rt_text_file_close(RtTextFile *file) {
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
 * TextFile Instance Writing Methods
 * ============================================================================ */

/* Write single character to file */
void rt_text_file_write_char(RtTextFile *file, long ch) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.writeChar: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.writeChar: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fputc((int)ch, fp) == EOF) {
        fprintf(stderr, "TextFile.writeChar: write error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Write string to file */
void rt_text_file_write(RtTextFile *file, const char *text) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.write: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.write: file is not open\n");
        exit(1);
    }
    if (text == NULL) {
        return;  /* Nothing to write */
    }

    FILE *fp = (FILE *)file->fp;
    size_t len = strlen(text);
    if (len > 0) {
        size_t written = fwrite(text, 1, len, fp);
        if (written != len) {
            fprintf(stderr, "TextFile.write: write error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
    }
}

/* Write string followed by newline */
void rt_text_file_write_line(RtTextFile *file, const char *text) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.writeLine: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.writeLine: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Write the text if not null */
    if (text != NULL) {
        size_t len = strlen(text);
        if (len > 0) {
            size_t written = fwrite(text, 1, len, fp);
            if (written != len) {
                fprintf(stderr, "TextFile.writeLine: write error on file '%s': %s\n",
                        file->path ? file->path : "(unknown)", strerror(errno));
                exit(1);
            }
        }
    }

    /* Write the newline */
    if (fputc('\n', fp) == EOF) {
        fprintf(stderr, "TextFile.writeLine: write error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Write string to file (alias for write, used with interpolated strings) */
void rt_text_file_print(RtTextFile *file, const char *text) {
    rt_text_file_write(file, text);
}

/* Write string followed by newline (alias for writeLine) */
void rt_text_file_println(RtTextFile *file, const char *text) {
    rt_text_file_write_line(file, text);
}

/* ============================================================================
 * TextFile State Methods
 * ============================================================================ */

/* Check if more characters are available to read */
int rt_text_file_has_chars(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.hasChars: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.hasChars: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 0;  /* No more characters */
    }
    ungetc(c, fp);
    return 1;  /* More characters available */
}

/* Check if at end of file */
int rt_text_file_is_eof(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.isEof: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.isEof: file is not open\n");
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

/* Check if more whitespace-delimited words are available */
int rt_text_file_has_words(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.hasWords: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.hasWords: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    long original_pos = ftell(fp);

    /* Skip whitespace */
    int c;
    while ((c = fgetc(fp)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
        /* Skip whitespace */
    }

    int has_word = (c != EOF);

    /* Restore original position */
    fseek(fp, original_pos, SEEK_SET);

    return has_word;
}

/* Check if more lines are available to read */
int rt_text_file_has_lines(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.hasLines: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.hasLines: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 0;  /* No more content */
    }
    ungetc(c, fp);
    return 1;  /* More lines available */
}

/* ============================================================================
 * TextFile Control Methods
 * ============================================================================ */

/* Get current byte position in file */
long rt_text_file_position(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.position: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.position: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    long pos = ftell(fp);
    if (pos < 0) {
        fprintf(stderr, "TextFile.position: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
    return pos;
}

/* Seek to byte position */
void rt_text_file_seek(RtTextFile *file, long pos) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.seek: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.seek: file is not open\n");
        exit(1);
    }
    if (pos < 0) {
        fprintf(stderr, "TextFile.seek: invalid position %ld\n", pos);
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fseek(fp, pos, SEEK_SET) != 0) {
        fprintf(stderr, "TextFile.seek: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Return to beginning of file */
void rt_text_file_rewind(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.rewind: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.rewind: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    rewind(fp);
}

/* Force buffered data to disk */
void rt_text_file_flush(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.flush: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.flush: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fflush(fp) != 0) {
        fprintf(stderr, "TextFile.flush: failed to flush file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* ============================================================================
 * TextFile Properties
 * ============================================================================ */

/* Get full file path */
char *rt_text_file_get_path(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.path: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.path: file is NULL\n");
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
char *rt_text_file_get_name(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.name: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.name: file is NULL\n");
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
long rt_text_file_get_size(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.size: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.size: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Save current position */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "TextFile.size: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek to end to get size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "TextFile.size: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fprintf(stderr, "TextFile.size: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Restore original position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "TextFile.size: failed to restore position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return size;
}
