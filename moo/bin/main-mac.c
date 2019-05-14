#include <Types.h>
#include <Memory.h>
#include <QuickDraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <MacWindows.h>
#include <OSUtils.h>
#include <ToolUtils.h>


static void InitToolBox (void)
{
	GrafPtr wmgrPort;

#if defined(TARGET_API_MAC_CARBON) && (TARGET_API_MAC_CARBON > 0)
	/* no init required for the managers below in Carbon */
#else
	InitGraf (&qd.thePort);
	InitFonts ();
	InitWindows ();
	InitMenus ();
	TEInit ();
	InitDialogs (0L);
	MaxApplZone ();
#endif

	InitCursor ();

#if defined(TARGET_API_MAC_CARBON) && (TARGET_API_MAC_CARBON > 0)
	wmgrPort = CreateNewPort();
#else
	GetWMgrPort (&wmgrPort);
#endif

	SetPort (wmgrPort);
}


static void EventLoop (void)
{

}
