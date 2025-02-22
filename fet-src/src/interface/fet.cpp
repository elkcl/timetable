/*
File fet.cpp - this is where FET starts
*/

/***************************************************************************
                          fet.cpp  -  description
                             -------------------
    begin                : 2002
    copyright            : (C) 2002 by Lalescu Liviu
    email                : Please see https://lalescu.ro/liviu/ for details about contacting Liviu Lalescu (in particular, you can find here the e-mail address)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Affero General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************/

#include "fet.h"

#include "matrix.h"

#include "messageboxes.h"

#include "rules.h"

#ifndef FET_COMMAND_LINE
#include <QMessageBox>

#include <QWidget>
#endif

#include <QLocale>
#include <QTime>
#include <QDate>
#include <QDateTime>

#include <QSet>

static QSet<QString> languagesSet;

//#include <ctime>
#include <cstdlib>

#include "timetableexport.h"
#include "generate.h"

#include "timetable_defs.h"
#include "timetable.h"

extern MRG32k3a rng;

#ifndef FET_COMMAND_LINE
#include "fetmainform.h"

#include "helpaboutform.h"
#include "helpaboutlibrariesform.h"
#include "helpfaqform.h"
#include "helptipsform.h"
#include "helpinstructionsform.h"

#include "timetableshowconflictsform.h"
#include "timetableviewstudentsdayshorizontalform.h"
#include "timetableviewstudentstimehorizontalform.h"
#include "timetableviewteachersdayshorizontalform.h"
#include "timetableviewteacherstimehorizontalform.h"
#include "timetableviewroomsdayshorizontalform.h"
#include "timetableviewroomstimehorizontalform.h"
#endif

#include <QCoreApplication>

#ifndef FET_COMMAND_LINE
#include <QApplication>
#include <QWidgetList>

#include <QSettings>
#include <QRect>
#endif

#include <QMutex>
#include <QString>
#include <QTranslator>

#include <QDir>

#include <QTextStream>
#include <QFile>

#include <Qt>

#ifdef FET_COMMAND_LINE
#include <csignal>
#include <QtGlobal>
#endif

#include <iostream>
using namespace std;

#ifndef FET_COMMAND_LINE
extern QRect mainFormSettingsRect;
extern int MAIN_FORM_SHORTCUTS_TAB_POSITION;
#endif

extern Solution highestStageSolution;

extern int maxActivitiesPlaced;

#ifndef FET_COMMAND_LINE
extern int initialOrderOfActivitiesIndices[MAX_ACTIVITIES];
#else
int initialOrderOfActivitiesIndices[MAX_ACTIVITIES];
#endif

extern bool students_schedule_ready;
extern bool teachers_schedule_ready;
extern bool rooms_schedule_ready;

#ifndef FET_COMMAND_LINE
extern QMutex myMutex;
#else
QMutex myMutex;
#endif

void writeDefaultSimulationParameters();

QTranslator translator;

/**
The one and only instantiation of the main class.
*/
Timetable gt;

/**
The name of the file from where the rules are read.
*/
QString INPUT_FILENAME_XML;

/**
The working directory
*/
QString WORKING_DIRECTORY;

/**
The import directory
*/
QString IMPORT_DIRECTORY;

/*qint16 teachers_timetable_weekly[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
qint16 students_timetable_weekly[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
qint16 rooms_timetable_weekly[MAX_ROOMS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];*/
Matrix3D<int> teachers_timetable_weekly;
Matrix3D<int> students_timetable_weekly;
Matrix3D<int> rooms_timetable_weekly;
Matrix3D<QList<int>> virtual_rooms_timetable_weekly;
//QList<qint16> teachers_free_periods_timetable_weekly[TEACHERS_FREE_PERIODS_N_CATEGORIES][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
Matrix3D<QList<int>> teachers_free_periods_timetable_weekly;

#ifndef FET_COMMAND_LINE
QApplication* pqapplication=nullptr;

FetMainForm* pFetMainForm=nullptr;
#endif

//extern int XX;
//extern int YY;

Generate* terminateGeneratePointer;

#ifdef FET_COMMAND_LINE
#include "export.h"

extern bool EXPORT_CSV;
extern bool EXPORT_ALLOW_OVERWRITE;
extern bool EXPORT_FIRST_LINE_IS_HEADING;
extern int EXPORT_QUOTES;

extern const int EXPORT_DOUBLE_QUOTES;
extern const int EXPORT_SINGLE_QUOTES;
extern const int EXPORT_NO_QUOTES;

extern int EXPORT_FIELD_SEPARATOR;

extern const int EXPORT_COMMA;
extern const int EXPORT_SEMICOLON;
extern const int EXPORT_VERTICALBAR;

extern Solution best_solution;

extern QString DIRECTORY_CSV;
#endif

//for command line version, if the user stops using a signal
#ifdef FET_COMMAND_LINE
void terminate(int param)
{
	Q_UNUSED(param);

	assert(terminateGeneratePointer!=nullptr);
	
	terminateGeneratePointer->abortOptimization=true;
}

void usage(QTextStream* out, const QString& error)
{
	QString s="";
	
	if(!error.isEmpty()){
		s+=QString("Incorrect command-line parameters (%1).").arg(error);
	
		s+="\n\n";
	}
	
	s+=QString(
		"Usage: fet-cl --inputfile=FILE [other options]\n"
		"\t\tFILE is the input file, for instance \"data.fet\"\n"
		"\n"
		"Other options are:\n"
		"\n"
		"\t--outputdir=DIR\n"
		"\t\tDIR is the path to results directory, without trailing slash or backslash (default is current working path). "
		"Make sure you have write permissions there.\n"
		"\n"
		"\t--timelimitseconds=TLS\n"
		"\t\tTLS is an integer representing the time limit, in seconds, after which the program will stop the generation "
		"(default 2000000000, which is practically infinite).\n"
		"\n"
		"\t--htmllevel=LEVEL\n"
		"\t\tLEVEL is an integer from 0 to 7 and represents the detail level for the generated HTML timetables "
		"(default 2, larger values have more details/facilities and larger file sizes).\n"
		"\n"
		"\t--language=LANG\n"
		"\t\tLANG is one of: ar, ca, cs, da, de, el, en_GB, en_US, es, eu, fa, fr, gl, he, hu, id, it, ja, lt, mk, ms, nl, pl, pt_BR, ro, ru, si, sk, sq, sr, tr, "
		"uk, uz, vi, zh_CN, zh_TW (default en_US).\n"
		"\n"
		"\t--writetimetableconflicts=WT1\n"
		"\t--writetimetablesstatistics=WT2\n"
		"\t--writetimetablesxml=WT3\n"
		"\t--writetimetablesdayshorizontal=WT4\n"
		"\t--writetimetablesdaysvertical=WT5\n"
		"\t--writetimetablestimehorizontal=WT6\n"
		"\t--writetimetablestimevertical=WT7\n"
		"\t--writetimetablessubgroups=WT8\n"
		"\t--writetimetablesgroups=WT9\n"
		"\t--writetimetablesyears=WT10\n"
		"\t--writetimetablesteachers=WT11\n"
		"\t--writetimetablesteachersfreeperiods=WT12\n"
		"\t--writetimetablesrooms=WT13\n"
		"\t--writetimetablessubjects=WT14\n"
		"\t--writetimetablesactivitytags=WT15\n"
		"\t--writetimetablesactivities=WT16\n"
		"\t\tWT1 to WT16 are either true or false and represent whether you want the corresponding timetables to be written on the disk (default true).\n"
		"\n"
		"\t--printactivitytags=PAT\n"
		"\t\tPAT is either true or false and represets if you want activity tags to be present in the final HTML timetables (default true).\n"
		"\n"
		"\t--printnotavailable=PNA\n"
		"\t\tPNA is either true or false and represents if you want -x- (for true) or --- (for false) in the generated timetables for the "
		"not available slots (default true).\n"
		"\n"
		"\t--printbreak=PB\n"
		"\t\tPB is either true or false and represents if you want -X- (for true) or --- (for false) in the generated timetables for the "
		"break slots (default true).\n"
		"\n"
		"\t--dividetimeaxisbydays=DTAD\n"
		"\t\tDTAD is either true or false, represents if you want the HTML timetables with time-axis divided by days (default false).\n"
		"\n"
		"\t--duplicateverticalheaders=DVH\n"
		"\t\tDVH is either true or false, represents if you want the HTML timetables to duplicate vertical headers to the right of the tables, for easier reading (default false).\n"
		"\n"
		"\t--printsimultaneousactivities=PSA\n"
		"\t\tPSA is either true or false, represents if you want the HTML timetables to show related activities which have constraints with same starting time (default false). "
		"(for instance, if A1 (T1, G1) and A2 (T2, G2) have constraint activities same starting time, then in T1's timetable will appear also A2, at the same slot "
		"as A1).\n"
		"\n"
		"\t--randomseeds10=s10 --randomseeds11=s11 --randomseeds12=s12 --randomseeds20=s20 --randomseeds21=s21 --randomseeds22=s22\n"
		"\t\twhere you need to specify all the 6 random seed components, and s10, s11, and s12 are integers from minimum 0 to maximum 4294967086,"
		" not all 3 zero, and s20, s21, and s22 are integers from minimum 0 to maximum 4294944442, not all 3 zero "
		"(you can get the same timetable if the input file is identical, if the FET version is the same (or if the generation algorithm did not change),"
		" and if all the 6 random seed components are respectively equal).\n"
		"\n"
		"\t--warnifusingnotperfectconstraints=WNP\n"
		"\t\tWNP is either true or false, represents whether you want a message box to be shown, with a warning, if the input file contains not perfect constraints "
		"(activity tag max hours daily or students max gaps per day) (default true).\n"
		"\n"
		"\t--warnifusingstudentsminhoursdailywithallowemptydays=WSMHDAED\n"
		"\t\tSMHDAEDP is either true or false, represents whether you want a message box to be shown, with a warning, if the input file contains nonstandard constraints "
		"students min hours daily with allow empty days (default true).\n"
		"\n"
		"\t--warnifusinggroupactivitiesininitialorder=WGA\n"
		"\t\tWGA is either true or false, represents whether you want a message box to be shown, with a warning, if the input file contains nonstandard timetable "
		"generation options to group activities in the initial order (default true).\n"
		"\n"
		"\t--warnsubgroupswiththesameactivities=WSSA\n"
		"\t\tWSSA is either true or false, represents whether you want a message box to be show, with a warning, if your input file contains subgroups which have "
		"the same activities (default true).\n"
		"\n"
		"\t--printdetailedtimetables=PDT\n"
		"\t\tPDT is either true or false, represents whether you want to show the detailed (true) or less detailed (false) years and groups timetables (default true).\n"
		"\n"
		"\t--printdetailedteachersfreeperiodstimetables=PDTFP\n"
		"\t\tPDTFP is either true or false, represents whether you want to show the detailed (true) or less detailed (false) teachers free periods timetables (default true).\n"
		"\n"
		"\t--exportcsv=ECSV\n"
		"\t\tECSV is either true or false, represents whether you want to export the CSV file and timetables (default false).\n"
		"\n"
		"\t--overwritecsv=OCSV\n"
		"\t\tOCSV is either true or false, represents whether you want to overwrite old CSV files, if they exist (default false).\n"
		"\n"
		"\t--firstlineisheadingcsv=FLHCSV\n"
		"\t\tFLHCSV is either true or false, represents whether you want the heading of the CSV files to be header (true) or data (false). The default value is true.\n"
		"\n"
		"\t--quotescsv=QCSV\n"
		"\t\tQCSV is one value from the set [doublequotes|singlequotes|none] (write a single value from these three exactly as it is written here). The default value is "
		"doublequotes.\n"
		"\n"
		"\t--fieldseparatorcsv=FSCSV\n"
		"\t\tFSCSV is one value from the set [comma|semicolon|verticalbar] (write a single value from these three exactly as it is written here). The default value is "
		"comma.\n"
		"\n"
		"\t--showvirtualrooms=SVR\n"
		"\t\tSVR is either true or false, represents whether you want to show virtual rooms in the timetables (default false).\n"
		"\n"
		"\t--warnifusingactivitiesnotfixedtimefixedspacevirtualroomsrealrooms=WANFTFS\n"
		"\t\tWANFTFS is either true or false, represents whether you want the program to issue a warning if you are using activites which are not fixed in time, "
		"but are fixed in space in a virtual room, specifying also the real rooms (which is not recommended) (default true).\n"
		"\n"
		"\t--warnifusingmaxhoursdailywithlessthan100percentweight=WMHDWLT100PW\n"
		"\t\tWMHDWLT100PW is either true or false, represents whether you want the program to issue a warning if you are using constraints of type teacher(s)/students (set) "
		"max hours daily with a weight less than 100% (default true).\n"
		"\n"
		"\t--verbose=VBS\n"
		"\t\tVBS is either true or false, represents whether you want additional generation messages and other messages to be shown on the command line (default false).\n"
		"\n"
		"Run \"fet-cl --help\" to get usage information.\n"
		"\n"
		"Run \"fet-cl --version\" to get version information.\n"
		"\n"
		"You can ask the FET command line process to stop the timetable generation, by sending it the SIGTERM (or SIGBREAK, on Windows) signal. "
		"FET will then write the current timetable and the highest stage timetable and exit. "
		"(If you send the FET command line the SIGINT signal it will stop immediately, without writing the timetable.)"
	);
	
	cout<<qPrintable(s)<<endl;
	if(out!=nullptr)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		(*out)<<qPrintable(s)<<Qt::endl;
#else
		(*out)<<qPrintable(s)<<endl;
#endif
}
#endif

