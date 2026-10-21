#include "ppropertyparser/PPropertyWriter.h"

#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyName.h"
#include "ppropertyparser/PPropertyValue.h"
#include "toshi/kernel/TLocale.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/strings/TWStringPool.h"

#include <cstdarg>
#include <new>

using namespace Toshi;

TClass PPropertyWriter::m_sClass("PPropertyWriter", &TObject::m_sClass, CreateObject, CreateObjectInPlace,
                                 InitialiseStatic, DeinitialiseStatic, 0x10000);

namespace
{

const unsigned short kCantSave[] = {'C', 'a', 'n', '\'', 't', ' ', 's', 'a', 'v', 'e', ':', ' ', 0};

// The C locale's isalpha/isalnum. The original passes sign-extended chars, so characters
// from 0x80 are never letters.
bool IsAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsAlnum(char c)
{
	return IsAlpha(c) || (c >= '0' && c <= '9');
}

bool IsIdentifierChar(char c)
{
	return static_cast<signed char>(c) >= 0 && (IsAlnum(c) || c == '_');
}

// Orders blocks by priority by subtraction, as the original.
int ComparePriority(const TManagedPtr<PPropertyBlock>& key, const TManagedPtr<PPropertyBlock>& element)
{
	const int difference = static_cast<int>(static_cast<unsigned int>(key->GetPriority()) -
	                                        static_cast<unsigned int>(element->GetPriority()));
	if (difference < 0)
		return -1;
	return key->GetPriority() == element->GetPriority() ? 0 : 1;
}

} // namespace

PPropertyWriter::PPropertyWriter()
    : m_pFile(nullptr), m_oDefinitionBlocks(10), m_pWriteBuffer(new char[kWriteBufferSize]), m_iWriteBufferCount(0),
      m_bPrintUnsignedAsHex(true)
{
}

PPropertyWriter::~PPropertyWriter()
{
	if (m_pFile != nullptr)
	{
		FlushWriteBuffer();
		TFile::Destroy(m_pFile);
	}
	delete[] m_pWriteBuffer;
}

TClass& PPropertyWriter::GetClass() const
{
	return m_sClass;
}

bool PPropertyWriter::Open(const TWString& fileName)
{
	if (m_pFile != nullptr)
	{
		FlushWriteBuffer();
		TFile::Destroy(m_pFile);
	}
	m_pFile = TFile::Create(TCString(fileName), 10);
	if (m_pFile != nullptr)
		return true;
	Error(TWString(kCantSave) + fileName.GetString(0));
	return false;
}

bool PPropertyWriter::Open(const TWString& fileName, TFile* file)
{
	if (m_pFile != nullptr)
	{
		FlushWriteBuffer();
		TFile::Destroy(m_pFile);
	}
	m_pFile = file;
	if (m_pFile != nullptr)
		return true;
	Error(TWString(kCantSave) + fileName.GetString(0));
	return false;
}

void PPropertyWriter::Close()
{
	if (m_pFile != nullptr)
	{
		FlushWriteBuffer();
		TFile::Destroy(m_pFile);
	}
	m_pFile = nullptr;
}

bool PPropertyWriter::IsFileValid()
{
	return m_pFile != nullptr;
}

void PPropertyWriter::AddDefinitionBlock(TManagedPtr<PPropertyBlock> block)
{
	bool found = false;
	const int index = m_oDefinitionBlocks.Search(block, ComparePriority, found);
	m_oDefinitionBlocks.InsertSlots(index, 1);
	m_oDefinitionBlocks[index] = block;
}

bool PPropertyWriter::Save()
{
	for (int i = 0; i < m_oDefinitionBlocks.count; ++i)
		if (!m_oDefinitionBlocks[i]->Save(this))
			return false;
	return true;
}

void PPropertyWriter::Error(const TWString& message)
{
	message.Print();
}

void PPropertyWriter::PrintUnsignedAsHex(bool hex)
{
	m_bPrintUnsignedAsHex = hex;
}

bool PPropertyWriter::PutBlockHeader(const TPCString& name)
{
	const TCString text(name.GetCString().GetString(0));
	return BufferCPrintf("\n[%s]\n", text.GetString(0)) > 0;
}

