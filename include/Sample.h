/*
 * Sample.h - a SampleBuffer with its own characteristics
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
*/

#ifndef SAMPLE_H
#define SAMPLE_H

#include <memory>
#include <QObject>

#include "SampleBufferV2.h"
#include "SampleBufferCache.h"

class QPainter;
class QRect;

class Sample : public QObject
{
	Q_OBJECT
public:
	enum class LoopMode
	{
		LoopOff, LoopOn, LoopPingPong
	};

	Sample();
	Sample(const QString& audioFile);
	Sample(const SampleBufferV2* buffer);
	Sample(const sampleFrame* data, const std::size_t numFrames);
	Sample(const std::size_t numFrames);

	bool play(sampleFrame* dst, const fpp_t numFrames, const float freq, const LoopMode = LoopMode::LoopOff);
	void visualize(QPainter& painter, const QRect& drawingRect, const QRect& clipRect, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);
	void visualize(QPainter& painter, const QRect& drawingRect, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);

	std::shared_ptr<const SampleBufferV2> sampleBuffer() const;
	float amplification() const;
	float frequency() const;
	bool reversed() const;
	bool varyingPitch() const;
	int interpolationMode() const;
	f_cnt_t startFrame() const;
	f_cnt_t endFrame() const;
	f_cnt_t loopStartFrame() const;
	f_cnt_t loopEndFrame() const;

	void setAmplification(float amplification);
	void setFrequency(float frequency);
	void setReversed(bool reversed);
	void setVaryingPitch(bool varyingPitch);
	void setInterpolationMode(int interpolationMode);
	void setStartFrame(f_cnt_t start);
	void setEndFrame(f_cnt_t end);
	void setLoopStartFrame(f_cnt_t loopStart);
	void setLoopEndFrame(f_cnt_t loopEnd);
	void setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd);

	int calculateSampleLength() const;

public slots:
	void sampleRateChanged();

private:
	std::vector<sampleFrame> getSampleFragment(f_cnt_t index, f_cnt_t end, f_cnt_t numFrames, f_cnt_t loopStart, f_cnt_t loopEnd, LoopMode loopMode, bool backwards) const;
	f_cnt_t getLoopedIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const;
	f_cnt_t getPingPongIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const;

private:
	std::shared_ptr<const SampleBufferV2> m_sampleBuffer;
	float m_amplification;
	float m_frequency;
	bool m_reversed;
	bool m_varyingPitch;
	int m_interpolationMode;
	f_cnt_t m_startFrame;
	f_cnt_t m_endFrame;
	f_cnt_t m_loopStartFrame;
	f_cnt_t m_loopEndFrame;

signals:
	void sampleUpdated();
};

#endif