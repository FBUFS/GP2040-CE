/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#include "addons/i2cdisplay.h"
#include "enums.h"
#include "helper.h"
#include "storagemanager.h"
#include "pico/stdlib.h"
#include "bitmaps.h"

bool I2CDisplayAddon::available() {
	BoardOptions boardOptions = Storage::getInstance().getBoardOptions();
	return boardOptions.hasI2CDisplay && boardOptions.i2cSDAPin != -1 && boardOptions.i2cSCLPin != -1;
}

void I2CDisplayAddon::setup() {
	BoardOptions boardOptions = Storage::getInstance().getBoardOptions();
	obdI2CInit(&obd,
	    boardOptions.displaySize,
		boardOptions.displayI2CAddress,
		boardOptions.displayFlip,
		boardOptions.displayInvert,
		DISPLAY_USEWIRE,
		boardOptions.i2cSDAPin,
		boardOptions.i2cSCLPin,
		boardOptions.i2cBlock == 0 ? i2c0 : i2c1,
		-1,
		boardOptions.i2cSpeed);
	obdSetContrast(&obd, 0xFF);
	obdSetBackBuffer(&obd, ucBackBuffer);
	clearScreen(1);
	//State setups
	
	// Setup splash mode
	switch (1)
	{
		case STATICSPLASH: // Default, display static or custom image
            //setState(new StaticSplashState(), 0);
			break;
		case CLOSEIN: // Close-in. Animate the GP2040 logo
			setState(new CloseinSplashState(), 0);
			break;
        case CLOSEINCUSTOM: // Close-in on custom image or delayed close-in if custom image does not exist
            //setState(new CloseinCustomSplashState(), 0);
            break;
		default:
			setState(&displayState, 0);
	}
	
}

void I2CDisplayAddon::process()
{
	// Calc DT now, should be a method.
	int millis = getMillis();
	dt = millis - ldt;
	ldt = millis;
	
	// Process active state, switch if asked to
	if (state->process(this)) setState(nextState, 1);
	
	// Process message queue
	messageState.process(this);

	//Drop DeltaTime
	drawText(0,1, std::to_string(dt));

	obdDumpBuffer(&obd, NULL);

}


void I2CDisplayAddon::DisplayState::enter(I2CDisplayAddon* st) {
	std::string msgText { "Entered Display State" };
	st->messageState.send(msgText);
}

