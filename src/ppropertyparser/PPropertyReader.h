#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "ppropertyparser/PPropertyBlock.h"
#include "ppropertyparser/PPropertyValue.h"
#include "toshi/kernel/TArray.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TEvent.h"
#include "toshi/kernel/TFileLexer.h"
#include "toshi/kernel/TManagedPtr.h"
#include "toshi/kernel/TMap.h"
#include "toshi/kernel/TObject.h"

namespace Toshi
{
class TFile;
}

class PProperties;

// A reader or lexer error, thrown through PPropertyReader's parse error emitter. Warnings use
// the same event.
class PPropertyReaderParseError
{
public:
	PPropertyReaderParseError();
	PPropertyReaderParseError(const PPropertyReaderParseError& other);
	~PPropertyReaderParseError();
	PPropertyReaderParseError& operator=(const PPropertyReaderParseError& other);

	// The offending token; EOF at line 0 for errors without one, and EOF at the error line for
	// lexer errors.
	Toshi::TFileLexerUTF8::Token m_oToken;
	// Only set by Error(message); the original leaves it uninitialised otherwise (null here).
	const char* m_szFileName;
	const char* m_szMessage;
};

JPOG_ASSERT_SIZE(PPropertyReaderParseError, 0x14);

// Parser for the text property format (PPropertyParser.dll):
//   [Block]                      block header, dispatched to a definition block by name
//   // comment                   kept as the next property's comment when comments are loaded
//   name = value;                values: numbers, strings, identifiers (true/false are bools),
//   name[sub] = value;           $id / $123 (localised strings), name[sub] (property names)
//   name { ... }                 and { a, b, ... } (arrays, trailing comma allowed)
//   a.b.c = value;               nested sets; "name;" is a property without a value
class PPropertyReader : public Toshi::TObject
{
public:
	PPropertyReader();
	~PPropertyReader() override;

	Toshi::TClass& GetClass() const override;
	// Opens the file and a lexer on it. Declared in this order for the original vtable order
	// (MSVC places overloads in reverse): the TFile overload is slot 3. It takes the file over.
	virtual bool Open(const Toshi::TCString& fileName);
	virtual bool Open(const Toshi::TCString& fileName, Toshi::TFile* file);
	void Close();

	void AddDefinitionBlock(Toshi::TManagedPtr<PPropertyBlock> block);
	// Reads block after block, handing each to its definition (unknown blocks are skipped
	// with a warning) until the end of the input.
	bool Load();
	// Opens the file and reads one property set from it; closes it again on failure.
	bool Load(const Toshi::TCString& fileName, PProperties& properties);
	// "[name]", after any comments when they are loaded.
	bool LoadBlockHeader(Toshi::TPCString& name);
	// Reads properties into `properties` until a '}' (consumed), the end of the input or a
	// token that cannot start a property.
	bool LoadProperty(PProperties* properties);
	// A new set read with LoadProperty, or null at the end of the input or on failure (the
	// set is leaked then, as in the original).
	PProperties* LoadPropertyBlock();
	bool LoadPropertyBlock(PProperties& properties);
	// "name" or "name[sub]"; the sub-name may be an identifier, a string or an integer.
	bool LoadPropertyName(Toshi::TPCString& name, Toshi::TPCString& subName);
	bool GetValue(PPropertyValue& value);
	// The value of a number, string or identifier token; undefined for other tokens.
	PPropertyValue Token2Value(const Toshi::TFileLexerUTF8::Token& token);

	void Error(const Toshi::TCString& message);
	void Error(const Toshi::TFileLexerUTF8::Token& token, const Toshi::TCString& message);
	// Does nothing.
	void Warning(const Toshi::TCString& message);
	void Warning(const Toshi::TFileLexerUTF8::Token& token, const Toshi::TCString& message);

	Toshi::TEmitter<PPropertyReader, PPropertyReaderParseError>* GetParseErrorEmitter();
	Toshi::TFileLexerUTF8* GetLexer();
	// Stored only.
	void SetAssertOnError(bool assertOnError);
	// Takes effect with the next Open.
	void SetLoadComments(bool loadComments);

	static Toshi::TClass m_sClass;

private:
	using BlockMap = Toshi::TMap<Toshi::TPCString, Toshi::TManagedPtr<PPropertyBlock>, Toshi::TPCString::Comparator>;

	// Skips statements up to the next block header.
	bool SkipBlock();

	static bool TSTDCALL OnLexerParseError(PPropertyReader* reader, Toshi::TFileLexerUTF8* lexer,
	                                       Toshi::TFileLexerUTF8::ParseError* error);

	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	Toshi::TCString m_oFileName;
	Toshi::TFile* m_pFile;
	Toshi::TFileLexerUTF8* m_pLexer;
	BlockMap m_oDefinitionBlocks;
	bool m_bLoadComments;
	bool m_bAssertOnError;
	// Start lines of the open '{' sets, for the end-of-file error.
	Toshi::TArray<int> m_oBlockLines;
	Toshi::TEmitter<PPropertyReader, PPropertyReaderParseError> m_oParseErrorEmitter;
	Toshi::TListener<PPropertyReader, Toshi::TFileLexerUTF8, Toshi::TFileLexerUTF8::ParseError> m_oLexerListener;
};

JPOG_ASSERT_SIZE(PPropertyReader, 0x58);
