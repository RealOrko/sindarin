// tests/runtime_arena_tests.c
// Tests for the runtime arena memory management system (RtArena)

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../debug.h"

void test_rt_arena_create()
{
    printf("Testing rt_arena_create...\n");

    // Create arena with default size
    RtArena *arena = rt_arena_create(NULL);
    assert(arena != NULL);
    assert(arena->parent == NULL);
    assert(arena->first != NULL);
    assert(arena->current == arena->first);
    assert(arena->default_block_size == RT_ARENA_DEFAULT_BLOCK_SIZE);
    assert(arena->total_allocated > 0);

    rt_arena_destroy(arena);
}

void test_rt_arena_create_sized()
{
    printf("Testing rt_arena_create_sized...\n");

    // Create arena with custom size
    RtArena *arena = rt_arena_create_sized(NULL, 1024);
    assert(arena != NULL);
    assert(arena->default_block_size == 1024);
    assert(arena->first->size == 1024);

    rt_arena_destroy(arena);

    // Create arena with zero size (should use default)
    arena = rt_arena_create_sized(NULL, 0);
    assert(arena != NULL);
    assert(arena->default_block_size == RT_ARENA_DEFAULT_BLOCK_SIZE);

    rt_arena_destroy(arena);
}

void test_rt_arena_create_with_parent()
{
    printf("Testing rt_arena_create with parent...\n");

    RtArena *parent = rt_arena_create(NULL);
    RtArena *child = rt_arena_create(parent);

    assert(child->parent == parent);
    assert(parent->parent == NULL);

    rt_arena_destroy(child);
    rt_arena_destroy(parent);
}

void test_rt_arena_alloc_small()
{
    printf("Testing rt_arena_alloc small allocations...\n");

    RtArena *arena = rt_arena_create_sized(NULL, 256);

    // Allocate a few small blocks
    void *p1 = rt_arena_alloc(arena, 16);
    void *p2 = rt_arena_alloc(arena, 32);
    void *p3 = rt_arena_alloc(arena, 8);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);

    // Pointers should be within the same block and properly aligned
    assert(p2 > p1);
    assert(p3 > p2);

    // Write to the allocated memory to ensure it's usable
    memset(p1, 0xAA, 16);
    memset(p2, 0xBB, 32);
    memset(p3, 0xCC, 8);

    rt_arena_destroy(arena);
}

void test_rt_arena_alloc_large()
{
    printf("Testing rt_arena_alloc large allocations...\n");

    RtArena *arena = rt_arena_create_sized(NULL, 64);

    // Allocate more than block size, should create new block
    void *p1 = rt_arena_alloc(arena, 100);
    assert(p1 != NULL);

    // Original block and new block should exist
    assert(arena->first != NULL);
    assert(arena->current != arena->first);

    rt_arena_destroy(arena);
}

void test_rt_arena_alloc_zero()
{
    printf("Testing rt_arena_alloc zero size...\n");

    RtArena *arena = rt_arena_create(NULL);

    void *p = rt_arena_alloc(arena, 0);
    assert(p == NULL);

    rt_arena_destroy(arena);
}

void test_rt_arena_alloc_null_arena()
{
    printf("Testing rt_arena_alloc with NULL arena...\n");

    void *p = rt_arena_alloc(NULL, 16);
    assert(p == NULL);
}

void test_rt_arena_calloc()
{
    printf("Testing rt_arena_calloc...\n");

    RtArena *arena = rt_arena_create(NULL);

    // Allocate and zero 10 integers
    int *arr = rt_arena_calloc(arena, 10, sizeof(int));
    assert(arr != NULL);

    // Verify all zeroed
    for (int i = 0; i < 10; i++) {
        assert(arr[i] == 0);
    }

    rt_arena_destroy(arena);
}

void test_rt_arena_alloc_aligned()
{
    printf("Testing rt_arena_alloc_aligned...\n");

    RtArena *arena = rt_arena_create(NULL);

    // Test 16-byte alignment
    void *p1 = rt_arena_alloc_aligned(arena, 32, 16);
    assert(p1 != NULL);
    assert(((size_t)p1 % 16) == 0);

    // Test 32-byte alignment
    void *p2 = rt_arena_alloc_aligned(arena, 64, 32);
    assert(p2 != NULL);
    assert(((size_t)p2 % 32) == 0);

    rt_arena_destroy(arena);
}

void test_rt_arena_strdup()
{
    printf("Testing rt_arena_strdup...\n");

    RtArena *arena = rt_arena_create(NULL);

    // NULL input
    char *s1 = rt_arena_strdup(arena, NULL);
    assert(s1 == NULL);

    // Empty string
    char *s2 = rt_arena_strdup(arena, "");
    assert(s2 != NULL);
    assert(strcmp(s2, "") == 0);

    // Normal string
    char *s3 = rt_arena_strdup(arena, "hello world");
    assert(s3 != NULL);
    assert(strcmp(s3, "hello world") == 0);

    // Long string
    const char *long_str = "This is a longer string that should still work correctly with the arena allocator.";
    char *s4 = rt_arena_strdup(arena, long_str);
    assert(s4 != NULL);
    assert(strcmp(s4, long_str) == 0);

    rt_arena_destroy(arena);
}

void test_rt_arena_strndup()
{
    printf("Testing rt_arena_strndup...\n");

    RtArena *arena = rt_arena_create(NULL);

    // NULL input
    char *s1 = rt_arena_strndup(arena, NULL, 5);
    assert(s1 == NULL);

    // String shorter than n
    char *s2 = rt_arena_strndup(arena, "hello", 10);
    assert(s2 != NULL);
    assert(strcmp(s2, "hello") == 0);

    // String longer than n
    char *s3 = rt_arena_strndup(arena, "hello world", 5);
    assert(s3 != NULL);
    assert(strcmp(s3, "hello") == 0);

    // n = 0
    char *s4 = rt_arena_strndup(arena, "hello", 0);
    assert(s4 != NULL);
    assert(strcmp(s4, "") == 0);

    rt_arena_destroy(arena);
}

