//module;

// do traditional includes for now....

#include <pjsua2.hpp>

#include <iostream>
#include <sstream>

#include <functional>

#include <cstdlib>
#include <cassert>


#include "Sipper.h"

#include <QDateTime>
#include <QSettings>


class Settings : public QSettings
{
public:
	std::string operator[] (const char *key)	{ return value (key).toString().toStdString(); }
};

namespace {
std::string Setting (const char *key)	{ return Settings() [key]; }
}

void OgmPlayer::onEof2()
{
	m_Call.OgmPhaseDone();
}



/*!	\brief	Call object
 */

Call::Call (pj::Account& acc, int call_id, Sipper& sipper)
:
	pj::Call 				{acc, call_id},
	m_Sipper			{sipper},
	m_AudDevMan 	{pj::Endpoint::instance().audDevManager()},
	m_PlayDevice	{m_AudDevMan.getPlaybackDevMedia()},
	m_OgmMessage	{*this},
	m_OgmBeep		{*this}
{
	Respond (PJSIP_SC_RINGING);

	m_Ringer.createPlayer (Setting ("Wavs/Ring"));
	m_Ringer.startTransmit (m_PlayDevice);
}

void Call::onCallState (pj::OnCallStateParam& prm)
{
	auto ci {getInfo()};

	std::cout << "call event " << prm.e.type << " state " << ci.stateText
		<< " reason " << ci.lastReason <<  std::endl;

	switch (ci.state)
	{
	case PJSIP_INV_STATE_DISCONNECTED:
		m_Sipper.emitStateEvent (StateEvent::DISCONNECTED);
		break;

	default:
		;
	}
}

void Call::onCallMediaState (pj::OnCallMediaStateParam& /*unused */)
{
	std::cout << "call media state" << std::endl;

	m_Ringer.stopTransmit (m_PlayDevice);

	m_CallMedia = getAudioMedia (-1);

	if (m_VoicemailMode)
	{
		m_OgmMessage.createPlayer (Setting ("Wavs/Ogm"), PJMEDIA_FILE_NO_LOOP);
		m_OgmMessage.startTransmit (m_CallMedia);
	}
	else
	{
		// Connect the call audio media to sound device
		m_CallMedia.startTransmit (m_PlayDevice);
		m_AudDevMan.getCaptureDevMedia().startTransmit (m_CallMedia);
	}
}

void Call::Accept()
{
	// if (voicemail is recording)
		/* ...delete recording and switch media streams... */
	// else

	m_VoicemailMode = false;
	Respond (PJSIP_SC_OK);
}

void Call::VoiceMail()
{
	// if (call already answered)
		/* ... switch media streams... */
	// else

	m_VoicemailMode = true;
	Respond (PJSIP_SC_OK);
}


void Call::OgmPhaseDone()
{
	switch (m_OgmPhase++)
	{
	case 0:
		m_OgmMessage.stopTransmit (m_CallMedia);
#if 0
		m_OgmBeep.createPlayer (Setting ("Wavs/Beep"), PJMEDIA_FILE_NO_LOOP);
		m_OgmBeep.startTransmit (m_CallMedia);

		break;

	case 1:
		m_OgmBeep.stopTransmit (m_CallMedia);
#endif
		{
		const auto& uri {getInfo().remoteUri};

		auto p1 {uri.find ('"')};
		auto p2 {uri.find ('"', p1 + 1)};
		auto number {uri.substr (p1 + 1, p2 - p1 - 1)};

		std::ostringstream foss;
		foss << Setting ("Wavs/RecordingDir") << "/sipper-" << number << '-' << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << ".wav";
		auto fname = foss.str();

		std::cout << "Recording to " << fname << std::endl;

		try
		{
			m_Recorder.createRecorder (fname);
			m_CallMedia.startTransmit (m_Recorder);
		}
		catch (const pj::Error& e)
		{
			std::cout << e.info (true) << std::endl;
		}
		}
		break;

	default:
		;
	}
}




void Call::Respond (pjsip_status_code code)
{
	m_Op.statusCode = code;

	try
	{
		answer (m_Op);
	}
	catch (const pj::Error& err)
	{
		std::cerr << "Call response failed: " << err.info() << std::endl;
	}
}



/*!	\brief		Account object
 */

void Account::onRegState (pj::OnRegStateParam& params)
{
	pj::AccountInfo ai = getInfo();
	if (ai.regIsActive)
	{
		if (!m_Registered)
		{
			m_Registered = true;
			m_Sipper.emitStateEvent (StateEvent::REGISTERED);
		}
	}
	else
		std::cout << "*** Unregister: code=" << params.code << std::endl;
}


void Account::onIncomingCall (pj::OnIncomingCallParam& prm)
{
	assert (!m_Call);

	m_Call = new Call (*this, prm.callId, m_Sipper);

	auto info {m_Call->getInfo()};

	std::ostringstream oss;
	oss << info.remoteUri;
	m_IncomingInfo  = oss.str();

	m_Sipper.emitStateEvent (StateEvent::INCOMING);
}



Sipper::Sipper()
{
	libCreate();

	// Initialize endpoint
	pj::EpConfig config;

	config.uaConfig.userAgent = "Sipper";
//		config.logConfig.msgLogging = PJ_TRUE;
		config.logConfig.level = 0;//PJ_LOG_MAX_LEVEL;
//		config.logConfig.consoleLevel = 0;//PJ_LOG_MAX_LEVEL;

//		config.medConfig.channelCount = 1;

	libInit (config);

	// Create SIP transport.
	pj::TransportConfig tcfg;
	tcfg.port = 5060;
	//    tcfg.port = 5061;
	try
	{
		transportCreate (PJSIP_TRANSPORT_UDP, tcfg);
		//        transportCreate (PJSIP_TRANSPORT_TLS, tcfg);
	}
	catch (const pj::Error& err)
	{
		std::cerr << err.info() << std::endl;
		throw;
	}

	// Configure an AccountConfig
	Settings settings;

	pj::AccountConfig acfg;
	acfg.idUri = std::string {"sip:"} + settings ["SIP/Id"] + "@" + settings ["SIP/Registrar"];
	acfg.regConfig.registrarUri = std::string {"sip:"} + settings ["SIP/Registrar"];
	pj::AuthCredInfo cred {"digest", "*", settings ["SIP/Id"], 0, settings ["SIP/Auth"]};		// config.id, config.auth
	acfg.sipConfig.authCreds.push_back (cred);

	try {
	// Create the account
	m_Account = std::make_unique<Account> (*this);


	m_Account->create (acfg);
	} catch (const pj::Error& e)
	{
		std::cout << acfg.idUri << '\n';
		std::cout << e.info() << std::endl;
		}
	// Start the library (worker threads etc)
	libStart();
}


Sipper::~Sipper()
{
	libDestroy();
}