#ifndef FET_COMMAND_LINE
void readSimulationParameters()
{
	const QString predefDir=QDir::homePath()+FILE_SEP+"fet-results";

	QSettings newSettings(COMPANY, PROGRAM);

	if(newSettings.contains("output-directory")){
		OUTPUT_DIR=newSettings.value("output-directory").toString();
		QDir dir;
		if(!dir.exists(OUTPUT_DIR)){
			bool t=dir.mkpath(OUTPUT_DIR);
			if(!t){
				QMessageBox::warning(nullptr, FetTranslate::tr("FET warning"), FetTranslate::tr("Output directory %1 does not exist and cannot be"
				 " created - output directory will be made the default value %2")
				 .arg(QDir::toNativeSeparators(OUTPUT_DIR)).arg(QDir::toNativeSeparators(predefDir)));
				OUTPUT_DIR=predefDir;
			}
		}
	}
	else{
		OUTPUT_DIR=predefDir;
	}

#ifndef USE_SYSTEM_LOCALE
	FET_LANGUAGE=newSettings.value("language", "en_US").toString();
#else
	if(newSettings.contains("language")){
		FET_LANGUAGE=newSettings.value("language").toString();
	}
	else{
		FET_LANGUAGE=QLocale::system().name();

		bool ok=false;
		for(const QString& s : qAsConst(languagesSet)){
			if(FET_LANGUAGE.left(s.length())==s){
				FET_LANGUAGE=s;
				ok=true;
				break;
			}
		}
		if(!ok)
			FET_LANGUAGE="en_US";
	}
#endif
	
	WORKING_DIRECTORY=newSettings.value("working-directory", "examples").toString();
	IMPORT_DIRECTORY=newSettings.value("import-directory", OUTPUT_DIR).toString();
	
	QDir d(WORKING_DIRECTORY);
	if(!d.exists())
		WORKING_DIRECTORY="examples";
	QDir d2(WORKING_DIRECTORY);
	if(!d2.exists())
		WORKING_DIRECTORY=QDir::homePath();
	else
		WORKING_DIRECTORY=d2.absolutePath();

	QDir i(IMPORT_DIRECTORY);
	if(!i.exists())
		IMPORT_DIRECTORY=OUTPUT_DIR;
	
	checkForUpdates=newSettings.value("check-for-updates", "false").toBool();

	QString ver=newSettings.value("version", "-1").toString();
	
	TIMETABLE_HTML_LEVEL=newSettings.value("html-level", "2").toInt();
	TIMETABLE_HTML_PRINT_ACTIVITY_TAGS=newSettings.value("print-activity-tags", "true").toBool();
	PRINT_DETAILED_HTML_TIMETABLES=newSettings.value("print-detailed-timetables", "true").toBool();
	PRINT_DETAILED_HTML_TEACHERS_FREE_PERIODS=newSettings.value("print-detailed-teachers-free-periods-timetables", "true").toBool();
	PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=newSettings.value("print-activities-with-same-starting-time", "false").toBool();
	PRINT_NOT_AVAILABLE_TIME_SLOTS=newSettings.value("print-not-available", "true").toBool();
	PRINT_BREAK_TIME_SLOTS=newSettings.value("print-break", "true").toBool();
	DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=newSettings.value("divide-html-timetables-with-time-axis-by-days", "false").toBool();
	TIMETABLE_HTML_REPEAT_NAMES=newSettings.value("timetables-repeat-vertical-names", "false").toBool();
	
	USE_GUI_COLORS=newSettings.value("use-gui-colors", "false").toBool();

	SHOW_SUBGROUPS_IN_COMBO_BOXES=newSettings.value("show-subgroups-in-combo-boxes", "true").toBool();
	SHOW_SUBGROUPS_IN_ACTIVITY_PLANNING=newSettings.value("show-subgroups-in-activity-planning", "true").toBool();
	
	WRITE_TIMETABLE_CONFLICTS=newSettings.value("write-timetable-conflicts", "true").toBool();

	WRITE_TIMETABLES_STATISTICS=newSettings.value("write-timetables-statistics", "true").toBool();
	WRITE_TIMETABLES_XML=newSettings.value("write-timetables-xml", "true").toBool();
	WRITE_TIMETABLES_DAYS_HORIZONTAL=newSettings.value("write-timetables-days-horizontal", "true").toBool();
	WRITE_TIMETABLES_DAYS_VERTICAL=newSettings.value("write-timetables-days-vertical", "true").toBool();
	WRITE_TIMETABLES_TIME_HORIZONTAL=newSettings.value("write-timetables-time-horizontal", "true").toBool();
	WRITE_TIMETABLES_TIME_VERTICAL=newSettings.value("write-timetables-time-vertical", "true").toBool();

	WRITE_TIMETABLES_SUBGROUPS=newSettings.value("write-timetables-subgroups", "true").toBool();
	WRITE_TIMETABLES_GROUPS=newSettings.value("write-timetables-groups", "true").toBool();
	WRITE_TIMETABLES_YEARS=newSettings.value("write-timetables-years", "true").toBool();
	WRITE_TIMETABLES_TEACHERS=newSettings.value("write-timetables-teachers", "true").toBool();
	WRITE_TIMETABLES_TEACHERS_FREE_PERIODS=newSettings.value("write-timetables-teachers-free-periods", "true").toBool();
	WRITE_TIMETABLES_ROOMS=newSettings.value("write-timetables-rooms", "true").toBool();
	WRITE_TIMETABLES_SUBJECTS=newSettings.value("write-timetables-subjects", "true").toBool();
	WRITE_TIMETABLES_ACTIVITY_TAGS=newSettings.value("write-timetables-activity-tags", "true").toBool();
	WRITE_TIMETABLES_ACTIVITIES=newSettings.value("write-timetables-activities", "true").toBool();
	
	if(newSettings.contains("students-combo-boxes-style"))
		STUDENTS_COMBO_BOXES_STYLE=newSettings.value("students-combo-boxes-style").toInt();
	else
		STUDENTS_COMBO_BOXES_STYLE=STUDENTS_COMBO_BOXES_STYLE_SIMPLE;

/////////confirmations
	CONFIRM_ACTIVITY_PLANNING=newSettings.value("confirm-activity-planning", "true").toBool();
	CONFIRM_SPREAD_ACTIVITIES=newSettings.value("confirm-spread-activities", "true").toBool();
	CONFIRM_REMOVE_REDUNDANT=newSettings.value("confirm-remove-redundant", "true").toBool();
	CONFIRM_SAVE_TIMETABLE=newSettings.value("confirm-save-data-and-timetable", "true").toBool();
/////////

	ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY=newSettings.value("enable-activity-tag-max-hours-daily", "false").toBool();
	ENABLE_ACTIVITY_TAG_MIN_HOURS_DAILY=newSettings.value("enable-activity-tag-min-hours-daily", "false").toBool();
	ENABLE_STUDENTS_MAX_GAPS_PER_DAY=newSettings.value("enable-students-max-gaps-per-day", "false").toBool();
	SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=newSettings.value("warn-if-using-not-perfect-constraints", "true").toBool();
	SHOW_WARNING_FOR_SUBGROUPS_WITH_THE_SAME_ACTIVITIES=newSettings.value("warn-subgroups-with-the-same-activities", "true").toBool();
	SHOW_WARNING_FOR_ACTIVITIES_FIXED_SPACE_VIRTUAL_REAL_ROOMS_BUT_NOT_FIXED_TIME=newSettings.value("warn-activities-not-fixed-time-fixed-space-virtual-real", "true").toBool();
	SHOW_WARNING_FOR_MAX_HOURS_DAILY_WITH_UNDER_100_WEIGHT=newSettings.value("warn-max-hours-daily-with-under-100-weight", "true").toBool();
	ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=newSettings.value("enable-students-min-hours-daily-with-allow-empty-days", "false").toBool();
	SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=newSettings.value("warn-if-using-students-min-hours-daily-with-allow-empty-days", "true").toBool();
	
	ENABLE_GROUP_ACTIVITIES_IN_INITIAL_ORDER=newSettings.value("enable-group-activities-in-initial-order", "false").toBool();
	SHOW_WARNING_FOR_GROUP_ACTIVITIES_IN_INITIAL_ORDER=newSettings.value("warn-if-using-group-activities-in-initial-order", "true").toBool();

	SHOW_VIRTUAL_ROOMS_IN_TIMETABLES=newSettings.value("show-virtual-rooms-in-timetables", "false").toBool();
	
	//main form
	QRect rect=newSettings.value("FetMainForm/geometry", QRect(0,0,0,0)).toRect();
	mainFormSettingsRect=rect;
	//MAIN_FORM_SHORTCUTS_TAB_POSITION=newSettings.value("FetMainForm/shortcuts-tab-position", "0").toInt();
	MAIN_FORM_SHORTCUTS_TAB_POSITION=0; //always restoring to the first page, as suggested by a user
	SHOW_SHORTCUTS_ON_MAIN_WINDOW=newSettings.value("FetMainForm/show-shortcuts", "true").toBool();

	SHOW_TOOLTIPS_FOR_CONSTRAINTS_WITH_TABLES=newSettings.value("FetMainForm/show-tooltips-for-constraints-with-tables", "false").toBool();
	
	BEEP_AT_END_OF_GENERATION=newSettings.value("beep-at-the-end-of-generation", "true").toBool();
	ENABLE_COMMAND_AT_END_OF_GENERATION=newSettings.value("enable-command-at-the-end-of-generation", "false").toBool();
	commandAtEndOfGeneration=newSettings.value("command-at-the-end-of-generation", "").toString();
//	DETACHED_NOTIFICATION=newSettings.value("detached-notification", "false").toBool();
//	terminateCommandAfterSeconds=newSettings.value("terminate-command-at-the-end-of-generation-after-seconds", "0").toInt();
//	killCommandAfterSeconds=newSettings.value("kill-command-at-the-end-of-generation-after-seconds", "0").toInt();
	
	if(VERBOSE){
		cout<<"Settings read"<<endl;
	}
}

