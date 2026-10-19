#include "ppropertyparser/PPropertyReader.h"

#include "ppropertyparser/PProperties.h"
#include "toshi/kernel/TFile.h"
#include "toshi/kernel/TLocale.h"
#include "toshi/kernel/TSystem.h"

#include <new>

using namespace Toshi;

using Token = TFileLexerUTF8::Token;

TClass PPropertyReader::m_sClass("PPropertyReader", &TObject::m_sClass, CreateObject, CreateObjectInPlace,
                                 InitialiseStatic, DeinitialiseStatic, 0x10000);

namespace
{

bool HasString(TFileLexer::TokenType type)
{
	return type == TFileLexer::TOKEN_IDENT || type == TFileLexer::TOKEN_STRING;
}

} // namespace

// --- PPropertyReaderParseError --------------------------------------------------------------

PPropertyReaderParseError::PPropertyReaderParseError() : m_szFileName(nullptr), m_szMessage(nullptr)
{
}

PPropertyReaderParseError::PPropertyReaderParseError(const PPropertyReaderParseError& other) = default;

PPropertyReaderParseError::~PPropertyReaderParseError()
{
}

PPropertyReaderParseError& PPropertyReaderParseError::operator=(const PPropertyReaderParseError& other)
{
	m_oToken.assign(other.m_oToken);
	m_szFileName = other.m_szFileName;
	m_szMessage = other.m_szMessage;
	return *this;
}

// --- PPropertyReader ------------------------------------------------------------------------

PPropertyReader::PPropertyReader()
    : m_pFile(nullptr), m_pLexer(nullptr), m_oDefinitionBlocks(10), m_bLoadComments(false), m_bAssertOnError(true),
      m_oBlockLines(10, 0), m_oParseErrorEmitter(this)
{
}

PPropertyReader::~PPropertyReader()
{
	if (m_pFile != nullptr)
		TFile::Destroy(m_pFile);
	delete m_pLexer;
}

TClass& PPropertyReader::GetClass() const
{
	return m_sClass;
}

bool PPropertyReader::Open(const TCString& fileName)
{
	m_oFileName = static_cast<const char*>(fileName);
	Close();
	m_pFile = TFile::Create(fileName, 1);
	if (m_pFile == nullptr)
	{
		TCString message;
		Warning(message.Format("Can't open file", fileName.GetString(0)));
		return false;
	}
	m_pLexer = new TFileLexerUTF8(m_pFile, 2);
	m_pLexer->SetOutputComments(m_bLoadComments);
	m_oLexerListener.Connect(m_pLexer->GetParseErrorEmitter(), this, OnLexerParseError, 0);
	return true;
}

bool PPropertyReader::Open(const TCString& fileName, TFile* file)
{
	m_oFileName = static_cast<const char*>(fileName);
	Close();
	m_pFile = file;
	m_pLexer = new TFileLexerUTF8(m_pFile, 2);
	m_pLexer->SetOutputComments(m_bLoadComments);
	m_oLexerListener.Connect(m_pLexer->GetParseErrorEmitter(), this, OnLexerParseError, 0);
	return true;
}

void PPropertyReader::Close()
{
	if (m_pFile != nullptr)
	{
		TFile::Destroy(m_pFile);
		m_pFile = nullptr;
	}
	if (m_pLexer != nullptr)
	{
		delete m_pLexer;
		m_pLexer = nullptr;
	}
}

void PPropertyReader::AddDefinitionBlock(TManagedPtr<PPropertyBlock> block)
{
	m_oDefinitionBlocks.Insert(block->GetName(), block);
}

bool PPropertyReader::Load()
{
	Token token = m_pLexer->PeekNextToken(0);
	while (token.GetType() != TFileLexer::TOKEN_EOF)
	{
		TPCString name;
		if (!LoadBlockHeader(name))
			return false;
		const int index = m_oDefinitionBlocks.FindFirst(name);
		if (index == -1)
		{
			TCString message;
			Warning(token,
			        message.Format("Unknown block labelled '%s' found -- skipping", name.GetCString().GetString(0)));
			SkipBlock();
		}
		else
		{
			// The result is ignored.
			TManagedPtr<PPropertyBlock> block = m_oDefinitionBlocks[index].second;
			block->Load(this);
		}
		token = m_pLexer->PeekNextToken(0);
	}
	return true;
}

