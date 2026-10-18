#include "support/Differential.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TDebug.h"
#include "toshi/kernel/TFile.h"
#include "toshi/kernel/TNullFile.h"

#include <doctest/doctest.h>

using Toshi::TDebug;

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TDebug")
{
	TEST_CASE("debug file")
	{
		Toshi::TFileManager manager;
		Toshi::TNullFileSystem system("null");
		CHECK_FALSE(TDebug::IsDebugFileEnabled());
		CHECK(TDebug::OpenDebugFile(const_cast<char*>("null:debug.txt")));
		CHECK(TDebug::IsDebugFileEnabled());
		auto* file = static_cast<Toshi::TNullFile*>(TDebug::GetDebugFile());
		REQUIRE(file != nullptr);
		TDebug::DebugFilePrintString(const_cast<char*>("abc"));
		CHECK(file->GetBytesWritten() == 3);
		CHECK(TDebug::EnableDebugFile(false));
		CHECK_FALSE(TDebug::EnableDebugFile(true));
		TDebug::CloseDebugFile();
		CHECK(TDebug::GetDebugFile() == nullptr);
		CHECK_FALSE(TDebug::IsDebugFileEnabled());
		CHECK(TDebug::IsValidAddress(nullptr));
		CHECK(TDebug::s_iMessageLevel == TDebug::MSGLEVEL_2);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

using namespace jpog::test;

TEST_SUITE("TDebug differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("defaults match the original")
	{
		CHECK(*static_cast<int*>(Original<void*>("?s_iMessageLevel@TDebug@Toshi@@2W4MSGLEVEL@12@A")) ==
		      TDebug::s_iMessageLevel);
		CHECK(*static_cast<bool*>(Original<void*>("?m_bEnableDebugFile@TDebug@Toshi@@2_NA")) == false);
		CHECK(*static_cast<void**>(Original<void*>("?m_pDebugFile@TDebug@Toshi@@2PAVTFile@2@A")) == nullptr);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
