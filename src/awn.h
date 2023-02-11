#ifndef AWN_H
#define AWN_H

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Types
#include <stdint.h>
#include <stddef.h>

#define function static
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

typedef size_t usize;

typedef float f32;
typedef double f64;

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Some syntatic stuff for C.

#ifndef __cplusplus

    #define and &&
    #define or ||
    #define not !

#include <stdbool.h>

#endif

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Struct define

#define NEED_STRUCT(name)\
typedef struct _##name name

#define STRUCT(name)\
typedef struct _##name name;\
struct _##name

#define NEED_UNION(name)\
typedef union _##name name

#define UNION(name)\
typedef union _##name name;\
union _##name

#define NEED_ENUM(name)\
typedef enum _##name name

#define ENUM(name)\
typedef enum _##name name;\
enum _##name

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Macros that make grepping easier.

#define cast(type, var) ((type))(var)
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define clamp(mi, mx, c) (max(min((c), mi), mx))
#define clamp01(a) clamp(0, 1, a)

#define lerp(a, b, t) ((1 - (t)) * (a) + (t) * (b))

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Array functions

#define AWN_ArrayCount(array) (sizeof(array) / sizeof(*(array)))

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Assert and todo macros.

#include <assert.h>

#define assertln(c, m) assert(c && m)
#define todo TODO!!!
#define todoln() assert(0 && "This code is yet to be completed.")

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Rust and C++ style optionals

#define Option(type) opt_##type
#define DefineOptional(type) typedef struct { bool present; type value; } Option(type)
#define DefineNamedOptional(type, name) \
        typedef type name; \
        DefineOptional(name)
#define Some(type, val) ( \
        (Option(type)) { \
            .present = true, \
            .value = val, \
        } \
)
#define None(type) ( \
    (Option(type)) { \
        .present = false, \
        .value = 0, \
    } \
)
#define IsSome(option) (option.present)
#define IsNone(option) (!IsSome(option))
#define Unwrap(option, val) ( \
    (IsSome(option) ? (val = option.value, true) : (false)) \
)
#define AssertUnwrap(option, val) \
        if (IsNone(option)) { \
            val = 0; \
            assertln(false, "Failed to unwrap"); \
        } else { \
            Unwrap(option, val); \
        }

// These are all the optionals for standard types.
DefineOptional(int);
DefineOptional(float);
DefineOptional(char);
DefineOptional(bool);

DefineOptional(i8);
DefineOptional(i16);
DefineOptional(i32);
DefineOptional(i64); 

DefineOptional(u8);
DefineOptional(u16);
DefineOptional(u32);
DefineOptional(u64); 
DefineOptional(usize);

DefineOptional(f32);
DefineOptional(f64);

////////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): List functions

#define AWN_SLLPushBack(f, l, n) ( ((f) == (0)) ? ((f) = (l) = (n),\
            (n)->next = 0) : ((l)->next = (n),\
            (l) = (n), (n)->next = 0) )
#define AWN_SLLPushFront(f, l, n) ( ((f) == (0)) ? ((f) = (l) = (n),\
            (n)->next = 0) : ((n)->next = (f),\
            (f) = (n)) ) 
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
// NOTE(Alex): Arena type (https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002)
// TODO(Alex): Should Arena work with Optionals?

STRUCT(arena_t)
{
    u8 *buffer;
    usize pos;
    usize pos_prev;
    usize cap;
    bool auto_grow;
};

#define AWN_ARENA_AUTOGROW_ENABLED 1

DefineOptional(arena_t);

arena_t AWN_ArenaCreateFromBuffer(void* mem_buffer, usize mem_size);
arena_t AWN_ArenaCreate(usize mem_size);
arena_t AWN_ArenaCreateEmpty();
void* AWN_ArenaPush(arena_t *arena, usize push_size);
void* AWN_ArenaResize(arena_t *arena, void* old_memory, usize old_size, usize new_size);
void AWN_ArenaClear(arena_t *arena);
void AWN_ArenaGrow(arena_t *arena, usize new_size);
void AWN_ArenaGrowFromBuffer(arena_t *arena, void* new_buffer, usize new_size);
void AWN_ArenaShrink(arena_t *arena, usize new_size);
void AWN_ArenaShrinkFromBuffer(arena_t *arena, void* new_buffer, usize new_size);

// NOTE(Alex): This is the system for recording arena states (for temporary allocations).
//              Allows us to use the linear allocator like a stack allocator.
STRUCT(arena_state_t)
{
    arena_t *arena;
    usize pos_prev;
    usize pos_cur;
};

DefineOptional(arena_state_t);

arena_state_t AWN_ArenaStateRecord(arena_t *a);
void AWN_ArenaStateRestore(arena_state_t);

#endif // End of header.

#ifdef AWN_IMPLEMENTATION

///////////////////////////////////////////////////////////////////////////////
// NOTE(Alex): Arena allocator implementation

#include <stdlib.h>