bool PPropertyReader::Load(const TCString& fileName, PProperties& properties)
{
	if (!Open(fileName))
		return false;
	if (m_pLexer->PeekNextToken(0).GetType() != TFileLexer::TOKEN_EOF)
	{
		m_oBlockLines.Resize(0);
		if (LoadProperty(&properties))
		{
			if (m_pLexer->PeekNextToken(0).GetType() == TFileLexer::TOKEN_EOF)
				return true;
			Error(TCString("Unexpected data at end of file!"));
		}
	}
	Close();
	return false;
}

bool PPropertyReader::LoadBlockHeader(TPCString& name)
{
	while (m_bLoadComments && m_pLexer->PeekNextToken(0).GetType() == TFileLexer::TOKEN_COMMENT)
		m_pLexer->GetNextToken();

	// The original builds these messages from UTF-16 literals.
	Token bracket;
	if (!m_pLexer->Expect(TFileLexer::TOKEN_OPENSQR, bracket))
	{
		Error(bracket, TCString("Block header not found (expected '[')"));
		return false;
	}
	Token identifier;
	if (!m_pLexer->Expect(TFileLexer::TOKEN_IDENT, identifier))
	{
		Error(identifier, TCString("Block header not found (expected identifier)"));
		return false;
	}
	if (!m_pLexer->Expect(TFileLexer::TOKEN_CLOSESQR, bracket))
	{
		Error(bracket, TCString("Block header not found (expected ']')"));
		return false;
	}
	name = identifier.GetString();
	return true;
}

bool PPropertyReader::LoadProperty(PProperties* properties)
{
	Token semicolon;
	// Quirk: only an assignment returns to `properties`; after "a.b" or "a;" or a nested set
	// the next property goes into the set last selected, as in the original.
	PProperties* current = properties;
	for (;;)
	{
		Token token = m_pLexer->PeekNextToken(0);
		if (token.GetType() == TFileLexer::TOKEN_CLOSEBRACE)
		{
			m_oBlockLines.Pop();
			m_pLexer->GetNextToken();
			return true;
		}
		if (token.GetType() == TFileLexer::TOKEN_EOF && m_oBlockLines.GetNumElements() != 0)
		{
			TCString message;
			Error(token, message.Format("Unexpected end of file in middle of property block (started at line %d)",
			                            m_oBlockLines[m_oBlockLines.GetNumElements() - 1]));
			m_oBlockLines.Pop();
			return false;
		}
		if (!HasString(token.GetType()) && token.GetType() != TFileLexer::TOKEN_COMMENT)
			return true;

		TPCString comment;
		TPCString name;
		TPCString subName;
		while (token.GetType() == TFileLexer::TOKEN_COMMENT)
		{
			comment = token.GetString();
			m_pLexer->GetNextToken();
			token = m_pLexer->PeekNextToken(0);
		}
		if (!LoadPropertyName(name, subName))
			return false;
		const PPropertyName propertyName(name, subName);

		const Token op = m_pLexer->GetNextToken();
		switch (op.GetType())
		{
		case TFileLexer::TOKEN_OPENBRACE:
		{
			m_oBlockLines.Push(op.GetLine());
			PProperties* child = new PProperties(current);
			current->PutProperty(propertyName, PPropertyValue(child), comment);
			if (!LoadProperty(child))
				return false;
			break;
		}
		case TFileLexer::TOKEN_DOT:
		{
			const PPropertyValue* value = current->GetProperty(propertyName);
			if (value == nullptr)
			{
				PProperties* child = new PProperties(current);
				current->PutProperty(propertyName, PPropertyValue(child), comment);
				current = child;
			}
			else if (value->GetProperties() != nullptr)
				current = value->GetProperties();
			else
			{
				// The original goes on with a null set and crashes ("a = 1; a.b = 2;").
				TCString message;
				Error(op, message.Format("Property '%s' is not a property set",
				                         propertyName.GetString().GetCString().GetString(0)));
				return false;
			}
			break;
		}
		case TFileLexer::TOKEN_SEMI:
			current->PutProperty(propertyName, PPropertyValue(), comment);
			break;
		case TFileLexer::TOKEN_EQUAL:
		{
			PPropertyValue value;
			if (!GetValue(value))
				return false;
			current->PutProperty(propertyName, value, comment);
			if (!m_pLexer->Expect(TFileLexer::TOKEN_SEMI, semicolon))
			{
				Error(semicolon, TCString("Expected semicolon at end of property assignment"));
				return false;
			}
			current = properties;
			break;
		}
		default:
		{
			TCString message;
			Error(op, message.Format("Unexpected operator after property '%s'",
			                         propertyName.GetString().GetCString().GetString(0)));
			return false;
		}
		}
	}
}

