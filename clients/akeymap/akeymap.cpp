#include <interception.h>
#include <utils.h>
#include <Windows.h>  //for Sleep()

#include <iostream>

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

int main()
{
    const int MAX_KEYMACRO_LENGTH = 100;

    InterceptionContext context;
    InterceptionDevice device;
    InterceptionKeyStroke stroke, strokeM1, strokeM2;

    bool modeDebug = true;
    bool modeSlashShift = true;
    bool modeFlipZy = true;
    bool modeFlipAltWin = false;

    int  layer = 1;
    bool capsDown = false;
    InterceptionKeyStroke keyMacro[MAX_KEYMACRO_LENGTH];
    int keyMacroLength = 0;
    InterceptionKeyStroke tmpstroke;

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

        cout << endl << endl << (modeSlashShift ? "ON :" : "OFF:") << "Slashes -> Shift ";
        cout << endl << (modeFlipZy ? "ON :" : "OFF:") << "Z<->Y ";
        cout << endl << (modeFlipAltWin ? "ON :" : "OFF:") << "ALT<->WIN ";

        cout << endl <<  endl << "akeymap running..." << "Press left (CTRL + CAPS + ESC) to stop." << endl;
        */
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
#pragma region core commands
        bool blockKey = false;

        //normalize extended scancodes (state -> code.bit7)
        unsigned short scancode = stroke.code;

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
            case SC_LALT:
            case SC_RALT:
                modeFlipAltWin = !modeFlipAltWin;
                cout << endl << endl << "Flip ALT<>WIN mode: " << (modeFlipAltWin ? "ON" : "OFF") << endl;
                break;
            }
            stroke.state++; //send UP for the last stroke in case the key itself is remapped
            interception_send(context, device, (InterceptionStroke *)&stroke, 1);
            continue;
        }
#pragma endregion core commands

        if (modeDebug)
            cout << endl << " [" << hex << stroke.code << "/" << hex << scancode << " " << stroke.information << " " << stroke.state << "]";

        if (layer == 0)
        {
        }
        else if (scancode == SC_CAPS)
        {
            if (stroke.state == KEYSTATE_UP)
                capsDown = false;
            else if (stroke.state == KEYSTATE_DOWN)
                capsDown = true;
            strokeM2 = strokeM1;
            strokeM1 = stroke;
            continue;
        }
        else if (capsDown)
        {
            switch (scancode)
            {
            case SC_H:
                scancode = SC_BACK;
                break;
            case SC_BACKSLASH:
                scancode = SC_SLASH;
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
                if (stroke.state & 1)    // ignore KEYSTATE_UP, macros must be complete
                    blockKey = true;
                else
                    createMacroKeypad(stroke, keyMacro, keyMacroLength);
                break;
            }
        }
        //pass 2: key remapping
        switch (scancode)
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

        //SEND
        strokeM2 = strokeM1;
        strokeM1 = stroke;

        if (keyMacroLength > 0)
        {
            if (modeDebug)
                cout << endl << "SEND MACRO (" << keyMacroLength << ")";
            for (int i = 0; i < keyMacroLength; i++)
            {
                keyMacro[i].code &= 0x7F;
                if (modeDebug)
                    cout << " " << keyMacro[i].code << ":" << keyMacro[i].state;
                interception_send(context, device, (InterceptionStroke *)&keyMacro[i], 1);

            }
            if (modeDebug)
                cout << endl;
            keyMacroLength = 0;
        }
        else
        {
            if (scancode >= 0x80)
            {
                stroke.state |= 2;
                stroke.code = scancode & 0x7F;
            }
            else
            {
                stroke.code = scancode;
                stroke.state &= 0xFD;
            }

            if (modeDebug)
            {
                cout << endl << " >" << hex << stroke.code << "/" << hex << scancode << " " << stroke.information << " " << stroke.state
                    << (blockKey ? " KEY IS BLOCKED " : "");
            }
            if (!blockKey)
                interception_send(context, device, (InterceptionStroke *)&stroke, 1);
        }
    }

    interception_destroy_context(context);

    cout << endl << "bye" << endl;
    return 0;
}

void createMacroKeypad(InterceptionKeyStroke stroke, InterceptionKeyStroke  keyMacro[100], int &keyMacroLength)
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
    if (controlDown)
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


