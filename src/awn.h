#ifndef AWN_H
#define AWN_H

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Types
#include <stdint.h>

#define internal static
#define global static
#define local_persist static

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64; 

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64; 

typedef float f32;
typedef double f64;

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Some syntatic stuff for C.

#ifndef __cplusplus

    #define and &&
    #define or ||
    #define true (0 == 0)
    #define false (0 == 1)

    typedef i32 bool;

#endif

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Array functions
#define AWN_ArrayCount(array) (sizeof(array) / sizeof(*(array)))

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Assert and todo macros.

#include <assert.h>

#define assertln(c, m) assert(c && m)
#define todoln() assert(0 && "This code is yet to be completed.")

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): List functions
#define AWN_SLLPushBack(f, l, n) ( ((f) == (0)) ? ((f) = (l) = (n), (n)->next = 0) : ((l)->next = (n), (l) = (n), (n)->next = 0) )
#define AWN_SLLPushFront(f, l, n) ( ((f) == (0)) ? ((f) = (l) = (n), (n)->next = 0) : ((n)->next = (f), (f) = (n)) ) 
#define AWN_SLLPop(f, l) ( ((f) == (l)) ? ((f) = (l) = (l)) : ((f) = (f)->next) )
#define AWN_SLLIter(type, list) for (type *node = list.first; node != 0; node = node->next)

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Data size functions

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Detect current compiler.

#ifdef _MSC_VER
    #define COMPILER_MSVC 1
#else
    #define COMPILER_MSVC 0
#endif

#ifdef __GNUC__
    #define COMPILER_GCC 1
#else
    #define COMPILER_GCC 0
#endif

#ifdef __clang__
    #define COMPILER_CLANG 1
#else
    #define COMPILER_CLANG 0
#endif

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Compiler (and c) specific macros.

#ifndef __cplusplus

    #if COMPILER_MSVC
        #warning MSVC compiler does not support auto or cleanup!
    #else
        #define cleanup(fn) __attribute__((__cleanup__(fn)))
        #define void_cleanup(fn) void* cleanup(fn) tmp_defer_scopevar_##__LINE__
        #define void_cleanup_fn(name) void name(void* nothing)
        #define auto __auto_type
    #endif

#else

    // NOTE(Alex): Scope based defer function from https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
    template<typename F> struct awn_privdefer 
    {
        F f;
        awn_privdefer(F f) : f(f) {}
        ~awn_privdefer() { f(); }
    };

    template<typename F> awn_privdefer<F> defer_func(F f)
    {
        return awn_privdefer<F>(f);
    }

    #define DEFER_1(x, y) x##y
    #define DEFER_2(x, y) DEFER_1(x, y)
    #define DEFER_3(x) DEFER_2(x, __COUNTER__)
    #define defer(code) auto DEFER_3(_defer_) = defer_func([&](){code;})

#endif

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Arena allocator header.

#define AWN_ARENA_DEFAULT_ALIGNMENT (2 * sizeof(void *))
#define AWN_ARENA_ALIGN_UP_POW_2(x, p) (((x + p) - 1) & ~(p - 1))

#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Arena type
typedef struct arena_t
{
    u8 *buffer;
    u64 pos;
    u64 pos_prev;
    u64 cap;
} arena_t;

void AWN_ArenaCreate(arena_t *arena, void* mem_buffer, u64 mem_size);
void* AWN_ArenaPush(arena_t *arena, u64 push_size);
void* AWN_ArenaResize(arena_t *arena, void* old_memory, u64 old_size, u64 new_size);
void AWN_ArenaClear(arena_t *arena);
void AWN_ArenaGrow(arena_t *arena, void* new_buffer, u64 new_size);
void AWN_ArenaShrink(arena_t *arena, void* new_buffer, u64 new_size);

// NOTE(Alex): This is the system for recording arena states (for temporary allocations).
//              Allows us to use the linear allocator like a stack allocator.
typedef struct arena_state_t
{
    arena_t *arena;
    u64 pos_prev;
    u64 pos_cur;
} arena_state_t;

