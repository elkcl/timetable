/***************************************************************************
                          timetableviewstudentsdayshorizontalform.cpp  -  description
                             -------------------
    begin                : Tue Apr 22 2003
    copyright            : (C) 2003 by Lalescu Liviu
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

#include <Qt>

#include <QtGlobal>

#include "longtextmessagebox.h"

#include "fetmainform.h"
#include "timetableviewstudentsdayshorizontalform.h"
#include "timetable_defs.h"
#include "timetable.h"
#include "fet.h"
#include "solution.h"

#include "matrix.h"

#include "lockunlock.h"

#include <QList>
#include <QSet>

#include <QMessageBox>

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

#include <QAbstractItemView>

#include <QCoreApplication>
#include <QApplication>

#include <QString>
#include <QStringList>

#include <QSplitter>
#include <QSettings>
#include <QObject>
#include <QMetaObject>

//begin by Marco Vassura
#include <QBrush>
#include <QColor>
//end by Marco Vassura

extern const QString COMPANY;
extern const QString PROGRAM;

extern bool students_schedule_ready;
extern bool teachers_schedule_ready;

extern bool simulation_running;

extern Solution best_solution;

extern Matrix3D<bool> subgroupNotAvailableDayHour;
extern Matrix2D<bool> breakDayHour;

extern QSet<int> idsOfLockedTime;		//care about locked activities in view forms
extern QSet<int> idsOfLockedSpace;		//care about locked activities in view forms
extern QSet<int> idsOfPermanentlyLockedTime;	//care about locked activities in view forms
extern QSet<int> idsOfPermanentlyLockedSpace;	//care about locked activities in view forms

extern CommunicationSpinBox communicationSpinBox;	//small hint to sync the forms

TimetableViewStudentsDaysHorizontalForm::TimetableViewStudentsDaysHorizontalForm(QWidget* parent): QDialog(parent)
{
	setupUi(this);
	
	closePushButton->setDefault(true);
	
	detailsTextEdit->setReadOnly(true);

	//columnResizeModeInitialized=false;

	verticalStudentsTableDetailsSplitter->setStretchFactor(0, 4);
	verticalStudentsTableDetailsSplitter->setStretchFactor(1, 1);
	horizontalSplitter->setStretchFactor(0, 3);
	horizontalSplitter->setStretchFactor(1, 10);
	
	studentsTimetableTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	
	yearsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	groupsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	subgroupsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	groupsListWidget->clear();
	subgroupsListWidget->clear();
	
	//These connect-s should be lower in the code
	//connect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
	//connect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
	//connect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
	connect(closePushButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(studentsTimetableTable, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)), this, SLOT(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)));
	connect(lockTimePushButton, SIGNAL(clicked()), this, SLOT(lockTime()));
	connect(lockSpacePushButton, SIGNAL(clicked()), this, SLOT(lockSpace()));
	connect(lockTimeSpacePushButton, SIGNAL(clicked()), this, SLOT(lockTimeSpace()));
	connect(helpPushButton, SIGNAL(clicked()), this, SLOT(help()));

	centerWidgetOnScreen(this);
	restoreFETDialogGeometry(this);

	//restore vertical students list splitter state
	QSettings settings(COMPANY, PROGRAM);
	if(settings.contains(this->metaObject()->className()+QString("/vertical-students-list-splitter-state")))
		verticalStudentsListSplitter->restoreState(settings.value(this->metaObject()->className()+QString("/vertical-students-list-splitter-state")).toByteArray());

	//restore vertical students table details splitter state
	//QSettings settings(COMPANY, PROGRAM);
	if(settings.contains(this->metaObject()->className()+QString("/vertical-students-table-details-splitter-state")))
		verticalStudentsTableDetailsSplitter->restoreState(settings.value(this->metaObject()->className()+QString("/vertical-students-table-details-splitter-state")).toByteArray());

	//restore horizontal splitter state
	//QSettings settings(COMPANY, PROGRAM);
	if(settings.contains(this->metaObject()->className()+QString("/horizontal-splitter-state")))
		horizontalSplitter->restoreState(settings.value(this->metaObject()->className()+QString("/horizontal-splitter-state")).toByteArray());

//////////just for testing
	QSet<int> backupLockedTime;
	QSet<int> backupPermanentlyLockedTime;
	QSet<int> backupLockedSpace;
	QSet<int> backupPermanentlyLockedSpace;

	backupLockedTime=idsOfLockedTime;
	backupPermanentlyLockedTime=idsOfPermanentlyLockedTime;
	backupLockedSpace=idsOfLockedSpace;
	backupPermanentlyLockedSpace=idsOfPermanentlyLockedSpace;

	//added by Volker Dirr
	//these lines are not really needed - just to be safer
	LockUnlock::computeLockedUnlockedActivitiesTimeSpace();
	
	assert(backupLockedTime==idsOfLockedTime);
	assert(backupPermanentlyLockedTime==idsOfPermanentlyLockedTime);
	assert(backupLockedSpace==idsOfLockedSpace);
	assert(backupPermanentlyLockedSpace==idsOfPermanentlyLockedSpace);
//////////

	//Commented on 2018-07-20
	//LockUnlock::increaseCommunicationSpinBox();

	studentsTimetableTable->setRowCount(gt.rules.nHoursPerDay);
	studentsTimetableTable->setColumnCount(gt.rules.nDaysPerWeek);
	for(int j=0; j<gt.rules.nDaysPerWeek; j++){
		QTableWidgetItem* item=new QTableWidgetItem(gt.rules.daysOfTheWeek[j]);
		studentsTimetableTable->setHorizontalHeaderItem(j, item);
	}
	for(int i=0; i<gt.rules.nHoursPerDay; i++){
		QTableWidgetItem* item=new QTableWidgetItem(gt.rules.hoursOfTheDay[i]);
		studentsTimetableTable->setVerticalHeaderItem(i, item);
	}
	for(int j=0; j<gt.rules.nHoursPerDay; j++){
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			QTableWidgetItem* item= new QTableWidgetItem();
			item->setTextAlignment(Qt::AlignCenter);
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

			studentsTimetableTable->setItem(j, k, item);

			//if(j==0 && k==0)
			//	teachersTimetableTable->setCurrentItem(item);
		}
	}
	
	//resize columns
	//if(!columnResizeModeInitialized){
	studentsTimetableTable->horizontalHeader()->setMinimumSectionSize(studentsTimetableTable->horizontalHeader()->defaultSectionSize());
	//	columnResizeModeInitialized=true;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	studentsTimetableTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
	studentsTimetableTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
	//}
	///////////////
	
	//2019-12-11: so that the years' and groups' list widgets are not void, even if selecting to view by groups or subgroups.
	yearsListWidget->clear();
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		yearsListWidget->addItem(sty->name);
	}
	if(yearsListWidget->count()>0){
		yearsListWidget->setCurrentRow(0);
		
		groupsListWidget->clear();
		for(StudentsGroup* stg : qAsConst(gt.rules.augmentedYearsList[0]->groupsList))
			groupsListWidget->addItem(stg->name);
		if(groupsListWidget->count()>0){
			groupsListWidget->setCurrentRow(0);
			
			subgroupsListWidget->clear();
			for(StudentsSubgroup* sts : qAsConst(gt.rules.augmentedYearsList[0]->groupsList.at(0)->subgroupsList))
				subgroupsListWidget->addItem(sts->name);
			if(subgroupsListWidget->count()>0)
				subgroupsListWidget->setCurrentRow(0);
		}
	}
	connect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
	connect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
	connect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
	
	shownComboBox->addItem(tr("Years"));
	shownComboBox->addItem(tr("Groups"));
	shownComboBox->addItem(tr("Subgroups"));

	shownComboBox->setCurrentIndex(-1);

	if(settings.contains(this->metaObject()->className()+QString("/shown-categories")))
		shownComboBox->setCurrentIndex(settings.value(this->metaObject()->className()+QString("/shown-categories")).toInt());
	else
		shownComboBox->setCurrentIndex(0);

	connect(shownComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(shownComboBoxChanged()));

	//added by Volker Dirr
	connect(&communicationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStudentsTimetableTable()));

	shownComboBoxChanged();
}

void TimetableViewStudentsDaysHorizontalForm::newTimetableGenerated()
{
	/*setupUi(this);
	
	closePushButton->setDefault(true);
	
	detailsTextEdit->setReadOnly(true);

	//columnResizeModeInitialized=false;

	verticalStudentsTableDetailsSplitter->setStretchFactor(0, 4);
	verticalStudentsTableDetailsSplitter->setStretchFactor(1, 1);
	horizontalSplitter->setStretchFactor(0, 3);
	horizontalSplitter->setStretchFactor(1, 10);
	
	studentsTimetableTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	
	yearsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	groupsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	subgroupsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);*/

	disconnect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
	disconnect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
	groupsListWidget->clear();
	subgroupsListWidget->clear();
	connect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
	connect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
	
	//This connect should be lower in the code
	//connect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
	/*connect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
	connect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
	connect(closePushButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(studentsTimetableTable, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)), this, SLOT(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)));
	connect(lockTimePushButton, SIGNAL(clicked()), this, SLOT(lockTime()));
	connect(lockSpacePushButton, SIGNAL(clicked()), this, SLOT(lockSpace()));
	connect(lockTimeSpacePushButton, SIGNAL(clicked()), this, SLOT(lockTimeSpace()));
	connect(helpPushButton, SIGNAL(clicked()), this, SLOT(help()));

	centerWidgetOnScreen(this);
	restoreFETDialogGeometry(this);

	//restore vertical students list splitter state
	QSettings settings(COMPANY, PROGRAM);
	if(settings.contains(this->metaObject()->className()+QString("/vertical-students-list-splitter-state")))
		verticalStudentsListSplitter->restoreState(settings.value(this->metaObject()->className()+QString("/vertical-students-list-splitter-state")).toByteArray());

	//restore vertical students table details splitter state
	//QSettings settings(COMPANY, PROGRAM);
	if(settings.contains(this->metaObject()->className()+QString("/vertical-students-table-details-splitter-state")))
		verticalStudentsTableDetailsSplitter->restoreState(settings.value(this->metaObject()->className()+QString("/vertical-students-table-details-splitter-state")).toByteArray());

	//restore horizontal splitter state
	//QSettings settings(COMPANY, PROGRAM);
	if(settings.contains(this->metaObject()->className()+QString("/horizontal-splitter-state")))
		horizontalSplitter->restoreState(settings.value(this->metaObject()->className()+QString("/horizontal-splitter-state")).toByteArray());*/

