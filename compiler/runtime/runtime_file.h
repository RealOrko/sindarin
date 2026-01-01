#ifndef RUNTIME_FILE_H
#define RUNTIME_FILE_H

#include <stdbool.h>
#include "runtime_arena.h"

/* ============================================================================
 * File Handle Types
 * ============================================================================
 * Text and binary file handles for I/O operations. Handles are automatically
 * closed when their arena is destroyed.
 * ============================================================================ */

/* Text file handle - for text-oriented file operations */
typedef struct RtTextFile {
    void *fp;                   /* FILE* pointer (void* to avoid including stdio.h) */
    char *path;                 /* Full path to file */
    bool is_open;               /* Whether file is still open */
    RtFileHandle *handle;       /* Arena tracking handle */
} RtTextFile;

/* Binary file handle - for binary file operations */
typedef struct RtBinaryFile {
    void *fp;                   /* FILE* pointer (void* to avoid including stdio.h) */
    char *path;                 /* Full path to file */
    bool is_open;               /* Whether file is still open */
    RtFileHandle *handle;       /* Arena tracking handle */
} RtBinaryFile;

/* ============================================================================
 * File Promotion Functions
 * ============================================================================
 * Functions for promoting file handles between arenas.
 * ============================================================================ */

/* Promote a text file handle to a destination arena */
RtTextFile *rt_text_file_promote(RtArena *dest, RtArena *src_arena, RtTextFile *src);

/* Promote a binary file handle to a destination arena */
RtBinaryFile *rt_binary_file_promote(RtArena *dest, RtArena *src_arena, RtBinaryFile *src);

/* ============================================================================
 * TextFile Static Methods
 * ============================================================================
 * Static methods for text file operations that don't require an open file handle.
 * ============================================================================ */

/* Open a text file for reading and writing */
RtTextFile *rt_text_file_open(RtArena *arena, const char *path);

/* Check if a text file exists */
int rt_text_file_exists(const char *path);

/* Delete a text file */
void rt_text_file_delete(const char *path);

/* Read entire contents of a text file as a string (static method) */
char *rt_text_file_read_all(RtArena *arena, const char *path);

/* Write string to text file, replacing any existing content */
void rt_text_file_write_all(const char *path, const char *content);

/* Copy a text file to a new location */
void rt_text_file_copy(const char *src, const char *dst);

/* Move a text file to a new location */
void rt_text_file_move(const char *src, const char *dst);

/* Close an open text file */
void rt_text_file_close(RtTextFile *file);

/* ============================================================================
 * TextFile Instance Reading Methods
 * ============================================================================
 * Instance methods for reading from an open text file handle.
 * ============================================================================ */

/* Read single character, returns -1 on EOF */
long rt_text_file_read_char(RtTextFile *file);

/* Read whitespace-delimited word, returns empty string on EOF */
char *rt_text_file_read_word(RtArena *arena, RtTextFile *file);

/* Read single line (strips trailing newline), returns NULL on EOF */
char *rt_text_file_read_line(RtArena *arena, RtTextFile *file);

/* Read all remaining content from open file */
char *rt_text_file_instance_read_all(RtArena *arena, RtTextFile *file);

/* Read all remaining lines as array of strings */
char **rt_text_file_read_lines(RtArena *arena, RtTextFile *file);

/* Read into character buffer, returns number of chars read */
long rt_text_file_read_into(RtTextFile *file, char *buffer);

/* ============================================================================
 * TextFile Instance Writing Methods
 * ============================================================================
 * Instance methods for writing to an open text file handle.
 * ============================================================================ */

/* Write single character to file */
void rt_text_file_write_char(RtTextFile *file, long ch);

/* Write string to file */
void rt_text_file_write(RtTextFile *file, const char *text);

/* Write string followed by newline */
void rt_text_file_write_line(RtTextFile *file, const char *text);

/* Write string to file (alias for write, used with interpolated strings) */
void rt_text_file_print(RtTextFile *file, const char *text);

/* Write string followed by newline (alias for writeLine) */
void rt_text_file_println(RtTextFile *file, const char *text);

/* ============================================================================
 * TextFile State Methods
 * ============================================================================
 * Instance methods for querying and controlling text file state.
 * ============================================================================ */

/* Check if more characters are available to read */
int rt_text_file_has_chars(RtTextFile *file);

