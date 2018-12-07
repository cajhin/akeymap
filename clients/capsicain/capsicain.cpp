//TODOvoid createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d, InterceptionKeyStroke *keyMacro, int &keyMacroLength);

//ctrl caps pgup
//alt + cursor = 10
//config file


#include <string>
#include <iostream>
#include <Windows.h>  //for Sleep()

#include <interception.h>
#include <utils.h>

#include "scancodes.h"
#include "capsicain.h"

using namespace std;

string version = "10";

enum KEYSTATE
{
    KEYSTATE_DOWN = 0,
    KEYSTATE_UP = 1,
    KEYSTATE_EXT_DOWN = 2,
    KEYSTATE_EXT_UP = 3,
};

enum CREATE_CHARACTER_MODE
{
    IBM,   //alt + 123
    ANSI,  //alt + 0123
    AHK,   //F14|F15 + char, let AHK handle it
};

const int MAX_KEYMACRO_LENGTH = 10000;  //for testing; in real life, 100 keys = 200 up/downs should be enough
const int DEFAULT_DELAY_SENDMACRO = 5;  //local needs ~1ms, Linux VM 5+ms, RDP fullscreen 10+ for 100% reliable keystroke detection
unsigned int delayBetweenMacroKeysMS = DEFAULT_DELAY_SENDMACRO;  //AHK drops keys when they are sent too fast

bool modeDebug = false;
unsigned short modiState = 0;
bool keysDownReceived[256];
bool keysDownSent[256];
int  activeLayer = 1;
string deviceIdKeyboard = "";
bool deviceIsAppleKeyboard = false;
CREATE_CHARACTER_MODE characterCreationMode = IBM;


string errorLog = "";
void error(string txt)
{
    cout << endl << "ERROR: " << txt;
    errorLog += "\r\n" + txt;
}

