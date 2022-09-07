/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <string>
#include <hardware/i2c.h>
#include "OneBitDisplay.h"
#include "BoardConfig.h"
#include "gpaddon.h"
#include "gamepad.h"
#include <vector>
#include <array>

#ifndef HAS_I2C_DISPLAY
#define HAS_I2C_DISPLAY -1
#endif

#ifndef DISPLAY_I2C_ADDR
#define DISPLAY_I2C_ADDR 0x3C
#endif

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN -1
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN -1
#endif

#ifndef I2C_BLOCK
#define I2C_BLOCK i2c0
#endif

#ifndef I2C_SPEED
#define I2C_SPEED 800000
#endif

#ifndef DISPLAY_SIZE
#define DISPLAY_SIZE OLED_128x64
#endif

#ifndef DISPLAY_FLIP
#define DISPLAY_FLIP 0
#endif

#ifndef DISPLAY_INVERT
#define DISPLAY_INVERT 0
#endif

#ifndef DISPLAY_USEWIRE
#define DISPLAY_USEWIRE 1
#endif

// i2c Display Module
#define I2CDisplayName "I2CDisplay"

// i2C OLED Display
class I2CDisplayAddon : public GPAddon
{
public:
	virtual bool available();  // GPAddon
	virtual void setup();
	virtual void process();
	virtual std::string name() { return I2CDisplayName; }
	void clearScreen(int render); // DisplayModule
	void drawStickless(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawWasdBox(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawArcadeStick(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawStatusBar();
	void drawText(int startX, int startY, std::string text);
	void initMenu(char**);
	//Adding my stuff here, remember to sort before PR
	void drawDiamond(int cx, int cy, int size, uint8_t colour, uint8_t filled);
	void drawUDLR(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawMAMEA(int startX, int startY, int buttonSize, int buttonPadding);
	void drawMAMEB(int startX, int startY, int buttonSize, int buttonPadding);
	void drawKeyboardAngled(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawVewlix(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawVewlix7(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawSega2p(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawNoir8(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawCapcom(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawCapcom6(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawSticklessButtons(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawWasdButtons(int startX, int startY, int buttonRadius, int buttonPadding);
	void drawArcadeButtons(int startX, int startY, int buttonRadius, int buttonPadding);
	//void drawSplashScreen(int splashMode, int splashSpeed);
	void drawDancepadA(int startX, int startY, int buttonSize, int buttonPadding);
	void drawDancepadB(int startX, int startY, int buttonSize, int buttonPadding);
	uint8_t ucBackBuffer[1024];
	OBDISP obd;
	std::string statusBar;
	// State Engine
	//TODO: Clean this up! OMG UGLY!
	class State
	{
	public:
		State() {}
		virtual ~State() {}
		virtual void enter(I2CDisplayAddon*) = 0;
		virtual bool process(I2CDisplayAddon*) = 0;
    	virtual void exit() = 0;
	}; //TODO: Can we setup these classes outside of the addon class?
	class StaticSplashState : public State
	{
	public:
		StaticSplashState() {}
		~StaticSplashState() {}
		void enter(I2CDisplayAddon*);
		bool process(I2CDisplayAddon*);
    	void exit();
	private:
		double counter { 0 };
		const double ttl { 3750 }; // x2 //TODO: This shouldn't be x2
	};
	class CloseinSplashState : public State
	{
	public:
		CloseinSplashState() {}
		~CloseinSplashState() {}
		void enter(I2CDisplayAddon*);
		bool process(I2CDisplayAddon*);
    	void exit();
	private:
		int splashSpeed = 90; //TODO: Remove
		int startMils; //TODO: Remove
		int stopMils = 7500; //TODO: Remove
		double counter { 0 };
		int y { 0 };
		int y2 { 64 };
		const double ttl { 7500 }; 
	};
	class CustomcloseinSplashState : public State
	{
	public:
		CustomcloseinSplashState() {}
		~CustomcloseinSplashState() {}
		void enter(I2CDisplayAddon*);
		bool process(I2CDisplayAddon*);
    	void exit();
	private:
		int splashSpeed = 90; //TODO: Remove
		int startMils; //TODO: Remove
		int stopMils = 7500; //TODO: Remove
		double counter { 0 };
		int y { 0 };
		int y2 { 0 };
		const double ttl { 7500 }; 
	};
	class DisplayState : public State
	{
	public:
		DisplayState() {}
		~DisplayState() {}
		void enter(I2CDisplayAddon*);
		bool process(I2CDisplayAddon*);
    	void exit();
	} displayState; //TODO: Does not need to be static. new/delete code needed.
	class MessageState : public State
	{
	public:
		MessageState();
		~MessageState() {}
		void enter(I2CDisplayAddon*);
		bool process(I2CDisplayAddon*);
		void exit();
		void send(std::string);
	private:
		double counter { 0 };
		const double height { 11 };
		const double ttl { 1000 }; // This is actually x3 //TODO: This shouldn't be x3
		int step { 0 };
		int y { 54 };
		const int sy { 53 };
		int t { 54 };
		std::vector<std::string> queue;
		//std::vector<uint8_t*> queue;
		//std::vector<uint8_t[1024]> queue;
		//std::vector<std::array<uint8_t*, 1024>> queue;
		uint8_t ucBackBuffer[1024];
		OBDISP obd;
	} messageState; //MessageState needs to always be active and will not be swapped like the other display states.

	State *state;
	State *nextState; //TODO: We don't need the extra pointer, fix after we fix the static states.
	void setState(State*, bool);

	//Message Center
	//Todo: Is this needed still?
	int msgx { 0 };
	int msgy { 0 };
	int msghalt { 0 };

	//Delta time
	uint32_t dt { 0 };
	uint32_t ldt { getMillis() };
};

#endif
