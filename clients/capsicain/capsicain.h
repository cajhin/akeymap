void makeKeyMacro(unsigned short scancode);
void breakKeyMacro(unsigned short scancode);
void makeBreakKeyMacro(unsigned short scancode);

void createMacroKeyCombo(int a, int b, int c, int d);
void createMacroKeyComboRemoveShift(int a, int b, int c, int d);
void createMacroKeyCombo10timesIfAltDown(int a, int b, int c, int d, unsigned short modifiers);
void createMacroKeyComboNtimes(int a, int b, int c, int d, int repeat);

void createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d);
void processCapsTapped(unsigned short scancode);

void playMacro(const InterceptionContext &context, const InterceptionDevice &device);

void processCoreCommands(unsigned short scancode, InterceptionContext &context, InterceptionDevice &device, bool &isCapsDown, bool &isCapsTapped, bool &modeFlipAltWin, bool &modeSlashShift, bool &modeFlipZy);

void processRemapModifiers(bool &isFinalScancode, unsigned short &scancode, bool modeSlashShift, bool isCapsDown, bool modeFlipAltWin);

void processTrackModifierState(unsigned short scancode, bool isDownstroke, bool isCapsDown, bool &blockKey);

void processCapsDownAction(unsigned short &scancode, bool isDownstroke, bool &blockKey, bool &isFinalScancode);

void processRemapCharacterLayout(unsigned short &scancode, bool modeFlipZy);

void scancode2stroke(unsigned short scancode,  InterceptionKeyStroke &istroke);
void normalizeKeyStroke(InterceptionKeyStroke &keystroke);
void getHardwareId(const InterceptionContext &context, const InterceptionDevice &device);
void sendStroke(InterceptionContext context, InterceptionDevice device, InterceptionKeyStroke &stroke);

void printStatus();
void printHelp();
void reset(InterceptionContext context, InterceptionDevice device);

#define IFDEBUG if(modeDebug) 
#define BITMASK_LSHIFT 0x01
#define BITMASK_RSHIFT 0x10
#define BITMASK_LCONTROL 0x02
#define BITMASK_RCONTROL 0x20
#define BITMASK_LALT 0x04
#define BITMASK_RALT 0x40
#define BITMASK_LWIN 0x08
#define BITMASK_RWIN 0x80

#define IS_SHIFT_DOWN (modiState & 0x01 || modiState & 0x10)
#define IS_LSHIFT_DOWN (modiState & 0x01)
#define IS_RSHIFT_DOWN (modiState & 0x10)
#define IS_LCONTROL_DOWN (modiState & 0x02)
#define IS_LALT_DOWN (modiState & 0x04)

