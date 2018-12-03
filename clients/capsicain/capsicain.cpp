#include <interception.h>
#include <utils.h>
#include <Windows.h>  //for Sleep()

#include <iostream>
#include <string>

#include "scancodes.h"
#include "capsicain.h"

using namespace std;

int version = 4;

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

bool modeDebug = false;
unsigned short modiState = 0;
bool keysDownReceived[256];
bool keysDownSent[256];
int  activeLayer = 1;
string deviceIdKeyboard = "";
bool deviceIsAppleKeyboard = false;
CREATE_CHARACTER_MODE createCharacterMode = AHK;


string errorLog = "";
void error(string txt)
{
    if (modeDebug)
        cout << endl << "ERROR: " << txt;
    errorLog += "\r\n" + txt;
}

int main()
{
    /*
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    puts(buffer);
    */


    const int MAX_KEYMACRO_LENGTH = 100;

    InterceptionContext context;
    InterceptionDevice device;
    InterceptionKeyStroke stroke, strokeM1, strokeM2;

    bool modeSlashShift = true;
    bool modeFlipZy = true;
    bool modeFlipAltWin = true;

    bool capsDown = false;
    bool capsTapped = false;

    InterceptionKeyStroke keyMacro[MAX_KEYMACRO_LENGTH];
    int keyMacroLength = 0;
    InterceptionKeyStroke tmpstroke;
    wchar_t hardware_id[500];

#pragma region setup
    Sleep(700); //time to release Win from AHK Start shortcut
    raise_process_priority();
    context = interception_create_context();
    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    cout << "Capsicain v" << version << endl << endl
         << "(LCTRL + CAPS + ESC) to stop." << endl
         << "(LCTRL + CAPS + H) for Help";

    {
        cout << endl << endl << (modeSlashShift ? "ON :" : "OFF:") << "Slashes -> Shift ";
        cout << endl << (modeFlipZy ? "ON :" : "OFF:") << "Z<->Y ";
        cout << endl <<  endl << "capsicain running..." << endl;

        strokeM1.code = 0;
        strokeM2.code = 0;
    }
#pragma endregion setup 


    while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0)
    {
        bool blockKey = false;

        if (deviceIdKeyboard.length()<2)
        {
            cout << endl << "detecting keyboard...";
            getHardwareId(context, device);
            modeFlipAltWin = !deviceIsAppleKeyboard;
            cout << endl << (deviceIsAppleKeyboard ? "APPLE keyboard" : "PC keyboard (flipping Win<>Alt)");
        }

        if (stroke.code > 0xFF)
            error("Unexpected scancode > 255: ");
        else
            keysDownReceived[(unsigned char)stroke.code] = (stroke.state & 1) ? false : true;

        //normalize extended scancodes (state.bit1 -> code.bit7)
        unsigned short scancode = stroke.code;

#pragma region core commands
        if (stroke.state & 2)  //extended code, so I set the high bit of the code. Assuming Interception never sends code > 0x7F
        {
            if (scancode & 0x80)
            {
                cout << "ERROR: received scancode > 0x79: " << scancode;
                return -1;
            }
            scancode |= 0x80;
        }

        //command stroke: LCTRL + CAPS + stroke
        if (strokeM1.code == SC_CAPS && strokeM1.state == KEYSTATE_DOWN &&
            strokeM2.code == SC_LCONTROL && strokeM2.state == KEYSTATE_DOWN &&
            (stroke.state & 1) == KEYSTATE_DOWN)
        {
            if (scancode == SC_ESCAPE)
            {
                break;
            }

            switch (scancode)
            {
            case SC_0:
                activeLayer = 0;
                cout << endl << endl << "LAYER 0 active" << endl;
                break;
            case SC_1:
                activeLayer = 1;
                cout << endl << endl << "LAYER 1 active" << endl;
                break;
            case SC_2:
                activeLayer = 2;
                cout << endl << endl << "LAYER 2 active" << endl;
                break;
            case SC_R:
                reset(context, device);
                capsDown = false;
                capsTapped = false;
                getHardwareId(context, device);
                modeFlipAltWin = !deviceIsAppleKeyboard;
                cout << endl << (deviceIsAppleKeyboard ? "APPLE keyboard" : "PC keyboard (flipping Win<>Alt)");
                break;
            case SC_D:
                modeDebug = !modeDebug;
                cout << endl << endl << "DEBUG mode: " << (modeDebug ? "ON" : "OFF") << endl;
                break;
            case SC_SLASH:
                modeSlashShift = !modeSlashShift;
                cout << endl << endl << "SlashShift mode: " << (modeSlashShift ? "ON" : "OFF") << endl;
                break;
            case SC_Z:
                modeFlipZy = !modeFlipZy;
                cout << endl << endl << "Flip Z<>Y mode: " << (modeFlipZy ? "ON" : "OFF") << endl;
                break;
            case SC_E:
                cout << endl << endl << "Error Log: " << errorLog << endl;
                break;
            break;            
            case SC_S:
                printStatus();
                break;
            case SC_W:
                modeFlipAltWin = !modeFlipAltWin;
                cout << endl << endl << "Flip ALT<>WIN mode: " << (modeFlipAltWin ? "ON" : "OFF") << endl;
                break;
            case SC_H:
                printHelp();
                break;
            case SC_C:
                switch (createCharacterMode)
                {
                case IBM:
                    createCharacterMode = ANSI;
                    break;
                case ANSI:
                    createCharacterMode = AHK;
                    break;
                case AHK:
                    createCharacterMode = IBM;
                    break;
                }
                cout << endl << "Character creation mode: " << createCharacterMode;
                break;
            }
            continue;
        }
#pragma endregion core commands

        if (modeDebug)
            cout << endl << " [" << hex << stroke.code << "/" << hex << scancode << " " << stroke.information << " " << stroke.state << "]";

        if (activeLayer == 0)
        {
            interception_send(context, device, (InterceptionStroke *)&stroke, 1);
            continue;
        }

        if(stroke.code == SC_CAPS) {
            if (stroke.state == KEYSTATE_UP) {
                capsDown = false;
                if (strokeM1.code == SC_CAPS)
                    capsTapped = !capsTapped;
            } else if (stroke.state == KEYSTATE_DOWN)
                capsDown = true;
            strokeM2 = strokeM1;
            strokeM1 = stroke;
            continue;
        }


        // pass 3: layout-independent mappings
        if (capsDown)
        {
            //those suppress the key UP because DOWN is replaced with macro
            bool isDownstroke = (stroke.state & 1) ? false : true;
            bool blockingScancode = true;
            switch (scancode)
            {
                case SC_RSHIFT:
                    if(isDownstroke)
                        createMacroKeyCombo(SC_CAPS, 0, 0, 0, keyMacro, keyMacroLength);
                    break;
                //Undo Redo Cut Copy Paste
                case SC_BACK:
                    if(isDownstroke) {
                        if (IS_SHIFT_DOWN)
                            createMacroKeyCombo(SC_LCONTROL, SC_Y, 0, 0, keyMacro, keyMacroLength);
                        else
                            createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0, keyMacro, keyMacroLength);
                    }
                    break;
                case SC_S:
                    if(isDownstroke)
                        createMacroKeyCombo(SC_LCONTROL, SC_X, 0, 0, keyMacro, keyMacroLength);
                    break;
                case SC_D:
                    if(isDownstroke) 
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
                case SC_L:
                case SC_K:
                case SC_I:
                case SC_O:
                case SC_SEMICOLON:
                case SC_DOT:
                case SC_Y:
                case SC_U:
                case SC_N:
                case SC_M:
                    if(isDownstroke)
                        createMacroNumpadNavigation(stroke, keyMacro, keyMacroLength);
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
                default:
                    blockingScancode = false;
            }
            if(blockingScancode)
                blockKey = true;

            switch (scancode) {
            case SC_H:
                scancode = SC_BACK;
                break;
            case SC_BACKSLASH:
                scancode = SC_SLASH;
                break;
            }
        }

        //not caps down, key remapping
        else switch (scancode)
            {
                case SC_Z:
                    if (modeFlipZy)
                        scancode = SC_Y;
                    break;
                case SC_Y:
                    if (modeFlipZy)
                        scancode = SC_Z;
                    break;
                case SC_LBSLASH:
                    if (modeSlashShift)
                        scancode = SC_LSHIFT;
                    break;
                case SC_SLASH:
                    if (modeSlashShift && stroke.state < 2)  //do not remap keypad slash (state 2+3)
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
                        scancode = SC_LWIN;
                    break;
            }


        switch(scancode)
        {
            case SC_LSHIFT:
                stroke.state & 1 ? modiState &= ~BITMASK_LSHIFT : modiState |= BITMASK_LSHIFT;
                break;
            case SC_RSHIFT:
                stroke.state & 1 ? modiState &= ~BITMASK_RSHIFT : modiState |= BITMASK_RSHIFT;
                break;

        }

        if (capsTapped) //TODO? this allows the keycode break later
        {
            bool isDownstroke = (stroke.state & 1) ? false : true;
            processCapsTapped(scancode, keyMacro, keyMacroLength, isDownstroke);
            if(scancode != SC_LSHIFT && scancode != SC_RSHIFT)
                capsTapped = false;
        }

        //SEND
        strokeM2 = strokeM1;
        strokeM1 = stroke;
        if (modeDebug)
            cout << " modi: " << hex << modiState << " capsDown:" << capsDown;
        if (keyMacroLength > 0)
        {
            if (modeDebug)
                cout << " -> SEND MACRO (" << keyMacroLength << ")";
            for (int i = 0; i < keyMacroLength; i++)
            {
                normalizeKeyStroke(keyMacro[i]);
                if (modeDebug)
                    cout << " " << keyMacro[i].code << ":" << keyMacro[i].state;
                sendStroke(context, device, keyMacro[i]);
            }
            if (modeDebug)
                cout << endl;
            keyMacroLength = 0;
        }
        else
        {
            if (modeDebug)
            {
                if (blockKey)
                    cout << " BLOCKED ";
                if(stroke.code != (scancode & 0x79))
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

/* Sending special character is no fun.
   There are 3 createCharactermodes:
   1. IBM classic: ALT + NUMPAD 3-digits. Supported by some apps.
   2. ANSI: ALT + NUMPAD 4-digits. Supported by other apps, many Microsoft apps but not all.
   3. AHK: Sends F14|F15 + 1 base character. Requires an AHK script that translates these comobos to characters.
*/
void processCapsTapped(unsigned short scancode, InterceptionKeyStroke  keyMacro[100], int &keyMacroLength, bool isDownstroke)
{
    if (createCharacterMode == IBM)
    {
        switch (scancode)
        {
        case SC_O:
            if (IS_SHIFT_DOWN)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD5, SC_NUMPAD3, 0, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD4, SC_NUMPAD8, 0, keyMacro, keyMacroLength);
            break;
        case SC_A:
            if (IS_SHIFT_DOWN)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD4, SC_NUMPAD2, 0, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD3, SC_NUMPAD2, 0, keyMacro, keyMacroLength);
            break;
        case SC_U:
            if (IS_SHIFT_DOWN)
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
        }
    }
    else if (createCharacterMode == ANSI)
    {
        switch (scancode)
        {
        case SC_O:
            if (IS_SHIFT_DOWN)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD1, SC_NUMPAD4, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD4, SC_NUMPAD6, keyMacro, keyMacroLength);
            break;
        case SC_A:
            if (IS_SHIFT_DOWN)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD9, SC_NUMPAD6, keyMacro, keyMacroLength);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD8, keyMacro, keyMacroLength);
            break;
        case SC_U:
            if (IS_SHIFT_DOWN)
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
    else if (createCharacterMode == AHK)
    {
        unsigned short ahkFCode = SC_F14;
        if (IS_SHIFT_DOWN)
            ahkFCode = SC_F15;

        switch (scancode)
        {
        case SC_A:
        case SC_O:
        case SC_U:
        case SC_S:
        case SC_E:
        case SC_D:
        case SC_C:
            createMacroKeyCombo(ahkFCode, scancode, 0, 0, keyMacro, keyMacroLength);
            break;
        }
    }

    if (keyMacroLength == 0 && isDownstroke)
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
    cout << "****HELP****" << endl
        << "Press (LEFTCTRL + CAPS + key) for core commands" << endl
        << "ESC: quit" << endl
        << "S: Status" << endl
        << "R: Reset" << endl
        << "Z: (or Y on GER keyboard): flip Y<>Z keys" << endl
        << "W: flip ALT <> WIN" << endl
        << "SLASH: (or - on GER keyboard): slash -> Shift" << endl
        << "E: Error log" << endl
        << "D: Debug mode output" << endl
        << "0..9: switch layers" << endl
        << "C: switch character creation mode (Alt+Numpad or AHK)"
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
    cout << endl << endl << "::::STATUS::::" << endl
        << "Capsicain version: " << version << endl
        << "hardware id:" << deviceIdKeyboard << endl
        << "Apple keyboard: " << deviceIsAppleKeyboard << endl
        << "current LAYER is: " << activeLayer << endl
        << "modifier state: " << hex << modiState << endl
        << "character create mode: " << createCharacterMode << endl
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

