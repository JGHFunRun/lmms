/*
 * Sample.cpp - a SampleBuffer with its own characteristics
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

#include "Sample.h"
#include "Engine.h"
#include "AudioEngine.h"
#include "Note.h"

#include "interpolation.h"

#include <QObject>
#include <QtGlobal>
#include "FileDialog.h"

Sample::Sample() :
	m_sampleBuffer(nullptr),
	m_amplification(1.0f),
	m_frequency(DefaultBaseFreq),
	m_reversed(false),
	m_varyingPitch(false),
	m_interpolationMode(SRC_LINEAR),
	m_startFrame(0),
	m_endFrame(0),
	m_loopStartFrame(0),
	m_loopEndFrame(0)
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}

Sample::Sample(const QString& strData, SampleBufferV2::StrDataType dataType) : Sample()
{
	setSampleData(strData, dataType);
}

Sample::Sample(const sampleFrame* data, const std::size_t numFrames) : Sample()
{
	m_sampleBuffer = std::make_shared<const SampleBufferV2>(data, numFrames);
}

Sample::Sample(const std::size_t numFrames) : Sample() 
{
	m_sampleBuffer = std::make_shared<const SampleBufferV2>(numFrames);
}

bool Sample::play(sampleFrame* dst, const fpp_t numFrames, const float freq, const LoopMode loopMode) 
{
	//TODO
}

void Sample::visualize(QPainter& painter, const QRect& drawingRect, const QRect& clipRect, f_cnt_t fromFrame, f_cnt_t toFrame) 
{
	//TODO
}

void Sample::visualize(QPainter& painter, const QRect& drawingRect, f_cnt_t fromFrame, f_cnt_t toFrame) 
{
	//TODO
}

std::shared_ptr<const SampleBufferV2> Sample::sampleBuffer() const 
{
	return m_sampleBuffer;
}

sample_rate_t Sample::sampleRate() const 
{
	return m_sampleBuffer->sampleRate();
}

float Sample::amplification() const 
{
	return m_amplification;
}

float Sample::frequency() const
{
	return m_frequency;
}

bool Sample::reversed() const 
{
	return m_reversed;
}

bool Sample::varyingPitch() const 
{
	return m_varyingPitch;
}

int Sample::interpolationMode() const 
{
	return m_interpolationMode;
}

f_cnt_t Sample::startFrame() const 
{
	return m_startFrame;
}

f_cnt_t Sample::endFrame() const 
{
	return m_endFrame;
}

f_cnt_t Sample::loopStartFrame() const 
{
	return m_loopStartFrame;
}

f_cnt_t Sample::loopEndFrame() const 
{
	return m_loopEndFrame;
}

void Sample::setSampleData(const QString& strData, SampleBufferV2::StrDataType dataType) 
{
	auto cachedSampleBuffer = Engine::sampleBufferCache()->get(strData);
	if (cachedSampleBuffer) 
	{
		m_sampleBuffer = cachedSampleBuffer;
	}
	else 
	{
		m_sampleBuffer = Engine::sampleBufferCache()->add(strData, new SampleBufferV2(strData, dataType));
	}
}

void Sample::setAmplification(float amplification) 
{
	m_amplification = amplification;
}

void Sample::setFrequency(float frequency) 
{
	m_frequency = frequency;
}

void Sample::setReversed(bool reversed)
{
	m_reversed = reversed;
}

void Sample::setVaryingPitch(bool varyingPitch) 
{
	m_varyingPitch = varyingPitch;
}

void Sample::setInterpolationMode(int interpolationMode) 
{
	m_interpolationMode = interpolationMode;
}

void Sample::setStartFrame(f_cnt_t start) 
{
	m_startFrame = start;
}

void Sample::setEndFrame(f_cnt_t end) 
{
	m_endFrame = end;
}

void Sample::setLoopStartFrame(f_cnt_t loopStart) 
{
	m_loopStartFrame = loopStart;
}

void Sample::setLoopEndFrame(f_cnt_t loopEnd) 
{
	m_loopEndFrame = loopEnd;
}

void Sample::setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd) 
{
	m_startFrame = start;
	m_endFrame = end;
	m_loopStartFrame = loopStart;
	m_loopEndFrame = loopEnd;
}

Sample Sample::openSample() 
{
	//TODO
}

int Sample::calculateSampleLength() const
{
	return double(m_endFrame - m_startFrame) / m_sampleBuffer->sampleRate() * 1000;
}

sample_t Sample::userWaveSample(const float sample) const 
{
	f_cnt_t numFrames = m_sampleBuffer->numFrames();
	const sampleFrame* data = m_sampleBuffer->data().data();
	const float frame = sample * numFrames;
	f_cnt_t f1 = static_cast<f_cnt_t>(frame) % numFrames;
	if (f1 < 0)
	{
		f1 += numFrames;
	}
	return linearInterpolate(data[f1][0], data[(f1 + 1) % numFrames][0], fraction(frame));
}

void Sample::sampleRateChanged()
{
	auto numFrames = m_sampleBuffer->numFrames();
	auto oldRate = m_sampleBuffer->sampleRate();
	auto oldRateToNewRateRatio = static_cast<float>(Engine::audioEngine()->processingSampleRate()) / oldRate;
	m_startFrame = qBound(0, f_cnt_t(m_startFrame * oldRateToNewRateRatio), numFrames);
	m_endFrame = qBound(m_startFrame, f_cnt_t(m_endFrame * oldRateToNewRateRatio), numFrames);
	m_loopStartFrame = qBound(0, f_cnt_t(m_loopStartFrame * oldRateToNewRateRatio), numFrames);
	m_loopEndFrame = qBound(m_loopStartFrame, f_cnt_t(m_loopEndFrame * oldRateToNewRateRatio), numFrames);
}