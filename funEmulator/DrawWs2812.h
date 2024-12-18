#pragma once
#include <stdint.h>
#include <iostream>

#include <QPainter>
#include <QElapsedTimer>

#include "DrawBase.h"

namespace DrawWs2812
{
	struct Ws2812Settings: public CommonFanSettings
	{
		inline static const double timeoutSecTransfer1Bit = 1.25 / 1000.0 / 1000.0;
		inline static const double timeoutSecTransfer24Bit = 30.0 / 1000.0 / 1000.0;
		inline static const double timeoutSecTransferEnd = 50.0 / 1000.0 / 1000.0;

		int rgbLedCount = 32;

		double getTimeoutSecTransferAll()
		{
			return this->timeoutSecTransferEnd + this->rgbLedCount * this->timeoutSecTransfer24Bit;
		}

		double calculateCircularResolutionTransferAll(double secondOneCircle)
		{
			return secondOneCircle / this->getTimeoutSecTransferAll();
		}

		uint64_t calculateDmaRamSizeBytes1Frame(uint64_t stripCount)
		{
			uint64_t oneStripeBits1frame = 40 + 24 * this->rgbLedCount;//232 bits if 8 leds in one strip
			uint64_t oneStripeBytes1frame = oneStripeBits1frame;//232 bytes
			uint64_t manyStripesBytes1frame = oneStripeBytes1frame * stripCount;//1856 bytes if 8 stripes
			return manyStripesBytes1frame;
		}
	};

	inline void draw(
		QPainter& painter,
		Ws2812Settings settings,
		uint64_t indexOfThisFrame,
		double fanCenter,
		double fanRadiusPx)
	{
		QElapsedTimer t;
		t.restart();

		const double maxGlobalAngleThisRepaint = (indexOfThisFrame + 1) * settings.fanAngleSpeedDegreesEveryFrame();
		double globalCurrentAngleDegrees = indexOfThisFrame * settings.fanAngleSpeedDegreesEveryFrame();

		const uint32_t ledLineCount = settings.rgbLedCount * settings.fanBladesCount;
		const double ledStepPx = fanRadiusPx / ledLineCount;
		const double ledStartRadius = ledStepPx / 2;

		const double blinkCountEveryCircle = settings.calculateCircularResolutionTransferAll(1.0 / settings.fanRps);

		const double maxSpanAngleDegrees = 360.0 / blinkCountEveryCircle;

		uint64_t bitsPerFrameStatistic = 0;
		uint64_t pulseIndex = 0;
		for (; globalCurrentAngleDegrees < maxGlobalAngleThisRepaint; pulseIndex++)
		{
				for (int ledBlockIndex = 0; ledBlockIndex < ledLineCount; ledBlockIndex++)
				{
					double radius3 = ledStartRadius + ledStepPx * ledLineCount * pow((double)(ledLineCount - ledBlockIndex - 1) / ledLineCount, 0.8);

					painter.setPen(QColor(rand() % 256, rand() % 256, rand() % 256));

					double spanAngle = maxSpanAngleDegrees;
					QRectF r(
						fanCenter - radius3,
						fanCenter - radius3,
						radius3 * 2,
						radius3 * 2);
					//painter.drawArc(r, (globalCurrentAngleDegrees - spanAngle / 2) * 16, spanAngle * 16);
					drawLineFromArc(painter, r, (globalCurrentAngleDegrees - spanAngle / 2), spanAngle);

					bitsPerFrameStatistic += 24;
				}

			globalCurrentAngleDegrees += maxSpanAngleDegrees;
		}

		std::cout << "Async paint time=" << t.elapsed() << "; bits/frame = " << bitsPerFrameStatistic << "; pulses = "<< pulseIndex
			<< "; thisA"<< settings.fanAngleSpeedDegreesEveryFrame()
			<< "; ARes="<< blinkCountEveryCircle << std::endl;
	}
};