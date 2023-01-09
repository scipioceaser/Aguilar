/*
    Aguilar is a single command-line application that makes using C as a scripting language a lot easier.
    Aguilar is not itself a C compiler, and instead calls either the GNU or LLVM compilers.

    Commands:
        - new [name]: Create a new project based on a predefined template.
        - build (file): Build either a file or a project based on whether it can find a config file.
        - sync: Update an existing repository with any changes made to template files.   
        - run [file]: Build a single file (application is stored in a cache) and run it.
        - install: Install the application in the user's bin folder.
        - help: Print everything you need to know.
        - zen: Print a zen of code.
*/

// TODO(Alex): Define standard paths inside platform-specific macros.
// TODO(Alex): Code documentation.
// TODO(Alex): Structs for function arguments + cleaner handling of command line arguments.

// WARNING(Alex): This only works on Linux (Maybe MacOS?).

#define AWN_IMPLEMENTATION
#include "awn.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define AGUILAR_VERSION "0.1"

internal bool Aguilar_FileExists(const char* file, struct stat *sb)
{
    struct stat *ptr;
    struct stat tmp;

    if (sb == 0) {
        ptr = &tmp;
    } else {
        ptr = sb;
    }

    if (stat(file, ptr) == -1) {
        return false;
    }

    return true;
}

#define ERROR_STR_LEN 1024
char __error[ERROR_STR_LEN] = { 0 };

internal void Aguilar_SetError(char* error)
{
    if (strlen(error) < ERROR_STR_LEN) {
        memcpy(__error, error, strlen(error));
    }
}

internal char* Aguilar_GetError()
{
    return __error;
}

#define ENV_COMPILER "AGUILAR_COMPILER"

internal char* Aguilar_GetCompilerEnv()
{
    if (getenv(ENV_COMPILER) != NULL) {
        const char* env = getenv(ENV_COMPILER);
        
        if (strcmp(env, "GCC") == 0 or strcmp(env, "gcc") == 0) {
            return "gcc";
        }

        if (strcmp(env, "CLANG") == 0 or strcmp(env, "clang") == 0) {
            return "clang";
        }
    }

    return "gcc";
}

internal int Aguilar_NewProject(arena_t *arena, char* name)
{
    if (Aguilar_FileExists(name, 0)) {
        Aguilar_SetError("Directory already exists!");
        return -1;
    }

    if (mkdir(name, S_IRWXG | S_IRWXO | S_IRWXU) != 0) {
        Aguilar_SetError("System failed to create new directory!");
        return -1;
    }

    char* src_path = AWN_ArenaPush(arena, sizeof(char) * (strlen(name) + strlen("src") + 1));

    sprintf(src_path, "%s/%s", name, "src");

    int success = mkdir(src_path, S_IRWXG | S_IRWXO | S_IRWXU);

    // TODO(Alex): Delete parent directory on failure.
    if (success != 0) {
        Aguilar_SetError("System failed to create new directory!");
        return -1;
    }

    char* main_path = AWN_ArenaPush(arena, sizeof(char) * (strlen(name) * 2 + strlen("/src/") + 2));

    sprintf(main_path, "%s/src/%s.c", name, name);

    char* command = AWN_ArenaPush(arena, sizeof(char) * (strlen(main_path) + 128));

    sprintf(command, "cp ~/.local/bin/Aguilar_data/main.c %s", main_path);

    success = system(command);
    
    if (success != 0) {
        Aguilar_SetError("System command failed!");
        return -1;
    }

    sprintf(command, "rsync -a ~/.local/bin/Aguilar_data/ %s/src/ --exclude=main.c", name);
    success = system(command);

    if (success != 0) {
        Aguilar_SetError("System command failed!");
        return -1;
    }

    return 1;
}