bool I2CDisplayAddon::DisplayState::process(I2CDisplayAddon* st)
{
	st->clearScreen(0);
	bool configMode = Storage::getInstance().GetConfigMode();
	if (configMode == true ) {
		st->drawStatusBar();
		st->drawText(0, 3, "[Web Config Mode]");
		st->drawText(0, 4, std::string("GP2040-CE : ") + std::string(GP2040VERSION));
	} else {
		st->drawStatusBar();
		switch (BUTTON_LAYOUT)
		{
			case BUTTON_LAYOUT_STICK:
				st->drawArcadeStick(8, 28, 8, 2);
				break;

			case BUTTON_LAYOUT_STICKLESS:
				st->drawStickless(8, 20, 8, 2);
				break;

			case BUTTON_LAYOUT_BUTTONS_ANGLED:
				st->drawWasdBox(8, 28, 7, 3);
				break;
			case BUTTON_LAYOUT_BUTTONS_BASIC:
				st->drawUDLR(8, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_KEYBOARD_ANGLED:
				st->drawKeyboardAngled(18, 28, 5, 2);
				break;
			case BUTTON_LAYOUT_KEYBOARDA:
				st->drawMAMEA(8, 28, 10, 1);
				break;
			case BUTTON_LAYOUT_DANCEPADA:
				st->drawDancepadA(39, 12, 15, 2);
				break;
		}

		switch (BUTTON_LAYOUT_RIGHT)
		{
			case BUTTON_LAYOUT_ARCADE:
				st->drawArcadeButtons(8, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_STICKLESSB:
				st->drawSticklessButtons(8, 20, 8, 2);
				break;
			case BUTTON_LAYOUT_BUTTONS_ANGLEDB:
				st->drawWasdButtons(8, 28, 7, 3);
				break;
			case BUTTON_LAYOUT_VEWLIX:
				st->drawVewlix(8, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_VEWLIX7:
				st->drawVewlix7(8, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_CAPCOM:
				st->drawCapcom(6, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_CAPCOM6:
				st->drawCapcom6(16, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_SEGA2P:
				st->drawSega2p(8, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_NOIR8:
				st->drawNoir8(8, 28, 8, 2);
				break;
			case BUTTON_LAYOUT_KEYBOARDB:
				st->drawMAMEB(68, 28, 10, 1);
				break;
			case BUTTON_LAYOUT_DANCEPADB:
				st->drawDancepadB(39, 12, 15, 2);
				break;
		}
	}

	//obdDumpBuffer(&st->obd, NULL);

	return 0;
}


void I2CDisplayAddon::DisplayState::exit() {}

void I2CDisplayAddon::clearScreen(int render) {
	obdFill(&obd, 0, render);
}

void I2CDisplayAddon::drawDiamond(int cx, int cy, int size, uint8_t colour, uint8_t filled)
{
	if (filled) {
		int i;
		for (i = 0; i < size; i++) {
			obdDrawLine(&obd, cx - i, cy - size + i, cx + i, cy - size + i, colour, 0);
			obdDrawLine(&obd, cx - i, cy + size - i, cx + i, cy + size - i, colour, 0);
		}
		obdDrawLine(&obd, cx - size, cy, cx + size, cy, colour, 0); // Fill in the middle
	}
	obdDrawLine(&obd, cx - size, cy, cx, cy - size, colour, 0);
	obdDrawLine(&obd, cx, cy - size, cx + size, cy, colour, 0);
	obdDrawLine(&obd, cx + size, cy, cx, cy + size, colour, 0);
	obdDrawLine(&obd, cx, cy + size, cx - size, cy, colour, 0);
}

void I2CDisplayAddon::drawStickless(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	obdPreciseEllipse(&obd, startX, startY, buttonRadius, buttonRadius, 1, gamepad->pressedLeft());
	obdPreciseEllipse(&obd, startX + buttonMargin, startY, buttonRadius, buttonRadius, 1, gamepad->pressedDown());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 1.875), startY + (buttonMargin / 2), buttonRadius, buttonRadius, 1, gamepad->pressedRight());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.25), startY + buttonMargin * 1.875, buttonRadius, buttonRadius, 1, gamepad->pressedUp());
}

void I2CDisplayAddon::drawWasdBox(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// WASD
	obdPreciseEllipse(&obd, startX, startY + buttonMargin * 0.5, buttonRadius, buttonRadius, 1, gamepad->pressedLeft());
	obdPreciseEllipse(&obd, startX + buttonMargin, startY + buttonMargin * 0.875, buttonRadius, buttonRadius, 1, gamepad->pressedDown());
	obdPreciseEllipse(&obd, startX + buttonMargin * 1.5, startY - buttonMargin * 0.125, buttonRadius, buttonRadius, 1, gamepad->pressedUp());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2), startY + buttonMargin * 1.25, buttonRadius, buttonRadius, 1, gamepad->pressedRight());
}

void I2CDisplayAddon::drawUDLR(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// UDLR
	obdPreciseEllipse(&obd, startX, startY + buttonMargin / 2, buttonRadius, buttonRadius, 1, gamepad->pressedLeft());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 0.875), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedUp());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 0.875), startY + buttonMargin * 1.25, buttonRadius, buttonRadius, 1, gamepad->pressedDown());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 1.625), startY + buttonMargin / 2, buttonRadius, buttonRadius, 1, gamepad->pressedRight());
}