void reset(InterceptionContext context, InterceptionDevice device)
{
    cout << endl << "::::RESET:::::" << endl;
    for (int i = 0; i < 255; i++)
    {
        keysDownReceived[i] = 0;
        keysDownSent[i] = 0;
    }

    modiState = 0;

    if(modeDebug)
        cout << endl << "Resetting all modifiers to UP" << endl;
    InterceptionKeyStroke key;
    key.code = SC_LSHIFT;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_RSHIFT;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_LCONTROL;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_RCONTROL;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_LALT;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_RALT;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_LWIN;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_RWIN;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
    key.code = SC_CAPS;
    key.state = KEYSTATE_UP;
    sendStroke(context, device, key);
}

//press all scancodes, then release them. Pass a,b,0,0 if you need less than 4
//turn off shift temporarily
void createMacroKeyCombo(int a, int b, int c, int d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    int idx = 0;
    bool lcontrolDown = (modiState & BITMASK_LCONTROL) > 0;
    bool lshiftDown = (modiState & BITMASK_LSHIFT) > 0;
    if (lshiftDown)
    {
        keyMacro[idx].code = SC_LSHIFT;
        keyMacro[idx++].state = KEYSTATE_UP;
    }

    unsigned short fsc[] = { (unsigned short)a, (unsigned short)b, (unsigned short)c, (unsigned short)d };
    for(int i=0;i<4;i++)
    {   
        if (fsc[i] == 0)
            break;
        if (fsc[i] == SC_LCONTROL && lcontrolDown)
            continue;
        keyMacro[idx].code = fsc[i];
        keyMacro[idx++].state = KEYSTATE_DOWN;
    }

    for(int i=3;i>=0;i--)
    {
        if (fsc[i] == 0)
            continue;
        if (fsc[i] == SC_LCONTROL && lcontrolDown)
            continue;
        keyMacro[idx].code = fsc[i];
        keyMacro[idx++].state = KEYSTATE_UP;
    }

    if (lshiftDown)
    {
        keyMacro[idx].code = SC_LSHIFT;
        keyMacro[idx++].state = KEYSTATE_DOWN;
    }
    keyMacroLength = idx;
}