arena_state_t AWN_ArenaStateRecord(arena_t *a);
void AWN_ArenaStateRestore(arena_state_t);

#endif // End of header.

#ifdef AWN_IMPLEMENTATION

///////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Arena allocator implementation

void AWN_ArenaCreate(arena_t *arena, void* mem_buffer, u64 mem_size)
{
    arena->buffer = (u8 *)mem_buffer;
    arena->cap = mem_size;

    arena->pos = 0;
    arena->pos_prev = 0;
}

void* AWN_ArenaPush(arena_t *arena, u64 push_size)
{
    assertln(push_size > 0, "Arena push: Push size is zero.");
    void *result = 0;

    u64 cur_ptr = (u64)arena->buffer + arena->pos;
    u64 offset = AWN_ARENA_ALIGN_UP_POW_2(cur_ptr, AWN_ARENA_DEFAULT_ALIGNMENT);
    offset -= (u64)arena->buffer;

    if (offset + push_size <= arena->cap)
    {
        result = &arena->buffer[offset];
        arena->pos_prev = arena->pos;
        arena->pos = offset + push_size;
        memset(result, 0, push_size);
    }

    assertln(result != 0, "Arena push: Memory out of bounds.");
    return result;
}

void* AWN_ArenaResize(arena_t *arena, void* old_memory, u64 old_size, u64 new_size)
{
    u8* old_mem = (u8 *)old_memory;
    void* result = 0;

    // NOTE(Alex): If neither of these is true, memory is out of bounds.
    if (old_mem == 0 || old_size == 0)
    {
        return AWN_ArenaPush(arena, new_size);
    }
    // NOTE(Alex): This checks whether the old memory could actually be inside the arena's buffer.
    else if (arena->buffer <= old_mem && old_mem < arena->buffer + arena->cap)
    {
        // NOTE(Alex): Checks if the old memory was our last allocation.
        if (arena->buffer + arena->pos_prev == old_mem)
        {
            arena->pos = arena->pos_prev + new_size;
            // NOTE(Alex): If we're making the allocation larger, make sure we reset the data to zero.
            if (new_size > old_size)
            {
                memset(&arena->buffer[arena->pos], 0, new_size - old_size);
            }
            result = old_memory;
        }
        else
        {
            // NOTE(Alex): Because pos_prev does not align, that means that this was an even earlier allocation.
            //              Therefore we will just allocate new memory and copy old data in new buffer.
            //              This does mean that we lost the buffer we had already allocated, but that is just
            //              how arena allocators work.
            void *new_memory = AWN_ArenaPush(arena, new_size);
            u64 copy_size = old_size < new_size ? old_size : new_size;
            memcpy(new_memory, old_memory, copy_size);
            result = new_memory;
        }
    }

    assertln(result != 0, "Arena resize: Memory out of bounds.");
    return result;
}

void AWN_ArenaClear(arena_t *arena)
{
    arena->pos = 0;
    arena->pos_prev = 0;
}

void AWN_ArenaGrow(arena_t *arena, void* new_buffer, u64 new_size)
{
    if (new_size < arena->cap)
    {
        return;
    }

    memset(new_buffer, 0, new_size);
    memcpy(new_buffer, arena->buffer, arena->cap);

    arena->buffer = (u8 *)new_buffer;
    arena->cap = new_size;
}

void AWN_ArenaShrink(arena_t *arena, void* new_buffer, u64 new_size)
{
    if (new_size < arena->pos)
    {
        new_size = arena->pos;
    }

    memset(new_buffer, 0, new_size);
    memcpy(new_buffer, arena->buffer, arena->cap);

    arena->buffer = (u8 *)new_buffer;
    arena->cap = new_size;
}

arena_state_t AWN_ArenaStateRecord(arena_t *arena)
{
    arena_state_t state;
    state.arena = arena;
    state.pos_prev = arena->pos_prev;
    state.pos_cur = arena->pos;
    return state;
}

void AWN_ArenaStateRestore(arena_state_t state)
{
    state.arena->pos_prev = state.pos_prev;
    state.arena->pos = state.pos_cur;
}

#endif
