/*
  IR REMOTE ADD-ON - TEST
  Upload this on its own to check the IR receiver and the remote: open the Serial
  Monitor (115200 baud) and press the buttons - it shows which button you pressed and
  which car letter it sends. (The car does not move with this test.)

  To add the remote to the car's program, see README.md in this folder.
*/

#include "IR_Remote_Addon.h"

bool slowMode = false;

// Instead of driving the car, this test just shows the letter
void handleCommand(char c)
{
    Serial.print(F("button 0x"));
    Serial.print(IrReceiver.decodedIRData.command, HEX);
    Serial.print(F(" -> letter "));
    Serial.println(c);

    if (c == 'Q')
        slowMode = true;
    if (c == 'W')
        slowMode = false;
}

void setup()
{
    Serial.begin(115200);
    irSetup();
    Serial.println(F("IR remote test: press a button"));
}

void loop()
{
    irUpdate();
}
