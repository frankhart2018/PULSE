#ifndef pulse_vm_h
#define pulse_vm_h

#include "iota.h"
#include "value.h"

typedef struct {
  Iota* iota;
  uint8_t* ip;
  Value *stack;
  int top;
  int length;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(VM* vm);
void freeVM(VM* vm);
InterpretResult interpret(VM* vm, Iota* iota);
void push(VM* vm, Value value);
Value pop(VM* vm);

#endif