//////////just for testing
	QSet<int> backupLockedTime;
	QSet<int> backupPermanentlyLockedTime;
	QSet<int> backupLockedSpace;
	QSet<int> backupPermanentlyLockedSpace;

	backupLockedTime=idsOfLockedTime;
	backupPermanentlyLockedTime=idsOfPermanentlyLockedTime;
	backupLockedSpace=idsOfLockedSpace;
	backupPermanentlyLockedSpace=idsOfPermanentlyLockedSpace;

	//added by Volker Dirr
	//these lines are not really needed - just to be safer
	LockUnlock::computeLockedUnlockedActivitiesTimeSpace();
	
	assert(backupLockedTime==idsOfLockedTime);
	assert(backupPermanentlyLockedTime==idsOfPermanentlyLockedTime);
	assert(backupLockedSpace==idsOfLockedSpace);
	assert(backupPermanentlyLockedSpace==idsOfPermanentlyLockedSpace);
//////////

	//DON'T UNCOMMENT THIS CODE -> LEADS TO CRASH IF THERE ARE MORE VIEWS OPENED.
	//LockUnlock::increaseCommunicationSpinBox();

	studentsTimetableTable->clear();
	studentsTimetableTable->setRowCount(gt.rules.nHoursPerDay);
	studentsTimetableTable->setColumnCount(gt.rules.nDaysPerWeek);
	for(int j=0; j<gt.rules.nDaysPerWeek; j++){
		QTableWidgetItem* item=new QTableWidgetItem(gt.rules.daysOfTheWeek[j]);
		studentsTimetableTable->setHorizontalHeaderItem(j, item);
	}
	for(int i=0; i<gt.rules.nHoursPerDay; i++){
		QTableWidgetItem* item=new QTableWidgetItem(gt.rules.hoursOfTheDay[i]);
		studentsTimetableTable->setVerticalHeaderItem(i, item);
	}
	for(int j=0; j<gt.rules.nHoursPerDay; j++){
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			QTableWidgetItem* item= new QTableWidgetItem();
			item->setTextAlignment(Qt::AlignCenter);
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

			studentsTimetableTable->setItem(j, k, item);

			//if(j==0 && k==0)
			//	teachersTimetableTable->setCurrentItem(item);
		}
	}
	
	//resize columns
	//if(!columnResizeModeInitialized){
/*	studentsTimetableTable->horizontalHeader()->setMinimumSectionSize(studentsTimetableTable->horizontalHeader()->defaultSectionSize());
	//	columnResizeModeInitialized=true;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	studentsTimetableTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
	studentsTimetableTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
	//}
	///////////////
*/
	
	disconnect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
	yearsListWidget->clear();
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		yearsListWidget->addItem(sty->name);
	}
	if(yearsListWidget->count()>0)
		yearsListWidget->setCurrentRow(0);
	connect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
		
