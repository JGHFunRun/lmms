#include "ArgParser.h"

#include "lmmsconfig.h"
#include "lmmsversion.h"
#include "versioninfo.h"

#include "denormals.h"

#include "DataFile.h"

#include <QFileInfo>
//#include <QLocale>
//#include <QTimer>
//#include <QTranslator>
//#include <QApplication>
//#include <QMessageBox>
//#include <QPushButton>
//#include <QTextStream>

//#include <QDebug>
//#include <QFileInfo>
//#include <QLocale>
//#include <QTimer>
//#include <QTranslator>
//#include <QApplication>
//#include <QMessageBox>
//#include <QPushButton>
//#include <QTextStream>

//#ifdef LMMS_BUILD_WIN32
//#include <windows.h>
//#endif

//#ifdef LMMS_HAVE_SCHED_H
//#include "sched.h"
//#endif

//#ifdef LMMS_HAVE_PROCESS_H
//#include <process.h>
//#endif

//#ifdef LMMS_HAVE_UNISTD_H
//#include <unistd.h>
//#endif

//#include <csignal>

//#include "ArgParser.h"
//#include "ConfigManager.h"
//#include "DataFile.h"
//#include "embed.h"
//#include "Engine.h"
//#include "GuiApplication.h"
//#include "ImportFilter.h"
//#include "MainApplication.h"
//#include "MainWindow.h"
//#include "MixHelpers.h"
//#include "NotePlayHandle.h"
//#include "OutputSettings.h"
//#include "ProjectRenderer.h"
//#include "RenderManager.h"
//#include "Song.h"



namespace lmms {

ArgParser::ArgParser(int argc, char **argv) :
	m_argc(argc),
	m_argv(argv),
	m_execName(argv[0]),

