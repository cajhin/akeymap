#include <string>
#include <iostream>
#include <Windows.h>  //for Sleep()

#include <interception.h>
#include <utils.h>

#include "scancodes.h"
#include "capsicain.h"

using namespace std;

string version = "7";

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
CREATE_CHARACTER_MODE characterCreationMode = IBM;


string errorLog = "";
void error(string txt)
{
    if (modeDebug)
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

    const int MAX_KEYMACRO_LENGTH = 100;

    InterceptionContext context;
    InterceptionDevice device;
    InterceptionKeyStroke stroke, previousStroke;

    bool modeSlashShift = true;
    bool modeFlipZy = true;
    bool modeFlipAltWin = true;

    bool capsDown = false;
    bool capsTapped = false;

    InterceptionKeyStroke keyMacro[MAX_KEYMACRO_LENGTH];
    int keyMacroLength;
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

        previousStroke.code = 0;
    }
#pragma endregion setup 


    while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0)
    {
        bool blockKey = false;
        bool isFinalScancode = false;  //true -> don't remap the scancode anymore
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
        //command stroke: LCTRL + CAPS + stroke

        //TODO: double-check that caps and lcontrol are never left hanging on down, from Windows point of view
        if (isDownstroke && keysDownReceived[SC_CAPS] && keysDownReceived[SC_LCONTROL])
        {
            if (scancode == SC_ESCAPE)
            {
                break;  //break the main while() loop
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
                reset(context, device);
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
                switch (characterCreationMode)
                {
                case IBM:
                    characterCreationMode = ANSI;
                    break;
                case ANSI:
                    characterCreationMode = AHK;
                    break;
                case AHK:
                    characterCreationMode = IBM;
                    break;
                }
                cout << "Character creation mode: " << characterCreationMode;
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
                    scancode = SC_LWIN;
                break;
            default:
                isFinalScancode = false;
            }

            //evaluate and remember modifiers
            switch (scancode)
            {
            case SC_LSHIFT:
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
                cout << " {" << hex << modiState << (capsDown ? " C" : " _") << (capsTapped ? "T" : "_") << "} ";
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
            else
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
                if (modeFlipZy && !IS_LCONTROL_DOWN)
                    scancode = SC_Y;
                break;
            case SC_Y:
                if (modeFlipZy)  //do not remap LCTRL+Y (undo)
                    scancode = SC_Z;
                break;
            }
        }

        if (capsTapped)
        {
            if (scancode != SC_LSHIFT && scancode != SC_RSHIFT)
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
            if (modeDebug)
                cout << " -> SEND MACRO (" << keyMacroLength << ")";
            for (int i = 0; i < keyMacroLength; i++)
            {
                normalizeKeyStroke(keyMacro[i]);
                if (modeDebug)
                    cout << " " << keyMacro[i].code << ":" << keyMacro[i].state;
                sendStroke(context, device, keyMacro[i]);
                Sleep(10);  //TODO ; AHK Fullscreen RDP needs it for reliable key up recognition
            }
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

/* Sending special character with scancodes is no fun.
   There are 3 createCharactermodes:
   1. IBM classic: ALT + NUMPAD 3-digits. Supported by some apps.
   2. ANSI: ALT + NUMPAD 4-digits. Supported by other apps, many Microsoft apps but not all.
   3. AHK: Sends F14|F15 + 1 base character. Requires an AHK script that translates these comobos to characters.
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
    cout << endl << "STATUS" << endl << endl
        << "Capsicain version: " << version << endl
        << "hardware id:" << deviceIdKeyboard << endl
        << "Apple keyboard: " << deviceIsAppleKeyboard << endl
        << "current LAYER is: " << activeLayer << endl
        << "modifier state: " << hex << modiState << endl
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


//Macro wrapped in lshift off temporarily. Example: [shift+backspace] -> ctrl+Y  must suppress the shift, because shift+ctrl+Y is garbage
void createMacroKeyComboRemoveShift(int a, int b, int c, int d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
   createMacroKeyCombo(a, b, c, d, keyMacro, keyMacroLength);
   if (IS_LSHIFT_DOWN)
    {
        for (int i = keyMacroLength; i > 0; i--)
            keyMacro[i] = keyMacro[i - 1];
        keyMacro[keyMacroLength + 1].code = SC_LSHIFT;
        keyMacro[keyMacroLength + 1].state = KEYSTATE_DOWN;
        keyMacro[0].code = SC_LSHIFT;
        keyMacro[0].state = KEYSTATE_UP;

        keyMacroLength += 2;
    }
}

//press all scancodes, then release them. Pass a,b,0,0 if you need less than 4
void createMacroKeyCombo(int a, int b, int c, int d, InterceptionKeyStroke *keyMacro, int &keyMacroLength)
{
    int idx = 0;

    unsigned short fsc[] = { (unsigned short)a, (unsigned short)b, (unsigned short)c, (unsigned short)d };
    for(int i=0;i<4;i++)
    {   
        if (fsc[i] == 0)
            break;
        if (fsc[i] == SC_LCONTROL && IS_LCONTROL_DOWN)
            continue;
        if (fsc[i] == SC_LSHIFT && IS_LSHIFT_DOWN)
            continue;
        keyMacro[idx].code = fsc[i];
        keyMacro[idx++].state = KEYSTATE_DOWN;
    }

    for(int i=3;i>=0;i--)
    {
        if (fsc[i] == 0)
            continue;
        if (fsc[i] == SC_LCONTROL && IS_LCONTROL_DOWN)
            continue;
        if (fsc[i] == SC_LSHIFT && IS_LSHIFT_DOWN)
            continue;
        keyMacro[idx].code = fsc[i];
        keyMacro[idx++].state = KEYSTATE_UP;
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


