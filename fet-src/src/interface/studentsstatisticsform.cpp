/***************************************************************************
                          studentsstatisticform.cpp  -  description
                             -------------------
    begin                : March 25, 2006
    copyright            : (C) 2006 by Lalescu Liviu
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

#include "studentsstatisticsform.h"

#include "timetable_defs.h"
#include "timetable.h"

#include "fet.h"

#include "messageboxes.h"

#include "longtextmessagebox.h"

#include <Qt>

#include <QString>
#include <QStringList>

#include <QHash>

#include <QProgressDialog>

#include <QMessageBox>
#include <QApplication>

#include <QHeaderView>
#include <QTableWidget>

extern QApplication* pqapplication;

StudentsStatisticsForm::StudentsStatisticsForm(QWidget* parent): QDialog(parent)
{
	setupUi(this);
	
	closeButton->setDefault(true);

	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

	tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	centerWidgetOnScreen(this);
	restoreFETDialogGeometry(this);
	
	connect(showYearsCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxesModified()));
	connect(showGroupsCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxesModified()));
	connect(showSubgroupsCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxesModified()));

	connect(showCompleteStructureCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxesModified()));
	
	//2014-12-18
	QSet<StudentsYear*> allYears;
	QSet<StudentsGroup*> allGroups;
	QSet<StudentsSubgroup*> allSubgroups;
	
	QHash<QString, StudentsYear*> yearsHash;
	QHash<QString, StudentsGroup*> groupsHash;
	QHash<QString, StudentsSubgroup*> subgroupsHash;
	
	QHash<StudentsYear*, QSet<Activity*>> activitiesForYear;
	QHash<StudentsGroup*, QSet<Activity*>> activitiesForGroup;
	QHash<StudentsSubgroup*, QSet<Activity*>> activitiesForSubgroup;
	
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		yearsHash.insert(year->name, year);
		
		allYears.insert(year);
		
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			groupsHash.insert(group->name, group);
			
			allGroups.insert(group);
			
			for(StudentsSubgroup* subgroup : qAsConst(group->subgroupsList)){
				subgroupsHash.insert(subgroup->name, subgroup);

				allSubgroups.insert(subgroup);
			}
		}
	}
	
	QString warnings=QString("");
	for(Activity* act : qAsConst(gt.rules.activitiesList))
		if(act->active){
			for(const QString& sts : qAsConst(act->studentsNames)){
				if(yearsHash.contains(sts)){
					StudentsYear* year=yearsHash.value(sts);
	
					QSet<Activity*> acts=activitiesForYear.value(year, QSet<Activity*>());
					acts.insert(act);
					activitiesForYear.insert(year, acts);
				}
				else if(groupsHash.contains(sts)){
					StudentsGroup* group=groupsHash.value(sts);

					QSet<Activity*> acts=activitiesForGroup.value(group, QSet<Activity*>());
					acts.insert(act);
					activitiesForGroup.insert(group, acts);
				}
				else if(subgroupsHash.contains(sts)){
					StudentsSubgroup* subgroup=subgroupsHash.value(sts);

					QSet<Activity*> acts=activitiesForSubgroup.value(subgroup, QSet<Activity*>());
					acts.insert(act);
					activitiesForSubgroup.insert(subgroup, acts);
				}
				else
					warnings+=tr("Students set %1 from activity with id %2 is inexistent in the students list. Please correct this.").arg(sts).arg(act->id)+QString("\n");
			}
		}
	if(!warnings.isEmpty())
		FetMessage::warning(this, tr("FET warning"), warnings);
	
	//phase 1a
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		QSet<Activity*> actsYear=activitiesForYear.value(year, QSet<Activity*>());
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			QSet<Activity*> actsGroup=activitiesForGroup.value(group, QSet<Activity*>());
			actsGroup.unite(actsYear);
			activitiesForGroup.insert(group, actsGroup);
		}
	}
	//phase 1b
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			QSet<Activity*> actsGroup=activitiesForGroup.value(group, QSet<Activity*>());
			for(StudentsSubgroup* subgroup : qAsConst(group->subgroupsList)){
				QSet<Activity*> actsSubgroup=activitiesForSubgroup.value(subgroup, QSet<Activity*>());
				actsSubgroup.unite(actsGroup);
				activitiesForSubgroup.insert(subgroup, actsSubgroup);
			}
		}
	}
	//phase 2a
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			QSet<Activity*> actsGroup=activitiesForGroup.value(group, QSet<Activity*>());
			for(StudentsSubgroup* subgroup : qAsConst(group->subgroupsList)){
				QSet<Activity*> actsSubgroup=activitiesForSubgroup.value(subgroup, QSet<Activity*>());
				actsGroup.unite(actsSubgroup);
			}
			activitiesForGroup.insert(group, actsGroup);
		}
	}
	//phase 2b
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		QSet<Activity*> actsYear=activitiesForYear.value(year, QSet<Activity*>());
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			QSet<Activity*> actsGroup=activitiesForGroup.value(group, QSet<Activity*>());
			actsYear.unite(actsGroup);
		}
		activitiesForYear.insert(year, actsYear);
	}
	
	allActivities.clear();
	allHours.clear();
	
	for(StudentsYear* year : qAsConst(allYears)){
		QSet<Activity*> acts=activitiesForYear.value(year, QSet<Activity*>());
		int n=0, d=0;
		for(Activity* act : qAsConst(acts)){
			n++;
			d+=act->duration;
		}
		assert(!allActivities.contains(year->name));
		assert(!allHours.contains(year->name));
		allActivities.insert(year->name, n);
		allHours.insert(year->name, d);
	}
	
	for(StudentsGroup* group : qAsConst(allGroups)){
		QSet<Activity*> acts=activitiesForGroup.value(group, QSet<Activity*>());
		int n=0, d=0;
		for(Activity* act : qAsConst(acts)){
			n++;
			d+=act->duration;
		}
		assert(!allActivities.contains(group->name));
		assert(!allHours.contains(group->name));
		allActivities.insert(group->name, n);
		allHours.insert(group->name, d);
	}

	for(StudentsSubgroup* subgroup : qAsConst(allSubgroups)){
		QSet<Activity*> acts=activitiesForSubgroup.value(subgroup, QSet<Activity*>());
		int n=0, d=0;
		for(Activity* act : qAsConst(acts)){
			n++;
			d+=act->duration;
		}
		assert(!allActivities.contains(subgroup->name));
		assert(!allHours.contains(subgroup->name));
		allActivities.insert(subgroup->name, n);
		allHours.insert(subgroup->name, d);
	}
	
	checkBoxesModified();
}

StudentsStatisticsForm::~StudentsStatisticsForm()
{
	saveFETDialogGeometry(this);
}

void StudentsStatisticsForm::checkBoxesModified()
{
	bool complete=showCompleteStructureCheckBox->isChecked();
	
	QSet<QString> setOfStudents;

	setOfStudents.clear();
	int nStudentsSets=0;
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		bool sy=true;
		if(!complete){
			if(setOfStudents.contains(year->name))
				sy=false;
			else
				setOfStudents.insert(year->name);
		}
		if(showYearsCheckBox->isChecked() && sy)
			nStudentsSets++;
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			bool sg=true;
			if(!complete){
				if(setOfStudents.contains(group->name))
					sg=false;
				else
					setOfStudents.insert(group->name);
			}
			if(showGroupsCheckBox->isChecked() && sg)
				nStudentsSets++;
			for(StudentsSubgroup* subgroup : qAsConst(group->subgroupsList)){
				bool ss=true;
				if(!complete){
					if(setOfStudents.contains(subgroup->name))
						ss=false;
					else
						setOfStudents.insert(subgroup->name);
				}
				if(showSubgroupsCheckBox->isChecked() && ss)
					nStudentsSets++;
			}
		}
	}
	
	tableWidget->clear();
	tableWidget->setColumnCount(3);
	tableWidget->setRowCount(nStudentsSets);
	
	QStringList columns;
	columns<<tr("Students set");
	columns<<tr("No. of activities");
	columns<<tr("Duration");
	
	tableWidget->setHorizontalHeaderLabels(columns);
	
	setOfStudents.clear();
	
	int currentStudentsSet=-1;
	for(StudentsYear* year : qAsConst(gt.rules.yearsList)){
		bool sy=true;
		if(!complete){
			if(setOfStudents.contains(year->name))
				sy=false;
			else
				setOfStudents.insert(year->name);
		}

		if(showYearsCheckBox->isChecked() && sy){
			currentStudentsSet++;
			insertStudentsSet(year, currentStudentsSet);
		}
				
		for(StudentsGroup* group : qAsConst(year->groupsList)){
			bool sg=true;
			if(!complete){
				if(setOfStudents.contains(group->name))
					sg=false;
				else
					setOfStudents.insert(group->name);
			}

			if(showGroupsCheckBox->isChecked() && sg){
				currentStudentsSet++;
				insertStudentsSet(group, currentStudentsSet);
			}
			
			for(StudentsSubgroup* subgroup : qAsConst(group->subgroupsList)){
				bool ss=true;
				if(!complete){
					if(setOfStudents.contains(subgroup->name))
						ss=false;
					else
						setOfStudents.insert(subgroup->name);
				}

				if(showSubgroupsCheckBox->isChecked() && ss){
					currentStudentsSet++;
					insertStudentsSet(subgroup, currentStudentsSet);
				}
			}	
		}
	}
	
	tableWidget->resizeColumnsToContents();
	tableWidget->resizeRowsToContents();
}

void StudentsStatisticsForm::insertStudentsSet(StudentsSet* studentsSet, int row)
{
	QTableWidgetItem* newItem=new QTableWidgetItem(studentsSet->name);
	newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	tableWidget->setItem(row, 0, newItem);

	int nSubActivities=0;
	int nHours=0;
	
	if(allHours.contains(studentsSet->name))
		nHours=allHours.value(studentsSet->name);
	else
		assert(0);
		
	if(allActivities.contains(studentsSet->name))
		nSubActivities=allActivities.value(studentsSet->name);
	else
		assert(0);
		
	newItem=new QTableWidgetItem(CustomFETString::number(nSubActivities));
	newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	tableWidget->setItem(row, 1, newItem);

	newItem=new QTableWidgetItem(CustomFETString::number(nHours));
	newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	tableWidget->setItem(row, 2, newItem);
}

void StudentsStatisticsForm::on_helpPushButton_clicked()
{
	QString s;
	
	s+=tr("The check boxes '%1', '%2' and '%3': they permit you to show/hide information related to years, groups or subgroups")
	 .arg(tr("Show years"))
	 .arg(tr("Show groups"))
	 .arg(tr("Show subgroups"));
	
	s+="\n\n";
	
	s+=tr("The check box '%1': it has effect only if you have overlapping groups/years, and means that FET will show the complete tree structure"
	 ", even if that means that some subgroups/groups will appear twice or more in the table, with the same information."
	 " For instance, if you have year Y1, groups G1 and G2, subgroups S1, S2, S3, with structure: Y1 (G1 (S1, S2), G2 (S1, S3)),"
	 " S1 will appear twice in the table with the same information attached").arg(tr("Show duplicates"));
	
	LongTextMessageBox::largeInformation(this, tr("FET help"), s);
}