internal int Aguilar_SyncProject(arena_t* arena)
{
    if (!Aguilar_FileExists("src", 0)) {
        Aguilar_SetError("Not inside project directory!");
        return -1;
    }

    int success = system("rsync -a ~/.local/bin/Aguilar_data/ src/ --exclude=main.c");

    if (success != 0) {
        Aguilar_SetError("Command failed!");
        return -1;
    }

    return 0;
}

#define CALL_GCC "gcc"
#define CALL_CLANG "clang"
#define ARG_OUTPUT "-o"

internal int Aguilar_RunBuildInstruction(arena_t *arena, char* source, char* args, char* output)
{
    const char* compiler = Aguilar_GetCompilerEnv();

    size_t command_length = strlen(source) + strlen(compiler) + strlen(ARG_OUTPUT);

    // NOTE(Alex): Run with default options.
    if (args == 0) {
        args = "-Wall -g -O3";
    }

    command_length += strlen(args);

    const char* output_file = "app.out";

    if (output != 0) {
        output_file = output;
    }

    char* command = AWN_ArenaPush(arena, sizeof(char) * (command_length + 6 + strlen(output_file)));

    sprintf(command, "%s %s -o %s %s", compiler, args, output_file, source);

    printf("%s\n", command);

    int res = system(command);

    return res;
}

internal int Aguilar_Build(arena_t *arena)
{
    if (Aguilar_FileExists("build.sh", 0)) {
        system("./build.sh");
        return 1;
    }

    if (Aguilar_FileExists("Makefile", 0)) {
        system("make");
        return 1;
    }

    if (!Aguilar_FileExists("src", 0)) {
        Aguilar_SetError("No src directory found, cannot run!");
        return -1;
    }

    DIR *src_dir = opendir("src");

    if (src_dir == NULL) {
        Aguilar_SetError("Could not open the source directory!");
        return -1;
    }

    struct dirent *entry;

    char* path = AWN_ArenaPush(arena, 1);

    bool entry_found = false;

    while ((entry = readdir(src_dir)) != NULL and !entry_found) {
        path = AWN_ArenaResize(arena, path, (arena->pos - arena->pos_prev), sizeof(char) * (strlen("src/") + sizeof(entry->d_name)));

        sprintf(path, "src/%s", entry->d_name);

        FILE *file = fopen(path, "r");

        if (file != NULL) {
            char buff[1024];

            // TODO(Alex): There has to be a better way to do this.
            while (fgets(buff, sizeof(buff), file) != NULL) {
                if (strncmp(buff, "int main(", 9) == 0) {
                    entry_found = true;
                }
            }
        }
        
        fclose(file);
    }

    char cwd[128];
    getcwd(cwd, 128);

    int offset = 0;
    for (int i = 0; i < strlen(cwd); i++) {
        if (cwd[i] == '/') {
            offset = i + 1;
        }
    }

    char* out = AWN_ArenaPush(arena, sizeof(char) * (strlen(cwd) - offset + 1));

    memcpy(out, (cwd + offset), (strlen(cwd) - offset) );
    out[strlen(cwd) - offset] = '\0';

    Aguilar_RunBuildInstruction(arena, path, 0, out);

    closedir(src_dir);

    AWN_ArenaClear(arena);

    return 1;
}

#define CACHE_OUT_PATH "~/.cache/aguilar_program.out"
#define CACHE_CONFIG_PATH "/.cache/aguilar_program.cache"

internal char* Aguilar_FormatCacheSettingsPath(arena_t *arena)
{
    const char* home = getenv("HOME");
    if (home == NULL) {
        Aguilar_SetError("Failed to get home directory!");
        return NULL;
    }

    char* config_path = AWN_ArenaPush(arena, sizeof(char) * (strlen(CACHE_CONFIG_PATH) + strlen(home)));
    sprintf(config_path, "%s%s", home, CACHE_CONFIG_PATH);

    return config_path;
}

