#pragma once

#include <stdint.h>
#include <iostream>

#include <QPainter>
#include <QElapsedTimer>

#include "DrawBase.h"

namespace DrawMcuPwm
{
	inline void draw(
		QPainter& painter,
		int fps,
		uint64_t counterFrames,
		double fanCenter,
		double fanRadiusPx)
	{
		QElapsedTimer t;
		t.restart();

		const double maxGlobalAngleThisRepaint = (counterFrames + 1) * fanAngleSpeedDegreesEveryFrame;
		double globalCurrentAngleDegrees = counterFrames * fanAngleSpeedDegreesEveryFrame;

		const double ledStepPxBetweenRGB = 1;
		/**
			PWM can work left align:
			10000000 = LED 12.5%,
			11000000 = LED 25%,
			11110000 = LED 50%,
			11111100 = LED 75%,
			PWM can work center align:
			00011000 = LED 25%,
			00111100 = LED 50%,
			01111110 = LED 75%,*/
		enum PWM_ALIGNS { PWM_LEFT_ALIGN, PWM_CENTER_ALIGN };

		const PWM_ALIGNS pwmAlign = PWM_LEFT_ALIGN;

		const uint32_t COLOR_BIT_DEPTH = 8;

		/** MCU PWM base clock in hz. MCU_PWM_TICK_HZ = CPU_HZ / (2, 3, ...).
			For example CPU_HZ = 24000000.
			MCU_PWM_TICK_HZ = 24000000 / 2 = 12000000
			Better be const in MCU and set on compilation firmware, cut can be set dynamic.
		*/
		const uint32_t MCU_PWM_TICK_HZ = 12 * 1000 * 1000;

		/** One MCU PWM cycle length.
			For example: if MCU_PWM_CYCLE_LENGTH_TICKS == 8:
			For 8 bit color channel center align, MCU_PWM_CYCLE_LENGTH_TICKS = 512.
			Better be const in MCU and set on compilation firmware, cut can be set dynamic.
		*/
		const uint32_t MCU_PWM_CYCLE_LENGTH_TICKS = 1 << COLOR_BIT_DEPTH;

		/** MCU PWM one cycle frequency in hz.
			For example if MCU_PWM_TICK_HZ == 12 MHz, MCU_PWM_CYCLE_LENGTH_TICKS = 512
			MCU_PWM_CYCLE_HZ = 12000000 / 512 = 23437,5hz*/
		const double MCU_PWM_CYCLE_HZ = (double)MCU_PWM_TICK_HZ / MCU_PWM_CYCLE_LENGTH_TICKS;

		/** How many PWM cycles will be every cinema frame.
			For example if MCU_PWM_CYCLE_HZ == 23437,5hz and kFps == 24
			MCU_PWM_CYCLE_EVERY_FRAME = 23437,5 / 24 = 976,5625*/
		const double MCU_PWM_CYCLE_EVERY_REPAINT = MCU_PWM_CYCLE_HZ / (double) fps;

		/** How many degrees moved blade every 1 cycle of PWM.
			For example 8640° / 23437,5hz = 0,36864°/pwmCycle.*/
		const double pwmCycleDegrees = fanAngleSpeedDegreesEverySec / MCU_PWM_CYCLE_HZ;

		const uint8_t MCU_PWM_OUTPUTS = 16;

		/**
			MCU_PWM_STROBE_CHANNELS = 0, brightness = 100%, 16 LEDs, 5 RGB LEDs
			MCU_PWM_STROBE_CHANNELS = 4, brightness = 25%, 48 LEDs, 16 RGB LEDs
			MCU_PWM_STROBE_CHANNELS = 8, brightness = 12.5%, 64 LEDs, 21 RGB LEDs
		*/
		const uint8_t MCU_PWM_STROBE_CHANNELS = 8;
		const uint8_t MCU_PWM_LED_OUTPUTS = MCU_PWM_OUTPUTS - MCU_PWM_STROBE_CHANNELS;//12
		const uint32_t RGB_LED_COUNT = MCU_PWM_LED_OUTPUTS * MCU_PWM_STROBE_CHANNELS / 3;//16


		const double ledStepPx = fanRadiusPx / (RGB_LED_COUNT);
		const double ledStartRadius = ledStepPx / 2;

		const double maxSpanAngle = pwmCycleDegrees;

		for (uint64_t flash = 0; globalCurrentAngleDegrees < maxGlobalAngleThisRepaint; flash++)
		{
			const int offset = flash % 4;
			for (int i = offset; i < RGB_LED_COUNT; i += 4)
			{
				const double radius2 = ledStartRadius + ledStepPx * i;
				for (int color = 0; color < 3; color++)
				{
					double radius3 = radius2 + color * ledStepPxBetweenRGB;
					painter.setPen(rgbColors[color]);
					uint8_t brightness = rand() % 256;
					double spanAngle = brightness / 256.0 * maxSpanAngle;
					QRectF r(
						fanCenter - radius3,
						fanCenter - radius3,
						radius3 * 2,
						radius3 * 2);
					painter.drawArc(r, (globalCurrentAngleDegrees - spanAngle / 2) * 16, spanAngle * 16);
				}
			}
			globalCurrentAngleDegrees += maxSpanAngle;
		}

		std::cout << "Async paint time=" << t.elapsed() << std::endl;
	}
};
