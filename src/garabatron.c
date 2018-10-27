#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define STACK_MAX 256

// INITIAL_GC_THRESHOLD (a higher number = less time garbage collecting | a smaller number = more conservative with memory)
#define INITIAL_GC_THRESHOLD 8

// Interpreter datatypes
typedef enum {
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

// Object data structure
typedef struct sObject {
    ObjectType type;
    unsigned char marked;
    struct sObject* next;

    union {
        // OBJ_INT
        int value;

        // OBJ_PAIR
        struct {
            struct sObject* x;
            struct sObject* y;
        };
    };
} Object;

// VM stack
typedef struct {
    int numObjects;
    int maxObjects;

    Object* firstObj;
    Object* stack[STACK_MAX];
    int stackSize;
} VM;

void assert(int condition, const char* message) {
    if (!condition) {
        printf("%s\n", message);
        exit(1);
    }
}

// Inititate the virtual machine stack.
VM* initVM() {
    VM* vm = malloc(sizeof(VM));
    vm->numObjects = 0;
    vm->firstObj = NULL;
    vm->maxObjects = INITIAL_GC_THRESHOLD;
    vm->stackSize = 0;
    return vm;
}

// Appends object to the end of the stack
void vmPushStack(VM* vm, Object* value) {
    assert(vm->stackSize < STACK_MAX, "CRITICAL ERROR: Stack overflow");
    vm->stack[vm->stackSize++] = value;
}


// Returns the object in the stack situated at the index of stackSize - 1
Object* vmPoppedStack(VM* vm) {
    assert(vm->stackSize > 0, "CRITICAL ERROR: Stack underflow");
    return vm->stack[--(vm->stackSize)];
}

void objMark(Object* obj) {
    if (obj->marked) return;

    obj->marked = 1;

    if (obj->type == OBJ_PAIR) {
        objMark(obj->x);
        objMark(obj->y);
    }
}


void vmMarkAll(VM* vm) {
    for (int i = 0; i < vm->stackSize; i++) {
        objMark(vm->stack[i]);
    }
}

void vmSweep(VM* vm) {
    int sweepStage = 1;
    Object** obj = &vm->firstObj;
    while (*obj) {
        if (!(*obj)->marked) {
            // object couldnt be reached, thus memory is ripe for the taking
            Object* garbage = *obj;
            *obj = garbage->next;
            free(garbage);

            FILE *logFile = fopen("gc_output.log", "a");
            fprintf(logFile, "\n[sweep %d] cleared marked object", sweepStage);

            vm->numObjects--;
            sweepStage++;
        }
        else {
            (*obj)->marked = 0;
            obj = &(*obj)->next;
        }
    }
}


void gcStart(VM* vm) {
    int numObjects = vm->numObjects;

    vmMarkAll(vm);
    vmSweep(vm);

    vm->maxObjects = vm->numObjects * 2;

    FILE *logFile = fopen("gc_output.log", "a");

    fprintf(logFile, "\nCollected %d objects, %d remaining", numObjects - vm->numObjects, vm->numObjects);
}


// Instantiates our object in our virtual machine
Object* instantiateObject(VM* vm, ObjectType T) {
    if (vm->numObjects == vm->maxObjects) {
        gcStart(vm);
    }

    Object* obj = malloc(sizeof(Object));
    obj->marked = 0;
    obj->type = T;

    // Add to list of allocations within the vm
    obj->next = vm->firstObj;
    vm->firstObj = obj;

    vm->numObjects++;


    return obj;
}

// Moves the integer version of our object to the end of the stack.
void vmPushInt(VM* vm, int i) {
    Object* obj = instantiateObject(vm, OBJ_INT);
    obj->value = i;
    vmPushStack(vm, obj);
}

// Moves the pair version of our object to the end of the stack and returns it
Object* vmPushPair(VM* vm) {
    Object* obj = instantiateObject(vm, OBJ_PAIR);
    obj->x = vmPoppedStack(vm);
    obj->y = vmPoppedStack(vm);

    vmPushStack(vm, obj);
    return obj;
}

void vmFree(VM* vm) {
    vm->stackSize = 0;
    gcStart(vm);
    free(vm);
}

int main(const char* argv[]) {

    FILE *logFile = fopen("gc_output.log", "w");

    VM* vm = initVM();
    vmPushInt(vm, 1);
    vmPushInt(vm, 2);
    vmPushPair(vm);
    vmPushInt(vm, 3);
    vmPushInt(vm, 4);
    vmPushPair(vm);
    vmPushPair(vm);

    gcStart(vm);
    assert(vm->numObjects == 7, "Should have reached objects.");
    vmFree(vm);

    fclose(logFile);
}