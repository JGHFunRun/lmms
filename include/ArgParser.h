#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <cstdlib>

/*#include "lmmsconfig.h"
#include "lmmsversion.h"
#include "versioninfo.h"

#include "denormals.h"*/

#include "ProjectRenderer.h"

#include <QDebug>
#include <QFileInfo>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#endif

#ifdef LMMS_HAVE_SCHED_H
#include "sched.h"
#endif

#ifdef LMMS_HAVE_PROCESS_H
#include <process.h>
#endif

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <csignal>

#include "ArgParser.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "MainApplication.h"
#include "MainWindow.h"
#include "MixHelpers.h"
#include "NotePlayHandle.h"
#include "OutputSettings.h"
#include "ProjectRenderer.h"
#include "RenderManager.h"
#include "Song.h"

#ifdef LMMS_DEBUG_FPE
#include <fenv.h> // For feenableexcept
#include <execinfo.h> // For backtrace and backtrace_symbols_fd
#include <unistd.h> // For STDERR_FILENO
#include <csignal> // To register the signal handler
#endif

namespace lmms {

class ArgParser {
	int    m_argc;
	char **m_argv;

	void printVersion();
	void printHelp();  

	inline void usageError(const QString &message)
	{
		qCritical().noquote() << QString("\n%1.\n\nTry \"%2 --help\" for more information.\n\n")
					   .arg(message).arg(m_execName);

		exit(EXIT_FAILURE);
	}

	//! @param i A pointer to the current index number.
	//!
	//! Returns `nullptr` if no argument is supplied by user.
	inline char *getNextArg(int *i)
	{
		++*i;

		assert(m_argc >= *i);

		if (m_argc == *i)
		{
			return nullptr;
		}

		return m_argv[*i];
	}

	//! @param i A pointer to the current index number.
	//! @param error Error message for if argument is not supplied by user.
	inline char *expectNextArg(int *i, char const *error)
	{
		char *arg = getNextArg(i);

		if (arg == nullptr)
		{
			usageError(error);
		}

		return m_argv[*i];
	}

	//! @param i A pointer to the current index number.
	inline char *getInputFile(int *i)
	{
		return expectNextArg(i, "No input file specified");
	}


public:
	bool m_fullscreen      = true;
	bool m_coreOnly        = false;
	bool m_exitAfterImport = false;
	bool m_allowRoot       = false;
	bool m_renderLoop      = false;
	bool m_renderTracks    = false;

	char *m_execName;

	QString m_fileToLoad,
		m_fileToImport,
		m_renderOut,
		m_profilerOutputFile,
		m_configFile;

	AudioEngine::qualitySettings m_qs;
	OutputSettings m_os;
	ProjectRenderer::ExportFileFormats m_eff;


	ArgParser(int argc, char **argv);

	void stageOneParsing();
	void stageTwoParsing();
}; // class ArgParser

} // namespace lmms

#endif // ARG_PARSER_H
