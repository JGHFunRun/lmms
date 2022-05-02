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

#include <QObject>
#include <QtGlobal>
#include "FileDialog.h"
#include "ConfigManager.h"
#include "PathUtil.h"

#include <algorithm>
#include "SampleBufferV2.h"
#include "interpolation.h"
#include "lmms_basics.h"
#include "qglobal.h"

#include <QPainter>

#include <array>
#include <iostream>
#include <memory>
#include <samplerate.h>
#include <vector>

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
	m_loopEndFrame(0),
	m_frameIndex(0),
	m_sampleRate(0),
	m_resamplingState(nullptr, [](SRC_STATE* state){})
{
	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, &Sample::sampleRateChanged);
}

Sample::Sample(const QString &strData, SampleBufferV2::StrDataType dataType) : Sample()
{
	setSampleData(strData, dataType);
	m_endFrame = m_sampleBuffer->numFrames();
	m_loopEndFrame = m_sampleBuffer->numFrames();
	m_sampleRate = Engine::audioEngine()->processingSampleRate();
}

Sample::Sample(const sampleFrame *data, const std::size_t numFrames) : Sample()
{
	m_sampleBuffer = std::make_shared<const SampleBufferV2>(data, numFrames);
	m_endFrame = numFrames;
	m_loopEndFrame = numFrames;
	m_sampleRate = Engine::audioEngine()->processingSampleRate();
}

Sample::Sample(const SampleBufferV2 *buffer) : Sample()
{
	m_sampleBuffer.reset(buffer);
	m_endFrame = buffer->numFrames();
	m_loopEndFrame = buffer->numFrames();
	m_sampleRate = Engine::audioEngine()->processingSampleRate();
}

Sample::Sample(const std::size_t numFrames) : Sample()
{
	m_sampleBuffer = std::make_shared<const SampleBufferV2>(numFrames);
	m_endFrame = numFrames;
	m_loopEndFrame = numFrames;
	m_sampleRate = Engine::audioEngine()->processingSampleRate();
}

Sample::Sample(const Sample &other) :
	m_sampleBuffer(other.m_sampleBuffer),
	m_amplification(other.m_amplification),
	m_frequency(other.m_frequency),
	m_reversed(other.m_reversed),
	m_varyingPitch(other.m_varyingPitch),
	m_interpolationMode(other.m_interpolationMode),
	m_startFrame(other.m_startFrame),
	m_endFrame(other.m_endFrame),
	m_loopStartFrame(other.m_loopStartFrame),
	m_frameIndex(other.m_frameIndex),
	m_sampleRate(other.m_sampleRate),
	m_resamplingState(other.m_resamplingState) {}

Sample &Sample::operator=(const Sample &other)
{
	if (this == &other) 
	{
		return *this;
	}

	m_sampleBuffer = other.m_sampleBuffer;
	m_amplification = other.m_amplification;
	m_frequency = other.m_frequency;
	m_reversed = other.m_reversed;
	m_varyingPitch = other.m_varyingPitch;
	m_interpolationMode = other.m_interpolationMode;
	m_startFrame = other.m_startFrame;
	m_endFrame = other.m_endFrame;
	m_loopStartFrame = other.m_loopStartFrame;
	m_frameIndex = other.m_frameIndex;
	m_sampleRate = other.m_sampleRate;
	m_resamplingState = other.m_resamplingState;

	return *this;
}

bool Sample::play(sampleFrame *dst, const fpp_t numFrames, const float freq, const LoopMode loopMode)
{
	if (numFrames == 0 ||
		(loopMode == LoopMode::LoopOff && (m_frameIndex < m_startFrame || m_frameIndex > m_endFrame)) ||
		((loopMode == LoopMode::LoopOn || loopMode == LoopMode::LoopPingPong) && (m_frameIndex < m_loopStartFrame || m_frameIndex > m_loopEndFrame))) 
	{ 
		return false; 
	}

	auto buf = std::vector<sampleFrame>(numFrames);	
	const double freqFactor = (double) freq / (double) m_frequency *
		m_sampleRate / Engine::audioEngine()->processingSampleRate();

	//Check if the sample needs to be pitched
	if (freqFactor != 1.0 || m_varyingPitch) 
	{
		const f_cnt_t totalFramesForCurrentPitch = static_cast<f_cnt_t>((m_endFrame - m_startFrame) / freqFactor);
		if (totalFramesForCurrentPitch == 0) 
		{
			return false;
		}
		
		int src_error = 0;
		if (!m_resamplingState) 
		{ 
			m_resamplingState = std::shared_ptr<SRC_STATE>(
				src_new(m_interpolationMode, DEFAULT_CHANNELS, &src_error), 
				[](SRC_STATE* state){ src_delete(state); });
		}

		std::array<const f_cnt_t, 5> sampleMargin = { 64, 64, 64, 4, 4 };
		f_cnt_t fragmentSize = static_cast<f_cnt_t>(numFrames * freqFactor) + sampleMargin[m_interpolationMode];
		buf.resize(fragmentSize);

		SRC_DATA srcData;
		srcData.data_in = (m_sampleBuffer->sampleData().begin() + m_frameIndex)->data();
		srcData.input_frames = numFrames;
		srcData.data_out = buf.data()->data();
		srcData.output_frames = fragmentSize;
		srcData.src_ratio = 1.0 / freqFactor;
		srcData.end_of_input = 0;
		src_error = src_process(m_resamplingState.get(), &srcData);

		if (src_error != 0)
		{
			std::cout << "SampleBuffer: error while resampling: " << src_strerror(src_error) << '\n';
		}
		
		if (srcData.output_frames_gen > numFrames)
		{
			std::cout << "SampleBufferV2: not enough frames:" << srcData.output_frames_gen << "/" << numFrames << '\n';
		}
	}

	if (m_reversed)
	{
		std::copy(
			m_sampleBuffer->sampleData().rbegin() + m_frameIndex, 
			m_sampleBuffer->sampleData().rbegin() + m_frameIndex + buf.size(), buf.begin());
	}
	else 
	{
		std::copy(
			m_sampleBuffer->sampleData().begin() + m_frameIndex, 
			m_sampleBuffer->sampleData().begin() + m_frameIndex + buf.size(), buf.begin());
	}

	for (auto& frame : buf) 
	{
		frame[0] *= m_amplification;
		frame[1] *= m_amplification;
	}
	std::copy(buf.begin(), buf.end(), dst);

	//Advance
	switch (loopMode) 
	{
	case LoopMode::LoopOff:
		m_frameIndex += buf.size();
		break;
	case LoopMode::LoopOn:
		m_frameIndex = getLoopedIndex(m_frameIndex + buf.size(), m_loopStartFrame, m_loopEndFrame);
		break;
	case LoopMode::LoopPingPong:
		m_frameIndex = getPingPongIndex(m_frameIndex + buf.size(), m_loopStartFrame, m_loopEndFrame);
		break;
	}

	return true;
}

