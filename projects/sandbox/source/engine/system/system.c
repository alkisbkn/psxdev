#include "system.h"
#include "../gfx/gfx.h"
#include "../core/core.h"

static SystemInitInfo* g_initInfo = NULL;
static uint8 g_systemRunning = 1;

///////////////////////////////////////////////////
int16 System_Initialize(SystemInitInfo* i_info)
{
    int16 errcode = E_OK;

    g_initInfo = i_info;

	// Initialize core
	errcode |= Core_Initialize();

    // Initialize graphics
    errcode |= Gfx_Initialize(i_info->m_isHighResolution,
                              i_info->m_tvMode
                              );

    PadInit(0);     /* initialize input */

    if (g_initInfo && g_initInfo->AppStartFncPtr)
        g_initInfo->AppStartFncPtr();

    return errcode;
}

///////////////////////////////////////////////////
int16 System_MainLoop()
{
	float cpuMs = 0.0f;
	uint64 timeStart, timeEnd;

    while (g_systemRunning)
    {
		char dbgText[32];

		sprintf2(dbgText, "CPU: %.2f", cpuMs);

		FntLoad(960, 256);
		SetDumpFnt(FntOpen(4, 4, 320, 64, 0, 512));
        FntPrint(dbgText);
		FntFlush(-1);

        Gfx_BeginFrame(&timeStart);
		
        if (g_initInfo && g_initInfo->AppUpdateFncPtr)
        {
            g_initInfo->AppUpdateFncPtr();
        }

        Gfx_EndFrame(&timeEnd);
		
		cpuMs = (float)(timeEnd - timeStart) / 1000.0f;
    }

    return E_OK;
}

///////////////////////////////////////////////////
int16 System_Shutdown()
{
    int16 errcode = E_OK;

    if (g_initInfo && g_initInfo->AppShutdownFncPtr)
    {
        g_initInfo->AppShutdownFncPtr();
    }

	errcode |= Gfx_Shutdown();
	errcode |= Core_Shutdown();

	PadStop();

    return E_OK;
}
