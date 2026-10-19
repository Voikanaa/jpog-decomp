#include "game/AOptions.h"
#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyValue.h"
#include "toshi/kernel/TFile.h"
#include "toshi/kernel/TNativeFile.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/TVector2.h"
#include "toshi/kernel/TVector3.h"

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using Toshi::TCString;
using Toshi::TPCString;

namespace
{

// Exposes the file variants, which take the directory and name.
class TestOptions : public AOptions
{
public:
	using AOptions::LoadOptions;
	using AOptions::SaveOptions;
};

struct ChangeLog : Toshi::TGenericListener
{
	explicit ChangeLog(AOptions& options) { Connect(options.GetOptionChangeEmitter(), this, Record, 0); }
	~ChangeLog() { Disconnect(); }
	static bool TSTDCALL Record(void* receiver, void*, void* event)
	{
		const auto* change = static_cast<const AOptionChangeEvent*>(event);
		static_cast<ChangeLog*>(receiver)->names.push_back(change->m_oName.GetCString().GetString(0));
		return true;
	}
	std::vector<std::string> names;
};

} // namespace

TEST_SUITE("AOptions")
{
	TEST_CASE("typed options and groups")
	{
		TestOptions options;
		CHECK(AOptions::GetOptions() == &options);
		ChangeLog log(options);

		options.PutOption("Width", 800);
		options.PutOption("Hex", 7u);
		options.PutOption("Gamma", 1.5f);
		options.PutOption("Windowed", true);
		options.PutOption("Locale", Toshi::TSystem::GetCStringPool()->Get("English"));
		options.PutOption("Pos", Toshi::TVector3(1.0f, 2.0f, 3.0f));
		const unsigned int keys[] = {10, 20, 30};
		const int count = 3;
		options.PutOption("Keys", keys, count);
		options.PutOption("Width", 1024); // replaces
		CHECK(log.names ==
		      std::vector<std::string>{"Width", "Hex", "Gamma", "Windowed", "Locale", "Pos", "Keys", "Width"});
		CHECK(options.GetCurrentProperties()->GetPropertyCount() == 7);

		int width = 0;
		CHECK(options.GetOption("Width", width));
		CHECK(width == 1024);
		float gamma = 0.0f;
		CHECK(options.GetOption("Gamma", gamma));
		CHECK(gamma == 1.5f);
		CHECK(options.GetOption("Width", gamma)); // an int can be a float
		CHECK(gamma == 1024.0f);
		bool windowed = false;
		CHECK(options.GetOption("Windowed", windowed));
		CHECK(windowed);
		bool missing = true;
		CHECK_FALSE(options.GetOption("Missing", missing));
		CHECK(missing);
		TPCString locale;
		CHECK(options.GetOption("Locale", locale));
		CHECK(std::string(locale.GetCString().GetString(0)) == "English");
		Toshi::TVector3 position;
		CHECK(options.GetOption("Pos", position));
		CHECK(position.y == 2.0f);
		unsigned int read[3] = {};
		CHECK(options.GetOption("Keys", read, count));
		CHECK(read[2] == 30);

		CHECK_FALSE(options.GetGroup("Audio"));
		CHECK(options.PutGroup("Audio"));
		options.PutOption("Volume", 0.5f);
		options.CloseGroup();
		CHECK_FALSE(options.GetOption("Volume", gamma));
		CHECK(options.GetGroup("Audio"));
		CHECK(options.GetOption("Volume", gamma));
		CHECK(gamma == 0.5f);
		options.CloseGroup();
		CHECK_FALSE(options.GetGroup("Width")); // not a group
		CHECK_FALSE(options.PutGroup("Width"));
	}

	TEST_CASE("save and load")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_options_test";
		fs::create_directories(directory);
		Toshi::TFileManager manager;
		Toshi::TNativeFileSystem local("local"); // mounts itself
		manager.SetSystemPath("local");
		local.SetPrefix(directory.string().c_str());

		{
			TestOptions options;
			CHECK(options.LoadOptions(-1, -1, TCString(""), TCString("Options")) == AOptions::RESULT_LOAD_FAILED);
			CHECK(options.GetAutoSaveState() == 1);
			options.PutOption("ScreenWidth", 640);
			options.PutOption("ScreenGamma", 1.0f);
			options.PutGroup("Keys");
			options.PutOption("Jump", 32u);
			options.CloseGroup();
			CHECK(options.SaveOptions(-1, -1, TCString(""), TCString("Options")) == AOptions::RESULT_OK);
			CHECK(options.GetAutoSaveState() == 2);
		}
		std::ifstream in(directory / "Options.ini");
		std::stringstream text;
		text << in.rdbuf();
		CHECK(text.str() == "ScreenWidth = 640;\nScreenGamma = 1f;\nKeys {\n\tJump = 0x00000020u;\n}\n");
		in.close();

		{
			TestOptions options;
			CHECK(options.LoadOptions(-1, -1, TCString(""), TCString("Options")) == AOptions::RESULT_OK);
			int width = 0;
			CHECK(options.GetOption("ScreenWidth", width));
			CHECK(width == 640);
			CHECK(options.GetGroup("Keys"));
			unsigned int jump = 0;
			CHECK(options.GetOption("Jump", jump));
			CHECK(jump == 32);
		}
		fs::remove_all(directory);
	}
}
