#ifndef SIPPER_H
#define SIPPER_H


#include <memory>

#include <pjsua2.hpp>

#include <QObject>

class Sipper; // forward refs
class Call;

enum class StateEvent
{
	REGISTERED,
	INCOMING,
	DISCONNECTED
};

Q_DECLARE_METATYPE (StateEvent)



/*!	\brief		Outgoing Message Player
 *
 *		\remark	Plays a wav file, then starts recording at end
 */
class OgmPlayer : public pj::AudioMediaPlayer
{
public:
	OgmPlayer (Call& call) : m_Call {call} {}
	void onEof2() override;

private:
	Call&		m_Call;
	int			m_Stage {0};
};


/*!	\brief	Call object
 */
class Call : public pj::Call
{
public:
	Call (pj::Account& acc, int call_id, Sipper& sipper);

	void onCallState (pj::OnCallStateParam& prm) override;

	void onCallMediaState (pj::OnCallMediaStateParam& /*unused */) override;


	void Accept();
	void Hangup()	{ m_Op.statusCode = PJSIP_SC_DECLINE; pj::Call::hangup (m_Op); }
	void VoiceMail();
	void OgmPhaseDone();

private:
	void Respond (pjsip_status_code code);

	Sipper&							m_Sipper;

	pj::AudDevManager&		m_AudDevMan;
	pj::AudioMedia&				m_PlayDevice;
	pj::AudioMedia				m_CallMedia;
	pj::CallOpParam				m_Op;
	pj::AudioMediaPlayer		m_Ringer;
	pj::AudioMediaRecorder	m_Recorder;
	OgmPlayer						m_OgmMessage;
	OgmPlayer						m_OgmBeep;
	int									m_OgmPhase {0};

	bool								m_VoicemailMode;
};


/*!	\brief		Account object
 */
class Account : public pj::Account
{
public:
	Account (Sipper& sipper) : m_Sipper {sipper} {}

	void onRegState (pj::OnRegStateParam& params) override;

	void onIncomingCall (pj::OnIncomingCallParam& prm) override;

	const std::string& IncomingInfo() const { return m_IncomingInfo; }

	Call& getCall()	const
	{
		if (!m_Call)
			throw std::runtime_error ("no call to get");
		return *m_Call;
	}

	void onDisconnected()
	{
		delete m_Call;
		m_Call = nullptr;
	}


private:
	Sipper&		  m_Sipper;

	Call 		  *m_Call {nullptr};
	bool			m_Registered {false};

	std::string			m_IncomingInfo;

};



class Sipper :  public QObject, public pj::Endpoint
{
	Q_OBJECT

public:
	Sipper();
	~Sipper() override;

	Account&	getAccount()	{ return *m_Account; }
	Call&			getCall()			{ return m_Account->getCall(); }

	void emitStateEvent (StateEvent e)		{ emit sigStateEvent (e); }

signals:
	void		sigStateEvent (StateEvent e);

private:

	std::unique_ptr<Account>	m_Account;

};


#endif // SIPPER_H
