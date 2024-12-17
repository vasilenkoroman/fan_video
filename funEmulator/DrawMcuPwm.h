#pragma once

#include <stdint.h>
#include <iostream>

#include <QPainter>
#include <QElapsedTimer>

#include "DrawBase.h"

namespace DrawMcuPwm
{
	struct McuPwmSettings
	{
		/*Type of PWM modulation.*/
		enum PWM_ALIGNS
		{
			/*PWM modulation left align:
			10000000 = LED 12.5%,
			11000000 = LED 25%,
			11110000 = LED 50%,
			11111100 = LED 75%,*/
			PWM_LEFT_ALIGN,

			/*PWM modulation center align:
			00011000 = LED 25%,
			00111100 = LED 50%,
			01111110 = LED 75%,*/
			PWM_CENTER_ALIGN
		};

		/*Type of PWM modulation.*/
		const PWM_ALIGNS pwmAlign = PWM_CENTER_ALIGN;

		uint32_t COLOR_BIT_DEPTH = 8;//1 to 16

		/* One MCU PWM cycle length.
			For example: if getMcuPwmCycleLengthTicks == 8:
			For 8 bit color channel center align, getMcuPwmCycleLengthTicks = 512.
			Better be const in MCU and set on compilation firmware, cut can be set dynamic.
		*/
		uint32_t getMcuPwmCycleLengthTicks()
		{
			if (pwmAlign == PWM_CENTER_ALIGN)
				return 2 << COLOR_BIT_DEPTH;
			return 1 << COLOR_BIT_DEPTH;
		}

		uint32_t cpuFrequencyHz = 24000000;

		uint32_t cpuFrequencyDivider = 2;// From 2 to 256

		/* MCU PWM base clock in hz. MCU_PWM_TICK_HZ = CPU_HZ / (2, 3, ...).
			For example CPU_HZ = 24000000.
			MCU_PWM_TICK_HZ = 24000000 / 2 = 12000000 hz
			Better be const in MCU and set on compilation firmware, cut can be set dynamic.
			*/
		double getMcuPwmTickHz()
		{
			return (double)this->cpuFrequencyHz / this->cpuFrequencyDivider;
		}

		double getMcuPwmTickUSec()
		{
			return 1.0 / this->getMcuPwmTickHz() * 1000000.0;
		}

		/* MCU PWM one cycle frequency in hz.
			For example if MCU_PWM_TICK_HZ == 12 MHz, getMcuPwmCycleLengthTicks = 512
			getMcuPwmCycleHz = 12000000 / 512 = 23437,5 hz*/
		double getMcuPwmCycleHz()
		{
			return (double)getMcuPwmTickHz() / this->getMcuPwmCycleLengthTicks();
		}

		double getMcuPwmCycleUSec()
		{
			return 1.0 / this->getMcuPwmCycleHz() * 1000000.0;
		}

		/* How many PWM cycles will be every cinema frame.
			For example if getMcuPwmCycleHz == 23437,5hz and kFps == 24
			MCU_PWM_CYCLE_EVERY_FRAME = 23437,5 / 24 = 976,5625*/
			//const double MCU_PWM_CYCLE_EVERY_REPAINT = getMcuPwmCycleHz / (double)fps;

			/* How many degrees moved blade every 1 cycle of PWM.
				For example 8640° / 23437,5hz = 0,36864°/pwmCycle.*/
		double getMcuPwmCycleDegrees()
		{
			return fanAngleSpeedDegreesEverySec / this->getMcuPwmCycleHz();
		}

		/* Count of PWM outputs for MCU. For NRF52833 it equals 16.*/
		uint8_t mcuPwmHwOutputs = 16;

		/*
		mcuPwmCountChannelsStrobe = 0, brightness = 100%, 16 LEDs, 5 RGB LEDs
		mcuPwmCountChannelsStrobe = 4, brightness = 25%, 48 LEDs, 16 RGB LEDs
		mcuPwmCountChannelsStrobe = 8, brightness = 12.5%, 64 LEDs, 21 RGB LEDs
		can set from 0 to mcuPwmHwOutputs - 1
		*/
		uint8_t mcuPwmCountChannelsStrobe = 4;

		uint8_t mcuPwmCountChannelsLed()
		{
			return this->mcuPwmHwOutputs - this->mcuPwmCountChannelsStrobe;//12
		}

		uint32_t getRgbLedCount()
		{
			return this->mcuPwmCountChannelsLed() * this->mcuPwmCountChannelsStrobe / 3;
		}

		const double ledStepPxBetweenRGB = 3;
	};




	inline void draw(
		QPainter& painter,
		//McuPwmSettings& settings,
		//int fps,
		uint64_t indexOfThisFrame,
		double fanCenter,
		double fanRadiusPx)
	{
		QElapsedTimer t;
		t.restart();

		McuPwmSettings settings;


		const double maxGlobalAngleThisRepaint = (indexOfThisFrame + 1) * fanAngleSpeedDegreesEveryFrame;
		double globalCurrentAngleDegrees = indexOfThisFrame * fanAngleSpeedDegreesEveryFrame;


		const double ledStepPx = fanRadiusPx / settings.getRgbLedCount();
		const double ledStartRadius = ledStepPx / 2;

		const double maxSpanAngle = settings.getMcuPwmCycleDegrees();

		for (uint64_t flash = 0; globalCurrentAngleDegrees < maxGlobalAngleThisRepaint; flash++)
		{
			const int offset = flash % 4;
			for (int color = 0; color < 3; color++)
			{
				painter.setPen(rgbColors[color]);

				for (int i = offset; i < settings.getRgbLedCount(); i += 4)
				{
					const double radius2 = ledStartRadius + ledStepPx * i;


					double radius3 = radius2 + color * settings.ledStepPxBetweenRGB;
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