internal int Aguilar_WriteCacheSettings(arena_t *arena, const char* file, u64 mod_time)
{
    const char* config_path = Aguilar_FormatCacheSettingsPath(arena);
    if (config_path == NULL) {
        return -1;
    }

    FILE* settings_file = fopen(config_path, "w+");
    if (settings_file == NULL) {
        Aguilar_SetError("Failed to write cache settings file!");
        return -1;
    }

    fwrite(file, sizeof(char), strlen(file), settings_file);
    fwrite("\n", sizeof(char), 1, settings_file);

    char* mod_str = AWN_ArenaPush(arena, sizeof(char) * 512);

    snprintf(mod_str, sizeof(char) * 512, "%lu", mod_time);

    fwrite(mod_str, sizeof(char), strlen(mod_str), settings_file);

    fclose(settings_file);

    return 0;
}

internal int Aguilar_ReadCacheSettings(arena_t *arena, char** file, u64* mod_time, size_t string_max)
{
    const char* config_path = Aguilar_FormatCacheSettingsPath(arena);
    if (config_path == NULL) {
        *file = "";
        *mod_time = 0;
        return -1;
    }

    FILE* settings_file = fopen(config_path, "r");
    if (settings_file == NULL) {
        *file = "";
        *mod_time = 0;
        return 0;
    }

    const int file_buffer_size = sizeof(char) * string_max;

    if (*file == 0) {
        *file = AWN_ArenaPush(arena, file_buffer_size);
    }

    // TODO(Alex): At the moment we are assuming that the settings file 
    //              has been written in a specific order defined 
    //              in the write function.
    char line[string_max];
    fgets(line, string_max, settings_file);

    if (line[strlen(line) - 1] == '\n') {
        line[strlen(line) - 1] = '\0';
        strncpy(*file, line, file_buffer_size - strlen(*file) - 1);
    } else {
        strncpy(*file, line, file_buffer_size - strlen(*file) - 1);
    }

    fgets(line, string_max, settings_file);

    *mod_time = strtoul(line, 0, 0);

    return 0;
}

internal int Aguilar_Run(arena_t *arena, char* file, char* arg)
{
    struct stat sb;
    if (!Aguilar_FileExists(file, &sb)) {
        Aguilar_SetError("File does not exist!");
        return -1;
    }

    char* f = AWN_ArenaPush(arena, sizeof(char) * 1024);
    u64 m = 0;
    if (Aguilar_ReadCacheSettings(arena, &f, &m, 1024) != 0) {
        return -1;
    }

    if (strcmp(f, file) != 0 or m != sb.st_mtime) {
        int ret = Aguilar_RunBuildInstruction(arena, file, arg, CACHE_OUT_PATH);

        if (ret != 0) {
            Aguilar_SetError("Compiler encountered an error!");
            return -1;
        }

        if (Aguilar_WriteCacheSettings(arena, file, sb.st_mtime) != 0) {
            return -1;
        }
    } else {
        printf("No changes, not recompiling!\n");
    }

    system(CACHE_OUT_PATH);

    AWN_ArenaClear(arena);

    return 1;
}

internal int Aguilar_WriteBasicMainFile(const char* path)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        printf("At: %s, ", path);
        Aguilar_SetError("System failed to create new file!");
        return -1;
    }

    const char* basic_program = "int main(int argc, char** argv)\n{\n   return 0;\n}\n";
    fwrite(basic_program, sizeof(char), strlen(basic_program), file);

    fclose(file);

    return 1;
}

internal int Aguilar_Install(arena_t *arena)
{
    char cwd[128] = { 0 };
    getcwd(cwd, 128);

    char* command = AWN_ArenaPush(arena, sizeof(char) * 1024);

    sprintf(command, "cp %s/Aguilar ~/.local/bin/", cwd);

    int success = system(command);

    if (success != 0) {
        Aguilar_SetError("Failed to run system call!");
        return -1;
    }

    const char* home_dir = getenv("HOME");   
    
    if (home_dir == NULL) {
        Aguilar_SetError("Failed to get home directory!");
        return -1;
    }

    char* path = AWN_ArenaPush(arena, sizeof(char) * (strlen(home_dir) + 512));

    sprintf(path, "%s/.local/bin/Aguilar_data/", home_dir);

    const char* fname = "main.c";
    if (!Aguilar_FileExists(path, 0)) {
        if (mkdir(path, S_IRWXG | S_IRWXO | S_IRWXU) != 0) {
            Aguilar_SetError("System failed to create new directory!");
            return -1;
        }
    }

    strncat(path, fname, strlen(fname) + 1);

    if (Aguilar_WriteBasicMainFile(path) == -1) {
        return -1;
    }

    AWN_ArenaClear(arena);

    return 1;
}

