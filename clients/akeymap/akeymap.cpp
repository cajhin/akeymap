#include <interception.h>
#include <utils.h>
#include <Windows.h>

#include <iostream>

using namespace std;

bool DEBUGMODE = true;

enum ScanCode
{
    SCANCODE_ESC    = 0x01,
    SCANCODE_Y      = 0x15,
    SCANCODE_LSHIFT = 0x2A,
    SCANCODE_LSLASH = 0x56,   // iso only
    SCANCODE_Z      = 0x2C,
    SCANCODE_RSLASH = 0x35,   // or 2B on iso board
    SCANCODE_RSHIFT = 0x36,
    SCANCODE_LCTRL  = 0x1d,
    SCANCODE_LWIN   = 0x5b,
    SCANCODE_LALT   = 0x38,
};

int main()
{
    InterceptionContext context;
    InterceptionDevice device;
    InterceptionKeyStroke stroke, strokeM1, strokeM2;
    bool remapSlashShift = false;
    bool remapFlipZy = false;
    bool remapFlipWinAlt = false;

    cout << "setup... " << endl;
    Sleep(500);

    raise_process_priority();
    context = interception_create_context();
    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

#pragma region setup
    {  
        cout << endl << "Press Shift";
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        if (stroke.code == SCANCODE_LSLASH)
            remapSlashShift = true;

        cout << endl << "Press Z";
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        if (stroke.code == SCANCODE_Z)
            remapFlipZy = true;

        cout << endl << "Press CMD/WIN";
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1);
        if (stroke.code == SCANCODE_LALT)
            remapFlipWinAlt = true;

        cout << endl << endl << (remapSlashShift ? "ON :" : "OFF:") << "Slashes -> Shift ";
        cout << endl << (remapFlipZy ? "ON :" : "OFF:") << "Z<->Y ";
        cout << endl << (remapFlipWinAlt ? "ON :" : "OFF:") << "ALT<->WIN ";

        cout << endl <<  endl << "akeymap running..." << "Press left (CTRL + ALT + ESC) to stop." << endl;

        strokeM1 = stroke;
        strokeM2 = stroke;

    }
#pragma endregion setup 


    while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0)
    {
        if (stroke.code == SCANCODE_ESC &&
            strokeM1.code == SCANCODE_LALT &&
            strokeM2.code == SCANCODE_LCTRL)
        {
            break;
        }

        if(DEBUGMODE)
            cout << " " << hex << stroke.code;

        switch (stroke.code)
        {
            case SCANCODE_Z:		
                if(remapFlipZy)
                    stroke.code = SCANCODE_Y;           
                break;
            case SCANCODE_Y:		
                if (remapFlipZy)
                    stroke.code = SCANCODE_Z;
                break;
            case SCANCODE_LSLASH:
                if (remapSlashShift)
                    stroke.code = SCANCODE_LSHIFT;
                break;
            case SCANCODE_RSLASH:
                if (remapSlashShift)
                    stroke.code = SCANCODE_RSHIFT;
                break;
            case SCANCODE_LWIN:
                if (remapFlipWinAlt)
                    stroke.code = SCANCODE_LALT;
                break;
            case SCANCODE_LALT:
                if (remapFlipWinAlt)
                    stroke.code = SCANCODE_LWIN;
                break;
        }
        
        strokeM2 = strokeM1;
        strokeM1 = stroke;

        interception_send(context, device, (InterceptionStroke *)&stroke, 1);

    }

    interception_destroy_context(context);

    cout << endl << "bye" << endl;
    return 0;
}


