#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

// A named client of the engine (TKernelInterface.dll), registered in a fixed table of 50.
class TUser
{
public:
	// `name` is not copied.
	explicit TUser(const char* name);
	TUser(const TUser& other);
	TUser& operator=(const TUser& other);

	// Returns the slot (the user ID), or -1 when the table is full.
	short Register();
	void Deregister();
	bool IsRegistered();
	const char* GetName();
	short GetNameLength();
	short GetUserID();

private:
	friend class TUserHandler;

	const char* m_szName;
	short m_sNameLength;
	short m_sUserID; // -1 when not registered
};

JPOG_ASSERT_SIZE(TUser, 8);

class TUserHandler
{
public:
	TUserHandler& operator=(const TUserHandler& other);

	short RegisterUser(TUser& user);
	void DeregisterUser(TUser& user);
	// Case-insensitive.
	TUser* FindUser(const char* name);
	TUser* GetUser(int index);
	// Registrations so far (slots are not reused).
	int GetUserCount();

	static TUserHandler& TSTDCALL GetSingleton();

protected:
	TUserHandler();
	~TUserHandler();

private:
	static TUserHandler ms_oSingleton;

	int m_iUserCount;
	TUser* m_apUsers[50];
};

JPOG_ASSERT_SIZE(TUserHandler, 0xCC);

} // namespace Toshi
