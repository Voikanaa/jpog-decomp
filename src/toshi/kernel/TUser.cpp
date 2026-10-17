#include "toshi/kernel/TUser.h"

#include "toshi/kernel/TSystem.h"

namespace Toshi
{

TUserHandler TUserHandler::ms_oSingleton;

// --- TUser ---------------------------------------------------------------------------------

TUser::TUser(const char* name)
    : m_szName(name), m_sNameLength(static_cast<short>(TSystem::StringLength(name))), m_sUserID(-1)
{
}

TUser::TUser(const TUser& other)
    : m_szName(other.m_szName), m_sNameLength(other.m_sNameLength), m_sUserID(other.m_sUserID)
{
}

TUser& TUser::operator=(const TUser& other)
{
	m_szName = other.m_szName;
	m_sNameLength = other.m_sNameLength;
	m_sUserID = other.m_sUserID;
	return *this;
}

short TUser::Register()
{
	return TUserHandler::GetSingleton().RegisterUser(*this);
}

void TUser::Deregister()
{
	TUserHandler::GetSingleton().DeregisterUser(*this);
}

bool TUser::IsRegistered()
{
	return m_sUserID != -1;
}

const char* TUser::GetName()
{
	return m_szName;
}

short TUser::GetNameLength()
{
	return m_sNameLength;
}

short TUser::GetUserID()
{
	return m_sUserID;
}

// --- TUserHandler --------------------------------------------------------------------------

TUserHandler::TUserHandler() : m_iUserCount(0)
{
	TSystem::MemSet(m_apUsers, 0, sizeof(m_apUsers));
}

TUserHandler::~TUserHandler()
{
}

TUserHandler& TUserHandler::operator=(const TUserHandler& other)
{
	m_iUserCount = other.m_iUserCount;
	for (int i = 0; i < 50; ++i)
		m_apUsers[i] = other.m_apUsers[i];
	return *this;
}

short TUserHandler::RegisterUser(TUser& user)
{
	if (m_iUserCount >= 50)
	{
		user.m_sUserID = -1;
		return -1;
	}
	const int slot = m_iUserCount++;
	user.m_sUserID = static_cast<short>(slot);
	m_apUsers[user.m_sUserID] = &user;
	return user.m_sUserID;
}

void TUserHandler::DeregisterUser(TUser& user)
{
	if (user.m_sUserID != -1)
		m_apUsers[user.m_sUserID] = nullptr;
	user.m_sUserID = -1;
}

TUser* TUserHandler::FindUser(const char* name)
{
	const int length = TSystem::StringLength(name);
	for (TUser* user : m_apUsers)
		if (user != nullptr && length == user->m_sNameLength &&
		    TSystem::StringCompareNoCase(user->m_szName, name, -1) == 0)
			return user;
	return nullptr;
}

TUser* TUserHandler::GetUser(int index)
{
	return m_apUsers[index];
}

int TUserHandler::GetUserCount()
{
	return m_iUserCount;
}

TUserHandler& TSTDCALL TUserHandler::GetSingleton()
{
	return ms_oSingleton;
}

} // namespace Toshi
