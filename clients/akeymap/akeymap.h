void createMacroAscii(int a, int b, int c, int d, InterceptionKeyStroke keyMacro[100], int &keyMacroLength);
void createMacroKeypad(InterceptionKeyStroke  stroke, InterceptionKeyStroke  keyMacro[100], int &keyMacroLength);
void scancode2stroke(unsigned short scancode,  InterceptionKeyStroke &istroke);

#define BITMASK_LSHIFT 0x01
#define BITMASK_RSHIFT 0x10
#define BITMASK_LCONTROL 0x02
#define BITMASK_RCONTROL 0x20
#define BITMASK_LALT 0x04
#define BITMASK_RALT 0x40
#define BITMASK_LWIN 0x08
#define BITMASK_RWIN 0x80

#define IS_SHIFT_DOWN (modiState & 0x01 || modiState & 0x10)