//virtually push Alt+Numpad 0 1 2 4  for special characters
//pass a b c 0 for 3-digit combos
void createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    unsigned short fsc[] = { a, b, c, d };
    int idx = 0;
    bool lshift = (modiState & BITMASK_LSHIFT) > 0;
    if (lshift)
    {
        keyMacro[idx].code = SC_LSHIFT;
        keyMacro[idx++].state = KEYSTATE_UP;
    }
    bool rshift = (modiState & BITMASK_RSHIFT) > 0;
    if (rshift)
    {
        keyMacro[idx].code = SC_RSHIFT;
        keyMacro[idx++].state = KEYSTATE_UP;
    }
    keyMacro[idx].code = SC_RALT;
    keyMacro[idx++].state = KEYSTATE_DOWN;
    for (int i = 0; i < 4 && fsc[i] != 0; i++)
    {
        keyMacro[idx].code = fsc[i];
        keyMacro[idx++].state = KEYSTATE_DOWN;
        keyMacro[idx].code = fsc[i];
        keyMacro[idx++].state = KEYSTATE_UP;
    }
    keyMacro[idx].code = SC_RALT;
    keyMacro[idx++].state = KEYSTATE_UP;
    if (rshift)
    {
        keyMacro[idx].code = SC_RSHIFT;
        keyMacro[idx++].state = KEYSTATE_DOWN;
    }
    if (lshift)
    {
        keyMacro[idx].code = SC_LSHIFT;
        keyMacro[idx++].state = KEYSTATE_DOWN;
    }
    keyMacroLength = idx;
}

