#pragma once

#include "toshi/application/TApplication.h"

namespace Toshi
{
class TRenderInterface;
}

// The game's application object (SimJP.exe: AApplication, global g_oTheApp).
//
// Stage-1 bootstrap: the original OnCreate builds the string pool, locales, GUI modules and the
// task tree rooted at ARootTask, which creates the renderer and loads the front end. For now
// OnCreate only does the renderer part of ARootTask::CreateRenderInterface (load
// TRenderD3DInterface, create it, open the display), and OnUpdate presents empty frames. The
// remaining steps get reimplemented one by one on top of this.
class AApplication : public Toshi::TApplication
{
public:
	bool OnCreate(int argc, char** const argv) override;
	bool OnUpdate(float deltaTime) override;
	bool OnDestroy() override;

private:
	bool CreateRenderInterface();

	Toshi::TRenderInterface* m_pRenderInterface = nullptr;
};

extern AApplication g_oTheApp;
