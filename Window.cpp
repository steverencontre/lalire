
#include <iostream>
#include <chrono>

#include "Window.h"

#include <QMenu>

constexpr std::chrono::seconds VOICEMAIL_DELAY {20};

#include <QBoxLayout>

Window::Window()
{
	auto vlayout {new QVBoxLayout {this}};
	auto button_layout {new QHBoxLayout};

	m_Label = new QLabel {"Sipper registering..."};

	qRegisterMetaType<StateEvent>();
	connect (&m_Sipper, SIGNAL (sigStateEvent (StateEvent)), this, SLOT (onStateEvent (StateEvent)), Qt::QueuedConnection);

#define BUTTON(b)	\
	m_##b##Button = new QPushButton {#b};	\
	button_layout->addWidget (m_##b##Button);	\
	m_##b##Button->hide(); \
	connect (m_##b##Button, SIGNAL (clicked (bool)), this, SLOT (on##b##Button()))

	BUTTON (Ok);
	BUTTON (Answer);
	BUTTON (Ignore);
	BUTTON (Voicemail);
	BUTTON (Hangup);
	BUTTON (Quit);

	m_Label->show();
	m_QuitButton->show();

	vlayout->addWidget (m_Label);
	vlayout->addLayout (button_layout);

	m_VoicemailTimer = new QTimer {this};
	m_VoicemailTimer->setSingleShot (true);
	m_VoicemailTimer->callOnTimeout (this, &Window::onVoicemailButton);

	m_SysTrayIcon = new QSystemTrayIcon (QIcon::fromTheme ("call-start"), this);

	auto tray_menu {new QMenu {this}};
	tray_menu->addAction ("&Quit", this, &Window::onQuitButton);

	m_SysTrayIcon->setContextMenu (tray_menu);
	m_SysTrayIcon->show();
}




void Window::onStateEvent (StateEvent e)
{
	switch (e)
	{
	case StateEvent::REGISTERED:
		m_Label->setText ("Registered successfully");
		setButtonStates (0b100001);

		m_OkButton->setFocus();
		break;

	case StateEvent::INCOMING:
		m_Label->setText (QString::fromStdString (m_Sipper.getAccount().IncomingInfo()));
		setButtonStates (0b011101);

		m_AnswerButton->setFocus();
		m_VoicemailTimer->start (VOICEMAIL_DELAY);
		show();
		break;

	case StateEvent::DISCONNECTED:
		m_Sipper.getAccount().onDisconnected();
		m_VoicemailButton->setText ("Voicemail");
		m_VoicemailButton->setStyleSheet ("");
		hide();
		break;
	}
}

void Window::onAnswerButton()
{
	setButtonStates (0b000011);
	m_VoicemailTimer->stop();

	m_Sipper.getCall().Accept();
}

void Window::onVoicemailButton()
{
	setButtonStates (0b110111);
	m_VoicemailTimer->stop();

	m_Sipper.getCall().VoiceMail();

	m_VoicemailButton->setText ("Recording...");
	m_VoicemailButton->setStyleSheet ("color: red");

	std::cout << "to voicemail...\n";

	// ...start recording...
}


void Window::onHangupButton()
{
	setButtonStates (0b000001);
	m_VoicemailTimer->stop();

	m_Sipper.getCall().Hangup();
}


void Window::setButtonStates (unsigned mask)
{
	m_OkButton->setVisible				(mask & 0x20);
	m_AnswerButton->setVisible		(mask & 0x10);
	m_IgnoreButton->setVisible		(mask & 0x8);
	m_VoicemailButton->setVisible	(mask & 0x4);
	m_HangupButton->setVisible		(mask & 0x2);
	m_QuitButton->setVisible			(mask & 0x1);
}
