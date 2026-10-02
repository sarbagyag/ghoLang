#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl()
{
    char line[1024];

    for (;;)
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char *readStream(FILE *file, const char *path)
{
    size_t capacity = 4096;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity);

    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    for (;;)
    {
        if (length + 1 >= capacity)
        {
            capacity *= 2;
            char *newBuffer = (char *)realloc(buffer, capacity);

            if (newBuffer == NULL)
            {
                free(buffer);
                fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
                exit(74);
            }
            buffer = newBuffer;
        }

        size_t bytesRead = fread(buffer + length, sizeof(char), capacity - length - 1, file);
        length += bytesRead;

        if (bytesRead == 0)
            break;
    }

    buffer[length] = '\0';
    return buffer;
}

static char *readFile(const char *path)
{

    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    // ftell()/fseek() are only meaningful on seekable streams. A pipe (e.g.
    // `/dev/stdin`) reports failure or a bogus size, so fall back to an
    // incremental read instead of trusting the reported size.
    long size = -1;
    if (fseek(file, 0L, SEEK_END) == 0)
    {
        size = ftell(file);
    }

    char *buffer;

    if (size < 0)
    {
        buffer = readStream(file, path);
    }
    else
    {
        rewind(file);
        size_t fileSize = (size_t)size;
        buffer = (char *)malloc(fileSize + 1);

        if (buffer == NULL)
        {
            fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
            exit(74);
        }

        size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
        buffer[bytesRead] = '\0';
    }

    fclose(file);
    return buffer;
}

static void runFile(const char *path)
{

    char *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);

    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char *argv[])
{
    initVM();

    if (argc == 1)
    {
        repl();
    }
    else if (argc == 2)
    {
        runFile(argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}