void I2CDisplayAddon::drawArcadeStick(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// Stick
	obdPreciseEllipse(&obd, startX + (buttonMargin / 2), startY + (buttonMargin / 2), buttonRadius * 1.25, buttonRadius * 1.25, 1, 0);
	
	if (gamepad->pressedUp()) {
		if (gamepad->pressedLeft()) {
			obdPreciseEllipse(&obd, startX + (buttonMargin / 5), startY + (buttonMargin / 5), buttonRadius, buttonRadius, 1, 1);
		} else if (gamepad->pressedRight()) {
			obdPreciseEllipse(&obd, startX + (buttonMargin * 0.875), startY + (buttonMargin / 5), buttonRadius, buttonRadius, 1, 1);
		} else {
			obdPreciseEllipse(&obd, startX + (buttonMargin / 2), startY, buttonRadius, buttonRadius, 1, 1);
		}
	} else if (gamepad->pressedDown()) {
		if (gamepad->pressedLeft()) {
			obdPreciseEllipse(&obd, startX + (buttonMargin / 5), startY + (buttonMargin * 0.875), buttonRadius, buttonRadius, 1, 1);
		} else if (gamepad->pressedRight()) {
			obdPreciseEllipse(&obd, startX + (buttonMargin * 0.875), startY + (buttonMargin * 0.875), buttonRadius, buttonRadius, 1, 1);
		} else {
			obdPreciseEllipse(&obd, startX + buttonMargin / 2, startY + buttonMargin, buttonRadius, buttonRadius, 1, 1);
		}
	} else if (gamepad->pressedLeft()) {
		obdPreciseEllipse(&obd, startX, startY + buttonMargin / 2, buttonRadius, buttonRadius, 1, 1);
	} else if (gamepad->pressedRight()) {
		obdPreciseEllipse(&obd, startX + buttonMargin, startY + buttonMargin / 2, buttonRadius, buttonRadius, 1, 1);
	} else {
		obdPreciseEllipse(&obd, startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius, buttonRadius, 1, 1);
	}
}

void I2CDisplayAddon::drawMAMEA(int startX, int startY, int buttonSize, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + buttonSize;

	// MAME
	obdRectangle(&obd, startX, startY + buttonMargin, startX + buttonSize, startY + buttonSize + buttonMargin, 1, gamepad->pressedLeft());
	obdRectangle(&obd, startX + buttonMargin, startY + buttonMargin, startX + buttonSize + buttonMargin, startY + buttonSize + buttonMargin, 1, gamepad->pressedDown());
	obdRectangle(&obd, startX + buttonMargin, startY, startX + buttonSize + buttonMargin, startY + buttonSize, 1, gamepad->pressedUp());
	obdRectangle(&obd, startX + buttonMargin * 2, startY + buttonMargin, startX + buttonSize + buttonMargin * 2, startY + buttonSize + buttonMargin, 1, gamepad->pressedRight());
}

void I2CDisplayAddon::drawMAMEB(int startX, int startY, int buttonSize, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + buttonSize;

	// 6-button MAME Style
	obdRectangle(&obd, startX, startY, startX + buttonSize, startY + buttonSize, 1, gamepad->pressedB3());
	obdRectangle(&obd, startX + buttonMargin, startY, startX + buttonSize + buttonMargin, startY + buttonSize, 1, gamepad->pressedB4());
	obdRectangle(&obd, startX + buttonMargin * 2, startY, startX + buttonSize + buttonMargin * 2, startY + buttonSize, 1, gamepad->pressedR1());

	obdRectangle(&obd, startX, startY + buttonMargin, startX + buttonSize, startY + buttonMargin + buttonSize, 1, gamepad->pressedB1());
	obdRectangle(&obd, startX + buttonMargin, startY + buttonMargin, startX + buttonSize + buttonMargin, startY + buttonMargin + buttonSize, 1, gamepad->pressedB2());
	obdRectangle(&obd, startX + buttonMargin * 2, startY + buttonMargin, startX + buttonSize + buttonMargin * 2, startY + buttonMargin + buttonSize, 1, gamepad->pressedR2());

}

void I2CDisplayAddon::drawKeyboardAngled(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// MixBox
	drawDiamond(startX, startY, buttonRadius, 1, gamepad->pressedLeft());
	drawDiamond(startX + buttonMargin / 2, startY + buttonMargin / 2, buttonRadius, 1, gamepad->pressedDown());
	drawDiamond(startX + buttonMargin, startY, buttonRadius, 1, gamepad->pressedUp());
	drawDiamond(startX + buttonMargin, startY + buttonMargin, buttonRadius, 1, gamepad->pressedRight());
}

