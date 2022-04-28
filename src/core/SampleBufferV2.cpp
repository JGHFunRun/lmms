/*
 * SampleBufferV2.cpp - container class for immutable sample data
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

#include "base64.h"

#include "SampleBufferV2.h"
#include "AudioEngine.h"
#include "Engine.h"

#include <QFile>
#include <sndfile.h>

SampleBufferV2::SampleBufferV2()
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}

SampleBufferV2::SampleBufferV2(const QString &strData, StrDataType dataType) : SampleBufferV2()
{
	switch (dataType)
	{
	case StrDataType::AudioFile:
		loadFromAudioFile(strData);
		break;
	case StrDataType::Base64:
		loadFromBase64(strData);
		break;
	}
}

SampleBufferV2::SampleBufferV2(sampleFrame *data, const std::size_t numFrames) : SampleBufferV2()
{
	m_data = std::vector<sampleFrame>(data, data + numFrames);
	m_originalSampleRate = Engine::audioEngine()->processingSampleRate();
}

SampleBufferV2::SampleBufferV2(const std::size_t numFrames) : SampleBufferV2()
{
	m_data = std::vector<sampleFrame>(numFrames);
	m_originalSampleRate = Engine::audioEngine()->processingSampleRate();
}

SampleBufferV2::SampleBufferV2(SampleBufferV2&& other) : SampleBufferV2()
{
	m_data = std::move(other.m_data);
	m_originalSampleRate = std::move(other.m_originalSampleRate);
	m_filePath = std::move(other.m_filePath);
}

SampleBufferV2& SampleBufferV2::operator=(SampleBufferV2&& other) 
{
	if (this == &other) 
	{
		return *this;
	}

	m_data = std::move(other.m_data);
	m_originalSampleRate = std::move(other.m_originalSampleRate);
	m_filePath = std::move(other.m_filePath);
	return *this;
}
	
const std::vector<sampleFrame> &SampleBufferV2::data() const
{
	return m_data;
}

sample_rate_t SampleBufferV2::originalSampleRate() const
{
	return m_originalSampleRate;
}

const QString &SampleBufferV2::filePath() const
{
	return m_filePath;
}

bool SampleBufferV2::hasFilePath() const 
{
	return !m_filePath.isEmpty();
}

QString SampleBufferV2::toBase64() const
{
	QString dst = "";
	base64::encode((const char *) m_data.data(), m_data.size() * sizeof(sampleFrame), dst);
	return dst;
}

void SampleBufferV2::sampleRateChanged()
{
	resample(Engine::audioEngine()->processingSampleRate());
}

void SampleBufferV2::resample(const sample_rate_t newSampleRate) 
{
	//TODO
}

void SampleBufferV2::loadFromAudioFile(const QString& audioFilePath) 
{
	auto audioFile = QFile(audioFilePath);
	if (!audioFile.open(QIODevice::ReadOnly)) 
	{
		throw std::runtime_error("Could not open file " + audioFilePath.toStdString());
	}

	SF_INFO sfInfo;
	sfInfo.format = 0;

	auto sndFileDeleter = [&](SNDFILE* ptr)
	{ sf_close(ptr);
	  audioFile.close(); };

	auto sndFile = std::unique_ptr<SNDFILE, decltype(sndFileDeleter)>(sf_open_fd(audioFile.handle(), SFM_READ, &sfInfo, false), sndFileDeleter);

	if (!sndFile)
	{
		throw std::runtime_error(sf_strerror(sndFile.get()));
	}

	sf_count_t numSamples = sfInfo.frames * sfInfo.channels;
	auto samples = std::vector<sample_t>(numSamples);
	sf_count_t samplesRead = sf_read_float(sndFile.get(), samples.data(), numSamples);

	if (samplesRead != numSamples) 
	{
		throw std::runtime_error("Could not read sample");
	}

	sf_count_t numFrames = samplesRead / sfInfo.channels;
	m_data.resize(numFrames);

	int isMono = sfInfo.channels == 1 ? 1 : 0;
	for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) 
	{
		m_data[frameIdx][0] = samples[frameIdx * sfInfo.channels];
		m_data[frameIdx][1] = samples[frameIdx * sfInfo.channels + isMono];
	}

	m_filePath = audioFilePath;
	m_originalSampleRate = sfInfo.samplerate;
	
	sample_rate_t engineSampleRate = Engine::audioEngine()->processingSampleRate();
	if (m_originalSampleRate != engineSampleRate) 
	{
		resample(engineSampleRate);
	}	
}

void SampleBufferV2::loadFromBase64(const QString& data) 
{
	char * dst = nullptr;
	int dsize = 0;
	base64::decode(data, &dst, &dsize);

	m_data = std::vector<sampleFrame>(dsize / sizeof(sampleFrame));
	std::copy(dst, dst + dsize, m_data.begin());
}