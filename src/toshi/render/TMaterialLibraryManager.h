#pragma once

namespace Toshi
{

class TMaterial;

// The renderer's material libraries (TRenderInterface.dll), a member of TRenderInterface
// (+0x128). Declaration only.
class TMaterialLibraryManager
{
public:
	TMaterialLibraryManager();
	~TMaterialLibraryManager();

	void Create();
	void Destroy();
	// Loads a material library (`.tml`) and returns its handle.
	unsigned int LoadMaterialLibrary(const char* name);
	void UnloadMaterialLibrary(unsigned int handle);
	TMaterial* GetMaterial(const char* name);
	TMaterial* GetInvalidMaterial();
};

} // namespace Toshi