void writeSimulationParameters()
{
	QSettings settings(COMPANY, PROGRAM);

	settings.setValue("output-directory", OUTPUT_DIR);
	settings.setValue("language", FET_LANGUAGE);
	settings.setValue("working-directory", WORKING_DIRECTORY);
	settings.setValue("import-directory", IMPORT_DIRECTORY);
	settings.setValue("version", FET_VERSION);
	settings.setValue("check-for-updates", checkForUpdates);
	settings.setValue("html-level", TIMETABLE_HTML_LEVEL);
	settings.setValue("print-activity-tags", TIMETABLE_HTML_PRINT_ACTIVITY_TAGS);
	settings.setValue("print-detailed-timetables", PRINT_DETAILED_HTML_TIMETABLES);
	settings.setValue("print-detailed-teachers-free-periods-timetables", PRINT_DETAILED_HTML_TEACHERS_FREE_PERIODS);
	settings.setValue("print-activities-with-same-starting-time", PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME);
	settings.setValue("divide-html-timetables-with-time-axis-by-days", DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS);
	settings.setValue("timetables-repeat-vertical-names", TIMETABLE_HTML_REPEAT_NAMES);
	settings.setValue("print-not-available", PRINT_NOT_AVAILABLE_TIME_SLOTS);
	settings.setValue("print-break", PRINT_BREAK_TIME_SLOTS);
	
	settings.setValue("use-gui-colors", USE_GUI_COLORS);

	settings.setValue("show-subgroups-in-combo-boxes", SHOW_SUBGROUPS_IN_COMBO_BOXES);
	settings.setValue("show-subgroups-in-activity-planning", SHOW_SUBGROUPS_IN_ACTIVITY_PLANNING);
	
	settings.setValue("write-timetable-conflicts", WRITE_TIMETABLE_CONFLICTS);

	settings.setValue("write-timetables-statistics", WRITE_TIMETABLES_STATISTICS);
	settings.setValue("write-timetables-xml", WRITE_TIMETABLES_XML);
	settings.setValue("write-timetables-days-horizontal", WRITE_TIMETABLES_DAYS_HORIZONTAL);
	settings.setValue("write-timetables-days-vertical", WRITE_TIMETABLES_DAYS_VERTICAL);
	settings.setValue("write-timetables-time-horizontal", WRITE_TIMETABLES_TIME_HORIZONTAL);
	settings.setValue("write-timetables-time-vertical", WRITE_TIMETABLES_TIME_VERTICAL);

	settings.setValue("write-timetables-subgroups", WRITE_TIMETABLES_SUBGROUPS);
	settings.setValue("write-timetables-groups", WRITE_TIMETABLES_GROUPS);
	settings.setValue("write-timetables-years", WRITE_TIMETABLES_YEARS);
	settings.setValue("write-timetables-teachers", WRITE_TIMETABLES_TEACHERS);
	settings.setValue("write-timetables-teachers-free-periods", WRITE_TIMETABLES_TEACHERS_FREE_PERIODS);
	settings.setValue("write-timetables-rooms", WRITE_TIMETABLES_ROOMS);
	settings.setValue("write-timetables-subjects", WRITE_TIMETABLES_SUBJECTS);
	settings.setValue("write-timetables-activity-tags", WRITE_TIMETABLES_ACTIVITY_TAGS);
	settings.setValue("write-timetables-activities", WRITE_TIMETABLES_ACTIVITIES);
	
	settings.setValue("students-combo-boxes-style", STUDENTS_COMBO_BOXES_STYLE);

///////////confirmations
	settings.setValue("confirm-activity-planning", CONFIRM_ACTIVITY_PLANNING);
	settings.setValue("confirm-spread-activities", CONFIRM_SPREAD_ACTIVITIES);
	settings.setValue("confirm-remove-redundant", CONFIRM_REMOVE_REDUNDANT);
	settings.setValue("confirm-save-data-and-timetable", CONFIRM_SAVE_TIMETABLE);
///////////

	settings.setValue("enable-activity-tag-max-hours-daily", ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY);
	settings.setValue("enable-activity-tag-min-hours-daily", ENABLE_ACTIVITY_TAG_MIN_HOURS_DAILY);
	settings.setValue("enable-students-max-gaps-per-day", ENABLE_STUDENTS_MAX_GAPS_PER_DAY);
	settings.setValue("warn-if-using-not-perfect-constraints", SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS);
	settings.setValue("warn-subgroups-with-the-same-activities", SHOW_WARNING_FOR_SUBGROUPS_WITH_THE_SAME_ACTIVITIES);
	settings.setValue("warn-activities-not-fixed-time-fixed-space-virtual-real", SHOW_WARNING_FOR_ACTIVITIES_FIXED_SPACE_VIRTUAL_REAL_ROOMS_BUT_NOT_FIXED_TIME);
	settings.setValue("warn-max-hours-daily-with-under-100-weight", SHOW_WARNING_FOR_MAX_HOURS_DAILY_WITH_UNDER_100_WEIGHT);
	settings.setValue("enable-students-min-hours-daily-with-allow-empty-days", ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS);
	settings.setValue("warn-if-using-students-min-hours-daily-with-allow-empty-days", SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS);

	settings.setValue("enable-group-activities-in-initial-order", ENABLE_GROUP_ACTIVITIES_IN_INITIAL_ORDER);
	settings.setValue("warn-if-using-group-activities-in-initial-order", SHOW_WARNING_FOR_GROUP_ACTIVITIES_IN_INITIAL_ORDER);

	settings.setValue("show-virtual-rooms-in-timetables", SHOW_VIRTUAL_ROOMS_IN_TIMETABLES);

	//main form
	settings.setValue("FetMainForm/geometry", mainFormSettingsRect);
	//settings.setValue("FetMainForm/shortcuts-tab-position", MAIN_FORM_SHORTCUTS_TAB_POSITION);
	//settings.setValue("FetMainForm/shortcuts-tab-position", 0); //always starting on the first page, as suggested by a user
	settings.setValue("FetMainForm/show-shortcuts", SHOW_SHORTCUTS_ON_MAIN_WINDOW);

	settings.setValue("FetMainForm/show-tooltips-for-constraints-with-tables", SHOW_TOOLTIPS_FOR_CONSTRAINTS_WITH_TABLES);

	settings.setValue("beep-at-the-end-of-generation", BEEP_AT_END_OF_GENERATION);
	settings.setValue("enable-command-at-the-end-of-generation", ENABLE_COMMAND_AT_END_OF_GENERATION);
	settings.setValue("command-at-the-end-of-generation", commandAtEndOfGeneration);
//	settings.setValue("detached-notification", DETACHED_NOTIFICATION);
//	settings.setValue("terminate-command-at-the-end-of-generation-after-seconds", terminateCommandAfterSeconds);
//	settings.setValue("kill-command-at-the-end-of-generation-after-seconds", killCommandAfterSeconds);
	
}
#endif