void SetupConsoleWindow() {

    //disable quick edit
    HANDLE Handle = GetStdHandle(STD_INPUT_HANDLE);      // Get Handle
    DWORD mode;
    GetConsoleMode(Handle, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(Handle, mode);

    system("color 8E");  //byte1=background, byte2=text
    SetConsoleTitle(("Capsicain v"+version).c_str());
}

int main()
{
    SetupConsoleWindow();

    InterceptionContext context;
    InterceptionDevice device;
    InterceptionKeyStroke stroke, previousStroke;

    bool modeSlashShift = true;
    bool modeFlipZy = true;
    bool modeFlipAltWin = true;

    bool capsDown = false;
    bool capsTapped = false;

    InterceptionKeyStroke keyMacro[MAX_KEYMACRO_LENGTH];
    int keyMacroLength;  // >0 triggers sending of macro instead of scancode. MUST match the actual # of chars in keyMacro
    wchar_t hardware_id[500];

#pragma region setup
    Sleep(700); //time to release Win from AHK Start shortcut
    raise_process_priority();
    context = interception_create_context();
    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    cout << "Capsicain v" << version << endl << endl
         << "[LCTRL] + [CAPS] + [ESC] to stop." << endl
         << "[LCTRL] + [CAPS] + [H] for Help";

    {
        cout << endl << endl << (modeSlashShift ? "ON :" : "OFF:") << "Slashes -> Shift ";
        cout << endl << (modeFlipZy ? "ON :" : "OFF:") << "Z<->Y ";
        cout << endl <<  endl << "capsicain running..." << endl;

        previousStroke.code = 0;
    }
#pragma endregion setup 


    while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0)
    {
        bool blockKey = false;  //true: do not send the current key
        bool isFinalScancode = false;  //true: don't remap the scancode anymore
        bool isDownstroke = false;
        bool isExtendedCode = false;
        keyMacroLength = 0;


        //check device ID
        if (deviceIdKeyboard.length()<2)
        {
            cout << endl << "detecting keyboard...";
            getHardwareId(context, device);
            modeFlipAltWin = !deviceIsAppleKeyboard;
            cout << endl << (deviceIsAppleKeyboard ? "APPLE keyboard" : "IBM keyboard (flipping Win<>Alt)");
        }

        //evaluate and normalize the stroke
        isDownstroke = (stroke.state & 1) == 1 ? false : true;
        isExtendedCode = (stroke.state & 2) == 2;

        if (stroke.code > 0xFF)
        {
            error("Received unexpected stroke.code > 255: " + stroke.code);
            continue;
        }
        if (stroke.code & 0x80)
        {
            error("Received unexpected extended stroke.code > 0x79: " + stroke.code);
            continue;
        }

        unsigned short scancode = stroke.code;  //scancode will be altered, stroke won't
        if (isExtendedCode)  //I track the extended bit in the high bit of the scancode. Assuming Interception never sends stroke.code > 0x7F
            scancode |= 0x80;

        keysDownReceived[scancode] = isDownstroke;

#pragma region core commands
        //command stroke: RCTRL + CAPS + stroke

        //TODO: double-check that caps and rcontrol are never left hanging on down, from Windows point of view
        if  (isDownstroke 
            && (scancode != SC_CAPS && scancode != SC_RCONTROL) 
            && (keysDownReceived[SC_CAPS] && keysDownReceived[SC_RCONTROL])
            )
        {
            if (scancode == SC_ESCAPE)
            {
                break;  //break the main while() loop, exit
            }

            cout << endl << endl << "::";

            switch (scancode)
            {
            case SC_0:
                activeLayer = 0;
                cout << "LAYER 0 active";
                break;
            case SC_1:
                activeLayer = 1;
                cout << "LAYER 1 active";
                break;
            case SC_2:
                activeLayer = 2;
                cout << "LAYER 2 active";
                break;
            case SC_R:
                reset(keyMacro, keyMacroLength, context, device);
                capsDown = false;
                capsTapped = false;
                getHardwareId(context, device);
                modeFlipAltWin = !deviceIsAppleKeyboard;
                cout << "RESET" << endl << (deviceIsAppleKeyboard ? "APPLE keyboard" : "PC keyboard (flipping Win<>Alt)");
                break;
            case SC_D:
                modeDebug = !modeDebug;
                cout << "DEBUG mode: " << (modeDebug ? "ON" : "OFF");
                break;
            case SC_SLASH:
                modeSlashShift = !modeSlashShift;
                cout << "Slash-Shift mode: " << (modeSlashShift ? "ON" : "OFF");
                break;
            case SC_Z:
                modeFlipZy = !modeFlipZy;
                cout << "Flip Z<>Y mode: " << (modeFlipZy ? "ON" : "OFF");
                break;
            case SC_W:
                modeFlipAltWin = !modeFlipAltWin;
                cout << "Flip ALT<>WIN mode: " << (modeFlipAltWin ? "ON" : "OFF") << endl;
                break;
            case SC_E:
                cout << "ERROR LOG: " << endl << errorLog << endl;
                break;
            case SC_S:
                printStatus();
                break;
            case SC_H:
                printHelp();
                break;
            case SC_C:
                cout << "Character creation mode: ";
                switch (characterCreationMode)
                {
                case IBM:
                    characterCreationMode = ANSI;
                    cout << "ANSI";
                    break;
                case ANSI:
                    characterCreationMode = AHK;
                    cout << "AHK";
                    break;
                case AHK:
                    characterCreationMode = IBM;
                    cout << "IBM";
                    break;
                }
                cout << " (" << characterCreationMode << ")";
                break;
            case SC_LBRACKET:
                if (delayBetweenMacroKeysMS >= 2)
                    delayBetweenMacroKeysMS -= 1;
                cout << "delay between characters in macros (ms): " << dec << delayBetweenMacroKeysMS;
                break;
            case SC_RBRACKET:
                if (delayBetweenMacroKeysMS <= 100)
                    delayBetweenMacroKeysMS += 1;
                cout << "delay between characters in macros (ms): " << dec << delayBetweenMacroKeysMS;
                break;
            default:
                cout << "Unknown command";
                break;
            }
            
            continue;
        }
#pragma endregion core commands

        if (modeDebug)
            cout << endl << " [" << hex << stroke.code << "/" << hex << scancode << " " << stroke.information << " " << stroke.state << "]";

        if (activeLayer == 0)  //standard keyboard, except command strokes
        {
            interception_send(context, device, (InterceptionStroke *)&stroke, 1);
            continue;
        }

        //track but never forward CapsLock action
        if(scancode == SC_CAPS) {
            if (isDownstroke) {
                capsDown = true;
            }
            else
            {
                capsDown = false;
                if (previousStroke.code == SC_CAPS)
                {
                    capsTapped = !capsTapped;
                    if (modeDebug)
                        cout << " capsTap:" << (capsTapped ? "ON " : "OFF");
                }
            }
            previousStroke = stroke;
            continue;
        }

        //remap modifiers
        if(!isFinalScancode)
        {
            isFinalScancode = true;
            switch (scancode)
            {
            case SC_LBSLASH:
                if (modeSlashShift)
                    scancode = SC_LSHIFT;
                break;
            case SC_SLASH:
                if(modeSlashShift && !capsDown)
                    scancode = SC_RSHIFT;
                break;
            case SC_LWIN:
                if (modeFlipAltWin)
                    scancode = SC_LALT;
                break;
            case SC_RWIN:
                if (modeFlipAltWin)
                    scancode = SC_RALT;
                break;
            case SC_LALT:
                if (modeFlipAltWin)
                    scancode = SC_LWIN;
                break;
            case SC_RALT:
                if (modeFlipAltWin)
                    scancode = SC_RWIN;
                break;
            default:
                isFinalScancode = false;
            }

            //evaluate and remember modifiers
            switch (scancode)
            {
            case SC_LSHIFT:  //handle LShift+RShift -> CapsLock
                if (isDownstroke)
                {
                    modiState |= BITMASK_LSHIFT;
                    if (IS_RSHIFT_DOWN && (GetKeyState(VK_CAPITAL) & 0x0001))
                        createMacroKeyCombo(SC_CAPS, 0, 0, 0, keyMacro, keyMacroLength);
                }
                else
                    modiState &= ~BITMASK_LSHIFT;
                break;
            case SC_RSHIFT:
                if (isDownstroke)
                {
                    modiState |= BITMASK_RSHIFT;
                    if (IS_LSHIFT_DOWN && !(GetKeyState(VK_CAPITAL) & 0x0001))
                        createMacroKeyCombo(SC_CAPS, 0, 0, 0, keyMacro, keyMacroLength);
                }
                else
                    modiState &= ~BITMASK_RSHIFT;
                break;
            case SC_LALT:  //suppress LALT in CAPS+LALT combos
                if (isDownstroke)
                {
                    if (modiState & BITMASK_LALT)
                        blockKey = true;
                    else if (capsDown)
                        blockKey = true;
                    modiState |= BITMASK_LALT;
                }
                else
                {
                    if (!(modiState & BITMASK_LALT))
                        blockKey = true;
                    modiState &= ~BITMASK_LALT;
                }
                break;
            case SC_RALT:
                isDownstroke ? modiState |= BITMASK_RALT : modiState &= ~BITMASK_RALT;
                break;
            case SC_LCONTROL:
                isDownstroke ? modiState |= BITMASK_LCONTROL : modiState &= ~BITMASK_LCONTROL;
                break;
            case SC_RCONTROL:
                isDownstroke ? modiState |= BITMASK_RCONTROL : modiState &= ~BITMASK_RCONTROL;
                break;
            case SC_LWIN:
                isDownstroke ? modiState |= BITMASK_LWIN : modiState &= ~BITMASK_LWIN;
                break;
            case SC_RWIN:
                isDownstroke ? modiState |= BITMASK_RWIN : modiState &= ~BITMASK_RWIN;
                break;
            }
            if (modeDebug)
                cout << " [" << hex << modiState << (capsDown ? " C" : " _") << (capsTapped ? "T" : "_") << "] ";
        }

        // pass 3: layout-independent mappings
        if (capsDown && !isFinalScancode)
        {
            //those suppress the key UP because DOWN is replaced with macro
            bool blockingScancode = true;
            switch (scancode)
            {
                //Undo Redo Cut Copy Paste
            case SC_BACK:
                if (isDownstroke) {
                    if (IS_SHIFT_DOWN)
                        createMacroKeyComboRemoveShift(SC_LCONTROL, SC_Y, 0, 0, keyMacro, keyMacroLength);
                    else
                        createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0, keyMacro, keyMacroLength);
                }
                break;
            case SC_A:
                if (isDownstroke)
                    createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_S:
                    if (isDownstroke)
                        createMacroKeyCombo(SC_LCONTROL, SC_X, 0, 0, keyMacro, keyMacroLength);
                    break;
            case SC_D:
                if (isDownstroke)
                    createMacroKeyCombo(SC_LCONTROL, SC_C, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_F:
                if (isDownstroke)
                    createMacroKeyCombo(SC_LCONTROL, SC_V, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_P:
                if (isDownstroke)
                    createMacroKeyCombo(SC_NUMLOCK, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_LBRACKET:
                if (isDownstroke)
                    createMacroKeyCombo(SC_SCROLL, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_RBRACKET:
                if (isDownstroke)
                    createMacroKeyCombo(SC_SYSRQ, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_EQUALS:
                if (isDownstroke)
                    createMacroKeyCombo(SC_INSERT, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_J:
                if (isDownstroke)
                    createMacroKeyCombo(SC_LEFT, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_L:
                if (isDownstroke)
                    createMacroKeyCombo(SC_RIGHT, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_K:
                if (isDownstroke)
                    createMacroKeyCombo(SC_DOWN, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_I:
                if (isDownstroke)
                    createMacroKeyCombo(SC_UP, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_O:
                if (isDownstroke)
                    createMacroKeyCombo(SC_PGUP, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_SEMICOLON:
                if (isDownstroke)
                    createMacroKeyCombo(SC_DELETE, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_DOT:
                if (isDownstroke)
                    createMacroKeyCombo(SC_PGDOWN, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_Y:
                if (isDownstroke)
                    createMacroKeyCombo(SC_HOME, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_U:
                if (isDownstroke)
                    createMacroKeyCombo(SC_END, 0, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_N:
                if (isDownstroke)
                    createMacroKeyCombo(SC_LCONTROL, SC_LEFT, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_M:
                if (isDownstroke)
                    createMacroKeyCombo(SC_LCONTROL, SC_RIGHT, 0, 0, keyMacro, keyMacroLength);
                break;
            case SC_0:
            case SC_1:
            case SC_2:
            case SC_3:
            case SC_4:
            case SC_5:
            case SC_6:
            case SC_7:
            case SC_8:
            case SC_9:
                if (isDownstroke)
                    createMacroKeyCombo(SC_F14, scancode, 0, 0, keyMacro, keyMacroLength);
                break;
            default:
                blockingScancode = false;
            }

            if (blockingScancode)
                blockKey = true;
            else  //direct key remapping without macros
            {
                switch (scancode)
                {
                case SC_H:
                    scancode = SC_BACK;
                    isFinalScancode = true;
                    break;
                case SC_BACKSLASH:
                    scancode = SC_SLASH;
                    isFinalScancode = true;
                    break;
                }
            }
        }

        //not caps down, remap character layout
        else if(!isFinalScancode)
        {
            switch (scancode)
            {
            case SC_Z:
                if (modeFlipZy && !IS_LCONTROL_DOWN) //do not remap LCTRL+Z/Y (undo/redo)
                    scancode = SC_Y;
                break;
            case SC_Y:
                if (modeFlipZy && !IS_LCONTROL_DOWN)  
                    scancode = SC_Z;
                break;
            }
        }

        if (capsTapped) //so far, all caps tap actions are layout dependent (tap+A moves when A moves)
        {
            if (scancode != SC_LSHIFT && scancode != SC_RSHIFT)  //shift does not break the tapped status so I can type äÄ
            {
                if(isDownstroke)
                    processCapsTapped(scancode, keyMacro, keyMacroLength);
                else
                {
                    capsTapped = false;
                    blockKey = true;
                }
            }
        }

        //SEND
        previousStroke = stroke;
        if (keyMacroLength > 0)
        {
            playMacro(keyMacro, keyMacroLength, context, device);
        }
        else
        {
            if (modeDebug)
            {
                if (blockKey)
                    cout << " BLOCKED ";
                else if(stroke.code != (scancode & 0x7F))
                    cout << " -> " << hex << stroke.code << "/" << hex << scancode << " " << stroke.state;
            }
            if (!blockKey) {
                scancode2stroke(scancode, stroke);
                sendStroke(context, device, stroke);
            }
        }
    }

    interception_destroy_context(context);

    cout << endl << "bye" << endl;
    return 0;
}

void playMacro(InterceptionKeyStroke  keyMacro[], int keyMacroLength, const InterceptionContext &context, const InterceptionDevice &device)
{
    if (modeDebug)
        cout << " -> SEND MACRO (" << keyMacroLength << ")";
    for (int i = 0; i < keyMacroLength; i++)
    {
        normalizeKeyStroke(keyMacro[i]);
        if (modeDebug)
            cout << " " << keyMacro[i].code << ":" << keyMacro[i].state;
        sendStroke(context, device, keyMacro[i]);
        Sleep(delayBetweenMacroKeysMS);  //local sys needs ~1ms, RDP fullscreen ~3 ms delay so they can keep up with macro key action
    }
}


void getHardwareId(const InterceptionContext &context, const InterceptionDevice &device)
{
    {
        wchar_t  hardware_id[500];
        string id;
        size_t length = interception_get_hardware_id(context, device, hardware_id, sizeof(hardware_id));
        if (length > 0 && length < sizeof(hardware_id))
        {
            wstring wid(hardware_id);
            string sid(wid.begin(), wid.end());
            id = sid;
        }
        else
            id = "UNKNOWN_ID";

        deviceIdKeyboard = id;
        deviceIsAppleKeyboard = (id.find("VID_05AC") != string::npos);

        if (modeDebug)
            cout << endl << "getHardwareId:" << id <<" / Apple keyboard: " << deviceIsAppleKeyboard;
    }
}

void printHelp()
{
    cout << "HELP" << endl << endl
        << "[LEFTCTRL] + [CAPS] + [{key}] for core commands" << endl
        << "[ESC] quit" << endl
        << "[S] Status" << endl
        << "[R] Reset" << endl
        << "[E] Error log" << endl
        << "[D] Debug mode output" << endl
        << "[0]..[9] switch layers" << endl
        << "[Z] (or Y on GER keyboard): flip Y<>Z keys" << endl
        << "[W] flip ALT <> WIN" << endl
        << "[/] (is [-] on GER keyboard): Slash -> Shift" << endl
        << "[C] switch character creation mode (Alt+Numpad or AHK)" << endl
        << "[ and ]: pause between macro keys sent -/+ 10ms " << endl
        ;
}
void printStatus()
{
    int numMakeReceived = 0;
    int numMakeSent = 0;
    for (int i = 0; i < 255; i++)
    {
        if (keysDownReceived[i])
            numMakeReceived++;
        if (keysDownSent[i])
            numMakeSent++;
    }
    cout << "STATUS" << endl << endl
        << "Capsicain version: " << version << endl
        << "hardware id:" << deviceIdKeyboard << endl
        << "Apple keyboard: " << deviceIsAppleKeyboard << endl
        << "current LAYER is: " << activeLayer << endl
        << "modifier state: " << hex << modiState << endl
        << "macro key delay: " << delayBetweenMacroKeysMS << endl
        << "character create mode: " << characterCreationMode << endl
        << "# keys down received: " << numMakeReceived << endl
        << "# keys down sent: " << numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << errorLog.length() << " chars)" << endl
        ;
}

void normalizeKeyStroke(InterceptionKeyStroke &stroke) {
    if(stroke.code > 0x7F) {
        stroke.code &=0x7F;
        stroke.state |=2;
    }
}

void scancode2stroke(unsigned short scancode,  InterceptionKeyStroke &istroke)
{
    if (scancode >= 0x80)
    {
        istroke.code = static_cast<unsigned short>(scancode & 0x7F);
        istroke.state |= 2;
    }
    else
    {
        istroke.code = scancode;
        istroke.state &= 0xFD;
    }
}

void sendStroke(InterceptionContext context, InterceptionDevice device, InterceptionKeyStroke &stroke)
{
    if (stroke.code > 0xFF)
        error("Unexpected scancode > 255: " + stroke.code);
    else
        keysDownSent[(unsigned char)stroke.code] = (stroke.state & 1) ? false : true;

    interception_send(context, device, (InterceptionStroke *) &stroke, 1);
}

void reset(InterceptionKeyStroke  *keyMacro, int keyMacroLength, InterceptionContext context, InterceptionDevice device)
{
    cout << endl << "::::RESET:::::" << endl;
    for (int i = 0; i < 255; i++)
    {
        keysDownReceived[i] = 0;
        keysDownSent[i] = 0;
    }

    modiState = 0;
    delayBetweenMacroKeysMS = DEFAULT_DELAY_SENDMACRO;

    if(modeDebug)
        cout << endl << "Resetting all modifiers to UP" << endl;
    keyMacroLength = 0;
    InterceptionKeyStroke key;
    breakKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
    breakKeyMacro(SC_RSHIFT, keyMacro, keyMacroLength);
    breakKeyMacro(SC_LCONTROL, keyMacro, keyMacroLength);
    breakKeyMacro(SC_RCONTROL, keyMacro, keyMacroLength);
    breakKeyMacro(SC_LALT, keyMacro, keyMacroLength);
    breakKeyMacro(SC_RALT, keyMacro, keyMacroLength);
    breakKeyMacro(SC_LWIN, keyMacro, keyMacroLength);
    breakKeyMacro(SC_RWIN, keyMacro, keyMacroLength);
    breakKeyMacro(SC_CAPS, keyMacro, keyMacroLength);
    breakKeyMacro(SC_F14, keyMacro, keyMacroLength);
    breakKeyMacro(SC_F15, keyMacro, keyMacroLength);

    playMacro(keyMacro, keyMacroLength, context, device);
}


void makeKeyMacro(unsigned short scancode, InterceptionKeyStroke * keyMacro, int & keyMacroLength)
{
    keyMacro[keyMacroLength].code = scancode;
    keyMacro[keyMacroLength].state = KEYSTATE_DOWN;
    keyMacroLength++;
}
void breakKeyMacro(unsigned short scancode, InterceptionKeyStroke * keyMacro, int & keyMacroLength)
{
    keyMacro[keyMacroLength].code = scancode;
    keyMacro[keyMacroLength].state = KEYSTATE_UP;
    keyMacroLength++;
}
void makeBreakKeyMacro(unsigned short scancode, InterceptionKeyStroke * keyMacro, int & keyMacroLength)
{
    keyMacro[keyMacroLength].code = scancode;
    keyMacro[keyMacroLength].state = KEYSTATE_DOWN;
    keyMacroLength++;
    keyMacro[keyMacroLength].code = scancode;
    keyMacro[keyMacroLength].state = KEYSTATE_UP;
    keyMacroLength++;
}


//Macro wrapped in lshift off temporarily. Example: [shift+backspace] -> ctrl+Y  must suppress the shift, because shift+ctrl+Y is garbage
void createMacroKeyComboRemoveShift(int a, int b, int c, int d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    if (IS_LSHIFT_DOWN)
        breakKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
    createMacroKeyCombo(a, b, c, d, keyMacro, keyMacroLength);
    if (IS_LSHIFT_DOWN)
        makeKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
}

//press all scancodes, then release them. Pass a,b,0,0 if you need less than 4
void createMacroKeyCombo(int a, int b, int c, int d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    unsigned short scancodes[] = { (unsigned short)a, (unsigned short)b, (unsigned short)c, (unsigned short)d };
    for(int i=0;i<4;i++)
    {   
        if (scancodes[i] == 0)
            break;
        if (scancodes[i] == SC_LCONTROL && IS_LCONTROL_DOWN)
            continue;
        if (scancodes[i] == SC_LSHIFT && IS_LSHIFT_DOWN)
            continue;
        makeKeyMacro(scancodes[i],keyMacro, keyMacroLength);
    }

    for(int i=3;i>=0;i--)
    {
        if (scancodes[i] == 0)
            continue;
        if (scancodes[i] == SC_LCONTROL && IS_LCONTROL_DOWN)
            continue;
        if (scancodes[i] == SC_LSHIFT && IS_LSHIFT_DOWN)
            continue;
        breakKeyMacro(scancodes[i], keyMacro, keyMacroLength);
    }
}

//virtually push Alt+Numpad 0 1 2 4  for special characters
//pass a b c 0 for 3-digit combos
void createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    unsigned short fsc[] = { a, b, c, d };

    bool lshift = (modiState & BITMASK_LSHIFT) > 0;
    if (lshift)
        breakKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
    bool rshift = (modiState & BITMASK_RSHIFT) > 0;
    if (rshift)
        breakKeyMacro(SC_RSHIFT, keyMacro, keyMacroLength);
    makeKeyMacro(SC_LALT, keyMacro, keyMacroLength);

    for (int i = 0; i < 4 && fsc[i] != 0; i++)
        makeBreakKeyMacro(fsc[i], keyMacro, keyMacroLength);

    breakKeyMacro(SC_LALT, keyMacro, keyMacroLength);
    if (rshift)
        makeKeyMacro(SC_RSHIFT, keyMacro, keyMacroLength);
    if (lshift)
        makeKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
}


/* Sending special character with scancodes is no fun.
   There are 3 createCharactermodes:
   1. IBM classic: ALT + NUMPAD 3-digits. Supported by some apps.
   2. ANSI: ALT + NUMPAD 4-digits. Supported by other apps, many Microsoft apps but not all.
   3. AHK: Sends F14|F15 + 1 base character. Requires an AHK script that translates these comobos to characters.
   This is Windows only. For Linux, the combo is [Ctrl]+[Shift]+[U] , {unicode E4 is ö} , [Enter]
*/
void processCapsTapped(unsigned short scancode, InterceptionKeyStroke  keyMacro[100], int &keyMacroLength)
{
    bool shiftXorCaps = IS_SHIFT_DOWN != ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);

    if (characterCreationMode == IBM)
    {
        switch (scancode)
        {
        case SC_O:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD5, SC_NUMPAD3, 0, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD4, SC_NUMPAD8, 0, keyMacro, keyMacroLength);
            break;
        case SC_A:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD4, SC_NUMPAD2, 0, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD3, SC_NUMPAD2, 0, keyMacro, keyMacroLength);
            break;
        case SC_U:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD5, SC_NUMPAD4, 0, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD2, SC_NUMPAD9, 0, keyMacro, keyMacroLength);
            break;
        case SC_S:
            createMacroAltNumpad(SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD5, 0, keyMacro, keyMacroLength);
            break;
        case SC_E:
            createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD2, SC_NUMPAD8, keyMacro, keyMacroLength);
            break;
        case SC_D:
            createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD6, SC_NUMPAD7, 0, keyMacro, keyMacroLength);
            break;
        case SC_T: // test print from [1] to [Enter]
        {
            for (unsigned short i = 2; i <= 0x1C; i++)
                makeBreakKeyMacro(i, keyMacro, keyMacroLength);
            break;
        }
        case SC_R: // test2: print 50x10 ä
        {
            for (int outer = 0; outer < 4; outer++)
            {
                for (unsigned short i = 0; i < 40; i++)
                {

                    makeKeyMacro(SC_LALT, keyMacro, keyMacroLength);
                    makeBreakKeyMacro(SC_NUMPAD1, keyMacro, keyMacroLength);
                    makeBreakKeyMacro(SC_NUMPAD3, keyMacro, keyMacroLength);
                    makeBreakKeyMacro(SC_NUMPAD2, keyMacro, keyMacroLength);
                    breakKeyMacro(SC_LALT, keyMacro, keyMacroLength);
                }

                makeBreakKeyMacro(SC_RETURN, keyMacro, keyMacroLength);
            }
            break;
        }
        case SC_L: // Linux test: print 50x10 ä
        {
            for (int outer = 0; outer < 4; outer++)
            {
                for (unsigned short i = 0; i < 40; i++)
                {

                    makeKeyMacro(SC_LCONTROL, keyMacro, keyMacroLength);
                    makeKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
                    makeKeyMacro(SC_U, keyMacro, keyMacroLength);
                    breakKeyMacro(SC_U, keyMacro, keyMacroLength);
                    breakKeyMacro(SC_LSHIFT, keyMacro, keyMacroLength);
                    breakKeyMacro(SC_LCONTROL, keyMacro, keyMacroLength);
                    makeBreakKeyMacro(SC_E, keyMacro, keyMacroLength);
                    makeBreakKeyMacro(SC_4, keyMacro, keyMacroLength);
                    makeBreakKeyMacro(SC_RETURN, keyMacro, keyMacroLength);
                }
                makeBreakKeyMacro(SC_RETURN, keyMacro, keyMacroLength);
            }
            break;
        }
        }
    }
    else if (characterCreationMode == ANSI)
    {
        switch (scancode)
        {
        case SC_O:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD1, SC_NUMPAD4, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD4, SC_NUMPAD6, keyMacro, keyMacroLength);
            break;
        case SC_A:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD9, SC_NUMPAD6, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD8, keyMacro, keyMacroLength);
            break;
        case SC_U:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD0, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD5, SC_NUMPAD2, keyMacro, keyMacroLength);
            break;
        case SC_S:
            createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD3, keyMacro, keyMacroLength);
            break;
        case SC_E:
            createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD2, SC_NUMPAD8, keyMacro, keyMacroLength);
            break;
        case SC_D:
            createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD7, SC_NUMPAD6, 0, keyMacro, keyMacroLength);
            break;
        }
    }
    else if (characterCreationMode == AHK)
    {
        switch (scancode)
        {
        case SC_A:
        case SC_O:
        case SC_U:
            createMacroKeyCombo(shiftXorCaps ? SC_F15 : SC_F14, scancode, 0, 0, keyMacro, keyMacroLength);
            break;
        case SC_S:
        case SC_E:
        case SC_D:
        case SC_C:
            createMacroKeyCombo(SC_F14, scancode, 0, 0, keyMacro, keyMacroLength);
            break;
        }
    }

    if (keyMacroLength == 0)
    {
        switch (scancode) {
        case SC_0:
        case SC_1:
        case SC_2:
        case SC_3:
        case SC_4:
        case SC_5:
        case SC_6:
        case SC_7:
        case SC_8:
        case SC_9:
            createMacroKeyCombo(SC_F15, scancode, 0, 0, keyMacro, keyMacroLength);
            break;
        }
    }
}

