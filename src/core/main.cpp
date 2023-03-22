/*
 * main.cpp - just main.cpp which is starting up app...
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock    <p/at/pgiblock.net>
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

#include "lmmsconfig.h"
#include "lmmsversion.h"
#include "versioninfo.h"

#include "denormals.h"

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

using namespace lmms;


#ifdef LMMS_DEBUG_FPE
void signalHandlerFPE(int signum) {

	// Get a back trace
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	backtrace_symbols_fd(array, size, STDERR_FILENO);

	// cleanup and close up stuff here
	// terminate program

	exit(signum);
}

void enableDebugFPE() {
	// Enable exceptions for certain floating point results
	// FE_UNDERFLOW is disabled for the time being
	feenableexcept(FE_INVALID   |
	               FE_DIVBYZERO |
	               FE_OVERFLOW  /*|
	               FE_UNDERFLOW*/);

	// Install the trap handler
	// register signal SIGFPE and signal handler
	signal(SIGFPE, signalHandlerFPE);
}
#endif // LMMS_DEBUG_FPE

static inline QString baseName(const QString &file)
{
	return QFileInfo(file).absolutePath() + "/" +
	       QFileInfo(file).completeBaseName();
}


#ifdef LMMS_BUILD_WIN32
// Workaround for old MinGW
#ifdef __MINGW32__
extern "C" _CRTIMP errno_t __cdecl freopen_s(FILE** _File,
	const char *_Filename, const char *_Mode, FILE *_Stream);
#endif

// For qInstallMessageHandler
void consoleMessageHandler(QtMsgType type,
	const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", localMsg.constData());
}

void attachConsole() {
	// Don't touch redirected streams here
	// GetStdHandle should be called before AttachConsole
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	FILE *fIn, *fOut, *fErr;
	// Enable console output if available
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		if (!hStdIn)
		{
			freopen_s(&fIn, "CONIN$", "r", stdin);
		}
		if (!hStdOut)
		{
			freopen_s(&fOut, "CONOUT$", "w", stdout);
		}
		if (!hStdErr)
		{
			freopen_s(&fErr, "CONOUT$", "w", stderr);
		}
	}
	// Make Qt's debug message handlers work
	qInstallMessageHandler(consoleMessageHandler);
}
#endif // LMMS_BUILD_WIN32


#if !defined(LMMS_BUILD_WIN32) && !defined(LMMS_BUILD_HAIKU)
void rootTest(bool allowRoot) {
	if ((getuid() == 0 || geteuid() == 0) && !allowRoot)
	{
		printf("LMMS cannot be run as root.\nUse \"--allowroot\" to override.\n\n");
		exit(EXIT_FAILURE);
	}
}
#endif


inline void loadTranslation(const QString &tname,
                            const QString &dir = lmms::ConfigManager::inst()->localeDir())
{
	auto t = new QTranslator(QCoreApplication::instance());
	QString name = tname + ".qm";

	if (t->load(name, dir))
	{
		QCoreApplication::instance()->installTranslator(t);
	}
}




void fileCheck( QString &file )
{
	QFileInfo fileToCheck( file );

	if( !fileToCheck.size() )
	{
		printf( "The file %s does not have any content.\n",
				file.toUtf8().constData() );
		exit( EXIT_FAILURE );
	}
	else if( fileToCheck.isDir() )
	{
		printf( "%s is a directory.\n",
				file.toUtf8().constData() );
		exit( EXIT_FAILURE );
	}
}

/*
int usageError(const QString& message)
{
	qCritical().noquote() << QString("\n%1.\n\nTry \"%2 --help\" for more information.\n\n")
				   .arg( message ).arg( qApp->arguments()[0] );
	return EXIT_FAILURE;
}

int noInputFileError()
{
	return usageError( "No input file specified" );
}
*/


void initQCoreApp() {
#ifdef LMMS_BUILD_LINUX
	// don't let OS steal the menu bar. FIXME: only effective on Qt4
	QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}