void initLanguagesSet()
{
	//This is one of the two places to insert a new language in the sources (the other one is in fetmainform.cpp).
	languagesSet.clear();

	languagesSet.insert("en_US");
	languagesSet.insert("en_GB");

	languagesSet.insert("ar");
	languagesSet.insert("ca");
	languagesSet.insert("de");
	languagesSet.insert("el");
	languagesSet.insert("es");
	languagesSet.insert("fr");
	languagesSet.insert("hu");
	languagesSet.insert("id");
	languagesSet.insert("it");
	languagesSet.insert("lt");
	languagesSet.insert("mk");
	languagesSet.insert("ms");
	languagesSet.insert("nl");
	languagesSet.insert("pl");
	languagesSet.insert("ro");
	languagesSet.insert("tr");
	languagesSet.insert("ru");
	languagesSet.insert("fa");
	languagesSet.insert("uk");
	languagesSet.insert("pt_BR");
	languagesSet.insert("da");
	languagesSet.insert("si");
	languagesSet.insert("sk");
	languagesSet.insert("he");
	languagesSet.insert("sr");
	languagesSet.insert("gl");
	languagesSet.insert("vi");
	languagesSet.insert("uz");
	languagesSet.insert("sq");
	languagesSet.insert("zh_CN");
	languagesSet.insert("zh_TW");
	languagesSet.insert("eu");
	languagesSet.insert("cs");
	languagesSet.insert("ja");
}

#ifndef FET_COMMAND_LINE
void setLanguage(QApplication& qapplication, QWidget* parent)
#else
void setLanguage(QCoreApplication& qapplication, QWidget* parent)
#endif
{
	Q_UNUSED(qapplication); //silence wrong MSVC warning

	static int cntTranslators=0;
	
	if(cntTranslators>0){
		qapplication.removeTranslator(&translator);
		cntTranslators=0;
	}

	//translator stuff
	QDir d("/usr/share/fet/translations");
	
	bool translation_loaded=false;
	
	if(FET_LANGUAGE!="en_US" && languagesSet.contains(FET_LANGUAGE)){
		translation_loaded=translator.load("fet_"+FET_LANGUAGE, qapplication.applicationDirPath());
		if(!translation_loaded){
			translation_loaded=translator.load("fet_"+FET_LANGUAGE, qapplication.applicationDirPath()+"/translations");
			if(!translation_loaded){
				if(d.exists()){
					translation_loaded=translator.load("fet_"+FET_LANGUAGE, "/usr/share/fet/translations");
				}
			}
		}
	}
	else{
		if(FET_LANGUAGE!="en_US"){
			FetMessage::warning(parent, QString("FET warning"),
			 QString("Specified language is incorrect - making it en_US (US English)"));
			FET_LANGUAGE="en_US";
		}
		
		assert(FET_LANGUAGE=="en_US");
		
		translation_loaded=true;
	}
	
	if(!translation_loaded){
		FetMessage::warning(parent, QString("FET warning"),
		 QString("Translation for specified language not loaded - maybe the translation file is missing - setting the language to en_US (US English)")
		 +"\n\n"+
		 QString("FET searched for the translation file %1 in the directory %2, then in the directory %3 and "
		 "then in the directory %4 (under systems that support such a directory), but could not find it.")
		 .arg("fet_"+FET_LANGUAGE+".qm")
		 .arg(QDir::toNativeSeparators(qapplication.applicationDirPath()))
		 .arg(QDir::toNativeSeparators(qapplication.applicationDirPath()+"/translations"))
		 .arg("/usr/share/fet/translations")
		 );
		FET_LANGUAGE="en_US";
	}
	
	if(FET_LANGUAGE=="ar" || FET_LANGUAGE=="he" || FET_LANGUAGE=="fa" || FET_LANGUAGE=="ur" /* and others? */){
		LANGUAGE_STYLE_RIGHT_TO_LEFT=true;
	}
	else{
		LANGUAGE_STYLE_RIGHT_TO_LEFT=false;
	}
	
	if(FET_LANGUAGE=="zh_CN"){
		LANGUAGE_FOR_HTML="zh-Hans";
	}
	else if(FET_LANGUAGE=="zh_TW"){
		LANGUAGE_FOR_HTML="zh-Hant";
	}
	else{
		LANGUAGE_FOR_HTML=FET_LANGUAGE;
		LANGUAGE_FOR_HTML.replace(QString("_"), QString("-"));
	}
	
	assert(cntTranslators==0);
	if(FET_LANGUAGE!="en_US"){
		qapplication.installTranslator(&translator);
		cntTranslators=1;
	}
	
#ifndef FET_COMMAND_LINE
	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==true)
		qapplication.setLayoutDirection(Qt::RightToLeft);
	
	//retranslate
	//QList<QWidget*> tlwl=qapplication.topLevelWidgets();
	QWidgetList tlwl=QApplication::topLevelWidgets();

	for(QWidget* wi : qAsConst(tlwl))
		if(1){
		//if(wi->isVisible()){
			FetMainForm* mainform=qobject_cast<FetMainForm*>(wi);
			if(mainform!=nullptr){
				mainform->retranslateUi(mainform);
				continue;
			}

			//help
			HelpAboutForm* aboutf=qobject_cast<HelpAboutForm*>(wi);
			if(aboutf!=nullptr){
				aboutf->retranslateUi(aboutf);
				continue;
			}

			HelpAboutLibrariesForm* aboutlibsf=qobject_cast<HelpAboutLibrariesForm*>(wi);
			if(aboutlibsf!=nullptr){
				aboutlibsf->retranslateUi(aboutlibsf);
				continue;
			}

			HelpFaqForm* faqf=qobject_cast<HelpFaqForm*>(wi);
			if(faqf!=nullptr){
				faqf->retranslateUi(faqf);
				faqf->setText();
				continue;
			}

			HelpTipsForm* tipsf=qobject_cast<HelpTipsForm*>(wi);
			if(tipsf!=nullptr){
				tipsf->retranslateUi(tipsf);
				tipsf->setText();
				continue;
			}

			HelpInstructionsForm* instrf=qobject_cast<HelpInstructionsForm*>(wi);
			if(instrf!=nullptr){
				instrf->retranslateUi(instrf);
				instrf->setText();
				continue;
			}
			//////
			
			//timetable
			TimetableViewStudentsDaysHorizontalForm* vsdf=qobject_cast<TimetableViewStudentsDaysHorizontalForm*>(wi);
			if(vsdf!=nullptr){
				vsdf->retranslateUi(vsdf);
				vsdf->updateStudentsTimetableTable();
				continue;
			}

			TimetableViewStudentsTimeHorizontalForm* vstf=qobject_cast<TimetableViewStudentsTimeHorizontalForm*>(wi);
			if(vstf!=nullptr){
				vstf->retranslateUi(vstf);
				vstf->updateStudentsTimetableTable();
				continue;
			}

			TimetableViewTeachersDaysHorizontalForm* vtchdf=qobject_cast<TimetableViewTeachersDaysHorizontalForm*>(wi);
			if(vtchdf!=nullptr){
				vtchdf->retranslateUi(vtchdf);
				vtchdf->updateTeachersTimetableTable();
				continue;
			}

			TimetableViewTeachersTimeHorizontalForm* vtchtf=qobject_cast<TimetableViewTeachersTimeHorizontalForm*>(wi);
			if(vtchtf!=nullptr){
				vtchtf->retranslateUi(vtchtf);
				vtchtf->updateTeachersTimetableTable();
				continue;
			}

			TimetableViewRoomsDaysHorizontalForm* vrdf=qobject_cast<TimetableViewRoomsDaysHorizontalForm*>(wi);
			if(vrdf!=nullptr){
				vrdf->retranslateUi(vrdf);
				vrdf->updateRoomsTimetableTable();
				continue;
			}

			TimetableViewRoomsTimeHorizontalForm* vrtf=qobject_cast<TimetableViewRoomsTimeHorizontalForm*>(wi);
			if(vrtf!=nullptr){
				vrtf->retranslateUi(vrtf);
				vrtf->updateRoomsTimetableTable();
				continue;
			}

			TimetableShowConflictsForm* scf=qobject_cast<TimetableShowConflictsForm*>(wi);
			if(scf!=nullptr){
				scf->retranslateUi(scf);
				continue;
			}
		}
#endif
}