void test_rt_arena_reset()
{
    printf("Testing rt_arena_reset...\n");

    RtArena *arena = rt_arena_create_sized(NULL, 64);

    // Allocate enough to create multiple blocks
    for (int i = 0; i < 10; i++) {
        rt_arena_alloc(arena, 100);
    }

    // Verify multiple blocks exist
    assert(arena->first->next != NULL);

    // Reset the arena
    rt_arena_reset(arena);

    // After reset, should have only first block
    assert(arena->first != NULL);
    assert(arena->first->next == NULL);
    assert(arena->current == arena->first);
    assert(arena->first->used == 0);

    // Should be able to allocate again
    void *p = rt_arena_alloc(arena, 32);
    assert(p != NULL);

    rt_arena_destroy(arena);
}

void test_rt_arena_promote()
{
    printf("Testing rt_arena_promote...\n");

    RtArena *src_arena = rt_arena_create(NULL);
    RtArena *dest_arena = rt_arena_create(NULL);

    // Allocate and fill source data
    int *src_data = rt_arena_alloc(src_arena, sizeof(int) * 5);
    for (int i = 0; i < 5; i++) {
        src_data[i] = i * 10;
    }

    // Promote to destination arena
    int *dest_data = rt_arena_promote(dest_arena, src_data, sizeof(int) * 5);
    assert(dest_data != NULL);
    assert(dest_data != src_data);  // Different memory

    // Verify data was copied
    for (int i = 0; i < 5; i++) {
        assert(dest_data[i] == i * 10);
    }

    // Modify source, dest should be unchanged
    src_data[0] = 999;
    assert(dest_data[0] == 0);

    rt_arena_destroy(src_arena);
    rt_arena_destroy(dest_arena);
}

void test_rt_arena_promote_null()
{
    printf("Testing rt_arena_promote with NULL...\n");

    RtArena *arena = rt_arena_create(NULL);

    void *p1 = rt_arena_promote(NULL, "test", 4);
    assert(p1 == NULL);

    void *p2 = rt_arena_promote(arena, NULL, 4);
    assert(p2 == NULL);

    void *p3 = rt_arena_promote(arena, "test", 0);
    assert(p3 == NULL);

    rt_arena_destroy(arena);
}

void test_rt_arena_promote_string()
{
    printf("Testing rt_arena_promote_string...\n");

    RtArena *src_arena = rt_arena_create(NULL);
    RtArena *dest_arena = rt_arena_create(NULL);

    // Allocate source string
    char *src_str = rt_arena_strdup(src_arena, "hello from source");

    // Promote to destination
    char *dest_str = rt_arena_promote_string(dest_arena, src_str);
    assert(dest_str != NULL);
    assert(dest_str != src_str);
    assert(strcmp(dest_str, "hello from source") == 0);

    rt_arena_destroy(src_arena);
    rt_arena_destroy(dest_arena);
}

void test_rt_arena_total_allocated()
{
    printf("Testing rt_arena_total_allocated...\n");

    RtArena *arena = rt_arena_create_sized(NULL, 1024);
    size_t initial = rt_arena_total_allocated(arena);
    assert(initial > 0);

    // Allocate more than one block
    rt_arena_alloc(arena, 2000);
    size_t after = rt_arena_total_allocated(arena);
    assert(after > initial);

    // NULL arena
    assert(rt_arena_total_allocated(NULL) == 0);

    rt_arena_destroy(arena);
}

void test_rt_arena_destroy_null()
{
    printf("Testing rt_arena_destroy with NULL...\n");

    // Should not crash
    rt_arena_destroy(NULL);
}

void test_rt_arena_block_growth()
{
    printf("Testing rt_arena block growth...\n");

    RtArena *arena = rt_arena_create_sized(NULL, 32);

    // First block has 32 bytes
    assert(arena->first->size == 32);

    // Allocate 16 bytes (fits)
    void *p1 = rt_arena_alloc(arena, 16);
    assert(p1 != NULL);
    assert(arena->current == arena->first);

    // Allocate another 24 bytes (doesn't fit, need new block)
    void *p2 = rt_arena_alloc(arena, 24);
    assert(p2 != NULL);
    assert(arena->current != arena->first);
    assert(arena->current->size >= 24);

    rt_arena_destroy(arena);
}

void test_rt_arena_many_allocations()
{
    printf("Testing rt_arena with many allocations...\n");

    RtArena *arena = rt_arena_create(NULL);

    // Many small allocations
    for (int i = 0; i < 1000; i++) {
        void *p = rt_arena_alloc(arena, 64);
        assert(p != NULL);
        memset(p, i & 0xFF, 64);  // Write pattern
    }

    rt_arena_destroy(arena);
}

void test_rt_arena_main()
{
    test_rt_arena_create();
    test_rt_arena_create_sized();
    test_rt_arena_create_with_parent();
    test_rt_arena_alloc_small();
    test_rt_arena_alloc_large();
    test_rt_arena_alloc_zero();
    test_rt_arena_alloc_null_arena();
    test_rt_arena_calloc();
    test_rt_arena_alloc_aligned();
    test_rt_arena_strdup();
    test_rt_arena_strndup();
    test_rt_arena_reset();
    test_rt_arena_promote();
    test_rt_arena_promote_null();
    test_rt_arena_promote_string();
    test_rt_arena_total_allocated();
    test_rt_arena_destroy_null();
    test_rt_arena_block_growth();
    test_rt_arena_many_allocations();
}
