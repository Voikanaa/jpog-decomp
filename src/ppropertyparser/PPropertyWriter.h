#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "ppropertyparser/PPropertyBlock.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/containers/SortedArray.h"
#include "toshi/kernel/containers/TManagedPtr.h"
#include "toshi/kernel/file/TFileLexer.h"
#include "toshi/kernel/strings/TCString.h"
#include "toshi/kernel/strings/TWString.h"

namespace Toshi
{
class TFile;
class TLString;
class TPWString;
} // namespace Toshi

class PProperties;
class PPropertyName;
class PPropertyValue;

// Writer for the text property format read by PPropertyReader (PPropertyParser.dll).
// Characters are buffered (16 KB); every printf-style write flushes the buffer first and then
// prints straight to the file. Strings are quoted unless they are plain identifiers (and not
// true/false). Comments are not written.
class PPropertyWriter : public Toshi::TObject
{
public:
	PPropertyWriter();
	~PPropertyWriter() override;

	Toshi::TClass& GetClass() const override;

	// Flushes and destroys the current file first. The TFile overload takes the file over.
	bool Open(const Toshi::TWString& fileName);
	bool Open(const Toshi::TWString& fileName, Toshi::TFile* file);
	void Close();
	bool IsFileValid();
	// Definition blocks are saved in priority order (equal priorities in insertion order).
	void AddDefinitionBlock(Toshi::TManagedPtr<PPropertyBlock> block);
	// Calls Save on each definition block; stops at the first failure.
	bool Save();
	// Prints the message (debug output).
	void Error(const Toshi::TWString& message);
	// Unsigned values as 0x%08Xu (the default) or decimal.
	void PrintUnsignedAsHex(bool hex);

	bool PutBlockHeader(const Toshi::TPCString& name);
	// The properties at `indent` tabs, sets as "name {" ... "}" blocks.
	bool PutPropertyBlock(int indent, const PProperties* properties);
	bool PutPropertyBlock(const PProperties* properties);
	bool PutPropertyName(const Toshi::TPCString& name, const Toshi::TPCString& subName);
	bool Put(const PPropertyName& name);
	// False for types that cannot be written (objects).
	bool Put(const PPropertyValue& value);
	bool Put(const Toshi::TCString& string);
	bool Put(const Toshi::TLString& string);
	bool Put(const Toshi::TPCString& string);
	bool Put(const Toshi::TPWString& string);
	// `count` copies of the character.
	bool Put(char character, int count);
	bool Put(int value);
	// Quirk: decimal values are printed with %d, so values from 2^31 come out negative.
	bool Put(unsigned int value);
	bool Put(float value);
	bool Put(const char* string);
	bool Put(bool value);
	bool PutToken(Toshi::TFileLexer::TokenType token);
	bool PutLiteral(const char* text);
	bool PutNewLine();
	bool PutEndLine();
	bool PutSpace();

	static Toshi::TClass m_sClass;

protected:
	// The character's value, or the flush result when the buffer was full. Quirk: characters
	// from 0x80 are negative, which the callers take for a failure.
	int BufferCChar(const char& character);
	int BufferCPrintf(const char* format, ...);
	// The file's Write result, or -1 when the buffer is empty.
	int FlushWriteBuffer();
	// `length` -1 means NUL-terminated.
	bool string_needs_quotes(const char* string, int length) const;

private:
	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	static constexpr int kWriteBufferSize = 0x4000;

	Toshi::TFile* m_pFile;
	jpog::kernel::SortedArray<Toshi::TManagedPtr<PPropertyBlock>> m_oDefinitionBlocks;
	char* m_pWriteBuffer;
	int m_iWriteBufferCount;
	bool m_bPrintUnsignedAsHex;
};

JPOG_ASSERT_SIZE(PPropertyWriter, 0x24);
