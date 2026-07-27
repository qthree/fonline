#ifndef IM_GUI_OVERLAY_H
#define IM_GUI_OVERLAY_H
#ifndef CALCINATION
#define OVERLAY_OFF
#include "StdAfx.h"
#include "FonlineImgui.h"

namespace FOnline
{
	struct OverlayFlag;
	class Overlay;
	
	extern Overlay* CreateOverlay( );
	extern void DestroyOverlay( Overlay* overlay );
	extern Overlay* GetOverlay( );
	extern void FinishOverlay( );
	extern void LoopOverlay( );
	extern void FocusOverlay( );

	struct OverlayFlag
	{
		bool IsInit : 1;
		bool IsFinish : 1;
	};

	class Overlay
	{
		friend Overlay* GetOverlay( );
		friend void FinishOverlay( );

		virtual void Finish( ) = 0;

	protected:
		Overlay( );

		virtual void Render( ) = 0;

		const OverlayFlag* GetFlag( );
	
	public:
		virtual ImGuiWindowsData GetWindow( ) = 0;
		virtual void Loop( ) = 0;
		virtual void Init( ) = 0;
		virtual void InitWindow( ) = 0;
	};
}
#endif // CALCINATION
#endif