/* Check if more whitespace-delimited words are available */
int rt_text_file_has_words(RtTextFile *file);

/* Check if more lines are available to read */
int rt_text_file_has_lines(RtTextFile *file);

/* Check if at end of file */
int rt_text_file_is_eof(RtTextFile *file);

/* Get current byte position in file */
long rt_text_file_position(RtTextFile *file);

/* Seek to byte position */
void rt_text_file_seek(RtTextFile *file, long pos);

/* Return to beginning of file */
void rt_text_file_rewind(RtTextFile *file);

/* Force buffered data to disk */
void rt_text_file_flush(RtTextFile *file);

/* ============================================================================
 * TextFile Properties
 * ============================================================================
 * Instance methods for accessing text file properties.
 * ============================================================================ */

/* Get full file path */
char *rt_text_file_get_path(RtArena *arena, RtTextFile *file);

/* Get filename only (without directory) */
char *rt_text_file_get_name(RtArena *arena, RtTextFile *file);

/* Get file size in bytes */
long rt_text_file_get_size(RtTextFile *file);

/* ============================================================================
 * BinaryFile Static Methods
 * ============================================================================
 * Static methods for binary file operations that don't require an open file handle.
 * ============================================================================ */

/* Open binary file for reading and writing (panics if file doesn't exist) */
RtBinaryFile *rt_binary_file_open(RtArena *arena, const char *path);

/* Check if binary file exists without opening */
int rt_binary_file_exists(const char *path);

/* Read entire binary file contents as byte array */
unsigned char *rt_binary_file_read_all(RtArena *arena, const char *path);

/* Write byte array to binary file (creates or overwrites) */
void rt_binary_file_write_all(const char *path, unsigned char *data);

/* Delete binary file (panics if fails) */
void rt_binary_file_delete(const char *path);

/* Copy binary file to new location */
void rt_binary_file_copy(const char *src, const char *dst);

/* Move/rename binary file */
void rt_binary_file_move(const char *src, const char *dst);

/* Close an open binary file */
void rt_binary_file_close(RtBinaryFile *file);

/* ============================================================================
 * BinaryFile Instance Reading Methods
 * ============================================================================
 * Instance methods for reading from an open binary file handle.
 * ============================================================================ */

/* Read single byte, returns -1 on EOF */
long rt_binary_file_read_byte(RtBinaryFile *file);

/* Read N bytes into new array */
unsigned char *rt_binary_file_read_bytes(RtArena *arena, RtBinaryFile *file, long count);

/* Read all remaining bytes from open file */
unsigned char *rt_binary_file_instance_read_all(RtArena *arena, RtBinaryFile *file);

/* Read into byte buffer, returns number of bytes read */
long rt_binary_file_read_into(RtBinaryFile *file, unsigned char *buffer);

/* ============================================================================
 * BinaryFile Instance Writing Methods
 * ============================================================================
 * Instance methods for writing to an open binary file handle.
 * ============================================================================ */

/* Write single byte to file */
void rt_binary_file_write_byte(RtBinaryFile *file, long b);

/* Write byte array to file */
void rt_binary_file_write_bytes(RtBinaryFile *file, unsigned char *data);

/* ============================================================================
 * BinaryFile State Methods
 * ============================================================================
 * Instance methods for querying and controlling binary file state.
 * ============================================================================ */

/* Check if more bytes are available to read */
int rt_binary_file_has_bytes(RtBinaryFile *file);

/* Check if at end of file */
int rt_binary_file_is_eof(RtBinaryFile *file);

/* Get current byte position in file */
long rt_binary_file_position(RtBinaryFile *file);

/* Seek to byte position */
void rt_binary_file_seek(RtBinaryFile *file, long pos);

/* Return to beginning of file */
void rt_binary_file_rewind(RtBinaryFile *file);

/* Force buffered data to disk */
void rt_binary_file_flush(RtBinaryFile *file);

/* ============================================================================
 * BinaryFile Properties
 * ============================================================================
 * Instance methods for accessing binary file properties.
 * ============================================================================ */

/* Get full file path */
char *rt_binary_file_get_path(RtArena *arena, RtBinaryFile *file);

/* Get filename only (without directory) */
char *rt_binary_file_get_name(RtArena *arena, RtBinaryFile *file);

/* Get file size in bytes */
long rt_binary_file_get_size(RtBinaryFile *file);

#endif /* RUNTIME_FILE_H */
