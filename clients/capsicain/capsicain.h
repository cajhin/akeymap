void makeKeyMacro(unsigned short scancode);
void breakKeyMacro(unsigned short scancode);
void makeBreakKeyMacro(unsigned short scancode);

void createMacroKeyCombo(int a, int b, int c, int d);
void createMacroKeyComboRemoveShift(int a, int b, int c, int d);
void createMacroKeyCombo10timesIfAltDown(int a, int b, int c, int d, unsigned short modifiers);
void createMacroKeyComboNtimes(int a, int b, int c, int d, int repeat);

void createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d);
void processCapsTapped();

void playMacro();

void processCoreCommands();
void processRemapModifiers();
void processTrackModifierState();
void processCapsDownAction();
void processRemapCharacterLayout();

void PrintHello();

void scancode2stroke(unsigned short &scancode, InterceptionKeyStroke &istroke);
void normalizeKeyStroke(InterceptionKeyStroke &istroke);
void getHardwareId();
void sendStroke(InterceptionKeyStroke stroke);

void printStatus();
void printHelp();
void reset();

#define IFDEBUG if(mode.debug) 
#define BITMASK_LSHIFT 0x01
#define BITMASK_RSHIFT 0x10
#define BITMASK_LCONTROL 0x02
#define BITMASK_RCONTROL 0x20
#define BITMASK_LALT 0x04
#define BITMASK_RALT 0x40
#define BITMASK_LWIN 0x08
#define BITMASK_RWIN 0x80

#define IS_SHIFT_DOWN (state.modifiers & 0x01 || state.modifiers & 0x10)
#define IS_LSHIFT_DOWN (state.modifiers & 0x01)
#define IS_RSHIFT_DOWN (state.modifiers & 0x10)
#define IS_LCONTROL_DOWN (state.modifiers & 0x02)
#define IS_LALT_DOWN (state.modifiers & 0x04)