int main(int argc, char **argv)
{
#ifdef LMMS_DEBUG_FPE
	enableDebugFPE();
#endif

#ifdef LMMS_BUILD_WIN32
	attachConsole();
#endif

	NotePlayHandleManager::init(); // initialize memory managers
	srand(getpid() + time(nullptr)); // intialize RNG
	disable_denormals();

	ArgParser *argParser = new ArgParser(argc, argv);

	// first of two command-line parsing stages
	argParser->stageOneParsing();

#if !defined(LMMS_BUILD_WIN32) && !defined(LMMS_BUILD_HAIKU)
	rootTest(argParser->m_allowRoot);
#endif

	initQCoreApp();
	QCoreApplication *app = argParser->m_coreOnly
	                        ? new QCoreApplication(argc, argv)
	                        : new gui::MainApplication(argc, argv);

	// second of two command-line parsing stages
	argParser->stageTwoParsing();

	// Test file argument before continuing
	if(!argParser->m_fileToLoad.isEmpty())
	{
		fileCheck(argParser->m_fileToLoad);
	}
	else if(!argParser->m_fileToImport.isEmpty())
	{
		fileCheck(argParser->m_fileToImport);
	}

	ConfigManager::inst()->loadConfigFile(argParser->m_configFile);

	// Hidden settings
	MixHelpers::setNaNHandler(ConfigManager::inst()->value("app",
	                          "nanhandler", "1").toInt());

	// set language
	QString pos = ConfigManager::inst()->value("app", "language");
	if(pos.isEmpty())
	{
		pos = QLocale::system().name().left(2);
	}

	// load actual translation for LMMS
	loadTranslation(pos);

	// load translation for Qt-widgets/-dialogs
#ifdef QT_TRANSLATIONS_DIR
	// load from the original path first
	loadTranslation(QString("qt_") + pos, QT_TRANSLATIONS_DIR);
#endif
	// override it with bundled/custom one, if exists
	loadTranslation(QString("qt_") + pos, ConfigManager::inst()->localeDir());


	// try to set realtime priority
#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_FREEBSD)
#ifdef LMMS_HAVE_SCHED_H
#ifndef __OpenBSD__
	printf("Attempting to set realtime priority...\n");
	struct sched_param sparam;
	sparam.sched_priority = (sched_get_priority_max(SCHED_FIFO) +
	                         sched_get_priority_min(SCHED_FIFO)) / 2;
	if (sched_setscheduler( 0, SCHED_FIFO, &sparam ) == -1)
	{
		printf("Notice: could not set realtime priority.\n");
	}
#endif
#endif // LMMS_HAVE_SCHED_H
#endif

#ifdef LMMS_BUILD_WIN32
	if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
	{
		printf("Notice: could not set high priority.\n");
	}
#endif

#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_SIGINFO;
	if (sigemptyset(&sa.sa_mask))
	{
		fprintf(stderr, "Signal initialization failed.\n");
	}
	if (sigaction(SIGPIPE, &sa, nullptr))
	{
		fprintf(stderr, "Signal initialization failed.\n");
	}
#endif

	bool destroyEngine = false;

	// if we have an output file for rendering, just render the song
	// without starting the GUI
	if(!argParser->m_renderOut.isEmpty())
	{
		Engine::init(true);
		destroyEngine = true;

		printf("Loading project...\n" );
		Engine::getSong()->loadProject( argParser->m_fileToLoad );
		if (Engine::getSong()->isEmpty())
		{
			printf("The project %s is empty, aborting!\n", argParser->m_fileToLoad.toUtf8().constData() );
			exit( EXIT_FAILURE );
		}
		printf( "Done\n" );

		Engine::getSong()->setExportLoop( argParser->m_renderLoop );

		// when rendering multiple tracks, renderOut is a directory
		// otherwise, it is a file, so we need to append the file extension
		if ( !argParser->m_renderTracks )
		{
			argParser->m_renderOut = baseName( argParser->m_renderOut ) +
				ProjectRenderer::getFileExtensionFromFormat(argParser->m_eff);
		}

		// create renderer
		auto r = new RenderManager(argParser->m_qs, argParser->m_os, argParser->m_eff, argParser->m_renderOut);
		QCoreApplication::instance()->connect( r,
				SIGNAL(finished()), SLOT(quit()));

		// timer for progress-updates
		auto t = new QTimer(r);
		r->connect( t, SIGNAL(timeout()),
				SLOT(updateConsoleProgress()));
		t->start( 200 );

		if( argParser->m_profilerOutputFile.isEmpty() == false )
		{
			Engine::audioEngine()->profiler().setOutputFile( argParser->m_profilerOutputFile );
		}

		// start now!
		if ( argParser->m_renderTracks )
		{
			r->renderTracks();
		}
		else
		{
			r->renderProject();
		}
	}
	else // otherwise, start the GUI
	{
		using namespace lmms::gui;

		new GuiApplication();

		// re-intialize RNG - shared libraries might have srand() or
		// srandom() calls in their init procedure
		srand( getpid() + time( 0 ) );

		// recover a file?
		QString recoveryFile = ConfigManager::inst()->recoveryFile();

		bool recoveryFilePresent = QFileInfo( recoveryFile ).exists() &&
				QFileInfo( recoveryFile ).isFile();
		bool autoSaveEnabled =
			ConfigManager::inst()->value( "ui", "enableautosave" ).toInt();
		if( recoveryFilePresent )
		{
			QMessageBox mb;
			mb.setWindowTitle( MainWindow::tr( "Project recovery" ) );
			mb.setText(QString(
				"<html>"
				"<p style=\"margin-left:6\">%1</p>"
				"<table cellpadding=\"3\">"
				"  <tr>"
				"    <td><b>%2</b></td>"
				"    <td>%3</td>"
				"  </tr>"
				"  <tr>"
				"    <td><b>%4</b></td>"
				"    <td>%5</td>"
				"  </tr>"
				"</table>"
				"</html>" ).arg(
				MainWindow::tr( "There is a recovery file present. "
					"It looks like the last session did not end "
					"properly or another instance of LMMS is "
					"already running. Do you want to recover the "
					"project of this session?" ),
				MainWindow::tr( "Recover" ),
				MainWindow::tr( "Recover the file. Please don't run "
					"multiple instances of LMMS when you do this." ),
				MainWindow::tr( "Discard" ),
				MainWindow::tr( "Launch a default session and delete "
					"the restored files. This is not reversible." )
							) );

			mb.setIcon( QMessageBox::Warning );
			mb.setWindowIcon( embed::getIconPixmap( "icon_small" ) );
			mb.setWindowFlags( Qt::WindowCloseButtonHint );

			QPushButton * recover;
			QPushButton * discard;
			QPushButton * exit;

			// setting all buttons to the same roles allows us
			// to have a custom layout
			discard = mb.addButton( MainWindow::tr( "Discard" ),
								QMessageBox::AcceptRole );
			recover = mb.addButton( MainWindow::tr( "Recover" ),
								QMessageBox::AcceptRole );

			// have a hidden exit button
			exit = mb.addButton( "", QMessageBox::RejectRole);
			exit->setVisible(false);

			// set icons
			recover->setIcon( embed::getIconPixmap( "recover" ) );
			discard->setIcon( embed::getIconPixmap( "discard" ) );

			mb.setDefaultButton( recover );
			mb.setEscapeButton( exit );

			mb.exec();
			if( mb.clickedButton() == discard )
			{
				getGUI()->mainWindow()->sessionCleanup();
			}
			else if( mb.clickedButton() == recover ) // Recover
			{
				argParser->m_fileToLoad = recoveryFile;
				getGUI()->mainWindow()->setSession( MainWindow::SessionState::Recover );
			}
			else // Exit
			{
				return EXIT_SUCCESS;
			}
		}

		// first show the Main Window and then try to load given file

		// [Settel] workaround: showMaximized() doesn't work with
		// FVWM2 unless the window is already visible -> show() first
		getGUI()->mainWindow()->show();
		if( argParser->m_fullscreen )
		{
			getGUI()->mainWindow()->showMaximized();
		}

		// Handle macOS-style FileOpen QEvents
		QString queuedFile = static_cast<MainApplication *>( app )->queuedFile();
		if ( !queuedFile.isEmpty() ) {
			argParser->m_fileToLoad = queuedFile;
		}

		if( !argParser->m_fileToLoad.isEmpty() )
		{
			if( argParser->m_fileToLoad == recoveryFile )
			{
				Engine::getSong()->createNewProjectFromTemplate( argParser->m_fileToLoad );
			}
			else
			{
				Engine::getSong()->loadProject( argParser->m_fileToLoad );
			}
		}
		else if( !argParser->m_fileToImport.isEmpty() )
		{
			ImportFilter::import( argParser->m_fileToImport, Engine::getSong() );
			if( argParser->m_exitAfterImport )
			{
				return EXIT_SUCCESS;
			}
		}
		// If enabled, open last project if there is one. Else, create
		// a new one.
		else if( ConfigManager::inst()->
				value( "app", "openlastproject" ).toInt() &&
			!ConfigManager::inst()->
				recentlyOpenedProjects().isEmpty() &&
				!recoveryFilePresent )
		{
			QString f = ConfigManager::inst()->
					recentlyOpenedProjects().first();
			QFileInfo recentFile( f );

			if ( recentFile.exists() &&
				recentFile.suffix().toLower() != "mpt" )
			{
				Engine::getSong()->loadProject( f );
			}
			else
			{
				Engine::getSong()->createNewProject();
			}
		}
		else
		{
			Engine::getSong()->createNewProject();
		}

		// Finally we start the auto save timer and also trigger the
		// autosave one time as recover.mmp is a signal to possible other
		// instances of LMMS.
		if( autoSaveEnabled )
		{
			gui::getGUI()->mainWindow()->autoSaveTimerReset();
		}
	}

	const int ret = app->exec();
	delete app;

	if( destroyEngine )
	{
		Engine::destroy();
	}

	// ProjectRenderer::updateConsoleProgress() doesn't return line after render
	if( argParser->m_coreOnly )
	{
		printf( "\n" );
	}

#ifdef LMMS_BUILD_WIN32
	// Cleanup console
	HWND hConsole = GetConsoleWindow();
	if (hConsole)
	{
		SendMessage(hConsole, WM_CHAR, (WPARAM)VK_RETURN, (LPARAM)0);
		FreeConsole();
	}
#endif


	NotePlayHandleManager::free();

	return ret;
}