/*	shownComboBox->addItem(tr("Years"));
	shownComboBox->addItem(tr("Groups"));
	shownComboBox->addItem(tr("Subgroups"));

	shownComboBox->setCurrentIndex(-1);

	if(settings.contains(this->metaObject()->className()+QString("/shown-categories")))
		shownComboBox->setCurrentIndex(settings.value(this->metaObject()->className()+QString("/shown-categories")).toInt());
	else
		shownComboBox->setCurrentIndex(0);

	connect(shownComboBox, SIGNAL(activated(QString)), this, SLOT(shownComboBoxChanged(QString)));
*/

	//added by Volker Dirr
	//connect(&communicationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStudentsTimetableTable()));

	shownComboBoxChanged();
}

TimetableViewStudentsDaysHorizontalForm::~TimetableViewStudentsDaysHorizontalForm()
{
	saveFETDialogGeometry(this);

	//save vertical students list splitter state
	QSettings settings(COMPANY, PROGRAM);
	settings.setValue(this->metaObject()->className()+QString("/vertical-students-list-splitter-state"), verticalStudentsListSplitter->saveState());

	//save vertical students table details splitter state
	//QSettings settings(COMPANY, PROGRAM);
	settings.setValue(this->metaObject()->className()+QString("/vertical-students-table-details-splitter-state"), verticalStudentsTableDetailsSplitter->saveState());

	//save horizontal splitter state
	//QSettings settings(COMPANY, PROGRAM);
	settings.setValue(this->metaObject()->className()+QString("/horizontal-splitter-state"), horizontalSplitter->saveState());
	
	settings.setValue(this->metaObject()->className()+QString("/shown-categories"), shownComboBox->currentIndex());
}

void TimetableViewStudentsDaysHorizontalForm::resizeRowsAfterShow()
{
	studentsTimetableTable->resizeRowsToContents();
}

void TimetableViewStudentsDaysHorizontalForm::shownComboBoxChanged()
{
	if(shownComboBox->currentIndex()==0){
		//years, groups, and subgroups shown
		yearsListWidget->show();
		groupsListWidget->show();
		subgroupsListWidget->show();
		
		disconnect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
		yearsListWidget->clear();
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			yearsListWidget->addItem(sty->name);
		}
		if(yearsListWidget->count()>0)
			yearsListWidget->setCurrentRow(0);
		connect(yearsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(yearChanged(const QString&)));
		if(yearsListWidget->count()>0)
			yearChanged(yearsListWidget->item(0)->text());
	}
	else if(shownComboBox->currentIndex()==1){
		//only groups and subgroups shown
		yearsListWidget->hide();
		groupsListWidget->show();
		subgroupsListWidget->show();

		disconnect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
		groupsListWidget->clear();
		QSet<QString> groupsSet;
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			for(StudentsGroup* stg : qAsConst(sty->groupsList))
				if(!groupsSet.contains(stg->name)){
					groupsListWidget->addItem(stg->name);
					groupsSet.insert(stg->name);
				}
		}
		if(groupsListWidget->count()>0)
			groupsListWidget->setCurrentRow(0);
		connect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));
		if(groupsListWidget->count()>0)
			groupChanged(groupsListWidget->item(0)->text());
	}
	else if(shownComboBox->currentIndex()==2){
		//only subgroups shown
		yearsListWidget->hide();
		groupsListWidget->hide();
		subgroupsListWidget->show();

		disconnect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
		subgroupsListWidget->clear();
		QSet<QString> subgroupsSet;
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			for(StudentsGroup* stg : qAsConst(sty->groupsList))
				for(StudentsSubgroup* sts : qAsConst(stg->subgroupsList))
					if(!subgroupsSet.contains(sts->name)){
						subgroupsListWidget->addItem(sts->name);
						subgroupsSet.insert(sts->name);
					}
		}
		if(subgroupsListWidget->count()>0)
			subgroupsListWidget->setCurrentRow(0);
		connect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));
		if(subgroupsListWidget->count()>0)
			subgroupChanged(subgroupsListWidget->item(0)->text());
	}
	else{
		assert(0);
	}
}

void TimetableViewStudentsDaysHorizontalForm::yearChanged(const QString &yearName)
{
	if(!(students_schedule_ready && teachers_schedule_ready)){
		QMessageBox::warning(this, tr("FET warning"), tr("Timetable not available in view students timetable dialog - please generate a new timetable"));
		return;
	}
	assert(students_schedule_ready && teachers_schedule_ready);

	if(yearName==QString())
		return;
	int yearIndex=gt.rules.searchAugmentedYear(yearName);
	if(yearIndex<0){
		QMessageBox::warning(this, tr("FET warning"), tr("Invalid year - please close this dialog and open a new students view timetable dialog"));
		return;
	}

	disconnect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));

	groupsListWidget->clear();
	StudentsYear* sty=gt.rules.augmentedYearsList.at(yearIndex);
	for(int i=0; i<sty->groupsList.size(); i++){
		StudentsGroup* stg=sty->groupsList[i];
		groupsListWidget->addItem(stg->name);
	}

	connect(groupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(groupChanged(const QString&)));

	if(groupsListWidget->count()>0)
		groupsListWidget->setCurrentRow(0);
}

void TimetableViewStudentsDaysHorizontalForm::groupChanged(const QString &groupName)
{
	if(!(students_schedule_ready && teachers_schedule_ready)){
		QMessageBox::warning(this, tr("FET warning"), tr("Timetable not available in view students timetable dialog - please generate a new timetable"));
		return;
	}
	assert(students_schedule_ready && teachers_schedule_ready);

	if(groupName==QString())
		return;
	
	StudentsSet* ss=gt.rules.searchAugmentedStudentsSet(groupName);
	if(ss==nullptr){
		QMessageBox::warning(this, tr("FET warning"), tr("Inexistent group - please reload this dialog"));
		return;
	}
	if(ss->type!=STUDENTS_GROUP){
		QMessageBox::warning(this, tr("FET warning"), tr("Incorrect group settings - please reload this dialog"));
		return;
	}
	
	StudentsGroup* stg=(StudentsGroup*)ss;
	
	/*QString yearName=yearsListWidget->currentItem()->text();
	int yearIndex=gt.rules.searchAugmentedYear(yearName);
	if(yearIndex<0){
		QMessageBox::warning(this, tr("FET warning"), tr("Invalid year - please close this dialog and open a new students view timetable dialog"));
		return;
	}

	StudentsYear* sty=gt.rules.augmentedYearsList.at(yearIndex);
	int groupIndex=gt.rules.searchAugmentedGroup(yearName, groupName);
	if(groupIndex<0){
		QMessageBox::warning(this, tr("FET warning"),
		 tr("Invalid group in the selected year, or the groups of the current year are not updated")+
		 "\n\n"+
		 tr("Solution: please try to select a different year and after that select the current year again, "
		 "to refresh the groups list, or close this dialog and open again the students view timetable dialog"));
		return;
	}*/
	
	disconnect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));

	subgroupsListWidget->clear();
	
	//StudentsGroup* stg=sty->groupsList.at(groupIndex);
	for(int i=0; i<stg->subgroupsList.size(); i++){
		StudentsSubgroup* sts=stg->subgroupsList[i];
		subgroupsListWidget->addItem(sts->name);
	}

	connect(subgroupsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(subgroupChanged(const QString&)));

	if(subgroupsListWidget->count()>0)
		subgroupsListWidget->setCurrentRow(0);
}

