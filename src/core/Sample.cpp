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

#include <iostream>

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
	m_frameIndex(0)
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}

Sample::Sample(const QString &strData, SampleBufferV2::StrDataType dataType) : Sample()
{
	setSampleData(strData, dataType);
	m_endFrame = m_sampleBuffer->numFrames();
	m_loopEndFrame = m_sampleBuffer->numFrames();
}

Sample::Sample(const sampleFrame *data, const std::size_t numFrames) : Sample()
{
	auto buf = std::vector<sample_t>(numFrames * DEFAULT_CHANNELS);
	for (f_cnt_t frame = 0; frame < numFrames; ++frame) 
	{
		buf[frame] = data[frame][0];
		buf[frame + 1] = data[frame][1];
	}

	m_sampleBuffer = std::make_shared<const SampleBufferV2>(data, numFrames);
	m_endFrame = numFrames;
	m_loopEndFrame = numFrames;
}

Sample::Sample(const SampleBufferV2 *buffer) : Sample()
{
	m_sampleBuffer.reset(buffer);
	m_endFrame = buffer->numFrames();
	m_loopEndFrame = buffer->numFrames();
}

Sample::Sample(const std::size_t numFrames) : Sample()
{
	m_sampleBuffer = std::make_shared<const SampleBufferV2>(numFrames);
	m_endFrame = numFrames;
	m_loopEndFrame = numFrames;
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
	m_frameIndex(other.m_frameIndex) {}

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

	return *this;
}

bool Sample::play(sampleFrame *dst, const fpp_t numFrames, const float freq, const LoopMode loopMode)
{

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
	m_loopEndFrame = buffer->numFrames();
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
	// change dir to position of previously opened file
	ofd.setDirectory(dir);
	ofd.setFileMode(FileDialog::ExistingFiles);

	// set filters
	QStringList types;
	types << tr("All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
		"*.aif *.aiff *.au *.raw)")
		<< tr("Wave-Files (*.wav)")
		<< tr("OGG-Files (*.ogg)")
		<< tr("DrumSynth-Files (*.ds)")
		<< tr("FLAC-Files (*.flac)")
		<< tr("SPEEX-Files (*.spx)")
		<< tr("MP3-Files (*.mp3)")
		<< tr("VOC-Files (*.voc)")
		<< tr("AIFF-Files (*.aif *.aiff)")
		<< tr("AU-Files (*.au)")
		<< tr("RAW-Files (*.raw)");

	ofd.setNameFilters(types);
	if (m_sampleBuffer->hasFilePath())
	{
		// select previously opened file
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
	return double(m_endFrame - m_startFrame) / m_sampleBuffer->sampleRate() * 1000;
}

sample_t Sample::userWaveSample(const float sample) const
{
	//samplecaching. TODO: Sample::userWaveSample
	// f_cnt_t numFrames = m_sampleBuffer->numFrames();
	// auto samples = m_sampleBuffer->samples();
	// const float frame = sample * numFrames;
	// f_cnt_t f1 = static_cast<f_cnt_t>(frame) % numFrames;
	// if (f1 < 0)
	// {
	// 	f1 += numFrames;
	// }
	// return linearInterpolate(samples[f1 * DEFAULT_CHANNELS][0], samples[(f1 * DEFAULT_CHANNELS + 1) % numFrames][0], fraction(frame));
	return sample;
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
