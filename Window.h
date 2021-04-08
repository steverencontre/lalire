#ifndef WINDOW_H
#define WINDOW_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QCoreApplication>
#include <QTimer>
#include <QSystemTrayIcon>

#include "Sipper.h"

class Window : public QDialog
{
	Q_OBJECT

public:
	Window();
	~Window() override {}

public slots:
	void onStateEvent (StateEvent e);

private slots:
	void onOkButton()			{ hide(); }
	void onAnswerButton();
	void onIgnoreButton()		{ hide(); }
	void onVoicemailButton();
	void onHangupButton();
	void onQuitButton()			{ QCoreApplication::instance()->quit();}

private:
	void setButtonStates (unsigned mask);

	Sipper				m_Sipper;

	QLabel			  *m_Label;

	QPushButton 	  *m_OkButton;
	QPushButton 	  *m_AnswerButton;
	QPushButton 	  *m_IgnoreButton;
	QPushButton 	  *m_VoicemailButton;
	QPushButton	  *m_HangupButton;
	QPushButton 	  *m_QuitButton;

	QTimer			  *m_VoicemailTimer;

	QSystemTrayIcon	*m_SysTrayIcon;
};


#endif // WINDOW_H