void createMacroNumpadNavigation(InterceptionKeyStroke stroke, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    unsigned short keypadCode = 0;
    bool controlDown = false;
    switch (stroke.code)
    {
    case SC_J: keypadCode = SC_NUMPAD4; break;
    case SC_L: keypadCode = SC_NUMPAD6; break;
    case SC_K: keypadCode = SC_NUMPAD2; break;
    case SC_I: keypadCode = SC_NUMPAD8;
        break;
    case SC_SEMICOLON: keypadCode = SC_NUMPAD_DOT; break;
    case SC_O: keypadCode = SC_NUMPAD9; break;
    case SC_DOT: keypadCode = SC_NUMPAD3; break;
    case SC_Y: keypadCode = SC_NUMPAD7; break;
    case SC_U: keypadCode = SC_NUMPAD1; break;
    case SC_N: keypadCode = SC_NUMPAD4; controlDown = true;  break;
    case SC_M: keypadCode = SC_NUMPAD6; controlDown = true; break;
    }
    int n = -1;
    if (IS_LCONTROL_DOWN)
        controlDown = false;
    if (controlDown )
    {
        keyMacro[++n].code = SC_LCONTROL;
        keyMacro[n].state = KEYSTATE_DOWN;
    }
    keyMacro[++n].code = SC_LSHIFT;
    keyMacro[n].state = KEYSTATE_EXT_DOWN;
    keyMacro[++n].code = keypadCode;
    keyMacro[n].state = KEYSTATE_EXT_DOWN;
    keyMacro[++n].code = keypadCode;
    keyMacro[n].state = KEYSTATE_EXT_UP;
    keyMacro[++n].code = SC_LSHIFT;
    keyMacro[n].state = KEYSTATE_EXT_UP;
    if (controlDown)
    {
        keyMacro[++n].code = SC_LCONTROL;
        keyMacro[n].state = KEYSTATE_UP;
    }
    keyMacroLength = n + 1;
}