	m_qs(AudioEngine::qualitySettings::Mode::Mode_HighQuality),
	m_os(44100, OutputSettings::BitRateSettings(160, false), OutputSettings::Depth_16Bit, OutputSettings::StereoMode::StereoMode_JointStereo),
	m_eff(ProjectRenderer::WaveFile)
{}

void ArgParser::stageOneParsing() {
	for (int i = 1; i < m_argc; i++)
	{
		QString arg = m_argv[i];

		if(arg == "--help"    || arg == "-h" ||
		   arg == "--version" || arg == "-v" ||
		   arg == "render"    || arg == "--render" || arg == "-r")
		{
			m_coreOnly = true;
		}
		else if(arg == "rendertracks" || arg == "--rendertracks")
		{
			m_coreOnly = true;
			m_renderTracks = true;
		}
		else if(arg == "--allowroot")
		{
			m_allowRoot = true;
		}
		else if(arg == "--geometry" || arg == "-geometry")
		{
			if(arg == "--geometry")
			{
				// Delete the first "-" so Qt recognize the option
				strcpy(m_argv[i], "-geometry");
			}
			// option -geometry is filtered by Qt later,
			// so we need to check its presence now to
			// determine, if the application should run in
			// fullscreen mode (default, no -geometry given).
			m_fullscreen = false;
		} // Stage two of parsing will catch invalid options
	}
}


void ArgParser::printVersion() {
	printf("LMMS %s\n(%s %s, Qt %s, %s)\n\n"
	       "Copyright (c) %s\n\n"
	       "This program is free software; you can redistribute it and/or\n"
	       "modify it under the terms of the GNU General Public\n"
	       "License as published by the Free Software Foundation; either\n"
	       "version 2 of the License, or (at your option) any later version.\n\n"
	       "Try \"%s --help\" for more information.\n\n", LMMS_VERSION,
	       LMMS_BUILDCONF_PLATFORM, LMMS_BUILDCONF_MACHINE, QT_VERSION_STR, LMMS_BUILDCONF_COMPILER_VERSION,
	       LMMS_PROJECT_COPYRIGHT, m_execName);
}

void ArgParser::printHelp() {
	printf("LMMS %s\n"
	       "Copyright (c) %s\n\n"
	       "Usage: lmms [global options...] [<action> [action parameters...]]\n\n"
	       "Actions:\n"
	       "  <no action> [options...] [<project>]  Start LMMS in normal GUI mode\n"
	       "  dump <in>                             Dump XML of compressed file <in>\n"
	       "  compress <in>                         Compress file <in>\n"
	       "  render <project> [options...]         Render given project file\n"
	       "  rendertracks <project> [options...]   Render each track to a different file\n"
	       "  upgrade <in> [out]                    Upgrade file <in> and save as <out>\n"
	       "                                        Standard out is used if no output file\n"
	       "                                        is specified\n"
	       "  makebundle <in> [out]                 Make a project bundle from the project\n"
	       "                                        file <in> saving the resulting bundle\n"
	       "                                        as <out>\n"
	       "\nGlobal options:\n"
	       "      --allowroot                Bypass root user startup check (use with\n"
	       "          caution).\n"
	       "  -c, --config <configfile>      Get the configuration from <configfile>\n"
	       "  -h, --help                     Show this usage information and exit.\n"
	       "  -v, --version                  Show version information and exit.\n"
	       "\nOptions if no action is given:\n"
	       "      --geometry <geometry>      Specify the size and position of\n"
	       "          the main window\n"
	       "          geometry is <xsizexysize+xoffset+yoffsety>.\n"
	       "      --import <in> [-e]         Import MIDI or Hydrogen file <in>.\n"
	       "          If -e is specified lmms exits after importing the file.\n"
	       "\nOptions for \"render\" and \"rendertracks\":\n"
	       "  -a, --float                    Use 32bit float bit depth\n"
	       "  -b, --bitrate <bitrate>        Specify output bitrate in KBit/s\n"
	       "          Default: 160.\n"
	       "  -f, --format <format>         Specify format of render-output where\n"
	       "          Format is either 'wav', 'flac', 'ogg' or 'mp3'.\n"
	       "  -i, --interpolation <method>   Specify interpolation method\n"
	       "          Possible values:\n"
	       "            - linear\n"
	       "            - sincfastest (default)\n"
	       "            - sincmedium\n"
	       "            - sincbest\n"
	       "  -l, --loop                     Render as a loop\n"
	       "  -m, --mode                     Stereo mode used for MP3 export\n"
	       "          Possible values: s, j, m\n"
	       "            s: Stereo\n"
	       "            j: Joint Stereo\n"
	       "            m: Mono\n"
	       "          Default: j\n"
	       "  -o, --output <path>            Render into <path>\n"
	       "          For \"render\", provide a file path\n"
	       "          For \"rendertracks\", provide a directory path\n"
	       "          If not specified, render will overwrite the input file\n"
	       "          For \"rendertracks\", this might be required\n"
	       "  -p, --profile <out>            Dump profiling information to file <out>\n"
	       "  -s, --samplerate <samplerate>  Specify output samplerate in Hz\n"
	       "          Range: 44100 (default) to 192000\n"
	       "  -x, --oversampling <value>     Specify oversampling\n"
	       "          Possible values: 1, 2, 4, 8\n"
	       "          Default: 2\n\n",
	       LMMS_VERSION, LMMS_PROJECT_COPYRIGHT);
}


void ArgParser::stageTwoParsing() {
	for(int i = 1; i < m_argc; i++)
	{
		QString arg = m_argv[i];

		if (arg == "--version" || arg == "-v")
		{
			printVersion();
			exit(EXIT_SUCCESS);
		}
		else if (arg == "--help" || arg  == "-h")
		{
			printHelp();
			exit(EXIT_SUCCESS);
		}
		else if (arg == "upgrade" || arg == "--upgrade" || arg  == "-u")
		{
			QString inputFile = getInputFile(&i);

			DataFile dataFile(inputFile);

			QString outputFile = getNextArg(&i);

			if (outputFile.isEmpty()) // output file not specified, write to stdout
			{
				QTextStream ts(stdout);
				dataFile.write(ts);
				fflush(stdout);
			}
			else // output file specified, write to that
			{
				dataFile.writeFile(outputFile);
			}

			exit(EXIT_SUCCESS);
		}
		else if (arg == "makebundle")
		{
			QString inputFile = getInputFile(&i);

			DataFile dataFile(inputFile);

			QString bundleName = expectNextArg(&i, "No project bundle name given");

			printf("Making bundle\n");
			dataFile.writeFile(bundleName, true);
			exit(EXIT_SUCCESS);
		}
		else if (arg == "--allowroot")
		{
			// Processed earlier (during first stage)

			// Option is completely ignored on Windows
#ifdef LMMS_BUILD_WIN32
			if (m_allowRoot)
			{
				printf("\nOption \"--allowroot\" will be ignored on this platform.\n\n");
			}
#endif

		}
		else if (arg == "dump" || arg == "--dump" || arg  == "-d")
		{
			QString inputFile = getInputFile(&i);

			// TODO: add an optional argument for output file
			QFile f(inputFile);
			f.open(QIODevice::ReadOnly);
			QString d = qUncompress(f.readAll());
			printf("%s\n", d.toUtf8().constData());

			exit(EXIT_SUCCESS);
		}
		else if (arg == "compress" || arg == "--compress")
		{
			QString inputFile = getInputFile(&i);

			// TODO: add an optional argument for output file
			QFile f(inputFile);
			f.open(QIODevice::ReadOnly);
			QByteArray d = qCompress(f.readAll());
			fwrite(d.constData(), sizeof(char), d.size(), stdout);

			exit(EXIT_SUCCESS);
		}
		else if (arg == "render"       || arg == "--render"      || arg == "-r" ||
		         arg == "rendertracks" || arg == "--rendertracks")
		{
			m_fileToLoad = getInputFile(&i);
			m_renderOut = m_fileToLoad;
		}
		else if (arg == "--loop" || arg == "-l")
		{
			m_renderLoop = true;
		}
		else if (arg == "--output" || arg == "-o")
		{
			m_renderOut = expectNextArg(&i, "No output file specified");
		}
		else if (arg == "--format" || arg == "-f")
		{
			const QString ext = expectNextArg(&i, "No output format specified");

			if(ext == "wav")
			{
				m_eff = ProjectRenderer::WaveFile;
			}
			else if(ext == "ogg")
			{
#ifdef LMMS_HAVE_OGGVORBIS
				m_eff = ProjectRenderer::OggFile;
#else
				usageError("This copy of LMMS was not built with OGG support");
#endif
			}
			else if(ext == "mp3")
			{
#ifdef LMMS_HAVE_MP3LAME
				m_eff = ProjectRenderer::MP3File;
#else
				usageError("This copy of LMMS was not built with MP3 support");
#endif
			}
			else if (ext == "flac")
			{
				m_eff = ProjectRenderer::FlacFile;
			}
			else
			{
				usageError(QString("Invalid output format %1").arg(ext));
			}
		}
		else if(arg == "--samplerate" || arg == "-s")
		{
			QString srText = expectNextArg(&i, "No samplerate specified");
			sample_rate_t sr = srText.toUInt();
			if(sr >= 44100 && sr <= 192000)
			{
				m_os.setSampleRate(sr);
			}
			else
			{
				usageError(QString("Invalid samplerate %1").arg(sr));
			}
		}
		else if(arg == "--bitrate" || arg == "-b")
		{
			QString brText = expectNextArg(&i, "No bitrate specified");
			int br = brText.toUInt();

			if(br >= 64 && br <= 384)
			{
				OutputSettings::BitRateSettings bitRateSettings = m_os.getBitRateSettings();
				bitRateSettings.setBitRate(br);
				m_os.setBitRateSettings(bitRateSettings);
			}
			else
			{
				usageError(QString("Invalid bitrate %1").arg(br));
			}
		}
		else if(arg == "--mode" || arg == "-m")
		{
			QString const mode = expectNextArg(&i, "No stereo mode specified");

			// TODO: add long names (ie "stereo")
			if(mode == "s" || mode == "stereo")
			{
				m_os.setStereoMode(OutputSettings::StereoMode_Stereo);
			}
			else if(mode == "j" || mode == "joint-stereo")
			{
				m_os.setStereoMode(OutputSettings::StereoMode_JointStereo);
			}
			else if(mode == "m" || mode == "mono")
			{
				m_os.setStereoMode(OutputSettings::StereoMode_Mono);
			}
			else
			{
				usageError(QString("Invalid stereo mode %1").arg(mode));
			}
		}
		else if(arg == "--float" || arg == "-a")
		{
			m_os.setBitDepth(OutputSettings::Depth_32Bit);
		}
		else if(arg == "--interpolation" || arg == "-i")
		{
			const QString ip = expectNextArg(&i, "No interpolation method specified");

			if(ip == "linear")
			{
				m_qs.interpolation = AudioEngine::qualitySettings::Interpolation_Linear;
			}
			else if(ip == "sincfastest")
			{
				m_qs.interpolation = AudioEngine::qualitySettings::Interpolation_SincFastest;
			}
			else if(ip == "sincmedium")
			{
				m_qs.interpolation = AudioEngine::qualitySettings::Interpolation_SincMedium;
			}
			else if(ip == "sincbest")
			{
				m_qs.interpolation = AudioEngine::qualitySettings::Interpolation_SincBest;
			}
			else
			{
				usageError(QString("Invalid interpolation method %1").arg(ip));
			}
		}
		else if(arg == "--oversampling" || arg == "-x")
		{
			int o = QString(expectNextArg(&i, "No oversampling specified")).toUInt();

			switch(o)
			{
				case 1:
					m_qs.oversampling = AudioEngine::qualitySettings::Oversampling_None;
					break;
				case 2:
					m_qs.oversampling = AudioEngine::qualitySettings::Oversampling_2x;
					break;
				case 4:
					m_qs.oversampling = AudioEngine::qualitySettings::Oversampling_4x;
					break;
				case 8:
					m_qs.oversampling = AudioEngine::qualitySettings::Oversampling_8x;
					break;
				default:
					usageError(QString("Invalid oversampling %1").arg(o));
			}
		}
		else if(arg == "--import")
		{
			m_fileToImport = expectNextArg(&i, "No file specified for importing");

			// exit after import? (only for debugging)
			if (QString(m_argv[i+1]) == "-e")
			{
				m_exitAfterImport = true;
				i++;
			}
		}
		else if(arg == "--profile" || arg == "-p")
		{
			m_profilerOutputFile = expectNextArg(&i, "No profile specified");
		}
		else if(arg == "--config" || arg == "-c")
		{
			m_configFile = expectNextArg(&i, "No configuration file specified");
		}
		else
		{
			if(arg[0] == '-')
			{
				usageError(QString("Invalid option %1").arg(arg));
			}

			m_fileToLoad = arg;
		}
	}
}

} // namespace lmms