PProperties* PPropertyReader::LoadPropertyBlock()
{
	if (m_pLexer->PeekNextToken(0).GetType() == TFileLexer::TOKEN_EOF)
		return nullptr;
	PProperties* properties = new PProperties();
	m_oBlockLines.Resize(0);
	return LoadProperty(properties) ? properties : nullptr;
}

bool PPropertyReader::LoadPropertyBlock(PProperties& properties)
{
	if (m_pLexer->PeekNextToken(0).GetType() == TFileLexer::TOKEN_EOF)
		return false;
	m_oBlockLines.Resize(0);
	return LoadProperty(&properties);
}

bool PPropertyReader::LoadPropertyName(TPCString& name, TPCString& subName)
{
	const Token token = m_pLexer->GetNextToken();
	const Token next = m_pLexer->PeekNextToken(0);
	if (!HasString(token.GetType()))
	{
		Error(token, TCString("Expecting property name to be identifer"));
		return false;
	}
	name = token.GetString();
	if (next.GetType() != TFileLexer::TOKEN_OPENSQR)
		return true;

	m_pLexer->GetNextToken();
	const Token sub = m_pLexer->GetNextToken();
	Token bracket;
	if (!m_pLexer->Expect(TFileLexer::TOKEN_CLOSESQR, bracket))
	{
		Error(bracket, TCString("Expected close bracket after property sub-name"));
		return false;
	}
	if (HasString(sub.GetType()))
		subName = sub.GetString();
	else if (sub.GetType() == TFileLexer::TOKEN_INTEGER)
		subName = TSystem::GetCStringPool()->Get(sub.GetInteger());
	else
	{
		Error(sub, TCString("Expecting property subname to be identifier or integer"));
		return false;
	}
	return true;
}

bool PPropertyReader::GetValue(PPropertyValue& value)
{
	Token token = m_pLexer->PeekNextToken(0);
	if (token.GetType() == TFileLexer::TOKEN_OPENBRACE)
	{
		PPropertyValueArray* array = new PPropertyValueArray(3);
		m_pLexer->GetNextToken();
		for (;;)
		{
			if (m_pLexer->PeekNextToken(0).GetType() == TFileLexer::TOKEN_CLOSEBRACE)
			{
				m_pLexer->GetNextToken();
				break;
			}
			PPropertyValue& element = array->GetValues().Push(PPropertyValue());
			if (!GetValue(element))
			{
				array->Delete();
				return false;
			}
			const Token separator = m_pLexer->GetNextToken();
			if (separator.GetType() == TFileLexer::TOKEN_COMMA)
				continue;
			if (separator.GetType() != TFileLexer::TOKEN_CLOSEBRACE)
			{
				Error(separator, TCString("Expected a comma or close brace inside a list"));
				array->Delete();
				return false;
			}
			break;
		}
		value = array;
		return true;
	}

	if (token.GetType() == TFileLexer::TOKEN_DOLLAR)
	{
		m_pLexer->GetNextToken();
		token = m_pLexer->GetNextToken();
		if (token.GetType() == TFileLexer::TOKEN_IDENT)
			value = PPropertyValue(TSystem::GetLocale()->Get(token.GetString()));
		else if (token.GetType() == TFileLexer::TOKEN_INTEGER)
			value = PPropertyValue(TSystem::GetLocale()->Get(token.GetInteger()));
		else
		{
			Error(token, TCString("Expected identifier after '$' for localised string name"));
			return false;
		}
		return true;
	}

	if (m_pLexer->PeekNextToken(1).GetType() == TFileLexer::TOKEN_OPENSQR)
	{
		// Errors in the name are not checked, as in the original.
		TPCString name;
		TPCString subName;
		LoadPropertyName(name, subName);
		value = PPropertyValue(PPropertyName(name, subName));
		return true;
	}

	m_pLexer->GetNextToken();
	value = Token2Value(token);
	if (!value.IsDefined())
	{
		Error(token, TCString("Expected a value"));
		return false;
	}
	return true;
}

