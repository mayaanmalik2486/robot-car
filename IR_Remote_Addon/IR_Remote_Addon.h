// IR REMOTE ADD-ON for the Robot Car  (how to add it: see README.md in this folder)
//
// Every remote button "presses" a phone-app letter, so the remote does exactly what
// the app does. Arrows keep going while you hold them, and stop when you let go.
// Needs the IRremote library (version 4.x, by Armin Joachimsmeyer):
// Arduino IDE > Tools > Manage Libraries > search "IRremote" > Install.

#define DECODE_NEC                 // only understand NEC remotes (the car's is one): a smaller program
#include <IRremote.hpp>

const byte IR_PIN = 8;                     // the IR receiver's signal wire (sensor shield row 8)
const unsigned long IR_LET_GO_MS = 250;    // no signal from a held arrow for this long = let go

void handleCommand(char c);        // (these two are in the main program)
extern bool slowMode;

// Which letter each remote button sends (change them if you like - the letters are
// listed at the top of the main program, "BLUETOOTH LETTERS")
char irLetter(byte button)
{
    switch (button)
    {
    case 0x18: return 'F';                       // UP      forward
    case 0x52: return 'B';                       // DOWN    backward
    case 0x08: return 'L';                       // LEFT    spin left
    case 0x5A: return 'R';                       // RIGHT   spin right
    case 0x1C: return 'S';                       // OK      stop everything
    case 0x45: return 'U';                       // 1       180-degree turn
    case 0x46: return slowMode ? 'W' : 'Q';      // 2       slow / full speed
    case 0x47: return 'u';                       // 3       smooth brake
    case 0x44: return 'T';                       // 4       turbo
    case 0x40: return '4';                       // 5       dance (again = line up with the beat)
    case 0x43: return '1';                       // 6       chaos
    case 0x07: return 'G';                       // 7       curve left
    case 0x15: return 'H';                       // 8       curve right
    case 0x09: return 'I';                       // 9       search
    case 0x16: return '2';                       // *       auto forward
    case 0x19: return '3';                       // 0       donut
    case 0x0D: return 'J';                       // #       auto backward
    }
    return 0;                                    // a button with no job
}

byte irHeldButton = 0;             // the arrow being held down (0 = none)
unsigned long irHeldAt = 0;        // when its last "still held" signal came

// Call in setup(), on the line BEFORE setupBuzzer()
void irSetup()
{
    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);    // (the "L" LED on the board flickers when the remote sends)
}

// Call in loop()
void irUpdate()
{
    if (irHeldButton && millis() - irHeldAt > IR_LET_GO_MS)   // the arrow was let go: stop
    {
        irHeldButton = 0;
        handleCommand('S');
    }

    if (!IrReceiver.decode())
        return;

    byte button = IrReceiver.decodedIRData.command;
    bool stillHeld = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
    bool known = IrReceiver.decodedIRData.protocol != UNKNOWN;
    IrReceiver.resume();                         // get ready for the next button

    if (stillHeld)                               // the remote says "still held down" about every 0.1 s
    {
        if (button == irHeldButton)
            irHeldAt = millis();
        return;
    }

    char letter = irLetter(button);
    if (!known || letter == 0)
        return;

    bool arrow = (letter == 'F' || letter == 'B' || letter == 'L' || letter == 'R');
    irHeldButton = arrow ? button : 0;
    irHeldAt = millis();
    handleCommand(letter);
}
