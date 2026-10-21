#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/TRefCounted.h"
#include "toshi/kernel/strings/TCStringPool.h"

class PPropertyReader;
class PPropertyWriter;

// Handler for a "[Name]" block of a property file (PPropertyParser.dll). Readers and writers
// hold definitions by name; the reader calls Load for each block header it finds.
// Reference-counted (TManagedPtr).
class PPropertyBlock : public Toshi::TObject, public Toshi::TRefCounted
{
public:
	// The engine passes 1 and 3; the names of the values are not exported.
	enum Priority
	{
	};

	PPropertyBlock(const Toshi::TPCString& name, Priority priority);
	~PPropertyBlock() override;

	Toshi::TClass& GetClass() const override;
	// Reads the block body (after the header) from the reader's lexer.
	virtual bool Load(PPropertyReader* reader) = 0;
	virtual bool Save(PPropertyWriter* writer) = 0;

	const Toshi::TPCString& GetName() const;
	void SetName(const Toshi::TPCString& name);
	Priority GetPriority() const;

	static Toshi::TClass m_sClass;

private:
	// Abstract: both return null.
	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	Toshi::TPCString m_oName;
	Priority m_ePriority;
};

JPOG_ASSERT_SIZE(PPropertyBlock, 0x10);