void TimetableViewStudentsDaysHorizontalForm::subgroupChanged(const QString &subgroupName)
{
	Q_UNUSED(subgroupName);
	
	updateStudentsTimetableTable();
}

void TimetableViewStudentsDaysHorizontalForm::updateStudentsTimetableTable(){
	if(!(students_schedule_ready && teachers_schedule_ready)){
		QMessageBox::warning(this, tr("FET warning"), tr("Timetable not available in view students timetable dialog - please generate a new timetable "
		"or close the timetable view students dialog"));
		return;
	}
	assert(students_schedule_ready && teachers_schedule_ready);
	
	if(gt.rules.nInternalRooms!=gt.rules.roomsList.count()){
		QMessageBox::warning(this, tr("FET warning"), tr("Cannot display the timetable, because you added or removed some rooms. Please regenerate the timetable and then view it"));
		return;
	}

	QString s;
	//QString yearname;
	//QString groupname;
	QString subgroupname;

	/*if(yearsListWidget->currentRow()<0 || yearsListWidget->currentRow()>=yearsListWidget->count())
		return;
	if(groupsListWidget->currentRow()<0 || groupsListWidget->currentRow()>=groupsListWidget->count())
		return;
	if(subgroupsListWidget->currentRow()<0 || subgroupsListWidget->currentRow()>=subgroupsListWidget->count())
		return;*/

	//yearname = yearsListWidget->currentItem()->text();
	//groupname = groupsListWidget->currentItem()->text();
	subgroupname = subgroupsListWidget->currentItem()->text();

	StudentsSet* ss=gt.rules.searchAugmentedStudentsSet(subgroupname);
	if(ss==nullptr){
		QMessageBox::information(this, tr("FET warning"), tr("Inexistent subgroup - please reload this dialog"));
		return;
	}
	if(ss->type!=STUDENTS_SUBGROUP){
		QMessageBox::warning(this, tr("FET warning"), tr("Incorrect subgroup settings - please reload this dialog"));
		return;
	}
	StudentsSubgroup* sts=(StudentsSubgroup*)ss;

	s="";
	s += subgroupname;

	classNameTextLabel->setText(s);

	assert(gt.rules.initialized);

	assert(sts);
	int i;
	/*for(i=0; i<gt.rules.nInternalSubgroups; i++)
		if(gt.rules.internalSubgroupsList[i]==sts)
			break;
	assert(i==sts->indexInInternalSubgroupsList);*/
	i=sts->indexInInternalSubgroupsList;
	assert(i>=0);
	assert(i<gt.rules.nInternalSubgroups);
	for(int j=0; j<gt.rules.nHoursPerDay && j<studentsTimetableTable->rowCount(); j++){
		for(int k=0; k<gt.rules.nDaysPerWeek && k<studentsTimetableTable->columnCount(); k++){
			//begin by Marco Vassura
			// add colors (start)
			//if(USE_GUI_COLORS) {
			studentsTimetableTable->item(j, k)->setBackground(studentsTimetableTable->palette().color(QPalette::Base));
			studentsTimetableTable->item(j, k)->setForeground(studentsTimetableTable->palette().color(QPalette::Text));
			//}
			// add colors (end)
			//end by Marco Vassura
			s="";
			int ai=students_timetable_weekly[i][k][j]; //activity index
			if(ai!=UNALLOCATED_ACTIVITY){
				Activity* act=&gt.rules.internalActivitiesList[ai];
				assert(act!=nullptr);
				
				assert(act->studentsNames.count()>=1);
				if((act->studentsNames.count()==1 && act->studentsNames.at(0)!=subgroupname) || act->studentsNames.count()>=2){
					s+=act->studentsNames.join(", ");
					s+="\n";
				}
				
				if(TIMETABLE_HTML_PRINT_ACTIVITY_TAGS){
					QString ats=act->activityTagsNames.join(", ");
					s+=act->subjectName +" "+ ats;
				}
				else{
					s+=act->subjectName;
				}
				if(act->teachersNames.count()>0){
					s+="\n";
					s+=act->teachersNames.join(", ");
				}
				
				int r=best_solution.rooms[ai];
				if(r!=UNALLOCATED_SPACE && r!=UNSPECIFIED_ROOM){
					//s+=" ";
					//s+=tr("R:%1", "Room").arg(gt.rules.internalRoomsList[r]->name);
					s+="\n";
					s+=gt.rules.internalRoomsList[r]->name;

					if(gt.rules.internalRoomsList[r]->isVirtual==true){
						QStringList tsl;
						for(int i : qAsConst(best_solution.realRoomsList[ai]))
							tsl.append(gt.rules.internalRoomsList[i]->name);
						s+=QString(" (")+tsl.join(", ")+QString(")");
					}
				}

				//added by Volker Dirr (start)
				QString descr="";
				QString t="";
				if(idsOfPermanentlyLockedTime.contains(act->id)){
					descr+=QCoreApplication::translate("TimetableViewForm", "PLT", "Abbreviation for permanently locked time. There are 4 strings: permanently locked time, permanently locked space, "
						"locked time, locked space. Make sure their abbreviations contain different letters and are visually different, so user can easily differentiate between them."
						" These abbreviations may appear also in other places, please use the same abbreviations.");
					t=", ";
				}
				else if(idsOfLockedTime.contains(act->id)){
					descr+=QCoreApplication::translate("TimetableViewForm", "LT", "Abbreviation for locked time. There are 4 strings: permanently locked time, permanently locked space, "
						"locked time, locked space. Make sure their abbreviations contain different letters and are visually different, so user can easily differentiate between them."
						" These abbreviations may appear also in other places, please use the same abbreviations.");
					t=", ";
				}
				if(idsOfPermanentlyLockedSpace.contains(act->id)){
					descr+=t+QCoreApplication::translate("TimetableViewForm", "PLS", "Abbreviation for permanently locked space. There are 4 strings: permanently locked time, permanently locked space, "
						"locked time, locked space. Make sure their abbreviations contain different letters and are visually different, so user can easily differentiate between them."
						" These abbreviations may appear also in other places, please use the same abbreviations.");
				}
				else if(idsOfLockedSpace.contains(act->id)){
					descr+=t+QCoreApplication::translate("TimetableViewForm", "LS", "Abbreviation for locked space. There are 4 strings: permanently locked time, permanently locked space, "
						"locked time, locked space. Make sure their abbreviations contain different letters and are visually different, so user can easily differentiate between them."
						" These abbreviations may appear also in other places, please use the same abbreviations.");
				}
				if(descr!=""){
					descr.prepend("\n(");
					descr.append(")");
				}
				s+=descr;
				//added by Volker Dirr (end)
				
				//begin by Marco Vassura
				// add colors (start)
				if(USE_GUI_COLORS) {
					QBrush bg(stringToColor(act->subjectName));
					studentsTimetableTable->item(j, k)->setBackground(bg);
					double brightness = bg.color().redF()*0.299 + bg.color().greenF()*0.587 + bg.color().blueF()*0.114;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					if (brightness<0.5)
						studentsTimetableTable->item(j, k)->setForeground(QBrush(QColorConstants::White));
					else
						studentsTimetableTable->item(j, k)->setForeground(QBrush(QColorConstants::Black));
#else
					if (brightness<0.5)
						studentsTimetableTable->item(j, k)->setForeground(QBrush(Qt::white));
					else
						studentsTimetableTable->item(j, k)->setForeground(QBrush(Qt::black));
#endif
				}
				// add colors (end)
				//end by Marco Vassura
			}
			else{
				if(subgroupNotAvailableDayHour[i][k][j] && PRINT_NOT_AVAILABLE_TIME_SLOTS)
					s+="-x-";
				else if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS)
					s+="-X-";
			}
			studentsTimetableTable->item(j, k)->setText(s);
		}
	}

	studentsTimetableTable->resizeRowsToContents();
	
	detailActivity(studentsTimetableTable->currentItem());
}

