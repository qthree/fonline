#ifndef FONLINE_IMGUI_H
#define FONLINE_IMGUI_H
#ifndef CALCINATION
#include "StdAfx.h"
#include "imgui.h"

namespace FOnline
{
	typedef HWND ImGuiWindowsData;

	class FonlineImgui;
	extern FonlineImgui* GetMainImgui( );
	extern FonlineImgui* CreateImgui( std::string name );
	extern void DestroyImgui( FonlineImgui* imgui );

	struct FormatTextFlag
	{
		union
		{
			uint flag;
			struct
			{
				uint OnlyLastLine : 1;
			};
		};
	};

	extern char* FormatText( char* text, FormatTextFlag flag, uint& color );

	struct colorize
	{
		union
		{
			struct
			{
				UINT8 r;
				UINT8 g;
				UINT8 b;
				UINT8 a;
			};
			uint color;
		};
	};

	class FonlineImgui
	{
		FonlineImgui( ) {};

		ImGuiContext* Context;
		ImGuiWindowsData Window;

		uint WorkCounter;
		bool IsDemoWindow;

		void ScriptFrame( FOWindow* window );

		virtual void InitOS( ImGuiWindowsData window ) = 0;
		virtual void InitGraphics( Device_ device ) = 0;

		virtual void FinishOS( ) = 0;
		virtual void FinishGraphics( ) = 0;

		virtual void NewFrameOS( ) = 0;
		virtual void NewFrameGraphics( ) = 0;

		virtual void RenderGraphics( ) = 0;

		static ImFontAtlas FontAtlas;

	protected:
		inline ImDrawData *GetDrawData( ) { return ImGui::GetDrawData( ); }
		bool InitFlag;

	public:
		FonlineImgui( std::string name );

		std::string Name;

		void WorkContext( std::string info );
		void DropContext( std::string info );

		void ShowDemo( );
		void Init( ImGuiWindowsData window, Device_ device );
		void Finish( );
		void Frame( FOWindow* window );
		void RenderIface( );

		void NewFrame( );
		void EndFrame( );

		inline ImGuiIO& GetIO( ) { return ImGui::GetIO( ); }

		static void RenderAll( );

		inline bool IsMain( ) { return this == GetMainImgui( ); }
		inline ImGuiWindowsData GetWindow( ) { return Window; }

		virtual void MouseEvent( int event, int button, int dy ) = 0;
		virtual void MouseMoveEvent( int x, int y ) = 0;

		inline bool IsInit( ) { return InitFlag; }
	};
}
#endif // CALCINATION
#endif // FONLINE_IMGUI_H