void SomeQtTranslations()
{
	//This function is never actually used
	//It just contains some commonly used Qt strings, so that some Qt strings of FET are translated.
	QString s1=QCoreApplication::translate("QDialogButtonBox", "&OK", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s1);
	QString s2=QCoreApplication::translate("QDialogButtonBox", "OK");
	Q_UNUSED(s2);
	
	QString s3=QCoreApplication::translate("QDialogButtonBox", "&Cancel", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s3);
	QString s4=QCoreApplication::translate("QDialogButtonBox", "Cancel");
	Q_UNUSED(s4);
	
	QString s5=QCoreApplication::translate("QDialogButtonBox", "&Yes", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s5);
	QString s6=QCoreApplication::translate("QDialogButtonBox", "Yes to &All", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different. Please keep the translation short.");
	Q_UNUSED(s6);
	QString s7=QCoreApplication::translate("QDialogButtonBox", "&No", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s7);
	QString s8=QCoreApplication::translate("QDialogButtonBox", "N&o to All", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different. Please keep the translation short.");
	Q_UNUSED(s8);

	QString s9=QCoreApplication::translate("QDialogButtonBox", "Help");
	Q_UNUSED(s9);

	//It seems that Qt 5 uses other context
	QString s10=QCoreApplication::translate("QPlatformTheme", "&OK", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s10);
	QString s11=QCoreApplication::translate("QPlatformTheme", "OK");
	Q_UNUSED(s11);
	
	QString s12=QCoreApplication::translate("QPlatformTheme", "&Cancel", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s12);
	QString s13=QCoreApplication::translate("QPlatformTheme", "Cancel");
	Q_UNUSED(s13);
	
	QString s14=QCoreApplication::translate("QPlatformTheme", "&Yes", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s14);
	QString s15=QCoreApplication::translate("QPlatformTheme", "Yes to &All", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different. Please keep the translation short.");
	Q_UNUSED(s15);
	QString s16=QCoreApplication::translate("QPlatformTheme", "&No", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s16);
	QString s17=QCoreApplication::translate("QPlatformTheme", "N&o to All", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different. Please keep the translation short.");
	Q_UNUSED(s17);

	QString s18=QCoreApplication::translate("QPlatformTheme", "Help");
	Q_UNUSED(s18);

	QString s19=QCoreApplication::translate("QGnomeTheme", "&OK", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s19);
	QString s20=QCoreApplication::translate("QGnomeTheme", "&Cancel", "Accelerator key (letter after ampersand) for &OK, &Cancel, &Yes, Yes to &All, &No, N&o to All, must be different");
	Q_UNUSED(s20);
	
}

/**
FET starts here
*/
int main(int argc, char **argv)
{
#ifndef FET_COMMAND_LINE
	QApplication qapplication(argc, argv);
#else
	QCoreApplication qCoreApplication(argc, argv);
#endif

	initLanguagesSet();

	VERBOSE=false;

	terminateGeneratePointer=nullptr;
	
	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

#ifndef FET_COMMAND_LINE
	QObject::connect(&qapplication, SIGNAL(lastWindowClosed()), &qapplication, SLOT(quit()));
#endif

	//srand(unsigned(time(nullptr))); //useless, I use randomKnuth(), but just in case I use somewhere rand() by mistake...

	//initRandomKnuth();
	rng.initializeMRG32k3a();

	OUTPUT_DIR=QDir::homePath()+FILE_SEP+"fet-results";
	
	QStringList _args=QCoreApplication::arguments();

#ifndef FET_COMMAND_LINE
	if(_args.count()==1){
		readSimulationParameters();
	
		QDir dir;
	
		bool t=true;

		//make sure that the output directory exists
		if(!dir.exists(OUTPUT_DIR))
			t=dir.mkpath(OUTPUT_DIR);

		if(!t){
			QMessageBox::critical(nullptr, FetTranslate::tr("FET critical"), FetTranslate::tr("Cannot create or use %1 directory (where the results should be stored) - you can continue operation, but you might not be able to work with FET."
			 " Maybe you can try to change the output directory from the 'Settings' menu. If this is a bug - please report it.").arg(QDir::toNativeSeparators(OUTPUT_DIR)));
		}
		
		QString testFileName=OUTPUT_DIR+FILE_SEP+"test_write_permissions_1.tmp";
		QFile test(testFileName);
		bool existedBefore=test.exists();
		bool t_t=test.open(QIODevice::ReadWrite);
		if(!t_t){
			QMessageBox::critical(nullptr, FetTranslate::tr("FET critical"), FetTranslate::tr("You don't have write permissions in the output directory "
			 "(FET cannot open or create file %1) - you might not be able to work correctly with FET. Maybe you can try to change the output directory from the 'Settings' menu."
			 " If this is a bug - please report it.").arg(testFileName));
		}
		else{
			test.close();
			if(!existedBefore)
				test.remove();
		}

		setLanguage(qapplication, nullptr);

		QCoreApplication::setApplicationName(FetTranslate::tr("FET"));

		pqapplication=&qapplication;
		FetMainForm fetMainForm;
		pFetMainForm=&fetMainForm;
		fetMainForm.show();

		int tmp2=qapplication.exec();
	
		writeSimulationParameters();
	
		if(VERBOSE){
			cout<<"Settings saved"<<endl;
		}
	
		pFetMainForm=nullptr;
	
		return tmp2;
	}
	else{
		QMessageBox::warning(nullptr, FetTranslate::tr("FET warning"), FetTranslate::tr("To start FET in interface mode, please do"
		 " not give any command-line parameters to the FET executable"));
		
		return 1;
	}
#else
	/////////////////////////////////////////////////
	//begin command line
	
	if(_args.count()>1){
		bool showHelp=false;
	
		qint64 randomSeedS10=-1;
		qint64 randomSeedS11=-1;
		qint64 randomSeedS12=-1;

		qint64 randomSeedS20=-1;
		qint64 randomSeedS21=-1;
		qint64 randomSeedS22=-1;

		bool randomSeedS10Specified=false;
		bool randomSeedS11Specified=false;
		bool randomSeedS12Specified=false;

		bool randomSeedS20Specified=false;
		bool randomSeedS21Specified=false;
		bool randomSeedS22Specified=false;

		bool randomSeedXSpecified=false;
		bool randomSeedYSpecified=false;
	
		QString outputDirectory="";
	
		INPUT_FILENAME_XML="";
		
		QString filename="";
		
		int secondsLimit=2000000000;
		
		TIMETABLE_HTML_LEVEL=2;
		
		TIMETABLE_HTML_PRINT_ACTIVITY_TAGS=true;

		PRINT_DETAILED_HTML_TIMETABLES=true;

		PRINT_DETAILED_HTML_TEACHERS_FREE_PERIODS=true;

		FET_LANGUAGE="en_US";
		
		PRINT_NOT_AVAILABLE_TIME_SLOTS=true;
		
		PRINT_BREAK_TIME_SLOTS=true;
		
		WRITE_TIMETABLE_CONFLICTS=true;
	
		WRITE_TIMETABLES_STATISTICS=true;
		WRITE_TIMETABLES_XML=true;
		WRITE_TIMETABLES_DAYS_HORIZONTAL=true;
		WRITE_TIMETABLES_DAYS_VERTICAL=true;
		WRITE_TIMETABLES_TIME_HORIZONTAL=true;
		WRITE_TIMETABLES_TIME_VERTICAL=true;

		WRITE_TIMETABLES_SUBGROUPS=true;
		WRITE_TIMETABLES_GROUPS=true;
		WRITE_TIMETABLES_YEARS=true;
		WRITE_TIMETABLES_TEACHERS=true;
		WRITE_TIMETABLES_TEACHERS_FREE_PERIODS=true;
		WRITE_TIMETABLES_ROOMS=true;
		WRITE_TIMETABLES_SUBJECTS=true;
		WRITE_TIMETABLES_ACTIVITY_TAGS=true;
		WRITE_TIMETABLES_ACTIVITIES=true;

		DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=false;
		
		TIMETABLE_HTML_REPEAT_NAMES=false;

		PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=false;
		
		QStringList unrecognizedOptions;
		
		SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=true;
		
		SHOW_WARNING_FOR_SUBGROUPS_WITH_THE_SAME_ACTIVITIES=true;
		
		SHOW_WARNING_FOR_ACTIVITIES_FIXED_SPACE_VIRTUAL_REAL_ROOMS_BUT_NOT_FIXED_TIME=true;

		SHOW_WARNING_FOR_MAX_HOURS_DAILY_WITH_UNDER_100_WEIGHT=true;
		
		SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=true;
		
		SHOW_WARNING_FOR_GROUP_ACTIVITIES_IN_INITIAL_ORDER=true;
		
		SHOW_VIRTUAL_ROOMS_IN_TIMETABLES=false;
		
		EXPORT_CSV=false;
		EXPORT_ALLOW_OVERWRITE=false;
		EXPORT_FIRST_LINE_IS_HEADING=true;
		EXPORT_QUOTES=EXPORT_DOUBLE_QUOTES;
		EXPORT_FIELD_SEPARATOR=EXPORT_COMMA;

		bool showVersion=false;
		
		for(int i=1; i<_args.count(); i++){
			QString s=_args[i];
			
			if(s.left(12)=="--inputfile=")
				filename=QDir::fromNativeSeparators(s.right(s.length()-12));
			else if(s.left(19)=="--timelimitseconds=")
				secondsLimit=s.right(s.length()-19).toInt();
			else if(s.left(21)=="--timetablehtmllevel=")
				TIMETABLE_HTML_LEVEL=s.right(s.length()-21).toInt();
			else if(s.left(12)=="--htmllevel=")
				TIMETABLE_HTML_LEVEL=s.right(s.length()-12).toInt();
			else if(s.left(20)=="--printactivitytags="){
				if(s.right(5)=="false")
					TIMETABLE_HTML_PRINT_ACTIVITY_TAGS=false;
			}
			else if(s.left(26)=="--printdetailedtimetables="){
				if(s.right(5)=="false")
					PRINT_DETAILED_HTML_TIMETABLES=false;
			}
			else if(s.left(45)=="--printdetailedteachersfreeperiodstimetables="){
				if(s.right(5)=="false")
					PRINT_DETAILED_HTML_TEACHERS_FREE_PERIODS=false;
			}
			else if(s.left(11)=="--language=")
				FET_LANGUAGE=s.right(s.length()-11);
			else if(s.left(20)=="--printnotavailable="){
				if(s.right(5)=="false")
					PRINT_NOT_AVAILABLE_TIME_SLOTS=false;
			}
			else if(s.left(13)=="--printbreak="){
				if(s.right(5)=="false")
					PRINT_BREAK_TIME_SLOTS=false;
			}
			else if(s.left(23)=="--dividetimeaxisbydays="){
				if(s.right(4)=="true")
					DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=true;
			}
			else if(s.left(27)=="--duplicateverticalheaders="){
				if(s.right(4)=="true")
					TIMETABLE_HTML_REPEAT_NAMES=true;
			}
			else if(s.left(12)=="--outputdir="){
				outputDirectory=QDir::fromNativeSeparators(s.right(s.length()-12));
			}
			else if(s.left(30)=="--printsimultaneousactivities="){
				if(s.right(4)=="true")
					PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=true;
			}
			//keep this to deny beginning the generation for FET-5.44.0 or later, because it is an obsolete option and we cannot bypass it
			else if(s.left(14)=="--randomseedx="){
				randomSeedXSpecified=true;
				//randomSeedX=s.right(s.length()-14).toInt();
			}
			//keep this to deny beginning the generation for FET-5.44.0 or later, because it is an obsolete option and we cannot bypass it
			else if(s.left(14)=="--randomseedy="){
				randomSeedYSpecified=true;
				//randomSeedY=s.right(s.length()-14).toInt();
			}

			else if(s.left(16)=="--randomseeds10="){
				randomSeedS10Specified=true;
				bool ok;
				randomSeedS10=s.right(s.length()-16).toLongLong(&ok);
				if(!ok)
					randomSeedS10=-1;
			}
			else if(s.left(16)=="--randomseeds11="){
				randomSeedS11Specified=true;
				bool ok;
				randomSeedS11=s.right(s.length()-16).toLongLong(&ok);
				if(!ok)
					randomSeedS11=-1;
			}
			else if(s.left(16)=="--randomseeds12="){
				randomSeedS12Specified=true;
				bool ok;
				randomSeedS12=s.right(s.length()-16).toLongLong(&ok);
				if(!ok)
					randomSeedS12=-1;
			}

			else if(s.left(16)=="--randomseeds20="){
				randomSeedS20Specified=true;
				bool ok;
				randomSeedS20=s.right(s.length()-16).toLongLong(&ok);
				if(!ok)
					randomSeedS20=-1;
			}
			else if(s.left(16)=="--randomseeds21="){
				randomSeedS21Specified=true;
				bool ok;
				randomSeedS21=s.right(s.length()-16).toLongLong(&ok);
				if(!ok)
					randomSeedS21=-1;
			}
			else if(s.left(16)=="--randomseeds22="){
				randomSeedS22Specified=true;
				bool ok;
				randomSeedS22=s.right(s.length()-16).toLongLong(&ok);
				if(!ok)
					randomSeedS22=-1;
			}

			else if(s.left(35)=="--warnifusingnotperfectconstraints="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=false;
			}
			else if(s.left(37)=="--warnsubgroupswiththesameactivities="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_SUBGROUPS_WITH_THE_SAME_ACTIVITIES=false;
			}
			else if(s.left(67)=="--warnifusingactivitiesnotfixedtimefixedspacevirtualroomsrealrooms="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_ACTIVITIES_FIXED_SPACE_VIRTUAL_REAL_ROOMS_BUT_NOT_FIXED_TIME=false;
			}
			else if(s.left(55)=="--warnifusingmaxhoursdailywithlessthan100percentweight="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_MAX_HOURS_DAILY_WITH_UNDER_100_WEIGHT=false;
			}
			else if(s.left(43)=="--warnifusinggroupactivitiesininitialorder="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_GROUP_ACTIVITIES_IN_INITIAL_ORDER=false;
			}
			else if(s.left(53)=="--warnifusingstudentsminhoursdailywithallowemptydays="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=false;
			}
			else if(s.left(19)=="--showvirtualrooms="){
				if(s.right(4)=="true")
					SHOW_VIRTUAL_ROOMS_IN_TIMETABLES=true;
			}
			else if(s.left(10)=="--verbose="){
				if(s.right(4)=="true")
					VERBOSE=true;
			}
			else if(s=="--help"){
				showHelp=true;
			}
			else if(s=="--version"){
				showVersion=true;
			}
			///
			else if(s.left(26)=="--writetimetableconflicts="){
				if(s.right(5)=="false")
					WRITE_TIMETABLE_CONFLICTS=false;
			}
			//
			else if(s.left(28)=="--writetimetablesstatistics="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_STATISTICS=false;
			}
			else if(s.left(21)=="--writetimetablesxml="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_XML=false;
			}
			else if(s.left(32)=="--writetimetablesdayshorizontal="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_DAYS_HORIZONTAL=false;
			}
			else if(s.left(30)=="--writetimetablesdaysvertical="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_DAYS_VERTICAL=false;
			}
			else if(s.left(32)=="--writetimetablestimehorizontal="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_TIME_HORIZONTAL=false;
			}
			else if(s.left(30)=="--writetimetablestimevertical="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_TIME_VERTICAL=false;
			}
			//
			else if(s.left(27)=="--writetimetablessubgroups="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_SUBGROUPS=false;
			}
			else if(s.left(24)=="--writetimetablesgroups="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_GROUPS=false;
			}
			else if(s.left(23)=="--writetimetablesyears="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_YEARS=false;
			}
			else if(s.left(26)=="--writetimetablesteachers="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_TEACHERS=false;
			}
			else if(s.left(37)=="--writetimetablesteachersfreeperiods="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_TEACHERS_FREE_PERIODS=false;
			}
			else if(s.left(23)=="--writetimetablesrooms="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_ROOMS=false;
			}
			else if(s.left(26)=="--writetimetablessubjects="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_SUBJECTS=false;
			}
			else if(s.left(30)=="--writetimetablesactivitytags="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_ACTIVITIES=false;
			}
			else if(s.left(28)=="--writetimetablesactivities="){
				if(s.right(5)=="false")
					WRITE_TIMETABLES_ACTIVITIES=false;
			}
			//Export CSV
			else if(s.left(12)=="--exportcsv="){
				if(s.right(4)=="true")
					EXPORT_CSV=true;
			}
			else if(s.left(15)=="--overwritecsv="){
				if(s.right(4)=="true")
					EXPORT_ALLOW_OVERWRITE=true;
			}
			else if(s.left(24)=="--firstlineisheadingcsv="){
				if(s.right(5)=="false")
					EXPORT_FIRST_LINE_IS_HEADING=false;
			}
			else if(s.left(12)=="--quotescsv="){
				if(s.right(12)=="singlequotes")
					EXPORT_QUOTES=EXPORT_SINGLE_QUOTES;
				else if(s.right(4)=="none")
					EXPORT_QUOTES=EXPORT_NO_QUOTES;
			}
			else if(s.left(20)=="--fieldseparatorcsv="){
				if(s.right(9)=="semicolon")
					EXPORT_FIELD_SEPARATOR=EXPORT_SEMICOLON;
				else if(s.right(11)=="verticalbar")
					EXPORT_FIELD_SEPARATOR=EXPORT_VERTICALBAR;
			}
			///
			else
				unrecognizedOptions.append(s);
		}
		
		if(filename==""){
			if(unrecognizedOptions.count()>0){
				for(const QString& s : qAsConst(unrecognizedOptions)){
					cout<<"Unrecognized option: "<<qPrintable(s)<<endl;
				}
				cout<<endl;
			}

			if(showHelp){
				usage(nullptr, QString(""));
				return 0;
			}
			else if(showVersion){
				cout<<"FET version "<<qPrintable(FET_VERSION)<<endl;
				cout<<"Free timetabling software, licensed under the GNU Affero General Public License version 3 or later"<<endl;
				cout<<"Copyright (C) 2002-2021 Liviu Lalescu, Volker Dirr"<<endl;
				cout<<"Homepage: https://lalescu.ro/liviu/fet/"<<endl;
				cout<<"This program uses Qt version "<<qVersion()<<", Copyright (C) The Qt Company Ltd and other contributors."<<endl;
				cout<<"Depending on the platform and compiler, this program may use libraries from:"<<endl;
				cout<<"  gcc, Copyright (C) Free Software Foundation, Inc."<<endl;
				cout<<"  mingw-w64, Copyright (c) by the mingw-w64 project"<<endl;
				return 0;
			}
			else{
				usage(nullptr, QString("Input file not specified"));
				return 1;
			}
		}
		else if(!QFile::exists(filename)){
			if(unrecognizedOptions.count()>0){
				for(const QString& s : qAsConst(unrecognizedOptions)){
					cout<<"Unrecognized option: "<<qPrintable(s)<<endl;
				}
				cout<<endl;
			}

			cout<<"Error: the specified input file "<<qPrintable(filename)<<" is not existing"<<endl;
			return 1;
		}
		
		INPUT_FILENAME_XML=filename;
		
		QString initialDir=outputDirectory;
		if(initialDir!="")
			initialDir.append(FILE_SEP);
			
		QString csvOutputDirectory=outputDirectory;
		//cout<<"csvOutputDirectory="<<qPrintable(csvOutputDirectory)<<endl;
		
		if(outputDirectory!="")
			outputDirectory.append(FILE_SEP);
		outputDirectory.append("timetables");

		/*if(csvOutputDirectory!="")
			csvOutputDirectory.append(FILE_SEP);
		csvOutputDirectory.append("csv");*/

		//////////
		if(INPUT_FILENAME_XML!=""){
			outputDirectory.append(FILE_SEP);
			outputDirectory.append(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.lastIndexOf(FILE_SEP)-1));
			if(outputDirectory.right(4)==".fet")
				outputDirectory=outputDirectory.left(outputDirectory.length()-4);

			/*csvOutputDirectory.append(FILE_SEP);
			csvOutputDirectory.append(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.lastIndexOf(FILE_SEP)-1));
			if(csvOutputDirectory.right(4)==".fet")
				csvOutputDirectory=csvOutputDirectory.left(csvOutputDirectory.length()-4);*/
		}
		//////////
		
		QDir dir;
		QString logsDir=initialDir+"logs";
		if(!dir.exists(logsDir))
			dir.mkpath(logsDir);
		logsDir.append(FILE_SEP);
		
		////////
		QFile logFile(logsDir+"result.txt");
		bool tttt=logFile.open(QIODevice::WriteOnly);
		if(!tttt){
			cout<<"FET critical - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(logsDir)<<"result.txt)."
			 " If this is a bug - please report it."<<endl;
			return 1;
		}
		QTextStream out(&logFile);
		///////
		
		if(unrecognizedOptions.count()>0){
			for(const QString& s : qAsConst(unrecognizedOptions)){
				cout<<"Unrecognized option: "<<qPrintable(s)<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
				out<<"Unrecognized option: "<<qPrintable(s)<<Qt::endl;
#else
				out<<"Unrecognized option: "<<qPrintable(s)<<endl;
#endif
			}
			cout<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<Qt::endl;
#else
			out<<endl;
#endif
		}
		
		//Cleanup the previous unsuccessful generation, if any. No need to remove the other files, they are overwritten.
		QFile oldDifficultActivitiesFile(logsDir+"difficult_activities.txt");
		if(oldDifficultActivitiesFile.exists()){
			bool t=oldDifficultActivitiesFile.remove();
			if(!t){
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
				out<<"Cannot remove the old existing file "<<qPrintable(logsDir)<<"difficult_activities.txt"<<Qt::endl;
#else
				out<<"Cannot remove the old existing file "<<qPrintable(logsDir)<<"difficult_activities.txt"<<endl;
#endif
				cout<<"Cannot remove the old existing file "<<qPrintable(logsDir)<<"difficult_activities.txt"<<endl;
			}
		}
		
		setLanguage(qCoreApplication, nullptr);
		
		QCoreApplication::setApplicationName(FetTranslate::tr("FET-CL"));
		
		QFile maxPlacedActivityFile(logsDir+"max_placed_activities.txt");
		maxPlacedActivityFile.open(QIODevice::WriteOnly);
		QTextStream maxPlacedActivityStream(&maxPlacedActivityFile);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		maxPlacedActivityStream.setEncoding(QStringConverter::Utf8);
#else
		maxPlacedActivityStream.setCodec("UTF-8");
#endif
		maxPlacedActivityStream.setGenerateByteOrderMark(true);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		maxPlacedActivityStream<<FetTranslate::tr("This is the list of max placed activities, chronologically. If FET could reach maximum n-th activity, look at the n+1-st activity"
			" in the initial order of the activities")<<Qt::endl<<Qt::endl;
#else
		maxPlacedActivityStream<<FetTranslate::tr("This is the list of max placed activities, chronologically. If FET could reach maximum n-th activity, look at the n+1-st activity"
			" in the initial order of the activities")<<endl<<endl;
#endif
		
		QFile initialOrderFile(logsDir+"initial_order.txt");
		initialOrderFile.open(QIODevice::WriteOnly);
		QTextStream initialOrderStream(&initialOrderFile);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		initialOrderStream.setEncoding(QStringConverter::Utf8);
#else
		initialOrderStream.setCodec("UTF-8");
#endif
		initialOrderStream.setGenerateByteOrderMark(true);
		
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		out<<"This file contains the result (log) of last operation"<<Qt::endl<<Qt::endl;
#else
		out<<"This file contains the result (log) of last operation"<<endl<<endl;
#endif
		
		QDate dat=QDate::currentDate();
		QTime tim=QTime::currentTime();
		QLocale loc(FET_LANGUAGE);
		QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		out<<"FET command line simulation started on "<<qPrintable(sTime)<<Qt::endl<<Qt::endl;
#else
		out<<"FET command line simulation started on "<<qPrintable(sTime)<<endl<<endl;
#endif
		
		if(outputDirectory!="")
			if(!dir.exists(outputDirectory))
				dir.mkpath(outputDirectory);
		
		if(outputDirectory!="")
			outputDirectory.append(FILE_SEP);
			
		QFile test(outputDirectory+"test_write_permissions_2.tmp");
		bool existedBefore=test.exists();
		bool t_t=test.open(QIODevice::ReadWrite);
		if(!t_t){
			cout<<"fet: critical error - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(outputDirectory)<<"test_write_permissions_2.tmp)."
			 " If this is a bug - please report it."<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"fet: critical error - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(outputDirectory)<<"test_write_permissions_2.tmp)."
			 " If this is a bug - please report it."<<Qt::endl;
#else
			out<<"fet: critical error - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(outputDirectory)<<"test_write_permissions_2.tmp)."
			 " If this is a bug - please report it."<<endl;
#endif
			return 1;
		}
		else{
			test.close();
			if(!existedBefore)
				test.remove();
		}

//		if(filename==""){
//			usage(/*&out*/nullptr, QString("Input file not specified"));
//			logFile.close();
//			return 1;
//		}
		if(secondsLimit==0){
			usage(nullptr, QString("Time limit is 0 seconds"));
			logFile.close();
			return 1;
		}
		if(TIMETABLE_HTML_LEVEL>7 || TIMETABLE_HTML_LEVEL<0){
			usage(nullptr, QString("The html level must be 0, 1, 2, 3, 4, 5, 6, or 7"));
			logFile.close();
			return 1;
		}
		if(randomSeedXSpecified || randomSeedYSpecified){
			usage(nullptr, QString("Starting with FET version 5.44.0 the random number generator was changed to a better one. Please see usage for instructions"
			 " on how to specify the random number generator seed at the start of the program (or do not specify a random seed at all)."
			 " The program will now abort the generation"));
			logFile.close();
			return 1;
		}
		if(randomSeedS10Specified && randomSeedS11Specified && randomSeedS12Specified
		 && randomSeedS20Specified && randomSeedS21Specified && randomSeedS22Specified){
			if(randomSeedS10<0 || randomSeedS10>=rng.m1){
				usage(nullptr, QString("The random seed s10 component must be an integer number at least %1 and at most %2").arg(0).arg(rng.m1-1));
				logFile.close();
				return 1;
			}
			if(randomSeedS11<0 || randomSeedS11>=rng.m1){
				usage(nullptr, QString("The random seed s11 component must be an integer number at least %1 and at most %2").arg(0).arg(rng.m1-1));
				logFile.close();
				return 1;
			}
			if(randomSeedS12<0 || randomSeedS12>=rng.m1){
				usage(nullptr, QString("The random seed s12 component must be an integer number at least %1 and at most %2").arg(0).arg(rng.m1-1));
				logFile.close();
				return 1;
			}

			if(randomSeedS20<0 || randomSeedS20>=rng.m1){
				usage(nullptr, QString("The random seed s20 component must be an integer number at least %1 and at most %2").arg(0).arg(rng.m1-1));
				logFile.close();
				return 1;
			}
			if(randomSeedS21<0 || randomSeedS21>=rng.m1){
				usage(nullptr, QString("The random seed s21 component must be an integer number at least %1 and at most %2").arg(0).arg(rng.m1-1));
				logFile.close();
				return 1;
			}
			if(randomSeedS22<0 || randomSeedS22>=rng.m1){
				usage(nullptr, QString("The random seed s22 component must be an integer number at least %1 and at most %2").arg(0).arg(rng.m1-1));
				logFile.close();
				return 1;
			}
			
			if(randomSeedS10==0 && randomSeedS11==0 && randomSeedS12==0){
				usage(nullptr, QString("The random seed numbers for component 1: s10, s11, and s12, must not all be zero"));
				logFile.close();
				return 1;
			}

			if(randomSeedS20==0 && randomSeedS21==0 && randomSeedS22==0){
				usage(nullptr, QString("The random seeds numbers for component 2: s20, s21, and s22, must not all be zero"));
				logFile.close();
				return 1;
			}

			rng.initializeMRG32k3a(randomSeedS10, randomSeedS11, randomSeedS12,
			 randomSeedS20, randomSeedS21, randomSeedS22);
		}
		else if(randomSeedS10Specified || randomSeedS11Specified || randomSeedS12Specified
		 || randomSeedS20Specified || randomSeedS21Specified || randomSeedS22Specified){
			QStringList specified, notSpecified;

			if(randomSeedS10Specified)
				specified.append("s10");
			else
				notSpecified.append("s10");
				
			if(randomSeedS11Specified)
				specified.append("s11");
			else
				notSpecified.append("s11");
				
			if(randomSeedS12Specified)
				specified.append("s12");
			else
				notSpecified.append("s12");

			if(randomSeedS20Specified)
				specified.append("s20");
			else
				notSpecified.append("s20");
				
			if(randomSeedS21Specified)
				specified.append("s21");
			else
				notSpecified.append("s21");
				
			if(randomSeedS22Specified)
				specified.append("s22");
			else
				notSpecified.append("s22");
				
			usage(nullptr, QString("If you want to specify the random seed, you need to specify all the 6 components. You specified %1, but you did not"
			 " specify %2.").arg(specified.join(", ")).arg(notSpecified.join(", ")));
			logFile.close();
			return 1;
		}
		
		/*if(randomSeedXSpecified != randomSeedYSpecified){
			if(randomSeedXSpecified){
				usage(nullptr, QString("If you want to specify the random seed, you need to specify both the X and the Y components, not only the X component"));
			}
			else{
				assert(randomSeedYSpecified);
				usage(nullptr, QString("If you want to specify the random seed, you need to specify both the X and the Y components, not only the Y component"));
			}
			logFile.close();
			return 1;
		}
		assert(randomSeedXSpecified==randomSeedYSpecified);
		if(randomSeedXSpecified){
			if(randomSeedX<=0 || randomSeedX>=MM){
				usage(nullptr, QString("Random seed X component must be at least 1 and at most %1").arg(MM-1));
				logFile.close();
				return 1;
			}
		}
		if(randomSeedYSpecified){
			if(randomSeedY<=0 || randomSeedY>=MMM){
				usage(nullptr, QString("Random seed Y component must be at least 1 and at most %1").arg(MMM-1));
				logFile.close();
				return 1;
			}
		}
		
		if(randomSeedXSpecified){
			assert(randomSeedYSpecified);
			if(randomSeedX>0 && randomSeedX<MM && randomSeedY>0 && randomSeedY<MMM){
				XX=randomSeedX;
				YY=randomSeedY;
			}
		}*/
		
		if(TIMETABLE_HTML_LEVEL>7 || TIMETABLE_HTML_LEVEL<0)
			TIMETABLE_HTML_LEVEL=2;
	
		bool t=gt.rules.read(nullptr, filename, true, initialDir);
		if(!t){
			cout<<"fet-cl: cannot read input file (not existing, in use, or incorrect file) - aborting"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"Cannot read input file (not existing, in use, or incorrect file) - aborting"<<Qt::endl;
#else
			out<<"Cannot read input file (not existing, in use, or incorrect file) - aborting"<<endl;
#endif
			logFile.close();
			return 1;
		}
		
		//2019-09-21
		int count=0;
		for(int i=0; i<gt.rules.activitiesList.size(); i++){
			Activity* act=gt.rules.activitiesList[i];
			if(act->active)
				count++;
		}
		if(count<1){
			cout<<"Please input at least one active activity before generating"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"Please input at least one active activity before generating"<<Qt::endl;
#else
			out<<"Please input at least one active activity before generating"<<endl;
#endif
			logFile.close();
			return 1;
		}
		
		t=gt.rules.computeInternalStructure(nullptr);
		if(!t){
			cout<<"Cannot compute internal structure - aborting"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"Cannot compute internal structure - aborting"<<Qt::endl;
#else
			out<<"Cannot compute internal structure - aborting"<<endl;
#endif
			logFile.close();
			return 1;
		}
	
		Generate gen;

		terminateGeneratePointer=&gen;
		signal(SIGTERM, terminate);
#ifdef SIGBREAK
		signal(SIGBREAK, terminate);
#endif
	
		gen.abortOptimization=false;
		bool ok=gen.precompute(nullptr, &initialOrderStream);
		
		initialOrderFile.close();
		
		if(!ok){
			cout<<"Cannot precompute - data is wrong - aborting"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"Cannot precompute - data is wrong - aborting"<<Qt::endl;
#else
			out<<"Cannot precompute - data is wrong - aborting"<<endl;
#endif
			logFile.close();
			return 1;
		}
	
		bool impossible, timeExceeded;
		
		cout<<"Starting timetable generation..."<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		out<<"Starting timetable generation..."<<Qt::endl;
#else
		out<<"Starting timetable generation..."<<endl;
#endif
		if(VERBOSE){
			cout<<"secondsLimit=="<<secondsLimit<<endl;
		}
		//out<<"secondsLimit=="<<secondsLimit<<endl;
				
		TimetableExport::writeRandomSeedCommandLine(nullptr, outputDirectory, true); //true represents 'before' state

		gen.generate(secondsLimit, impossible, timeExceeded, false, &maxPlacedActivityStream); //false means no thread
		
		maxPlacedActivityFile.close();
	
		if(impossible){
			cout<<"Impossible"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"Impossible"<<Qt::endl;
#else
			out<<"Impossible"<<endl;
#endif
			
			//2016-11-17 - suggested by thanhnambkhn, FET will write the impossible activity and the current and highest-stage timetables
			//(which should be identical)

			Solution& cc=gen.c;

			//needed to find the conflicts strings
			FakeString tmp;
			cc.fitness(gt.rules, &tmp);

			TimetableExport::getStudentsTimetable(cc);
			TimetableExport::getTeachersTimetable(cc);
			TimetableExport::getRoomsTimetable(cc);

			QString toc=outputDirectory;
			if(toc!="" && toc.count()>=1 && toc.endsWith(FILE_SEP)){
				toc.chop(1);
				toc+=QString("-current"+FILE_SEP);
			}
			else if(toc==""){
				toc=QString("current"+FILE_SEP);
			}
			
			if(toc!="")
				if(!dir.exists(toc))
					dir.mkpath(toc);

			TimetableExport::writeSimulationResultsCommandLine(nullptr, toc);
			
			QString s;

			s+=TimetableExport::tr("Please check the constraints related to the "
			 "activity below, which might be impossible to schedule:");
			s+="\n\n";
			for(int i=0; i<gen.nDifficultActivities; i++){
				int ai=gen.difficultActivities[i];

				s+=TimetableExport::tr("No: %1").arg(i+1);

				s+=", ";

				s+=TimetableExport::tr("Id: %1 (%2)", "%1 is id of activity, %2 is detailed description of activity")
					.arg(gt.rules.internalActivitiesList[ai].id)
					.arg(getActivityDetailedDescription(gt.rules, gt.rules.internalActivitiesList[ai].id));

				s+="\n";
			}

			QFile difficultActivitiesFile(logsDir+"difficult_activities.txt");
			bool t=difficultActivitiesFile.open(QIODevice::WriteOnly);
			if(!t){
				cout<<"FET critical - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(logsDir)<<"difficult_activities.txt)."
				 " If this is a bug - please report it."<<endl;
				return 1;
			}
			QTextStream difficultActivitiesOut(&difficultActivitiesFile);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			difficultActivitiesOut.setEncoding(QStringConverter::Utf8);
#else
			difficultActivitiesOut.setCodec("UTF-8");
#endif
			difficultActivitiesOut.setGenerateByteOrderMark(true);
			
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			difficultActivitiesOut<<s<<Qt::endl;
#else
			difficultActivitiesOut<<s<<endl;
#endif
			
			//2011-11-11 (2)
			//write highest stage timetable
			Solution& ch=highestStageSolution;

			//needed to find the conflicts strings
			FakeString tmp2;
			ch.fitness(gt.rules, &tmp2);

			TimetableExport::getStudentsTimetable(ch);
			TimetableExport::getTeachersTimetable(ch);
			TimetableExport::getRoomsTimetable(ch);

			QString toh=outputDirectory;
			if(toh!="" && toh.count()>=1 && toh.endsWith(FILE_SEP)){
				toh.chop(1);
				toh+=QString("-highest"+FILE_SEP);
			}
			else if(toh==""){
				toh=QString("highest"+FILE_SEP);
			}
			
			if(toh!="")
				if(!dir.exists(toh))
					dir.mkpath(toh);

			TimetableExport::writeSimulationResultsCommandLine(nullptr, toh);

			QString oldDir=OUTPUT_DIR;
			OUTPUT_DIR=csvOutputDirectory;
			Export::exportCSV(&highestStageSolution, &gen.c);
			OUTPUT_DIR=oldDir;
		}
		//2012-01-24 - suggestion and code by Ian Holden (ian@ianholden.com), to write best and current timetable on time exceeded
		//previously, FET saved best and current timetable only on receiving SIGTERM
		//by Ian Holden (begin)
		else if(timeExceeded || gen.abortOptimization){
			if(timeExceeded){
				cout<<"Time exceeded"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
				out<<"Time exceeded"<<Qt::endl;
#else
				out<<"Time exceeded"<<endl;
#endif
			}
			else if(gen.abortOptimization){
				cout<<"Simulation stopped"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
				out<<"Simulation stopped"<<Qt::endl;
#else
				out<<"Simulation stopped"<<endl;
#endif
			}
			//by Ian Holden (end)
			
			//2011-11-11 (1)
			//write current stage timetable
			Solution& cc=gen.c;

			//needed to find the conflicts strings
			FakeString tmp;
			cc.fitness(gt.rules, &tmp);

			TimetableExport::getStudentsTimetable(cc);
			TimetableExport::getTeachersTimetable(cc);
			TimetableExport::getRoomsTimetable(cc);

			QString toc=outputDirectory;
			if(toc!="" && toc.count()>=1 && toc.endsWith(FILE_SEP)){
				toc.chop(1);
				toc+=QString("-current"+FILE_SEP);
			}
			else if(toc==""){
				toc=QString("current"+FILE_SEP);
			}
			
			if(toc!="")
				if(!dir.exists(toc))
					dir.mkpath(toc);

			TimetableExport::writeSimulationResultsCommandLine(nullptr, toc);
			
			QString s;

			if(maxActivitiesPlaced>=0 && maxActivitiesPlaced<gt.rules.nInternalActivities
			 && initialOrderOfActivitiesIndices[maxActivitiesPlaced]>=0 && initialOrderOfActivitiesIndices[maxActivitiesPlaced]<gt.rules.nInternalActivities){
				s=FetTranslate::tr("FET managed to schedule correctly the first %1 most difficult activities."
				 " You can see initial order of placing the activities in the corresponding output file. The activity which might cause problems"
				 " might be the next activity in the initial order of evaluation. This activity is listed below:").arg(maxActivitiesPlaced);
				s+=QString("\n\n");
			
				int ai=initialOrderOfActivitiesIndices[maxActivitiesPlaced];

				s+=FetTranslate::tr("Id: %1 (%2)", "%1 is id of activity, %2 is detailed description of activity")
				 .arg(gt.rules.internalActivitiesList[ai].id)
				 .arg(getActivityDetailedDescription(gt.rules, gt.rules.internalActivitiesList[ai].id));
			}
			else
				s=FetTranslate::tr("Difficult activity cannot be computed - please report possible bug");
			
			s+=QString("\n\n----------\n\n");
			
			s+=FetTranslate::tr("Here are the placed activities which lead to an inconsistency, "
			 "in order from the first one to the last (the last one FET failed to schedule "
			 "and the last ones are most likely impossible):");
			s+="\n\n";
			for(int i=0; i<gen.nDifficultActivities; i++){
				int ai=gen.difficultActivities[i];

				s+=FetTranslate::tr("No: %1").arg(i+1);
		
				s+=", ";

				s+=FetTranslate::tr("Id: %1 (%2)", "%1 is id of activity, %2 is detailed description of activity")
					.arg(gt.rules.internalActivitiesList[ai].id)
					.arg(getActivityDetailedDescription(gt.rules, gt.rules.internalActivitiesList[ai].id));

				s+="\n";
			}
			
			QFile difficultActivitiesFile(logsDir+"difficult_activities.txt");
			bool t=difficultActivitiesFile.open(QIODevice::WriteOnly);
			if(!t){
				cout<<"FET critical - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(logsDir)<<"difficult_activities.txt)."
				 " If this is a bug - please report it."<<endl;
				return 1;
			}
			QTextStream difficultActivitiesOut(&difficultActivitiesFile);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			difficultActivitiesOut.setEncoding(QStringConverter::Utf8);
