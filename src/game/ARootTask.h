#pragma once

#include "toshi/kernel/TManagedPtr.h"
#include "toshi/kernel/TScheduler.h"
#include "toshi/render/TRenderInterface.h"

// The game's root task (SimJP.exe), a child of AApplication's Update2 task. Its constructor
// allocates the game's systems and OnCreate brings them up.
//
// So far only the renderer and ARenderer: the original OnCreate also loads the options, creates
// the animation, input, GUI, game state, mission, sound and camera systems, loads the material
// libraries, the movie player and the front end (issue #21). The layout is not the original's.
class ARootTask : public Toshi::TTask
{
public:
	ARootTask();
	~ARootTask() override;

	Toshi::TClass& GetClass() const override;
	Toshi::TManagedPtr<Toshi::TRenderInterface> GetRenderInterface();

	static Toshi::TClass m_sClass;

protected:
	bool OnCreate() override;
	bool OnUpdate(float deltaTime) override;

private:
	void AllocateRenderInterface();
	void AllocateARenderer();
	bool CreateRenderInterface();
	void CreateARenderer();

	static Toshi::TObject* __stdcall CreateObject();
	static Toshi::TObject* __stdcall CreateObjectInPlace(void* memory);
	static void __stdcall InitialiseStatic();
	static void __stdcall DeinitialiseStatic();

	Toshi::TManagedPtr<Toshi::TRenderInterface> m_pRenderInterface; // original +0xD0
	Toshi::TTask* m_pARenderer = nullptr;                           // original +0xD4
};
