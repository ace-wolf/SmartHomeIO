
void changePowerState(bool power)
{
	relaisOn = power;
	if (power)
	{
		Serial.println("Switching on");
		memSetString("1", memoryAddresses[7]);
		eepromCommit();
		digitalWrite(RELAIS_PIN, HIGH);
	}
	else
	{
		Serial.println("Switching off");
		memSetString("0", memoryAddresses[7]);
		eepromCommit();
		digitalWrite(RELAIS_PIN, LOW);
	}

}


void buttonWatcher()
{
	int buttonState = !digitalRead(BUTTON_PIN);
	if (buttonState != lastButtonState)
	{
		lastButtonState = buttonState;
		if (buttonState == HIGH)
		{
			Serial.println("Button pressed");
			if (relaisOn)
			{
				changePowerState(false);
			}
			else
			{
				changePowerState(true);
			}
		}
	}
}