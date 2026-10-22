#include "game/AConstants.h"
#include "game/AFadeDist.h"
#include "game/AParticleSystem.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/file/TNativeFile.h"

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>

TEST_SUITE("graphics detail")
{
	TEST_CASE("fade distances per draw distance level")
	{
		// Defaults before any level is loaded.
		CHECK(AFadeDist::GetFadeNear(AFadeDist::TYPE_TERRAIN) == 290.0f);
		CHECK(AFadeDist::GetFadeFar(AFadeDist::TYPE_DEFAULT) == 10001.0f);

		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_fade_test";
		fs::create_directories(directory / "Data");
		{
			std::ofstream out(directory / "Data" / "Constant.ini");
			out << "FloatConstants {\n"
			       "  LOW_TreeFadeNearDistance = 10;\n  LOW_TreeFadeFarDistance = 20;\n"
			       "  MEDIUM_GrassFadeFarDistance = 33;\n"
			       "  HIGH_DinoFadeNearDistance = 44;\n  HIGH_VisitorFadeFarDistance = 55;\n}\n";
		}
		{
			Toshi::TFileManager manager;
			Toshi::TNativeFileSystem local("local"); // mounts itself
			manager.SetSystemPath("local");
			local.SetPrefix(directory.string().c_str());
			AConstants::Initialise();
		}
		fs::remove_all(directory);

		AFadeDist::LoadFadeData(0);
		CHECK(AFadeDist::GetFadeNear(AFadeDist::TYPE_TREE) == 10.0f);
		CHECK(AFadeDist::GetFadeFar(AFadeDist::TYPE_TREE) == 20.0f);
		AFadeDist::LoadFadeData(1);
		CHECK(AFadeDist::GetFadeFar(AFadeDist::TYPE_GRASS) == 33.0f);
		CHECK(AFadeDist::GetFadeNear(AFadeDist::TYPE_TREE) == 0.0f);
		AFadeDist::LoadFadeData(7); // anything else is high
		CHECK(AFadeDist::GetFadeNear(AFadeDist::TYPE_DINO) == 44.0f);
		CHECK(AFadeDist::GetFadeFar(AFadeDist::TYPE_VISITOR) == 55.0f);
		// Never loaded.
		CHECK(AFadeDist::GetFadeNear(AFadeDist::TYPE_TERRAIN) == 290.0f);
	}

	TEST_CASE("particle detail budgets")
	{
		AParticleSystem::SetDetail(AParticleSystem::PARTICLEDETAIL_LOW);
		CHECK(AParticleSystem::GetDetail() == AParticleSystem::PARTICLEDETAIL_LOW);
		CHECK(AParticleSystem::GetMaxVerts() == 600);
		CHECK(AParticleSystem::GetFarDistModifier() == 1.0f);
		CHECK(AParticleSystem::GetNumMeshes(0) == 8);
		CHECK(AParticleSystem::GetNumMeshes(1) == 0);
		CHECK(AParticleSystem::GetNumMeshes(10) == 12);
		CHECK(AParticleSystem::GetNumMeshes(16) == 12);
		CHECK(AParticleSystem::GetNumSystems(4) == 50);

		AParticleSystem::SetDetail(AParticleSystem::PARTICLEDETAIL_MEDIUM);
		CHECK(AParticleSystem::GetMaxVerts() == 1300);
		CHECK(AParticleSystem::GetFarDistModifier() == 0.85f);
		CHECK(AParticleSystem::GetNumMeshes(0) == 15);
		CHECK(AParticleSystem::GetNumMeshes(2) == 20);

		AParticleSystem::SetDetail(static_cast<AParticleSystem::PARTICLEDETAIL>(5)); // high
		CHECK(AParticleSystem::GetMaxVerts() == 2000);
		CHECK(AParticleSystem::GetFarDistModifier() == 0.7f);
		CHECK(AParticleSystem::GetNumMeshes(16) == 30);
		CHECK(AParticleSystem::GetMaxNumMeshes() == 20);
	}
}
