/*
 * SampleBufferV2.h - container class for immutable sample data
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

#ifndef SAMPLE_BUFFER_V2_H
#define SAMPLE_BUFFER_V2_H

#include <memory>
#include <vector>
#include "lmms_basics.h"
#include "samplerate.h"

#include <QObject>
#include <QString>

class SampleBufferV2 : public QObject
{
	Q_OBJECT
public:
	enum class StrDataType 
	{
		AudioFile, Base64
	};

	SampleBufferV2();
	SampleBufferV2(const QString& strData, StrDataType dataType);
	SampleBufferV2(const sampleFrame *data, const std::size_t numFrames);
	explicit SampleBufferV2(const std::size_t numFrames);

	SampleBufferV2(SampleBufferV2&& other);
	SampleBufferV2& operator=(SampleBufferV2&& other);

	SampleBufferV2(const SampleBufferV2& other) = delete;
	SampleBufferV2& operator=(const SampleBufferV2& other) = delete;

	const std::vector<sampleFrame>& sampleData() const;
	sample_rate_t originalSampleRate() const;
	sample_rate_t sampleRate() const;
	f_cnt_t numFrames() const;

	const QString& filePath() const;
	bool hasFilePath() const;

	QString toBase64() const;

private:
	void loadFromAudioFile(const QString& audioFilePath);
	void loadFromBase64(const QString& str);
	void resample(const sample_rate_t newSampleRate);
	
private:
	std::vector<sampleFrame> m_data;
	sample_rate_t m_originalSampleRate;
	sample_rate_t m_sampleRate;
	f_cnt_t m_numFrames;

	//TODO: C++17 and above: use std::optional<QString>
	QString m_filePath;
};

#endif