//begin by Marco Vassura
//slightly modified by Liviu Lalescu on 2021-03-01
QColor TimetableViewStudentsDaysHorizontalForm::stringToColor(const QString& s)
{
	// CRC-24 based on RFC 2440 Section 6.1
	unsigned long int crc = 0xB704CEUL;
	QByteArray ba=s.toUtf8();
	for(char c : qAsConst(ba)){
		unsigned char uc=(unsigned char)(c);
		crc ^= (uc & 0xFF) << 16;
		for (int i = 0; i < 8; i++) {
			crc <<= 1;
			if (crc & 0x1000000UL)
				crc ^= 0x1864CFBUL;
		}
	}
	return QColor::fromRgb(int((crc>>16) & 0xFF), int((crc>>8) & 0xFF), int(crc & 0xFF));
}
//end by Marco Vassura

void TimetableViewStudentsDaysHorizontalForm::resizeEvent(QResizeEvent* event)
{
	QDialog::resizeEvent(event);

	studentsTimetableTable->resizeRowsToContents();
}

void TimetableViewStudentsDaysHorizontalForm::currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous)
{
	Q_UNUSED(previous);
	
	detailActivity(current);
}

void TimetableViewStudentsDaysHorizontalForm::detailActivity(QTableWidgetItem* item)
{
	if(item==nullptr){
		detailsTextEdit->setPlainText(QString(""));
		return;
	}
	
	if(item->row()>=gt.rules.nHoursPerDay || item->column()>=gt.rules.nDaysPerWeek){
		QMessageBox::warning(this, tr("FET warning"), tr("Timetable not available in view students timetable dialog - please generate a new timetable "
		"or close the timetable view students dialog"));
		return;
	}

	if(!(students_schedule_ready && teachers_schedule_ready)){
		QMessageBox::warning(this, tr("FET warning"), tr("Timetable not available in view students timetable dialog - please generate a new timetable"));
		return;
	}
	assert(students_schedule_ready && teachers_schedule_ready);

	if(gt.rules.nInternalRooms!=gt.rules.roomsList.count()){
		QMessageBox::warning(this, tr("FET warning"), tr("Cannot display the timetable, because you added or removed some rooms. Please regenerate the timetable and then view it"));
		return;
	}

	QString s;
	//QString yearname; //not really necessary
	//QString groupname; //not really necessary
	QString subgroupname;
	
	//Note: the years and the groups list widgets might contain useless information if the user selected to see only groups or subgroups.
	//However, in this case they are not void and the currentRow() will be 0 (first year) for the year list widget and, if the variant is
	//to show only the subgroups, the same for the groups list widget (initialized in the dialog's constructor).
	
	//if(yearsListWidget->currentRow()<0 || yearsListWidget->currentRow()>=yearsListWidget->count())
	//	return;
	//if(groupsListWidget->currentRow()<0 || groupsListWidget->currentRow()>=groupsListWidget->count())
	//	return;
	if(subgroupsListWidget->currentRow()<0 || subgroupsListWidget->currentRow()>=subgroupsListWidget->count())
		return;

	//yearname = yearsListWidget->currentItem()->text();
	//groupname = groupsListWidget->currentItem()->text();
	subgroupname = subgroupsListWidget->currentItem()->text();

	StudentsSet* ss=gt.rules.searchAugmentedStudentsSet(subgroupname);
	if(ss==nullptr){
		QMessageBox::information(this, tr("FET warning"), tr("Inexistent subgroup - please reload this dialog"));
		return;
	}
	if(ss->type!=STUDENTS_SUBGROUP){
		QMessageBox::warning(this, tr("FET warning"), tr("Incorrect subgroup settings - please reload this dialog"));
		return;
	}
	StudentsSubgroup* sts=(StudentsSubgroup*)ss;

	/*StudentsSubgroup* sts=(StudentsSubgroup*)gt.rules.searchAugmentedStudentsSet(subgroupname);
	if(!sts){
		QMessageBox::warning(this, tr("FET warning"), tr("Invalid students set - please close this dialog and open a new view students timetable dialog"));
		return;
	}*/
	
	assert(sts);
	int i;
	/*for(i=0; i<gt.rules.nInternalSubgroups; i++)
		if(gt.rules.internalSubgroupsList[i]==sts)
			break;
	assert(i==sts->indexInInternalSubgroupsList);*/
/*	if(!(i<gt.rules.nInternalSubgroups)){
		QMessageBox::warning(this, tr("FET warning"), tr("Invalid students set - please close this dialog and open a new view students dialog"));
		return;
	}*/
	i=sts->indexInInternalSubgroupsList;
	assert(i>=0);
	assert(i<gt.rules.nInternalSubgroups);
	int j=item->row();
	int k=item->column();
	s="";
	if(j>=0 && k>=0){
		int ai=students_timetable_weekly[i][k][j]; //activity index
		//Activity* act=gt.rules.activitiesList.at(ai);
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			assert(act!=nullptr);
			//s+=act->getDetailedDescriptionWithConstraints(gt.rules);
			s+=act->getDetailedDescription(gt.rules);

			int r=best_solution.rooms[ai];
			if(r!=UNALLOCATED_SPACE && r!=UNSPECIFIED_ROOM){
				s+="\n";
				s+=tr("Room: %1").arg(gt.rules.internalRoomsList[r]->name);

				if(gt.rules.internalRoomsList[r]->isVirtual==true){
					QStringList tsl;
					for(int i : qAsConst(best_solution.realRoomsList[ai]))
						tsl.append(gt.rules.internalRoomsList[i]->name);
					s+=QString(" (")+tsl.join(", ")+QString(")");
				}

				if(gt.rules.internalRoomsList[r]->building!=""){
					s+="\n";
					s+=tr("Building=%1").arg(gt.rules.internalRoomsList[r]->building);
				}
				s+="\n";
				s+=tr("Capacity=%1").arg(gt.rules.internalRoomsList[r]->capacity);
			}
			//added by Volker Dirr (start)
			QString descr="";
			QString t="";
			if(idsOfPermanentlyLockedTime.contains(act->id)){
				descr+=QCoreApplication::translate("TimetableViewForm", "permanently locked time", "refers to activity");
				t=", ";
			}
			else if(idsOfLockedTime.contains(act->id)){
				descr+=QCoreApplication::translate("TimetableViewForm", "locked time", "refers to activity");
				t=", ";
			}
			if(idsOfPermanentlyLockedSpace.contains(act->id)){
				descr+=t+QCoreApplication::translate("TimetableViewForm", "permanently locked space", "refers to activity");
			}
			else if(idsOfLockedSpace.contains(act->id)){
				descr+=t+QCoreApplication::translate("TimetableViewForm", "locked space", "refers to activity");
			}
			if(descr!=""){
				descr.prepend("\n(");
				descr.append(")");
			}
			s+=descr;
			//added by Volker Dirr (end)
		}
		else{
			if(subgroupNotAvailableDayHour[i][k][j]){
				s+=tr("Students subgroup is not available 100% in this slot");
				s+="\n";
			}
			if(breakDayHour[k][j]){
				s+=tr("Break with weight 100% in this slot");
				s+="\n";
			}
		}
	}
	detailsTextEdit->setPlainText(s);
}