internal void Aguilar_Help()
{
    printf("Aguilar is a single command-line application that makes using C as a scripting language a lot easier.\n");
    printf("Aguilar is not itself a C compiler, and instead calls either the GNU or LLVM compilers.\n");
    printf("\n");
    printf("Version: %s\n", AGUILAR_VERSION);
    printf("\n");
    printf("Commands:\n");
    printf("\n");
    printf("    - new [name]: Create a new project based on a predefined template.\n");
    printf("    - build (file): Build either a file or a project based on whether it can find a config file.\n");
    printf("    - sync: Update an existing repository with any changes made to template files.\n");
    printf("    - run [file]: Build a single file (application is stored in a cache) and run it.\n");
    printf("    - install: Install the application in the user's bin folder.\n");
    printf("    - help: Print everything you need to know.\n");
    printf("    - zen: Print a zen of code.\n");
}

internal void Aguilar_Zen()
{
    printf("The rules of zen:\n");
    printf("    1. Build your code with a reader in mind.\n");
    printf("    2. A compiler failure is better than a runtime error.\n");
}

internal char* Aguilar_MergeArgs(arena_t *arena, char** args, int merge_offset, int merge_num)
{
    size_t merge_size = 0;

    for (int i = merge_offset; i < merge_num; i++) {
        merge_size += strlen(args[i]) + 1;
    }

    if (merge_size == 0) {
        return 0;
    }

    const int dest_size = sizeof(char) * merge_size;
    char* merge = AWN_ArenaPush(arena, dest_size);

    for (int i = merge_offset; i < merge_num; i++) {
        strncat(merge, args[i], dest_size - strlen(merge) - 1);
        strncat(merge, " ", 2);
    }

    return merge;
}

int main(int argc, char** argv)
{
    if (argc < 2 or strlen(argv[1]) < 1) {
        Aguilar_Help();
        return 0;
    }

    arena_t arena;
    AWN_ArenaCreate(&arena, malloc(KB(128)), KB(128));

    switch (argv[1][0])
    {
        case 'n': {
            if (argc > 2 and strlen(argv[2]) > 0) {
                if (Aguilar_NewProject(&arena, argv[2]) < 0) {
                    printf("Failed to create new project: %s\n", Aguilar_GetError());
                }
            }
        } break;
        case 'b': {
            if (Aguilar_Build(&arena) < 0) {
                printf("Failed to build: %s\n", Aguilar_GetError());
            }
        } break;
        case 's': {
            if (Aguilar_SyncProject(&arena) < 0) {
                printf("Failed to sync: %s\n", Aguilar_GetError());
            }
        } break;
        case 'r': {
            if (argc > 2 and strlen(argv[2]) > 0) {
                char* arg = Aguilar_MergeArgs(&arena, argv, 3, argc);

                if (Aguilar_Run(&arena, argv[2], arg) < 0) {
                    printf("Failed to run: %s\n", Aguilar_GetError());
                }
            } else {
                printf("Need to specify file to run!\n");
            }
        } break;
        case 'i': { 
            if (Aguilar_Install(&arena) < 0) {
                printf("Failed to install: %s\n", Aguilar_GetError());
            }
        } break;
        case 'z': Aguilar_Zen(); break;
        default: case 'h': Aguilar_Help();
    }

    free(arena.buffer);
}
