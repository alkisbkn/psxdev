#include "../engine/system/system.h"

int main()
{
    SystemInitInfo sysInitInfo;
    memset(&sysInitInfo, 0, sizeof(SystemInitInfo));

    sysInitInfo.m_isHighResolution  = 0;
	sysInitInfo.m_isInterlaced		= 0;
    sysInitInfo.m_tvMode            = MODE_PAL; // <-- TODO: Pick from BIOS

    System_Initialize(&sysInitInfo);
    System_MainLoop();
    System_Shutdown();
    return 0;
}