void TimetableViewStudentsDaysHorizontalForm::lockTime()
{
	lock(true, false);
}

void TimetableViewStudentsDaysHorizontalForm::lockSpace()
{
	lock(false, true);
}

void TimetableViewStudentsDaysHorizontalForm::lockTimeSpace()
{
	lock(true, true);
}

void TimetableViewStudentsDaysHorizontalForm::lock(bool lockTime, bool lockSpace)
{
	if(simulation_running){
		QMessageBox::information(this, tr("FET information"),
			tr("Allocation in course.\nPlease stop simulation before this."));
		return;
	}

	//find subgroup index
	if(!(students_schedule_ready && teachers_schedule_ready)){
		QMessageBox::warning(this, tr("FET warning"), tr("Timetable not available in view students timetable dialog - please generate a new timetable"));
		return;
	}
	assert(students_schedule_ready && teachers_schedule_ready);

	if(gt.rules.nInternalRooms!=gt.rules.roomsList.count()){
		QMessageBox::warning(this, tr("FET warning"), tr("Cannot display the timetable, because you added or removed some rooms. Please regenerate the timetable and then view it"));
		return;
	}

	//QString yearname; //not really necessary
	//QString groupname; //not really necessary
	QString subgroupname;
	
	//Note: the years and the groups list widgets might contain useless information if the user selected to see only groups or subgroups.
	//However, in this case they are not void and the currentRow() will be 0 (first year) for the year list widget and, if the variant is
	//to show only the subgroups, the same for the groups list widget (initialized in the dialog's constructor).
	
	/*if(yearsListWidget->currentRow()<0 || yearsListWidget->currentRow()>=yearsListWidget->count()){
		QMessageBox::information(this, tr("FET information"), tr("Please select a year"));
		return;
	}
	if(groupsListWidget->currentRow()<0 || groupsListWidget->currentRow()>=groupsListWidget->count()){
		QMessageBox::information(this, tr("FET information"), tr("Please select a group"));
		return;
	}*/
	if(subgroupsListWidget->currentRow()<0 || subgroupsListWidget->currentRow()>=subgroupsListWidget->count()){
		QMessageBox::information(this, tr("FET information"), tr("Please select a subgroup"));
		return;
	}

	//yearname = yearsListWidget->currentItem()->text();
	//groupname = groupsListWidget->currentItem()->text();
	subgroupname = subgroupsListWidget->currentItem()->text();

	Solution* tc=&best_solution;

	StudentsSet* ss=gt.rules.searchAugmentedStudentsSet(subgroupname);
	if(ss==nullptr){
		QMessageBox::information(this, tr("FET warning"), tr("Inexistent subgroup - please reload this dialog"));
		return;
	}
	if(ss->type!=STUDENTS_SUBGROUP){
		QMessageBox::warning(this, tr("FET warning"), tr("Incorrect subgroup settings - please reload this dialog"));
		return;
	}
	StudentsSubgroup* sts=(StudentsSubgroup*)ss;

	/*StudentsSubgroup* sts=(StudentsSubgroup*)gt.rules.searchAugmentedStudentsSet(subgroupname);
	if(!sts){
		QMessageBox::warning(this, tr("FET warning"), tr("Invalid students set - please close this dialog and open a new view students timetable dialog"));
		return;
	}*/
	
	assert(sts);
	int i;
	/*for(i=0; i<gt.rules.nInternalSubgroups; i++)
		if(gt.rules.internalSubgroupsList[i]==sts)
			break;
	assert(i==sts->indexInInternalSubgroupsList);*/
	i=sts->indexInInternalSubgroupsList;
	assert(i>=0);
	assert(i<gt.rules.nInternalSubgroups);

	bool report=false; //the messages are annoying
	
	int addedT=0, unlockedT=0;
	int addedS=0, unlockedS=0;

	//lock selected activities
	QSet<int> careAboutIndex;		//added by Volker Dirr. Needed, because of activities with duration > 1
	careAboutIndex.clear();
	for(int j=0; j<gt.rules.nHoursPerDay && j<studentsTimetableTable->rowCount(); j++){
		for(int k=0; k<gt.rules.nDaysPerWeek && k<studentsTimetableTable->columnCount(); k++){
			if(studentsTimetableTable->item(j, k)->isSelected()){
				int ai=students_timetable_weekly[i][k][j];
				if(ai!=UNALLOCATED_ACTIVITY && !careAboutIndex.contains(ai)){	//modified, because of activities with duration > 1
					careAboutIndex.insert(ai);					//Needed, because of activities with duration > 1
					int a_tim=tc->times[ai];
					int hour=a_tim/gt.rules.nDaysPerWeek;
					int day=a_tim%gt.rules.nDaysPerWeek;
					//Activity* act=gt.rules.activitiesList.at(ai);
					Activity* act=&gt.rules.internalActivitiesList[ai];
					
					if(lockTime){
						ConstraintActivityPreferredStartingTime* ctr=new ConstraintActivityPreferredStartingTime(100.0, act->id, day, hour, false);
						bool t=gt.rules.addTimeConstraint(ctr);
						QString s;
						if(t){ //modified by Volker Dirr, so you can also unlock (start)
							addedT++;
							idsOfLockedTime.insert(act->id);
							s+=tr("Added the following constraint:")+"\n"+ctr->getDetailedDescription(gt.rules);
						}
						else{
							delete ctr;
						
							QList<TimeConstraint*> tmptc;
							tmptc.clear();
							int count=0;

							for(ConstraintActivityPreferredStartingTime* c : gt.rules.apstHash.value(act->id, QSet<ConstraintActivityPreferredStartingTime*>())){
								assert(c->activityId==act->id);
								if(c->activityId==act->id && c->weightPercentage==100.0 && c->active && c->day>=0 && c->hour>=0){
									count++;
									if(c->permanentlyLocked){
										if(idsOfLockedTime.contains(c->activityId) || !idsOfPermanentlyLockedTime.contains(c->activityId)){
											QMessageBox::warning(this, tr("FET warning"), tr("Small problem detected")
											  +"\n\n"+tr("A possible problem might be that you have 2 or more constraints of type activity preferred starting time with weight 100% related to activity id %1, please leave only one of them").arg(act->id)
											  +"\n\n"+tr("A possible problem might be synchronization - so maybe try to close the timetable view dialog and open it again")
											  +"\n\n"+tr("Please report possible bug")
											);
										}
										else{
											s+=tr("Constraint %1 will not be removed, because it is permanently locked. If you want to unlock it you must go to the constraints menu.").arg("\n"+c->getDetailedDescription(gt.rules)+"\n");
										}
									}
									else{
										if(!idsOfLockedTime.contains(c->activityId) || idsOfPermanentlyLockedTime.contains(c->activityId)){
											QMessageBox::warning(this, tr("FET warning"), tr("Small problem detected")
											  +"\n\n"+tr("A possible problem might be that you have 2 or more constraints of type activity preferred starting time with weight 100% related to activity id %1, please leave only one of them").arg(act->id)
											  +"\n\n"+tr("A possible problem might be synchronization - so maybe try to close the timetable view dialog and open it again")
											  +"\n\n"+tr("Please report possible bug")
											);
										}
										else{
											tmptc.append((TimeConstraint*)c);
										}
									}
								}
							}
							if(count!=1)
								QMessageBox::warning(this, tr("FET warning"), tr("You may have a problem, because FET expected to delete 1 constraint, but will delete %1 constraints").arg(tmptc.size()));

							for(TimeConstraint* deltc : qAsConst(tmptc)){
								s+=tr("The following constraint will be deleted:")+"\n"+deltc->getDetailedDescription(gt.rules)+"\n";
								gt.rules.removeTimeConstraint(deltc);
								//delete deltc; - this is done by rules.removeTimeConstraint(...)
								idsOfLockedTime.remove(act->id);
								unlockedT++;
							}
							tmptc.clear();
						}  //modified by Volker Dirr, so you can also unlock (end)
						
						if(report){
							int k;
							k=QMessageBox::information(this, tr("FET information"), s,
							 tr("Skip information"), tr("See next"), QString(), 1, 0 );

		 					if(k==0)
								report=false;
						}
					}

					int ri=tc->rooms[ai];
					if(ri!=UNALLOCATED_SPACE && ri!=UNSPECIFIED_ROOM && lockSpace){
						QStringList tl;
						if(gt.rules.internalRoomsList[ri]->isVirtual==false)
							assert(tc->realRoomsList[ai].isEmpty());
						else
							for(int rr : qAsConst(tc->realRoomsList[ai]))
								tl.append(gt.rules.internalRoomsList[rr]->name);
						
						ConstraintActivityPreferredRoom* ctr=new ConstraintActivityPreferredRoom(100, act->id, (gt.rules.internalRoomsList[ri])->name, tl, false);
						bool t=gt.rules.addSpaceConstraint(ctr);

						QString s;
						
						if(t){ //modified by Volker Dirr, so you can also unlock (start)
							addedS++;
							idsOfLockedSpace.insert(act->id);
							s+=tr("Added the following constraint:")+"\n"+ctr->getDetailedDescription(gt.rules);
						}
						else{
							delete ctr;
						
							QList<SpaceConstraint*> tmpsc;
							tmpsc.clear();
							int count=0;

							for(ConstraintActivityPreferredRoom* c : gt.rules.aprHash.value(act->id, QSet<ConstraintActivityPreferredRoom*>())){
								assert(c->activityId==act->id);
								if(c->activityId==act->id && c->weightPercentage==100.0 && c->active
								 && (gt.rules.internalRoomsList[ri]->isVirtual==false
								 || (gt.rules.internalRoomsList[ri]->isVirtual==true && !c->preferredRealRoomsNames.isEmpty()))){
									count++;
									if(c->permanentlyLocked){
										if(idsOfLockedSpace.contains(c->activityId) || !idsOfPermanentlyLockedSpace.contains(c->activityId)){
											QMessageBox::warning(this, tr("FET warning"), tr("Small problem detected")
											  +"\n\n"+tr("A possible problem might be that you have 2 or more constraints of type activity preferred room with weight 100% related to activity id %1, please leave only one of them").arg(act->id)
											  +"\n\n"+tr("A possible problem might be synchronization - so maybe try to close the timetable view dialog and open it again")
											  +"\n\n"+tr("Please report possible bug")
											);
										}
										else{
											s+=tr("Constraint %1 will not be removed, because it is permanently locked. If you want to unlock it you must go to the constraints menu.").arg("\n"+c->getDetailedDescription(gt.rules)+"\n");
										}
									}
									else{
										if(!idsOfLockedSpace.contains(c->activityId) || idsOfPermanentlyLockedSpace.contains(c->activityId)){
											QMessageBox::warning(this, tr("FET warning"), tr("Small problem detected")
											  +"\n\n"+tr("A possible problem might be that you have 2 or more constraints of type activity preferred room with weight 100% related to activity id %1, please leave only one of them").arg(act->id)
											  +"\n\n"+tr("A possible problem might be synchronization - so maybe try to close the timetable view dialog and open it again")
											  +"\n\n"+tr("Please report possible bug")
											);
										}
										else{
											tmpsc.append((SpaceConstraint*)c);
										}
									}
								}
							}
							if(count!=1)
								QMessageBox::warning(this, tr("FET warning"), tr("You may have a problem, because FET expected to delete 1 constraint, but will delete %1 constraints").arg(tmpsc.size()));

							for(SpaceConstraint* delsc : qAsConst(tmpsc)){
								s+=tr("The following constraint will be deleted:")+"\n"+delsc->getDetailedDescription(gt.rules)+"\n";
								gt.rules.removeSpaceConstraint(delsc);
								idsOfLockedSpace.remove(act->id);
								unlockedS++;
								//delete delsc; done by rules.removeSpaceConstraint(...)
							}
							tmpsc.clear();
						}  //modified by Volker Dirr, so you can also unlock (end)
						
						if(report){
							int k;
							k=QMessageBox::information(this, tr("FET information"), s,
							 tr("Skip information"), tr("See next"), QString(), 1, 0 );
							
		 					if(k==0)
								report=false;
						}
					}
				}
			}
		}
	}
	
	QStringList added;
	QStringList removed;
	if(FET_LANGUAGE=="en_US"){
		if(addedT>0){
			if(addedT==1)
				added << QString("Added 1 locking time constraint.");
			else
				added << QString("Added %1 locking time constraints.").arg(addedT);
		}
		if(addedS>0){
			if(addedS==1)
				added << QString("Added 1 locking space constraint.");
			else
				added << QString("Added %1 locking space constraints.").arg(addedS);
		}
		if(unlockedT>0){
			if(unlockedT==1)
				removed << QString("Removed 1 locking time constraint.");
			else
				removed << QString("Removed %1 locking time constraints.").arg(unlockedT);
		}
		if(unlockedS>0){
			if(unlockedS==1)
				removed << QString("Removed 1 locking space constraint.");
			else
				removed << QString("Removed %1 locking space constraints.").arg(unlockedS);
		}
	}
	else{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		if(addedT>0){
			added << QCoreApplication::translate("TimetableViewForm", "Added %n locking time constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).",
			 addedT);
		}
		if(addedS>0){
			added << QCoreApplication::translate("TimetableViewForm", "Added %n locking space constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).",
			 addedS);
		}
		if(unlockedT>0){
			removed << QCoreApplication::translate("TimetableViewForm", "Removed %n locking time constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).",
			 unlockedT);
		}
		if(unlockedS>0){
			removed << QCoreApplication::translate("TimetableViewForm", "Removed %n locking space constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).",
			 unlockedS);
		}
#else
		if(addedT>0){
			added << QCoreApplication::translate("TimetableViewForm", "Added %n locking time constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).", QCoreApplication::UnicodeUTF8,
			 addedT);
		}
		if(addedS>0){
			added << QCoreApplication::translate("TimetableViewForm", "Added %n locking space constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).", QCoreApplication::UnicodeUTF8,
			 addedS);
		}
		if(unlockedT>0){
			removed << QCoreApplication::translate("TimetableViewForm", "Removed %n locking time constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).", QCoreApplication::UnicodeUTF8,
			 unlockedT);
		}
		if(unlockedS>0){
			removed << QCoreApplication::translate("TimetableViewForm", "Removed %n locking space constraint(s).",
			 "See http://doc.qt.io/qt-5/i18n-plural-rules.html for advice on how to correctly translate this field."
			 "Also, see http://doc.qt.io/qt-5/i18n-source-translation.html, section 'Handling Plurals'."
			 "You have two examples on how to translate this field in fet_en_GB.ts and in fet_ro.ts"
			 "(open these files with Qt Linguist and see the translation of this field).", QCoreApplication::UnicodeUTF8,
			 unlockedS);
		}
#endif
	}
	QString ad=added.join("\n");
	QString re=removed.join("\n");
	QStringList all;
	if(!ad.isEmpty())
		all<<ad;
	if(!re.isEmpty())
		all<<re;
	QString s=all.join("\n\n");
	if(s.isEmpty())
		s=QCoreApplication::translate("TimetableViewForm", "No locking constraints added or removed.");
	QMessageBox::information(this, tr("FET information"), s);

///////////just for testing
	QSet<int> backupLockedTime;
	QSet<int> backupPermanentlyLockedTime;
	QSet<int> backupLockedSpace;
	QSet<int> backupPermanentlyLockedSpace;
	
	backupLockedTime=idsOfLockedTime;
	backupPermanentlyLockedTime=idsOfPermanentlyLockedTime;
	backupLockedSpace=idsOfLockedSpace;
	backupPermanentlyLockedSpace=idsOfPermanentlyLockedSpace;
	
	LockUnlock::computeLockedUnlockedActivitiesTimeSpace(); //just to make sure, not really needed, but to test better
	
	assert(backupLockedTime==idsOfLockedTime);
	assert(backupPermanentlyLockedTime==idsOfPermanentlyLockedTime);
	assert(backupLockedSpace==idsOfLockedSpace);
	assert(backupPermanentlyLockedSpace==idsOfPermanentlyLockedSpace);
///////////

	LockUnlock::increaseCommunicationSpinBox(); //this is needed
}

