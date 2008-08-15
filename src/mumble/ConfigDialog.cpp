/* Copyright (C) 2005-2008, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ConfigDialog.h"
#include "AudioInput.h"
#include "AudioOutput.h"
#include "Global.h"

QMap<int, ConfigWidgetNew> *ConfigRegistrar::c_qmNew;

ConfigRegistrar::ConfigRegistrar(int priority, ConfigWidgetNew n) {
	if (! c_qmNew)
		c_qmNew = new QMap<int, ConfigWidgetNew>();
	iPriority = priority;
	c_qmNew->insert(priority,n);
}

ConfigRegistrar::~ConfigRegistrar() {
	c_qmNew->remove(iPriority);
}

ConfigWidget::ConfigWidget(Settings &st) : s(st) {
}

QIcon ConfigWidget::icon() const {
	return qApp->windowIcon();
}

void ConfigWidget::accept() const {
}

void ConfigWidget::loadSlider(QSlider *slider, int v) {
	if (v != slider->value()) {
		slider->setValue(v);
	} else {
		connect(this, SIGNAL(intSignal(int)), slider, SIGNAL(valueChanged(int)));
		emit intSignal(v);
		disconnect(SIGNAL(intSignal(int)));
	}
}

void ConfigWidget::loadCheckBox(QAbstractButton *c, bool v) {
	if (v != c->isChecked()) {
		c->setChecked(v);
	} else {
		connect(this, SIGNAL(intSignal(int)), c, SIGNAL(stateChanged(int)));
		emit intSignal(v ? 1 : 0);
		disconnect(SIGNAL(intSignal(int)));
	}
}

void ConfigWidget::loadComboBox(QComboBox *c, int v) {
	if (c->currentIndex() != v) {
		c->setCurrentIndex(v);
	} else {
		connect(this, SIGNAL(intSignal(int)), c, SIGNAL(currentIndexChanged(int)));
		emit intSignal(v);
		disconnect(SIGNAL(intSignal(int)));
	}
}

ConfigDialog::ConfigDialog(QWidget *p) : QDialog(p) {
	setupUi(this);

	s = g.s;

	QWidget *w;
	while ((w = qswPages->widget(0)))
		delete w;

	qcbExpert->setChecked(g.s.bExpert);

	unsigned int idx = 0;
	ConfigWidgetNew cwn;
	foreach(cwn, *ConfigRegistrar::c_qmNew) {
		QMessageBox::warning(NULL, tr("bLIPP"), tr("blopp"));
		addPage(cwn(s), ++idx);
	}

	on_qcbExpert_clicked(g.s.bExpert);

	QPushButton *okButton = dialogButtonBox->button(QDialogButtonBox::Ok);
	okButton->setToolTip(tr("Accept changes"));
	okButton->setWhatsThis(tr("This button will accept current settings and return to the application.<br />"
	                          "The settings will be stored to disk when you leave the application."));

	QPushButton *cancelButton = dialogButtonBox->button(QDialogButtonBox::Cancel);
	cancelButton->setToolTip(tr("Reject changes"));
	cancelButton->setWhatsThis(tr("This button will reject all changes and return to the application.<br />"
	                              "The settings will be reset to the previous positions."));

	QPushButton *applyButton = dialogButtonBox->button(QDialogButtonBox::Apply);
	applyButton->setToolTip(tr("Apply changes"));
	applyButton->setWhatsThis(tr("This button will immediately apply all changes."));

	QPushButton *resetButton = pageButtonBox->button(QDialogButtonBox::Reset);
	resetButton->setToolTip(tr("Undo changes for current page"));
	resetButton->setWhatsThis(tr("This button will revert any changes done on the current page to the most recent applied settings."));

	QPushButton *restoreButton = pageButtonBox->button(QDialogButtonBox::RestoreDefaults);
	restoreButton->setToolTip(tr("Restore defaults for current page"));
	restoreButton->setWhatsThis(tr("This button will restore the settings for the current page only to their defaults. Other pages will be not be changed.<br />"
	                               "To restore all settings to their defaults, you will have to use this button on every page."
	                              ));

}

void ConfigDialog::addPage(ConfigWidget *cw, unsigned int idx) {
	QDesktopWidget dw;

	int w = INT_MAX, h = INT_MAX;

	qWarning("Adding %s", qPrintable(cw->windowTitle()));


	for (int i=0;i<dw.numScreens();++i) {
		QRect ds=dw.availableGeometry(i);
		if (ds.isValid()) {
			w = qMin(w, ds.width());
			h = qMin(h, ds.height());
		}
	}

	QSize ms=cw->minimumSizeHint();
	cw->resize(ms);
	cw->setMinimumSize(ms);

	ms.rwidth() += 128;
	ms.rheight() += 192;
	if ((ms.width() > w) || (ms.height() > h)) {
		QScrollArea *qsa=new QScrollArea(this);
		qsa->setWidgetResizable(true);
		qsa->setWidget(cw);
		qswPages->addWidget(qsa);
		qhPages.insert(cw, qsa);
	} else {
		qswPages->addWidget(cw);
		qhPages.insert(cw, cw);
	}
	qmWidgets.insert(idx, cw);
	cw->load(g.s);
}

void ConfigDialog::on_qlwIcons_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
	if (!current)
		current = previous;

	if (current) {
		QWidget *w = qhPages.value(qhWidgets.value(current));
		if (w)
			qswPages->setCurrentWidget(w);
	}
}

void ConfigDialog::on_pageButtonBox_clicked(QAbstractButton *b) {
	ConfigWidget *conf = qobject_cast<ConfigWidget *>(qswPages->currentWidget());
	switch (pageButtonBox->standardButton(b)) {
		case QDialogButtonBox::RestoreDefaults: {
				Settings def;
				if (conf)
					conf->load(def);
				break;
			}
		case QDialogButtonBox::Reset: {
				if (conf)
					conf->load(g.s);
				break;
			}
		default:
			break;
	}
}

void ConfigDialog::on_dialogButtonBox_clicked(QAbstractButton *b) {
	switch (dialogButtonBox->standardButton(b)) {
		case QDialogButtonBox::Apply: {
				apply();
				break;
			}
		default:
			break;
	}
}

void ConfigDialog::on_qcbExpert_clicked(bool b) {
	ConfigWidget *ccw = qhWidgets.value(qlwIcons->currentItem());
	int ridx = -1;
	int row = -1;
	qhWidgets.clear();
	qlwIcons->clear();
	foreach(ConfigWidget *cw, qmWidgets) {
		bool showit = cw->expert(b);
		if (showit || b)  {
			++row;

			QListWidgetItem *i = new QListWidgetItem(qlwIcons);
			i->setIcon(cw->icon());
			i->setText(cw->title());
			i->setTextAlignment(Qt::AlignHCenter);
			i->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

			qhWidgets.insert(i, cw);

			if (cw == ccw)
				ridx = row;
		}
	}
	if (qlwIcons->count() > 0)
		qlwIcons->setCurrentRow(ridx >= 0 ? ridx : 0);
}

void ConfigDialog::apply() {
	foreach(ConfigWidget *cw, qmWidgets)
	cw->save();

	boost::weak_ptr<AudioInput> wai(g.ai);
	boost::weak_ptr<AudioOutput> wao(g.ao);

	g.ai.reset();
	g.ao.reset();

	while (! wai.expired() || ! wao.expired()) {
		// Where is QThread::yield() ?
	}

	g.s = s;

	foreach(ConfigWidget *cw, qmWidgets)
	cw->accept();

	g.ai = AudioInputRegistrar::newFromChoice(g.s.qsAudioInput);
	if (g.ai)
		g.ai->start(QThread::HighestPriority);
	g.ao = AudioOutputRegistrar::newFromChoice(g.s.qsAudioOutput);
	if (g.ao)
		g.ao->start(QThread::HighPriority);

	// They might have changed their keys.
	g.iPushToTalk = 0;
	g.iAltSpeak = 0;

	g.s.bExpert = qcbExpert->isChecked();
}

void ConfigDialog::accept() {
	apply();
	QDialog::accept();
}