PPropertyValue PPropertyReader::Token2Value(const Token& token)
{
	// The original builds these from UTF-16 literals.
	static const TPCString s_true = TSystem::GetCStringPool()->Get(TCString("true"));
	static const TPCString s_false = TSystem::GetCStringPool()->Get(TCString("false"));

	switch (token.GetType())
	{
	case TFileLexer::TOKEN_INTEGER:
		return PPropertyValue(token.GetInteger());
	case TFileLexer::TOKEN_UINTEGER:
		return PPropertyValue(token.GetUInteger());
	case TFileLexer::TOKEN_FLOAT:
		return PPropertyValue(token.GetFloat());
	case TFileLexer::TOKEN_STRING:
		return PPropertyValue(token.GetString());
	case TFileLexer::TOKEN_IDENT:
		if (token.GetString() == s_true)
			return PPropertyValue(true);
		if (token.GetString() == s_false)
			return PPropertyValue(false);
		return PPropertyValue(token.GetString());
	default:
		return PPropertyValue();
	}
}

void PPropertyReader::Error(const TCString& message)
{
	PPropertyReaderParseError error;
	error.m_szFileName = m_oFileName.GetString(0);
	error.m_szMessage = message.GetString(0);
	m_oParseErrorEmitter.Throw(&error);
}

void PPropertyReader::Error(const Token& token, const TCString& message)
{
	PPropertyReaderParseError error;
	error.m_oToken.assign(token);
	error.m_szMessage = message.GetString(0);
	m_oParseErrorEmitter.Throw(&error);
}

void PPropertyReader::Warning(const TCString&)
{
}

void PPropertyReader::Warning(const Token& token, const TCString& message)
{
	PPropertyReaderParseError error;
	error.m_oToken.assign(token);
	error.m_szMessage = message.GetString(0);
	m_oParseErrorEmitter.Throw(&error);
}

TEmitter<PPropertyReader, PPropertyReaderParseError>* PPropertyReader::GetParseErrorEmitter()
{
	return &m_oParseErrorEmitter;
}

TFileLexerUTF8* PPropertyReader::GetLexer()
{
	return m_pLexer;
}

void PPropertyReader::SetAssertOnError(bool assertOnError)
{
	m_bAssertOnError = assertOnError;
}

void PPropertyReader::SetLoadComments(bool loadComments)
{
	m_bLoadComments = loadComments;
}

bool PPropertyReader::SkipBlock()
{
	Token token = m_pLexer->PeekNextToken(0);
	if (token.GetType() == TFileLexer::TOKEN_OPENSQR)
		return true;
	// Quirk: the brace depth carries over between statements, as in the original.
	int depth = 0;
	do
	{
		if (token.GetType() == TFileLexer::TOKEN_EOF)
			break;
		token = m_pLexer->GetNextToken();
		while ((token.GetType() != TFileLexer::TOKEN_SEMI || depth > 0) && token.GetType() != TFileLexer::TOKEN_EOF)
		{
			if (token.GetType() == TFileLexer::TOKEN_OPENBRACE)
				++depth;
			if (token.GetType() == TFileLexer::TOKEN_CLOSEBRACE)
				--depth;
			token = m_pLexer->GetNextToken();
		}
		token = m_pLexer->PeekNextToken(0);
	} while (token.GetType() != TFileLexer::TOKEN_OPENSQR);
	return true;
}

bool TSTDCALL PPropertyReader::OnLexerParseError(PPropertyReader* reader, TFileLexerUTF8*,
                                                 TFileLexerUTF8::ParseError* error)
{
	PPropertyReaderParseError event;
	event.m_oToken.assign(Token(error->m_iLine));
	event.m_szMessage = error->m_szMessage;
	reader->m_oParseErrorEmitter.Throw(&event);
	return true;
}

TObject* TSTDCALL PPropertyReader::CreateObject()
{
	return new PPropertyReader();
}

TObject* TSTDCALL PPropertyReader::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : ::new (memory) PPropertyReader();
}

void TSTDCALL PPropertyReader::InitialiseStatic()
{
}

void TSTDCALL PPropertyReader::DeinitialiseStatic()
{
}