#else
			difficultActivitiesOut.setCodec("UTF-8");
#endif
			difficultActivitiesOut.setGenerateByteOrderMark(true);
			
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			difficultActivitiesOut<<s<<Qt::endl;
#else
			difficultActivitiesOut<<s<<endl;
#endif
			
			//2011-11-11 (2)
			//write highest stage timetable
			Solution& ch=highestStageSolution;

			//needed to find the conflicts strings
			FakeString tmp2;
			ch.fitness(gt.rules, &tmp2);

			TimetableExport::getStudentsTimetable(ch);
			TimetableExport::getTeachersTimetable(ch);
			TimetableExport::getRoomsTimetable(ch);

			QString toh=outputDirectory;
			if(toh!="" && toh.count()>=1 && toh.endsWith(FILE_SEP)){
				toh.chop(1);
				toh+=QString("-highest"+FILE_SEP);
			}
			else if(toh==""){
				toh=QString("highest"+FILE_SEP);
			}
			
			if(toh!="")
				if(!dir.exists(toh))
					dir.mkpath(toh);

			TimetableExport::writeSimulationResultsCommandLine(nullptr, toh);

			QString oldDir=OUTPUT_DIR;
			OUTPUT_DIR=csvOutputDirectory;
			Export::exportCSV(&highestStageSolution, &gen.c);
			OUTPUT_DIR=oldDir;
		}
		else{
			cout<<"Simulation successful"<<endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
			out<<"Simulation successful"<<Qt::endl;
#else
			out<<"Simulation successful"<<endl;
#endif
		
			TimetableExport::writeRandomSeedCommandLine(nullptr, outputDirectory, false); //false represents 'before' state

			Solution& c=gen.c;

			//needed to find the conflicts strings
			FakeString tmp;
			c.fitness(gt.rules, &tmp);
			
			TimetableExport::getStudentsTimetable(c);
			TimetableExport::getTeachersTimetable(c);
			TimetableExport::getRoomsTimetable(c);

			TimetableExport::writeSimulationResultsCommandLine(nullptr, outputDirectory);
			
			QString oldDir=OUTPUT_DIR;
			OUTPUT_DIR=csvOutputDirectory;
			Export::exportCSV(&c);
			OUTPUT_DIR=oldDir;
		}
	
		logFile.close();
		return 0;
	}
	else{
		usage(nullptr, QString("No arguments given"));
		return 1;
	}
	//end command line
#endif
	/////////////////////////////////////////////////
}
