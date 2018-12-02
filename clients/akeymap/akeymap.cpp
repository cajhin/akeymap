#include <interception.h>
#include <utils.h>
#include <Windows.h>  //for Sleep()

#include <iostream>
#include <string>

#include "scancodes.h"
#include "akeymap.h"

using namespace std;

enum KEYSTATE
{
    KEYSTATE_DOWN = 0,
    KEYSTATE_UP = 1,
    KEYSTATE_EXT_DOWN = 2,
    KEYSTATE_EXT_UP = 3,
};

bool modeDebug = true;
unsigned short modiState = 0;
bool keysDownReceived[256];
bool keysDownSent[256];
string deviceIdKeyboard = "";
bool deviceIsAppleKeyboard = false;


string errorLog = " ";
void error(string txt)
{
    if (modeDebug)
        cout << endl << txt;
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
    bool modeFlipAltWin = false;

    int  layer = 1;
    bool capsDown = false;
    bool capsTapped = false;

    InterceptionKeyStroke keyMacro[MAX_KEYMACRO_LENGTH];
    int keyMacroLength = 0;
    InterceptionKeyStroke tmpstroke;
    wchar_t hardware_id[500];

    raise_process_priority();
    context = interception_create_context();
    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    cout << "Press left (CTRL + CAPS + ESC) to stop." << endl;

#pragma region setup
    {
        /*
        cout << "setup... " << endl;
        Sleep(500);

        cout << endl << "Press Shift";
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        if (stroke.code == SC_LBSLASH)
            modeSlashShift = true;

        cout << endl << "Press Z";
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        if (stroke.code == SC_Z)
            modeFlipZy = true;

        cout << endl << "Press CMD/WIN";
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        if (stroke.code == SC_LALT)
            modeFlipAltWin = true;
*/
        cout << endl << endl << (modeSlashShift ? "ON :" : "OFF:") << "Slashes -> Shift ";
        cout << endl << (modeFlipZy ? "ON :" : "OFF:") << "Z<->Y ";
        cout << endl << (modeFlipAltWin ? "ON :" : "OFF:") << "ALT<->WIN ";
        cout << endl <<  endl << "akeymap running..." << "Press left (CTRL + CAPS + ESC) to stop." << endl;

        strokeM1.code = 0;
        strokeM2.code = 0;

        /*
        const int HID_BUFFER_SIZE = 200;
        char hid_buffer[HID_BUFFER_SIZE];
        interception_get_hardware_id(context, device, hid_buffer, HID_BUFFER_SIZE);
        cout << "hid: " << hid_buffer << endl;
        */
    }
#pragma endregion setup 


    while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0)
    {
        bool blockKey = false;
/*
        if (deviceIdKeyboard=="")
        {
            getHardwareId(context, device);
            modeFlipAltWin = !deviceIsAppleKeyboard;
            cout << endl << deviceIsAppleKeyboard ? "APPLE keyboard" : "PC keyboard";
        }
        */
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
                layer = 0;
                cout << endl << endl << "LAYER 0 active" << endl;
                break;
            case SC_1:
                layer = 1;
                cout << endl << endl << "LAYER 1 active" << endl;
                break;
            case SC_R:
                reset(context, device);
                capsDown = false;
                capsTapped = false;
                break;
            case SC_D:
                modeDebug = !modeDebug;
                cout << endl << endl << "DEBUG mode: " << (modeDebug ? "ON" : "OFF") << endl;
                break;
            case SC_L:
                cout << endl << endl << "current LAYER is: " << layer << endl;
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
                getHardwareId(context, device);
                printStatus();
                break;
            case SC_LALT:
            case SC_RALT:
                modeFlipAltWin = !modeFlipAltWin;
                cout << endl << endl << "Flip ALT<>WIN mode: " << (modeFlipAltWin ? "ON" : "OFF") << endl;
                break;
            }
            continue;
        }
#pragma endregion core commands

        if (modeDebug)
            cout << endl << " [" << hex << stroke.code << "/" << hex << scancode << " " << stroke.information << " " << stroke.state << "]";

        if (layer == 0)
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
                //Undo Cut Copy Paste All Save Find
                case SC_Z:
                    if(isDownstroke)
                        createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0, keyMacro, keyMacroLength);
                    break;
                case SC_BACK:
                    if(isDownstroke) {
                        if (IS_SHIFT_DOWN)
                            createMacroKeyCombo(SC_LCONTROL, SC_Y, 0, 0, keyMacro, keyMacroLength);
                        else
                            createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0, keyMacro, keyMacroLength);
                    }
                    break;
                case SC_X:
                    if(isDownstroke)
                        createMacroKeyCombo(SC_LCONTROL, SC_X, 0, 0, keyMacro, keyMacroLength);
                    break;
                case SC_SPACE:
                    if(isDownstroke) 
                        createMacroKeyCombo(SC_LCONTROL, SC_C, 0, 0, keyMacro, keyMacroLength);
                    break;
                case SC_C:
                    if(isDownstroke)
                        createMacroKeyCombo(SC_LCONTROL, SC_C, 0, 0, keyMacro, keyMacroLength);
                    break;
                case SC_RETURN:
                    if (isDownstroke) {
                        if (IS_SHIFT_DOWN)
                            createMacroKeyCombo(SC_LCONTROL, SC_X, 0, 0, keyMacro, keyMacroLength);
                        else
                            createMacroKeyCombo(SC_LCONTROL, SC_V, 0, 0, keyMacro, keyMacroLength);
                    }
                    break;
                case SC_V:
                    if(isDownstroke)
                        createMacroKeyCombo(SC_LCONTROL, SC_V, 0, 0, keyMacro, keyMacroLength);
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

        if (capsTapped)
        {
            switch (scancode)
            {
                case SC_O:
                    if(IS_SHIFT_DOWN)
                        createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD1, SC_NUMPAD4, keyMacro, keyMacroLength);
                    else
                        createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD4, SC_NUMPAD6, keyMacro, keyMacroLength);
                    break;
                case SC_A:
                    if(IS_SHIFT_DOWN)
                        createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD9, SC_NUMPAD6, keyMacro, keyMacroLength);
                    else
                        createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD8, keyMacro, keyMacroLength);
                    break;
                case SC_U:
                    if(IS_SHIFT_DOWN)
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
        << "hardware id:" << deviceIdKeyboard << endl
        << "Apple keyboard: " << deviceIsAppleKeyboard << endl
        << "modifier state: " << hex << modiState << endl
        << "# keys down received: " << numMakeReceived << endl
        << "# keys down sent: " << numMakeSent << endl
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
    case SC_I: keypadCode = SC_NUMPAD8; break;
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