void Sample::visualize(QPainter &painter, const QRect &drawingRect, const QRect &clipRect, f_cnt_t fromFrame, f_cnt_t toFrame)
{
	//TODO
}

void Sample::visualize(QPainter &painter, const QRect &drawingRect, f_cnt_t fromFrame, f_cnt_t toFrame)
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

f_cnt_t Sample::frameIndex() const
{
	return m_frameIndex;
}

void Sample::setSampleData(const QString &strData, SampleBufferV2::StrDataType dataType)
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

void Sample::setSampleBuffer(const SampleBufferV2 *buffer)
{
	m_sampleBuffer.reset(buffer);
	m_endFrame = buffer->numFrames();
	m_loopEndFrame = m_endFrame;
	m_frameIndex = 0;
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

void Sample::setFrameIndex(f_cnt_t frameIndex)
{
	m_frameIndex = frameIndex;
}

void Sample::setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd)
{
	m_startFrame = start;
	m_endFrame = end;
	m_loopStartFrame = loopStart;
	m_loopEndFrame = loopEnd;
}

QString Sample::openSample()
{
	FileDialog ofd(nullptr, tr("Open audio file"));

	QString dir;
	if (m_sampleBuffer->hasFilePath())
	{
		QString f = m_sampleBuffer->filePath();
		if (QFileInfo(f).isRelative())
		{
			f = ConfigManager::inst()->userSamplesDir() + f;
			if (QFileInfo(f).exists() == false)
			{
				f = ConfigManager::inst()->factorySamplesDir() +
					m_sampleBuffer->filePath();
			}
		}
		dir = QFileInfo(f).absolutePath();
	}
	else
	{
		dir = ConfigManager::inst()->userSamplesDir();
	}

	ofd.setDirectory(dir);
	ofd.setFileMode(FileDialog::ExistingFiles);

	QStringList types;
	types << tr("All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc " "*.aif *.aiff *.au *.raw)")
		<< tr("Wave-Files (*.wav)")
		<< tr("OGG-Files (*.ogg)")
		<< tr("DrumSynth-Files (*.ds)")
		<< tr("FLAC-Files (*.flac)")
		<< tr("SPEEX-Files (*.spx)")
		<< tr("VOC-Files (*.voc)")
		<< tr("AIFF-Files (*.aif *.aiff)")
		<< tr("AU-Files (*.au)")
		<< tr("RAW-Files (*.raw)");

	ofd.setNameFilters(types);
	if (m_sampleBuffer->hasFilePath())
	{
		ofd.selectFile(QFileInfo(m_sampleBuffer->filePath()).fileName());
	}

	if (ofd.exec() == QDialog::Accepted)
	{
		if (ofd.selectedFiles().isEmpty())
		{
			return QString();
		}
		return PathUtil::toShortestRelative(ofd.selectedFiles()[0]);
	}

	return QString();
}

int Sample::calculateSampleLength() const
{
	return static_cast<int>(1 / Engine::framesPerTick(m_sampleBuffer->sampleRate()) * m_sampleBuffer->numFrames());
}

void Sample::sampleRateChanged()
{
	auto numFrames = m_sampleBuffer->numFrames();
	auto oldRateToNewRateRatio = static_cast<float>(Engine::audioEngine()->processingSampleRate()) / m_sampleRate;
	m_startFrame = qBound(0, f_cnt_t(m_startFrame * oldRateToNewRateRatio), numFrames);
	m_endFrame = qBound(m_startFrame, f_cnt_t(m_endFrame * oldRateToNewRateRatio), numFrames);
	m_loopStartFrame = qBound(0, f_cnt_t(m_loopStartFrame * oldRateToNewRateRatio), numFrames);
	m_loopEndFrame = qBound(m_loopStartFrame, f_cnt_t(m_loopEndFrame * oldRateToNewRateRatio), numFrames);
}

f_cnt_t Sample::getLoopedIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const
{
	return index < endFrame ?
		index :
		startFrame + (index - startFrame) % (endFrame - startFrame);
}

f_cnt_t Sample::getPingPongIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const
{
	if (index < endFrame)
	{
		return index;
	}
	const f_cnt_t loopLen = endFrame - startFrame;
	const f_cnt_t loopPos = (index - endFrame) % (loopLen * 2);

	return (loopPos < loopLen)
		? endFrame - loopPos
		: startFrame + (loopPos - loopLen);
}