bool PPropertyWriter::PutPropertyBlock(int indent, const PProperties* properties)
{
	for (auto it = properties->Begin(); it != properties->End(); ++it)
	{
		const PPropertyValue& value = *it->GetValue();
		if (!Put('\t', indent) || !Put(it->GetName()))
			return false;
		if (value.IsType(PPropertyValue::TYPE_PROPS))
		{
			if (BufferCPrintf(" {\n") < 0 || !PutPropertyBlock(indent + 1, value.GetProperties()) ||
			    !Put('\t', indent) || BufferCPrintf("}\n") < 0)
				return false;
		}
		else if (value.IsType(PPropertyValue::TYPE_UNDEF))
		{
			if (BufferCPrintf(";\n") < 0)
				return false;
		}
		else if (BufferCPrintf(" = ") < 0 || !Put(value) || BufferCPrintf(";\n") < 0)
			return false;
	}
	return true;
}

bool PPropertyWriter::PutPropertyBlock(const PProperties* properties)
{
	return PutPropertyBlock(0, properties);
}

bool PPropertyWriter::PutPropertyName(const TPCString& name, const TPCString& subName)
{
	if (!Put(name))
		return false;
	if (subName.GetCString().IsEmpty())
		return true;
	return BufferCPrintf("[") >= 0 && Put(subName) && BufferCPrintf("]") >= 0;
}

bool PPropertyWriter::Put(const PPropertyName& name)
{
	return PutPropertyName(name.GetName(), name.GetSubName());
}

bool PPropertyWriter::Put(const PPropertyValue& value)
{
	const TClass* type = value.GetType();
	if (type == PPropertyValue::TYPE_INT)
		return Put(value.GetInteger());
	if (type == PPropertyValue::TYPE_UINT32)
		return Put(value.GetUINT32());
	if (type == PPropertyValue::TYPE_FLOAT)
		return Put(value.GetFloat());
	if (type == PPropertyValue::TYPE_BOOL)
		return Put(value.GetBoolean());
	if (type == PPropertyValue::TYPE_TPCSTRING)
		return Put(value.GetTPCString());
	if (type == PPropertyValue::TYPE_TPWSTRING)
		return Put(value.GetTPWString());
	if (type == PPropertyValue::TYPE_TLSTRING)
		return Put(value.GetTLString());
	if (type == PPropertyValue::TYPE_PROPNAME)
		return Put(value.GetPropertyName());
	if (type != PPropertyValue::TYPE_ARRAY)
		return false;

	if (!PutToken(TFileLexer::TOKEN_OPENBRACE))
		return false;
	const TArray<PPropertyValue>& values = value.GetArray()->GetValues();
	for (int i = 0; i < values.GetNumElements(); ++i)
		if ((i > 0 && !PutToken(TFileLexer::TOKEN_COMMA)) || !Put(values[i]))
			return false;
	return PutToken(TFileLexer::TOKEN_CLOSEBRACE);
}

bool PPropertyWriter::Put(const TCString& string)
{
	// Write errors are ignored.
	const bool quotes = string_needs_quotes(string.GetString(0), string.Length());
	if (quotes)
		BufferCChar('"');
	const char* text = string;
	for (int i = 0; i < string.Length(); ++i)
	{
		if (text[i] == '"' || text[i] == '\\')
			BufferCChar('\\');
		BufferCChar(text[i]);
	}
	if (quotes)
		BufferCChar('"');
	return true;
}

bool PPropertyWriter::Put(const TLString& string)
{
	if (string.IsTemporary())
		return Put(TCString(string.GetLocalisedString()));
	if (!string.GetIdentifierString().IsEmpty())
		return PutToken(TFileLexer::TOKEN_DOLLAR) && Put(string.GetIdentifierString());
	// The original reads the id through a null string (crash); we stop after the '$'.
	if (!PutToken(TFileLexer::TOKEN_DOLLAR) || string.m_pString == nullptr)
		return false;
	return Put(string.m_pString->GetIdentifierInt());
}