void TimetableViewStudentsDaysHorizontalForm::help()
{
	QString s="";
	s+=QCoreApplication::translate("TimetableViewForm", "Lock/unlock: you can select one or more activities in the table and toggle lock/unlock in time, space or both.");
	s+=" ";
	s+=QCoreApplication::translate("TimetableViewForm", "There will be added or removed locking constraints for the selected activities (they can be unlocked only if they are not permanently locked).");
	s+="\n\n";
	s+=QCoreApplication::translate("TimetableViewForm", "Locking time constraints are constraints of type activity preferred starting time. Locking space constraints are constraints of type"
		" activity preferred room. You can see these constraints in the corresponding constraints dialogs. New locking constraints are added at the end of the list of constraints.");
	s+="\n\n";
	s+=QCoreApplication::translate("TimetableViewForm", "If a cell is (permanently) locked in time or space, it contains abbreviations to show that: PLT (permanently locked time), LT (locked time), "
		"PLS (permanently locked space) or LS (locked space).", "Translate the abbreviations also. Make sure the abbreviations in your language are different between themselves "
		"and the user can differentiate easily between them. These abbreviations may appear also in other places, please use the same abbreviations.");
	s+="\n\n";
	s+=QCoreApplication::translate("TimetableViewForm", "There is a combo box in this dialog. You can choose each year, group, and subgroup if you select 'Years',"
		" each group and subgroup if you select 'Groups', or each subgroup if you select 'Subgroups'.");
	s+="\n\n";

	LongTextMessageBox::largeInformation(this, tr("FET help"), s);
}