arena_t AWN_ArenaCreateFromBuffer(void* mem_buffer, usize mem_size)
{
    arena_t arena;
    arena.buffer = (u8 *)mem_buffer;
    arena.cap = mem_size;

    arena.pos = 0;
    arena.pos_prev = 0;
#ifdef AWN_ARENA_AUTOGROW_ENABLED
    arena.auto_grow = true;
#else
    arena.auto_grow = false;
#endif
    return arena;
}

arena_t AWN_ArenaCreate(usize mem_size)
{
    void* buffer = malloc(mem_size);
    assertln(buffer != NULL, "Arena: Failed to allocate memory.");
    return AWN_ArenaCreateFromBuffer(malloc(mem_size), mem_size);
}

arena_t AWN_ArenaCreateEmpty()
{
    return AWN_ArenaCreate(2);
}

// NOTE(Alex): We would prefer it if the user never manually frees an arena buffer.
void AWN_ArenaFree(arena_t arena)
{
    if (arena.buffer != NULL) {
        free(arena.buffer);
    }
}

void* AWN_ArenaPush(arena_t *arena, usize push_size)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    assertln(push_size > 0, "Arena push: Push size is zero.");
    void *result = 0;

    usize cur_ptr = (usize)arena->buffer + arena->pos;
    usize offset = AWN_ARENA_ALIGN_UP_POW_2(cur_ptr, AWN_ARENA_DEFAULT_ALIGNMENT);
    offset -= (usize)arena->buffer;

    if (offset + push_size > arena->cap and arena->auto_grow) {
        AWN_ArenaGrow(arena, offset + push_size);
    }

    if (offset + push_size <= arena->cap) {
        result = &arena->buffer[offset];
        arena->pos_prev = arena->pos;
        arena->pos = offset + push_size;
        memset(result, 0, push_size);
    }

    assertln(result != 0, "Arena push: Memory out of bounds.");
    return result;
}

void* AWN_ArenaResize(arena_t *arena, void* old_memory, usize old_size, usize new_size)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    u8* old_mem = (u8 *)old_memory;
    void* result = 0;

    // NOTE(Alex): If neither of these is true, memory is out of bounds.
    if (old_mem == 0 || old_size == 0) {
        return AWN_ArenaPush(arena, new_size);
    } else if (arena->buffer <= old_mem && old_mem < arena->buffer + arena->cap) {
        // NOTE(Alex): This checks whether the old memory could actually be inside the arena's buffer.
        //
        // NOTE(Alex): Checks if the old memory was our last allocation.
        if (arena->buffer + arena->pos_prev == old_mem) {
            arena->pos = arena->pos_prev + new_size;
            // NOTE(Alex): If we're making the allocation larger, make sure we reset the data to zero.
            if (new_size > old_size) {
                memset(&arena->buffer[arena->pos], 0, new_size - old_size);
            }
            result = old_memory;
        } else {
            // NOTE(Alex): Because pos_prev does not align, that means that this was an even earlier allocation.
            //              Therefore we will just allocate new memory and copy old data in new buffer.
            //              This does mean that we lost the buffer we had already allocated, but that is just
            //              how arena allocators work.
            void *new_memory = AWN_ArenaPush(arena, new_size);
            usize copy_size = old_size < new_size ? old_size : new_size;
            memcpy(new_memory, old_memory, copy_size);
            result = new_memory;
        }
    }

    assertln(result != 0, "Arena resize: Memory out of bounds.");
    return result;
}

void AWN_ArenaClear(arena_t *arena)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    arena->pos = 0;
    arena->pos_prev = 0;
}

void AWN_ArenaGrow(arena_t *arena, usize new_size)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    if (new_size < arena->cap) {
        return;
    }

    void* new_buffer = realloc(arena->buffer, new_size);
    assertln(new_buffer != NULL, "Arena: Failed to reallocate memory for the arena.");
    AWN_ArenaGrowFromBuffer(arena, new_buffer, new_size);
}

void AWN_ArenaGrowFromBuffer(arena_t *arena, void* new_buffer, usize new_size)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    if (new_size < arena->cap) {
        return;
    }

    memset(new_buffer, 0, new_size);
    memcpy(new_buffer, arena->buffer, arena->cap);

    arena->buffer = (u8 *)new_buffer;
    arena->cap = new_size;
}

void AWN_ArenaShrink(arena_t *arena, usize new_size)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    if (new_size < arena->pos) {
        new_size = arena->pos;
    }

    void* new_buffer = realloc(arena->buffer, new_size);
    assertln(new_buffer != NULL, "Arena: Failed to reallocate memory for the arena.");
    AWN_ArenaShrinkFromBuffer(arena, new_buffer, new_size);
}

void AWN_ArenaShrinkFromBuffer(arena_t *arena, void* new_buffer, usize new_size)
{
    assertln(arena != NULL and arena->buffer != NULL, "Arena points to null.");
    if (new_size < arena->pos) {
        new_size = arena->pos;
    }

    memset(new_buffer, 0, new_size);
    memcpy(new_buffer, arena->buffer, arena->cap);

    arena->buffer = (u8 *)new_buffer;
    arena->cap = new_size;
}

arena_state_t AWN_ArenaStateRecord(arena_t *arena)
{
    assertln(arena != NULL, "Arena points to null.");
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