void I2CDisplayAddon::drawVewlix(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Vewlix
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + (buttonMargin * 0.2), buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75) - (buttonMargin / 3), startY + buttonMargin + (buttonMargin * 0.2), buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawVewlix7(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Vewlix
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + (buttonMargin * 0.2), buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75) - (buttonMargin / 3), startY + buttonMargin + (buttonMargin * 0.2), buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	//obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75) - (buttonMargin / 3), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawSega2p(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Sega2P
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + (buttonMargin / 3), buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY, buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + buttonMargin + (buttonMargin / 3), buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawNoir8(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Noir8
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + (buttonMargin / 3.5), buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY, buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + buttonMargin + (buttonMargin / 3.5), buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawCapcom(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button Capcom
	obdPreciseEllipse(&obd, startX + buttonMargin * 3.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 6.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + buttonMargin * 3.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 6.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawCapcom6(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 6-button Capcom
	obdPreciseEllipse(&obd, startX + buttonMargin * 3.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.25, startY, buttonRadius, buttonRadius, 1, gamepad->pressedR1());

	obdPreciseEllipse(&obd, startX + buttonMargin * 3.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedR2());
}

void I2CDisplayAddon::drawSticklessButtons(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button
	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY, buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY, buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + (buttonMargin * 2.75), startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 3.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 4.75), startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + (buttonMargin * 5.75), startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawWasdButtons(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button
	obdPreciseEllipse(&obd, startX + buttonMargin * 3.625, startY, buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.625, startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.625, startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 6.625, startY, buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + buttonMargin * 3.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.25, startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.25, startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 6.25, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

void I2CDisplayAddon::drawArcadeButtons(int startX, int startY, int buttonRadius, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + (buttonRadius * 2);

	// 8-button
	obdPreciseEllipse(&obd, startX + buttonMargin * 3.125, startY, buttonRadius, buttonRadius, 1, gamepad->pressedB3());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.125, startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB4());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.125, startY - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 6.125, startY, buttonRadius, buttonRadius, 1, gamepad->pressedL1());

	obdPreciseEllipse(&obd, startX + buttonMargin * 2.875, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedB1());
	obdPreciseEllipse(&obd, startX + buttonMargin * 3.875, startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedB2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 4.875, startY + buttonMargin - (buttonMargin / 4), buttonRadius, buttonRadius, 1, gamepad->pressedR2());
	obdPreciseEllipse(&obd, startX + buttonMargin * 5.875, startY + buttonMargin, buttonRadius, buttonRadius, 1, gamepad->pressedL2());
}

// I pulled this out of my PR, brought it back because of recent talks re: SOCD and rhythm games
// Enjoy!

void I2CDisplayAddon::drawDancepadA(int startX, int startY, int buttonSize, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + buttonSize;

	obdRectangle(&obd, startX, startY + buttonMargin, startX + buttonSize, startY + buttonSize + buttonMargin, 1, gamepad->pressedLeft());
	obdRectangle(&obd, startX + buttonMargin, startY + buttonMargin * 2, startX + buttonSize + buttonMargin, startY + buttonSize + buttonMargin * 2, 1, gamepad->pressedDown());
	obdRectangle(&obd, startX + buttonMargin, startY, startX + buttonSize + buttonMargin, startY + buttonSize, 1, gamepad->pressedUp());
	obdRectangle(&obd, startX + buttonMargin * 2, startY + buttonMargin, startX + buttonSize + buttonMargin * 2, startY + buttonSize + buttonMargin, 1, gamepad->pressedRight());
}

void I2CDisplayAddon::drawDancepadB(int startX, int startY, int buttonSize, int buttonPadding)
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	const int buttonMargin = buttonPadding + buttonSize;
	
	obdRectangle(&obd, startX, startY, startX + buttonSize, startY + buttonSize, 1, gamepad->pressedB2()); // Up/Left
	obdRectangle(&obd, startX, startY + buttonMargin * 2, startX + buttonSize, startY + buttonSize + buttonMargin * 2, 1, gamepad->pressedB4()); // Down/Left
	obdRectangle(&obd, startX + buttonMargin * 2, startY, startX + buttonSize + buttonMargin * 2, startY + buttonSize, 1, gamepad->pressedB1()); // Up/Right
	obdRectangle(&obd, startX + buttonMargin * 2, startY + buttonMargin * 2, startX + buttonSize + buttonMargin * 2, startY + buttonSize + buttonMargin * 2, 1, gamepad->pressedB3()); // Down/Right
}

void I2CDisplayAddon::setState(State* ptr, bool runExit)
{
    if (runExit == 1) state->exit(); // Exit current state
    ptr->enter(this); // Enter/Setup new state
	state = ptr; // Make new state active
}



void I2CDisplayAddon::CloseinSplashState::enter(I2CDisplayAddon* st)
{
	startMils = getMillis();
	std::string msgText { "Entered Splash State" };
	st->messageState.send(msgText);
}

bool I2CDisplayAddon::CloseinSplashState::process(I2CDisplayAddon *st)
{
	st->clearScreen(0);
	counter = counter + st->dt;
	if (counter < ttl)
	{
		y = 39 * ( counter / ttl);
		y2 = 20 * ( counter / ttl);
	}

	obdDrawSprite(&st->obd, (uint8_t *)bootLogoTop, 43, 39, 6, 43, y - 39, 1);
	obdDrawSprite(&st->obd, (uint8_t *)bootLogoBottom, 80, 21, 10, 24, 64 - y2, 1);

	if (counter > (ttl * 2)) {
		st->nextState = &st->displayState; 
		return 1;
	} else {
		return 0;
	}
}

void I2CDisplayAddon::CloseinSplashState::exit() {
	delete this;
}

/*
void I2CDisplayAddon::StaticSplashState::enter(I2CDisplayAddon* st)
{
	startMils = getMillis();
	std::string msgText { "Entered Splash State" };
	st->messageState.send(msgText);
}

bool I2CDisplayAddon::StaticSplashState::process(I2CDisplayAddon *st)
{
    const int mils = getMillis();
	const int milsPast = mils - startMils;
	st->clearScreen(0);

	if ((sizeof(splashCustom) / sizeof(*splashCustom)) > 0) {
		obdDrawSprite(&st->obd, (uint8_t *)splashCustom, 128, 64, 16, 0, 0, 1);
	} else {
		obdDrawSprite(&st->obd, (uint8_t *)splashImage, 128, 64, 16, 0, 0, 1);
	}

	if (milsPast > stopMils) {
		st->nextState = &st->displayState; 
		return 1;
	} else {
		return 0;
	}
}

void I2CDisplayAddon::StaticSplashState::exit() {}

void I2CDisplayAddon::CloseinCustomSplashState::enter(I2CDisplayAddon* st)
{
	startMils = getMillis();
	std::string msgText { "Entered Splash State" };
	st->messageState.send(msgText);
}

bool I2CDisplayAddon::CloseinCustomSplashState::process(I2CDisplayAddon *st)
{
    const int mils = getMillis();
	const int milsPast = mils - startMils;
	st->clearScreen(0);

	if ((sizeof(splashCustom) / sizeof(*splashCustom)) > 0) {
		obdDrawSprite(&st->obd, (uint8_t *)splashCustom, 128, 64, 16, 0, 0, 1);
	}
	if (milsPast > 2500) {
		int milss = milsPast - 2500;
		obdRectangle(&st->obd, 0, 0, 127, 1 + (milss / splashSpeed), 0, 1);
		obdRectangle(&st->obd, 0, 63, 127, 62 - (milss / (splashSpeed * 2)), 0, 1);
		obdDrawSprite(&st->obd, (uint8_t *)bootLogoTop, 43, 39, 6, 43, std::min<int>((milss / splashSpeed) - 39, 0), 1);
		obdDrawSprite(&st->obd, (uint8_t *)bootLogoBottom, 80, 21, 10, 24, std::max<int>(64 - (milss / (splashSpeed * 2)), 44), 1);
	}

	if (milsPast > stopMils) {
		st->nextState = &st->displayState; 
		return 1;
	} else {
		return 0;
	}
}

void I2CDisplayAddon::CloseinCustomSplashState::exit() {}
*/

void I2CDisplayAddon::drawText(int x, int y, std::string text) {
	obdWriteString(&obd, 0, x, y, (char*)text.c_str(), FONT_6x8, 0, 0);
}

void I2CDisplayAddon::drawStatusBar()
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	BoardOptions boardOptions = Storage::getInstance().getBoardOptions();

	// Limit to 21 chars with 6x8 font for now
	statusBar.clear();

	switch (gamepad->options.inputMode)
	{
		case INPUT_MODE_HID:    statusBar += "DINPUT"; break;
		case INPUT_MODE_SWITCH: statusBar += "SWITCH"; break;
		case INPUT_MODE_XINPUT: statusBar += "XINPUT"; break;
		case INPUT_MODE_CONFIG: statusBar += "CONFIG"; break;
	}

	if ( boardOptions.pinButtonTurbo != (uint8_t)-1 ) {
		statusBar += " T";
		if ( boardOptions.turboShotCount < 10 ) // padding
			statusBar += "0";
		statusBar += std::to_string(boardOptions.turboShotCount);
	} else {
		statusBar += "      "; // no turbo, don't show Txx setting
	}
	switch (gamepad->options.dpadMode)
	{

		case DPAD_MODE_DIGITAL:      statusBar += " DP"; break;
		case DPAD_MODE_LEFT_ANALOG:  statusBar += " LS"; break;
		case DPAD_MODE_RIGHT_ANALOG: statusBar += " RS"; break;
	}

	switch (gamepad->options.socdMode)
	{
		case SOCD_MODE_NEUTRAL:               statusBar += " SOCD-N"; break;
		case SOCD_MODE_UP_PRIORITY:           statusBar += " SOCD-U"; break;
		case SOCD_MODE_SECOND_INPUT_PRIORITY: statusBar += " SOCD-L"; break;
	}
	drawText(0, 0, statusBar);
}

I2CDisplayAddon::MessageState::MessageState()
{
	obdCreateVirtualDisplay(&obd, 128,32, ucBackBuffer);
}

void I2CDisplayAddon::MessageState::enter(I2CDisplayAddon*)
{

}
bool I2CDisplayAddon::MessageState::process(I2CDisplayAddon* st)
{
	if (queue.size() > 0)
	{	
		uint8_t bitmap[1024];
		std::string text = queue.at(0);
		if (counter <= 0)
		{
			counter = ttl;
			++step;
		}
		switch (step)
		{
			case 1:
				counter = counter - st->dt;
				y = height * ( counter / ttl );
				break;
			case 2:
				counter = counter - st->dt;
				break;
			case 3:
				counter = counter - st->dt;
				y = height - height * ( counter / ttl );
				break;
			case 4:
				step = 0;
				counter = 0;
				queue.erase(queue.begin());
				break;
		}
		//obdWriteString(&st->obd, 0, 0, 1, (char*)std::to_string(y).c_str(), FONT_6x8, 0, 0);
		//obdWriteString(&st->obd, 0, 0, 2, (char*)std::to_string(t).c_str(), FONT_6x8, 0, 0);
		//obdWriteString(&st->obd, 0, 0, 3, (char*)std::to_string(counter).c_str(), FONT_6x8, 0, 0);
		//obdWriteString(&st->obd, 0, 0, 3, (char*)std::to_string(step).c_str(), FONT_6x8, 0, 0);

		// This looks like it's expensive, so don't do this often.
		// obdDumpWindow doesn't work as described so we're copying the virtual display to a bitmap.
		obdDrawLine(&st->obd, 0, sy + y, 127, sy + y, 1, 0);
		obdRectangle(&st->obd, 0, sy + y + 1, 127, 63, 0, 1);
		obdWriteString(&obd, 0, 0, 0, (char*)text.c_str(), FONT_6x8, 0, 0);
		obdCopy(&obd, 2, bitmap);
		obdDrawSprite(&st->obd, bitmap, 128, 32, 16, 0, sy + y + 2, 1);
	}
	return 0;
}
void I2CDisplayAddon::MessageState::exit()
{

}
void I2CDisplayAddon::MessageState::send(std::string str)
{
	queue.push_back(str);
}