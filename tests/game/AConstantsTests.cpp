#include "game/AConstants.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/file/TNativeFile.h"

#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>

using C = AConstantTypes;

namespace
{

// Reads Data/Constant.ini below `directory` through the kernel's file system.
void Initialise(const std::filesystem::path& directory)
{
	Toshi::TFileManager manager;
	Toshi::TNativeFileSystem local("local"); // mounts itself
	manager.SetSystemPath("local");
	local.SetPrefix(directory.string().c_str());
	AConstants::Initialise();
}

} // namespace

TEST_SUITE("AConstants")
{
	TEST_CASE("name tables")
	{
		CHECK(C::FLOAT_CONSTANT_COUNT == 708);
		CHECK(C::INT_CONSTANT_COUNT == 154);
		CHECK(C::VECTOR3_CONSTANT_COUNT == 42);
		CHECK(std::strcmp(C::sm_caszFloatConstantNames[C::ParkKFee], "ParkKFee") == 0);
		CHECK(std::strcmp(C::sm_caszIntConstantNames[C::INT_CONSTANT_COUNT - 1], "TornadoBorderStartRange") == 0);
		CHECK(std::strcmp(C::sm_caszVector3ConstantNames[0], "DecalColourValid") == 0);
	}

	TEST_CASE("values from Data/Constant.ini")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_constants_test";
		fs::create_directories(directory / "Data");
		{
			std::ofstream out(directory / "Data" / "Constant.ini");
			out << "FloatConstants {\n  ParkKFee = 700;\n  ParkKSatMin = -.1;\n  ParkUpdatePeriod = \"x\";\n}\n"
			       "IntConstants {\n  MaxDinoPopulation = 60;\n  ParkMaxEntryFee = 2.5;\n}\n"
			       "Vector3Constants {\n  MiniMapEdgeCol = { 96, 94, 13 };\n  DecalColourValid = 1;\n}\n";
		}
		Initialise(directory);
		CHECK(AConstants::IsInitialised());
		CHECK(AConstants::GetFloatConstant(C::ParkKFee) == 700.0f);
		CHECK(AConstants::GetFloatConstant(C::ParkKSatMin) == -0.1f);
		CHECK(AConstants::GetFloatConstant(C::ParkUpdatePeriod) == 0.0f);           // not a number
		CHECK(AConstants::GetFloatConstant(C::GAMEYEARS_TO_ACTUALSECONDS) == 0.0f); // missing
		CHECK(AConstants::GetIntConstant(C::MaxDinoPopulation) == 60);
		CHECK(AConstants::GetIntConstant(C::ParkMaxEntryFee) == 0); // not integral
		const Toshi::TVector3& edge = AConstants::GetVector3Constant(C::MiniMapEdgeCol);
		CHECK(edge.x == 96.0f);
		CHECK(edge.y == 94.0f);
		CHECK(edge.z == 13.0f);
		CHECK(AConstants::GetVector3Constant(C::DecalColourValid).x == 0.0f); // not an array
		fs::remove_all(directory);
	}

	TEST_CASE("original data")
	{
		const std::filesystem::path original = JPOG_ORIGINAL_DIR;
		if (!std::filesystem::exists(original / "Data" / "Constant.ini"))
			return; // no game copy
		Initialise(original);
		REQUIRE(AConstants::IsInitialised());
		CHECK(AConstants::GetFloatConstant(C::GAMEYEARS_TO_ACTUALSECONDS) == 3600.0f);
		CHECK(AConstants::GetFloatConstant(C::LargeHerbivoreTreeNeedStressRate) == 0.003f);
		CHECK(AConstants::GetIntConstant(C::TornadoBorderStartRange) == 300);
		CHECK(AConstants::GetVector3Constant(C::MiniMapEdgeCol).y == 94.0f);
	}
}
