/***************************************************************************
                          addconstraintstudentsminhoursdailyform.cpp  -  description
                             -------------------
    begin                : July 19, 2007
    copyright            : (C) 2007 by Lalescu Liviu
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

#include <QMessageBox>

#include "longtextmessagebox.h"

#include "addconstraintstudentsminhoursdailyform.h"
#include "timeconstraint.h"

AddConstraintStudentsMinHoursDailyForm::AddConstraintStudentsMinHoursDailyForm(QWidget* parent): QDialog(parent)
{
	setupUi(this);

	addConstraintPushButton->setDefault(true);

	connect(addConstraintPushButton, SIGNAL(clicked()), this, SLOT(addCurrentConstraint()));
	connect(closePushButton, SIGNAL(clicked()), this, SLOT(close()));

	centerWidgetOnScreen(this);
	restoreFETDialogGeometry(this);
	
	if(ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS)
		allowLabel->setText(tr("Advanced usage: enabled"));
	else
		allowLabel->setText(tr("Advanced usage: not enabled"));
	
	minHoursSpinBox->setMinimum(1);
	minHoursSpinBox->setMaximum(gt.rules.nHoursPerDay);
	minHoursSpinBox->setValue(1);
}

AddConstraintStudentsMinHoursDailyForm::~AddConstraintStudentsMinHoursDailyForm()
{
	saveFETDialogGeometry(this);
}

void AddConstraintStudentsMinHoursDailyForm::addCurrentConstraint()
{
	TimeConstraint *ctr=nullptr;

	double weight;
	QString tmp=weightLineEdit->text();
	weight_sscanf(tmp, "%lf", &weight);
	if(weight<0.0 || weight>100.0){
		QMessageBox::warning(this, tr("FET information"),
			tr("Invalid weight (percentage)"));
		return;
	}
	if(weight!=100.0){
		QMessageBox::warning(this, tr("FET information"),
			tr("Invalid weight (percentage) - it has to be 100%"));
		return;
	}
	
	if(!ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS && allowEmptyDaysCheckBox->isChecked()){
		QMessageBox::warning(this, tr("FET warning"), tr("Empty days for students min hours daily constraints are not enabled. You must enable them from the Settings->Advanced menu."));
		return;
	}
	
	if(allowEmptyDaysCheckBox->isChecked() && minHoursSpinBox->value()<2){
		QMessageBox::warning(this, tr("FET warning"), tr("If you allow empty days, the min hours must be at least 2 (to make it a non-trivial constraint)"));
		return;
	}
	
	int minHours=minHoursSpinBox->value();

	ctr=new ConstraintStudentsMinHoursDaily(weight, minHours, allowEmptyDaysCheckBox->isChecked());

	bool tmp2=gt.rules.addTimeConstraint(ctr);
	if(tmp2)
		LongTextMessageBox::information(this, tr("FET information"),
			tr("Constraint added:")+"\n\n"+ctr->getDetailedDescription(gt.rules));
	else{
		QMessageBox::warning(this, tr("FET information"),
			tr("Constraint NOT added - please report error"));
		delete ctr;
	}
}

void AddConstraintStudentsMinHoursDailyForm::on_allowEmptyDaysCheckBox_toggled()
{
	bool k=allowEmptyDaysCheckBox->isChecked();
	
	if(k && !ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS){
		allowEmptyDaysCheckBox->setChecked(false);
		QString s=tr("Advanced usage is not enabled. To be able to select 'Allow empty days' for the constraints of type min hours daily for students, you must enable the option from the Settings->Advanced menu.",
			"'Allow empty days' is an option which the user can enable and then he can select it.");
		s+="\n\n";
		s+=tr("Explanation: only select this option if your institution allows empty days for students and a timetable is possible with empty days for students."
			" Otherwise, it is IMPERATIVE (for performance reasons) to not select this option (or FET may not be able to find a timetable).");
		s+="\n\n";
		s+=tr("Use with caution.");
		QMessageBox::information(this, tr("FET information"), s);
	}
}