bool PPropertyWriter::Put(const TPCString& string)
{
	return Put(string.GetCString());
}

bool PPropertyWriter::Put(const TPWString& string)
{
	return Put(TCString(string.GetWString().GetString(0)));
}

bool PPropertyWriter::Put(char character, int count)
{
	for (; count > 0; --count)
		if (BufferCChar(character) < 0)
			return false;
	return true;
}

bool PPropertyWriter::Put(int value)
{
	return BufferCPrintf("%d", value) >= 0;
}

bool PPropertyWriter::Put(unsigned int value)
{
	return BufferCPrintf(m_bPrintUnsignedAsHex ? "0x%08Xu" : "%du", value) >= 0;
}

bool PPropertyWriter::Put(float value)
{
	return BufferCPrintf("%gf", static_cast<double>(value)) >= 0;
}

bool PPropertyWriter::Put(const char* string)
{
	// Write errors are ignored.
	const bool quotes = string_needs_quotes(string, -1);
	if (quotes)
		BufferCChar('"');
	for (; *string != '\0'; ++string)
	{
		if (*string == '"' || *string == '\\')
			BufferCChar('\\');
		BufferCChar(*string);
	}
	if (quotes)
		BufferCChar('"');
	return true;
}

bool PPropertyWriter::Put(bool value)
{
	return BufferCPrintf(value ? "true" : "false") >= 0;
}

bool PPropertyWriter::PutToken(TFileLexer::TokenType token)
{
	static const char kCharacters[] = {0, ';', ':', ',', '.', '*', '$', '[', ']', '{', '}', '(', ')', '<', '>', '='};
	if (token < TFileLexer::TOKEN_SEMI || token > TFileLexer::TOKEN_EQUAL)
		return false;
	return BufferCChar(kCharacters[token]) != -1;
}

bool PPropertyWriter::PutLiteral(const char* text)
{
	const TCString literal(text);
	return BufferCPrintf("%s", literal.GetString(0)) >= 0;
}

bool PPropertyWriter::PutNewLine()
{
	return BufferCPrintf("\n") >= 0;
}

bool PPropertyWriter::PutEndLine()
{
	return BufferCPrintf(";\n") >= 0;
}

bool PPropertyWriter::PutSpace()
{
	return BufferCPrintf("\t") >= 0;
}

int PPropertyWriter::BufferCChar(const char& character)
{
	int result = static_cast<signed char>(character);
	if (m_iWriteBufferCount >= kWriteBufferSize)
		result = FlushWriteBuffer();
	m_pWriteBuffer[m_iWriteBufferCount++] = character;
	return result;
}

int __cdecl PPropertyWriter::BufferCPrintf(const char* format, ...)
{
	FlushWriteBuffer();
	va_list args;
	va_start(args, format);
	const int result = m_pFile->VCPrintf(format, args);
	va_end(args);
	return result;
}

int PPropertyWriter::FlushWriteBuffer()
{
	if (m_iWriteBufferCount <= 0)
		return -1;
	const int result = m_pFile->Write(m_pWriteBuffer, m_iWriteBufferCount);
	m_iWriteBufferCount = 0;
	return result;
}

bool PPropertyWriter::string_needs_quotes(const char* string, int length) const
{
	if (length == 0 || (length == -1 && *string == '\0'))
		return true;
	if (!IsAlpha(*string))
		return true;
	if (TSystem::StringCompare(string, "true", -1) == 0 || TSystem::StringCompare(string, "false", -1) == 0)
		return true;
	if (length == -1)
	{
		for (; *string != '\0'; ++string)
			if (!IsIdentifierChar(*string))
				return true;
		return false;
	}
	for (; length > 0; --length, ++string)
		if (!IsIdentifierChar(*string))
			return true;
	return false;
}

TObject* TSTDCALL PPropertyWriter::CreateObject()
{
	return new PPropertyWriter();
}

TObject* TSTDCALL PPropertyWriter::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : ::new (memory) PPropertyWriter();
}

void TSTDCALL PPropertyWriter::InitialiseStatic()
{
}

void TSTDCALL PPropertyWriter::DeinitialiseStatic()
{
}
