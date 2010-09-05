#include "capp.hpp"
#if EE_PLATFORM == EE_PLATFORM_LINUX
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#endif

#include "../helper/SOIL/image_helper.h"
#include "../helper/SOIL/stb_image.h"

static bool IsImage( const std::string& path ) {
	std::string mPath = path;

	if ( path.size() >= 7 && path.substr(0,7) == "file://" )
		mPath = path.substr( 7 );

	if ( !IsDirectory(mPath) && FileSize( mPath ) ) {
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		toLower( Ext );

		if ( Ext == "png" || Ext == "tga" || Ext == "bmp" || Ext == "jpg" || Ext == "gif" || Ext == "jpeg" || Ext == "dds" || Ext == "psd" || Ext == "hdr" || Ext == "pic" )
			return true;
	}

	return false;
}

cApp::cApp( int argc, char *argv[] ) :
	Fon(NULL),
	Mon(NULL),
	mCurImg(0),
	mShowInfo(true),
	mFade(true),
	mFading(false),
	mAlpha(255),
	mCurAlpha(255),
	mLateLoading(true),
	mLaterLoad(false),
	mCursor(true),
	mMouseLeftPressing(false),
	mMouseMiddlePressing(false),
	mImgRT(RN_NORMAL),
	mShowHelp(false),
	mBlockWheelSpeed(true),
	mWheelBlockTime(333),
	mFirstLoad(false),
	mUsedTempDir(false)
{
	mStorePath = StoragePath( "eeiv" ) + GetOSlash();
	mTmpPath = mStorePath + "tmp" + GetOSlash();

	std::string nstr;

	if ( argc > 1 )
		nstr.assign( argv[1] );
	else
		nstr.assign( argv[0] );

	LoadDir( nstr, false );
}

cApp::~cApp() {
	ClearTempDir();
	cEngine::DestroySingleton();
}

void cApp::LoadConfig() {
	std::string tPath = mStorePath + "eeiv.ini";
	cIniFile Ini( tPath );

	if ( FileExists( tPath ) ) {
		Ini.ReadFile();
		mConfig.Width = Ini.GetValueI( "Window", "Width", 1024 );
		mConfig.Height = Ini.GetValueI( "Window", "Height", 768 );
		mConfig.BitColor = Ini.GetValueI( "Window", "BitColor", 32 );
		mConfig.Windowed = Ini.GetValueB( "Window", "Windowed", true );
		mConfig.Resizeable = Ini.GetValueB( "Window", "Resizeable", true );
		mConfig.VSync = Ini.GetValueI( "Window", "VSync", true );
		mConfig.DoubleBuffering = Ini.GetValueB( "Window", "DoubleBuffering", true );
		mConfig.UseDesktopResolution = Ini.GetValueB( "Window", "UseDesktopResolution", false );
		mConfig.NoFrame = Ini.GetValueB( "Window", "NoFrame", false );
		mConfig.MaximizeAtStart = Ini.GetValueB( "Window", "MaximizeAtStart", true );
		mConfig.FrameLimit = Ini.GetValueI( "Window", "FrameLimit", 0 );
		mConfig.Fade = Ini.GetValueB( "Viewer", "Fade", true );
		mConfig.LateLoading = Ini.GetValueB( "Viewer", "LateLoading", true );
		mConfig.BlockWheelSpeed = Ini.GetValueB( "Viewer", "BlockWheelSpeed", true );
		mConfig.ShowInfo = Ini.GetValueB( "Viewer", "ShowInfo", true );
		mConfig.TransitionTime = Ini.GetValueI( "Viewer", "TransitionTime", 200 );
	} else {
		Ini.SetValueI( "Window", "Width", 1024 );
		Ini.SetValueI( "Window", "Height", 768 );
		Ini.SetValueI( "Window", "BitColor", 32);
		Ini.SetValueI( "Window", "Windowed", 1 );
		Ini.SetValueI( "Window", "Resizeable", 1 );
		Ini.SetValueI( "Window", "VSync", 1 );
		Ini.SetValueI( "Window", "DoubleBuffering", 1 );
		Ini.SetValueI( "Window", "UseDesktopResolution", 0 );
		Ini.SetValueI( "Window", "NoFrame", 0 );
		Ini.SetValueI( "Window", "MaximizeAtStart", 1 );
		Ini.SetValueI( "Window", "FrameLimit", 0 );
		Ini.SetValueI( "Viewer", "Fade", 1 );
		Ini.SetValueI( "Viewer", "LateLoading", 1 );
		Ini.SetValueI( "Viewer", "BlockWheelSpeed", 1 );
		Ini.SetValueI( "Viewer", "ShowInfo", 1 );
		Ini.SetValueI( "Viewer", "TransitionTime", 200 );

		if ( !IsDirectory( mStorePath ) )
			MakeDir( mStorePath );

		Ini.WriteFile();
		LoadConfig();
	}
}

bool cApp::Init() {
	LoadConfig();

	EE = cEngine::instance();
	if ( EE->Init( mConfig.Width, mConfig.Height, mConfig.BitColor, mConfig.Windowed, mConfig.Resizeable, mConfig.VSync, mConfig.DoubleBuffering, mConfig.UseDesktopResolution, mConfig.NoFrame ) ) {
		mFade = mConfig.Fade;
		mLateLoading = mConfig.LateLoading;
		mBlockWheelSpeed = mConfig.BlockWheelSpeed;
		mShowInfo = mConfig.ShowInfo;

		if ( mConfig.FrameLimit )
			EE->SetFrameRateLimit(60);

		TF = cTextureFactory::instance();

		MyPath = AppPath();

		Log = cLog::instance();
		KM = cInput::instance();
		KM->SetVideoResizeCallback( boost::bind( &cApp::VideoResize, this ) );
		RestoreMouse();

		if ( mConfig.MaximizeAtStart )
			EE->MaximizeWindow();

		std::string MyFontPath = MyPath + "fonts" + GetOSlash();

		if ( FileExists( MyFontPath + "DejaVuSans.dds" ) && FileExists( MyFontPath + "DejaVuSans.dat" ) && FileExists( MyFontPath + "DejaVuSansMono.dds" ) && FileExists( MyFontPath + "DejaVuSansMono.dat" ) ) {
			TexF 	= eeNew( cTextureFont, ( "DejaVuSans" ) );
			TexFMon = eeNew( cTextureFont, ( "DejaVuSansMono" ) );

			TexF->Load( TF->Load( MyFontPath + "DejaVuSans.dds" ), MyFontPath + "DejaVuSans.dat" );
			TexFMon->Load( TF->Load( MyFontPath + "DejaVuSansMono.dds" ), MyFontPath + "DejaVuSansMono.dat" );

			Fon = reinterpret_cast<cFont*> ( TexF );
			Mon = reinterpret_cast<cFont*> ( TexFMon );
		} else {
			TTF 	= eeNew( cTTFFont, ( "DejaVuSans" ) );
			TTFMon 	= eeNew( cTTFFont, ( "DejaVuSansMono" ) );

			#if EE_PLATFORM == EE_PLATFORM_WIN32
			std::string WinPath = GetWindowsPath() + "\\Fonts\\";
			if ( FileExists( WinPath + "DejaVuSans.ttf" ) && FileExists( WinPath + "DejaVuSansMono.ttf" ) ) {
				TTF->Load( WinPath + "DejaVuSans.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
				TTFMon->Load( WinPath + "DejaVuSansMono.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
			#else
			if ( FileExists( "/usr/share/fonts/truetype/DejaVuSans.ttf" ) && FileExists( "/usr/share/fonts/truetype/DejaVuSansMono.ttf" ) ) {
				TTF->Load( "/usr/share/fonts/truetype/DejaVuSans.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
				TTFMon->Load( "/usr/share/fonts/truetype/DejaVuSansMono.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
			#endif
			} else if ( FileExists( MyFontPath + "DejaVuSans.ttf" ) && FileExists( MyFontPath + "DejaVuSansMono.ttf" ) ) {
				TTF->Load( MyFontPath + "DejaVuSans.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
				TTFMon->Load( MyFontPath + "DejaVuSansMono.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
			} else {
				return false;
			}

			Fon = reinterpret_cast<cFont*> ( TTF );
			Mon = reinterpret_cast<cFont*> ( TTFMon );
		}


		if ( !Fon && !Mon )
			return false;

		if ( FileExists( MyPath + "eeiv.png" ) ) {
			EE->SetIcon( TF->Load( MyPath + "eeiv.png" ) );
		}

		Con.Create( Mon, true, 1024000 );
		Con.IgnoreCharOnPrompt( L'º' );

		Con.AddCommand( L"loaddir", boost::bind( &cApp::CmdLoadDir, this, _1) );
		Con.AddCommand( L"loadimg", boost::bind( &cApp::CmdLoadImg, this, _1) );
		Con.AddCommand( L"setbackcolor", boost::bind( &cApp::CmdSetBackColor, this, _1) );
		Con.AddCommand( L"setimgfade", boost::bind( &cApp::CmdSetImgFade, this, _1) );
		Con.AddCommand( L"setlateloading", boost::bind( &cApp::CmdSetLateLoading, this, _1) );
		Con.AddCommand( L"setblockwheel", boost::bind( &cApp::CmdSetBlockWheel, this, _1) );
		Con.AddCommand( L"moveto", boost::bind( &cApp::CmdMoveTo, this, _1 ) );
		Con.AddCommand( L"batchimgresize", boost::bind( &cApp::CmdBatchImgResize, this, _1 ) );
		Con.AddCommand( L"batchimgchangeformat", boost::bind( &cApp::CmdBatchImgChangeFormat, this, _1 ) );
		Con.AddCommand( L"imgchangeformat", boost::bind( &cApp::CmdImgChangeFormat, this, _1 ) );
		Con.AddCommand( L"imgresize", boost::bind( &cApp::CmdImgResize, this, _1 ) );
		Con.AddCommand( L"imgscale", boost::bind( &cApp::CmdImgScale, this, _1 ) );

		PrepareFrame();
		GetImages();

		if ( mFile != "" ) {
			FastLoadImage( CurImagePos( mFile ) );
		} else {
			if ( mFiles.size() )
				FastLoadImage( 0 );
		}
		return true;
	}
	return false;
}

void cApp::RestoreMouse() {
	#if EE_PLATFORM == EE_PLATFORM_LINUX
	Cursor cursor;
	SDL_SysWMinfo info;
	info.version.major = 1;
	int ret = SDL_GetWMInfo(&info);
	if(ret == 1) {
		info.info.x11.lock_func();
		cursor = XcursorLibraryLoadCursor(info.info.x11.display, "");
		XDefineCursor(info.info.x11.display, info.info.x11.window, cursor);
		info.info.x11.unlock_func();
	}
	#endif
}

void cApp::VideoResize() {
	RestoreMouse();
}

void cApp::Process() {
	if ( Init() ) {
		do {
			ET = EE->Elapsed();

			Input();

			TEP.Reset();

			Render();

			if ( KM->IsKeyUp(KEY_F12) ) EE->TakeScreenshot();

			EE->Display();

			RET = TEP.ElapsedSinceStart();

			if ( mLateLoading && mLaterLoad ) {
				if ( eeGetTicks() - mLastLaterTick > mConfig.TransitionTime ) {
					UpdateImages();
					mLaterLoad = false;
				}
			}

			if ( mFirstLoad ) {
				UpdateImages();
				mFirstLoad = false;
			}
		} while( EE->Running() );
	}
	End();
}

void cApp::LoadDir( const std::string& path, const bool& getimages ) {
	if ( !IsDirectory( path ) ) {
		if ( path.substr(0,7) == "file://" ) {
			mFilePath = path.substr( 7 );
			mFilePath = mFilePath.substr( 0, mFilePath.find_last_of( GetOSlash() ) );
		} else if ( path.substr(0,7) == "http://" || path.substr(0,6) == "ftp://" || path.substr(0,8) == "https://" ) {
			mUsedTempDir = true;

			if ( !IsDirectory( mTmpPath ) )
				MakeDir( mTmpPath );

			#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_APPLE
				std::string cmd = "wget -q -P \"" + mTmpPath + "\" \"" + path + "\"";
				system( cmd.c_str() );
			#elif EE_PLATFORM == EE_PLATFORM_WIN32
				return;
			#endif

			mFilePath = mTmpPath;
		} else {
			mFilePath = path.substr( 0, path.find_last_of( GetOSlash() ) );
		}

		size_t res;
		do {
			res = mFilePath.find( "%20" );
			if ( res != std::string::npos )
				mFilePath.replace( res, 3, " " );
		} while ( res != std::string::npos );

		if ( mFilePath == "" ) {
			#if EE_PLATFORM == EE_PLATFORM_WIN32
				mFilePath = "C:\\";
			#else
				mFilePath = GetOSlash();
			#endif
		}

		DirPathAddSlashAtEnd( mFilePath );

		std::string tmpFile = path.substr( path.find_last_of( GetOSlash() ) + 1 );

		if ( IsImage( mFilePath + tmpFile ) )
			mFile = tmpFile;
		else
			return;
	} else {
		mFilePath = path;

		DirPathAddSlashAtEnd( mFilePath );
	}

	mCurImg = 0;

	if ( getimages )
		GetImages();

	if ( mFiles.size() ) {
		if ( mFile.size() )
			mCurImg = CurImagePos( mFile );

		if ( EE->Running() )
			UpdateImages();
	}
}

void cApp::ClearTempDir() {
	if ( mUsedTempDir ) {
		GetImages();

		for ( Uint32 i = 0; i < mFiles.size(); i++ ) {
			std::string Delfile = mFilePath + mFiles[i].Path;
			remove( Delfile.c_str() );
		}
	}
}

void cApp::GetImages() {
	cTimeElapsed TE;

	Uint32 i;
	std::vector<std::string> tStr;
	mFiles.clear();

	std::vector<std::string> tmpFiles = FilesGetInPath( mFilePath );
	for ( i = 0; i < tmpFiles.size(); i++ )
		if ( IsImage( mFilePath + tmpFiles[i] ) )
			tStr.push_back( tmpFiles[i] );

	std::sort( tStr.begin(), tStr.end() );

	for ( i = 0; i < tStr.size(); i++ ) {
		mImage tmpI;
		tmpI.Path = tStr[i];
		tmpI.Tex = 0;

		mFiles.push_back( tmpI );
	}

	Con.PushText( "Image list loaded in %f ms.", TE.ElapsedSinceStart() );

	Con.PushText( "Directory: \"" + mFilePath + "\"" );
	for ( Uint32 i = 0; i < mFiles.size(); i++ )
		Con.PushText( "	" + mFiles[i].Path );
}

Uint32 cApp::CurImagePos( const std::string& path ) {
	for ( Uint32 i = 0; i < mFiles.size(); i++ ) {
		if ( mFiles[i].Path == path )
			return i;
	}
	return 0;
}

void cApp::FastLoadImage( const Uint32& ImgNum ) {
	mCurImg = ImgNum;
	mFiles[ mCurImg ].Tex = LoadImage( mFiles[ mCurImg ].Path, true );
	mFirstLoad = true;
}

void cApp::SetImage( const Uint32& Tex, const std::string& path ) {
	if ( Tex ) {
		mFiles[ mCurImg ].Tex = Tex;

		mImgRT = RN_NORMAL;

		mImg.CreateStatic( Tex );
		mImg.ScaleCentered(true);
		mImg.SetRenderType( mImgRT );
		mImg.Scale( 1.f );
		mImg.UpdatePos( 0.0f, 0.0f );

		if ( path != mFiles[ mCurImg ].Path )
			mCurImg = CurImagePos( path );

		mFile = mFiles[ mCurImg ].Path;

		ScaleToScreen();

		cTexture * pTex = TF->GetTexture( Tex );

		if ( NULL != pTex ) {
			Fon->SetText(
				"File: " + mFile +
				"\nWidth: " + intToStr( pTex->Width() ) +
				"\nHeight: " + intToStr( pTex->Height() ) +
				"\n" + intToStr( mCurImg+1 ) + "/" + intToStr( mFiles.size() )
			);
		}
	} else {
		Fon->SetText( "File: " + path + " failed to load. \nReason: " + SOIL_last_result() );
	}
}

Uint32 cApp::LoadImage( const std::string& path, const bool& SetAsCurrent ) {
	Uint32 TexId 		= 0;

	TE.Reset();

	TexId = TF->Load( mFilePath + path );

	Con.PushText( "%s loaded in %f ms.", path.c_str(), TE.ElapsedSinceStart() );

	if ( SetAsCurrent )
		SetImage( TexId, path );

	return TexId;
}

void cApp::UpdateImages() {
	for ( Int32 i = 0; i < (Int32)mFiles.size(); i++ ) {
		if ( !( i == ( mCurImg - 1 ) || i == mCurImg || i == ( mCurImg + 1 ) )  ) {
			UnloadImage( i );
		}

		if ( i == ( mCurImg - 1 ) || i == ( mCurImg + 1 ) ) {
			if ( mFiles[ i ].Tex == 0 ) {
				mFiles[ i ].Tex = LoadImage( mFiles[ i ].Path );
			}
		}

		if ( i == mCurImg ) {
			if ( mFiles[ i ].Tex == 0 )
			{
				mFiles[ i ].Tex = LoadImage( mFiles[ i ].Path, true );
			}
			else
				SetImage( mFiles[ i ].Tex, mFiles[ i ].Path );
		}
	}
}

void cApp::UnloadImage( const Uint32& img ) {
	if ( mFiles[ img ].Tex != 0 ) {
		TF->Remove( mFiles[ img ].Tex );
		mFiles[ img ].Tex = 0;
	}
}

void cApp::OptUpdate() {
	mImg.CreateStatic( mFiles [ mCurImg ].Tex );
	mImg.Scale( 1.f );
	mImg.UpdatePos( 0.0f, 0.0f );
	mImg.ScaleCentered(true);

	ScaleToScreen();

	if ( mLateLoading ) {
		mLaterLoad = true;
		mLastLaterTick = eeGetTicks();

		cTexture * Tex = TF->GetTexture( mFiles [ mCurImg ].Tex );

		if ( Tex ) {
			Fon->SetText(
				"File: " + mFiles [ mCurImg ].Path +
				"\nWidth: " + intToStr( Tex->Width() ) +
				"\nHeight: " + intToStr( Tex->Height() ) +
				"\n" + intToStr( mCurImg + 1 ) + "/" + intToStr( mFiles.size() )
			);
		}
	} else
		UpdateImages();
}

void cApp::LoadFirstImage() {
	if ( mCurImg != 0 )
		FastLoadImage( 0 );
}

void cApp::LoadLastImage() {
	if ( mCurImg != (Int32)( mFiles.size() - 1 ) )
		FastLoadImage( mFiles.size() - 1 );
}

void cApp::LoadNextImage() {
	if ( ( mCurImg + 1 ) < (Int32)mFiles.size() ) {
		CreateFade();
		mCurImg++;
		OptUpdate();
	}
}

void cApp::LoadPrevImage() {
	if ( ( mCurImg - 1 ) >= 0 ) {
		CreateFade();
		mCurImg--;
		OptUpdate();
	}
}

void cApp::SwitchFade() {
	if ( mFade ) {
		mAlpha = 255.0f;
		mCurAlpha = 255;
		mFading = false;
	}

	mFade = !mFade;
	mLateLoading = !mLateLoading;
	mBlockWheelSpeed = !mBlockWheelSpeed;
}

void cApp::Input() {
	KM->Update();
	Mouse = KM->GetMousePos();

	if ( KM->IsKeyDown(KEY_TAB) && KM->AltPressed() )
		EE->MinimizeWindow();

	if ( KM->IsKeyDown(KEY_ESCAPE) || ( KM->IsKeyDown(KEY_Q) && !Con.Active() ) )
		EE->Running(false);

	if ( ( KM->AltPressed() && KM->IsKeyUp(KEY_RETURN) ) || ( KM->IsKeyUp(KEY_F) && !Con.Active() ) ) {
		if ( EE->Windowed() )
			EE->ChangeRes( EE->GetDeskWidth(), EE->GetDeskHeight(), false );
		else
			EE->ToggleFullscreen();

		RestoreMouse();

		PrepareFrame();
		ScaleToScreen();
	}

	if ( KM->IsKeyUp(KEY_F5) )
		SwitchFade();

	if ( KM->IsKeyUp(KEY_F3) || KM->IsKeyUp(KEY_WORLD_26) )
		Con.Toggle();

	if ( ( KM->IsKeyUp(KEY_S) && !Con.Active() ) || KM->IsKeyUp(KEY_F4) ) {
		mCursor = !mCursor;
		EE->ShowCursor( mCursor );

		if ( mCursor )
			RestoreMouse();
	}

	if ( KM->IsKeyUp(KEY_H) && !Con.Active() )
		mShowHelp = !mShowHelp;

	if ( ( ( KM->IsKeyUp(KEY_V) && KM->ControlPressed() ) || ( KM->IsKeyUp(KEY_INSERT) && KM->ShiftPressed() ) ) && !Con.Active() ) {
		std::string tPath = EE->GetClipboardText();
		if ( ( tPath.size() && IsImage( tPath ) ) || IsDirectory( tPath ) )
			LoadDir( tPath );
	}

	if ( !Con.Active() ) {
		if ( KM->MouseWheelUp() || KM->IsKeyUp(KEY_PAGEUP) ) {
			if ( !mBlockWheelSpeed || eeGetTicks() - mLastWheelUse > mWheelBlockTime ) {
				mLastWheelUse = eeGetTicks();
				LoadPrevImage();
			}
		}

		if ( KM->MouseWheelDown() || KM->IsKeyUp(KEY_PAGEDOWN) ) {
			if ( !mBlockWheelSpeed || eeGetTicks() - mLastWheelUse > mWheelBlockTime ) {
				mLastWheelUse = eeGetTicks();
				LoadNextImage();
			}
		}

		if ( KM->IsKeyUp(KEY_I) )
			mShowInfo = !mShowInfo;
	}

	if ( mFiles.size() && mFiles[ mCurImg ].Tex && !Con.Active() ) {
		if ( KM->IsKeyUp(KEY_HOME) )
			LoadFirstImage();

		if ( KM->IsKeyUp(KEY_END) )
			LoadLastImage();

		if ( KM->IsKeyUp(KEY_KP_MULTIPLY) )
			ScaleToScreen();

		if ( KM->IsKeyUp(KEY_KP_DIVIDE) )
			mImg.Scale( 1.f );

		if ( KM->IsKeyUp(KEY_Z) )
			ZoomImage();

		if ( eeGetTicks() - mZoomTicks >= 15 ) {
			mZoomTicks = eeGetTicks();

			if ( KM->IsKeyDown(KEY_KP_MINUS) )
				mImg.Scale( mImg.Scale() - 0.02f );


			if ( KM->IsKeyDown(KEY_KP_PLUS) )
				mImg.Scale( mImg.Scale() + 0.02f );

			if ( mImg.Scale() < 0.0125f )
				mImg.Scale( 0.0125f );

			if ( mImg.Scale() > 50.0f )
				mImg.Scale( 50.0f );
		}

		if ( KM->IsKeyDown(KEY_LEFT) ) {
			mImg.X( ( mImg.X() + ( (EE->Elapsed() * 0.4f) ) ) );
			mImg.X( static_cast<eeFloat> ( static_cast<Int32> ( mImg.X() ) ) );
		}

		if ( KM->IsKeyDown(KEY_RIGHT) ) {
			mImg.X( ( mImg.X() + ( -(EE->Elapsed() * 0.4f) ) ) );
			mImg.X( static_cast<eeFloat> ( static_cast<Int32> ( mImg.X() ) ) );
		}

		if ( KM->IsKeyDown(KEY_UP) ) {
			mImg.Y( ( mImg.Y() + ( (EE->Elapsed() * 0.4f) ) ) );
			mImg.Y( static_cast<eeFloat> ( static_cast<Int32> ( mImg.Y() ) ) );
		}

		if ( KM->IsKeyDown(KEY_DOWN) ) {
			mImg.Y( ( mImg.Y() + ( -(EE->Elapsed() * 0.4f) ) ) );
			mImg.Y( static_cast<eeFloat> ( static_cast<Int32> ( mImg.Y() ) ) );
		}

		if ( KM->MouseLeftClick() )
			mMouseLeftPressing = false;

		if ( KM->MouseLeftPressed() ) {
			eeVector2f mNewPos;
			if ( !mMouseLeftPressing ) {
				mMouseLeftStartClick = Mouse;
				mMouseLeftPressing = true;
			} else {
				mMouseLeftClick = Mouse;

				mNewPos.x = static_cast<eeFloat>(mMouseLeftClick.x) - static_cast<eeFloat>(mMouseLeftStartClick.x);
				mNewPos.y = static_cast<eeFloat>(mMouseLeftClick.y) - static_cast<eeFloat>(mMouseLeftStartClick.y);

				if ( mNewPos.x != 0 || mNewPos.y != 0 ) {
					mMouseLeftStartClick = Mouse;
					mImg.X( mImg.X() + mNewPos.x );
					mImg.Y( mImg.Y() + mNewPos.y );
				}
			}
		}

		if ( KM->MouseMiddleClick() )
			mMouseMiddlePressing = false;

		if ( KM->MouseMiddlePressed() ) {
			if ( !mMouseMiddlePressing ) {
				mMouseMiddleStartClick = Mouse;
				mMouseMiddlePressing = true;
			} else {
				mMouseMiddleClick = Mouse;

				eeFloat Dist = Distance( (eeFloat)mMouseMiddleStartClick.x, (eeFloat)mMouseMiddleStartClick.y, (eeFloat)mMouseMiddleClick.x, (eeFloat)mMouseMiddleClick.y ) * 0.01f;
				eeFloat Ang = LineAngle( (eeFloat)mMouseMiddleStartClick.x, (eeFloat)mMouseMiddleStartClick.y, (eeFloat)mMouseMiddleClick.x, (eeFloat)mMouseMiddleClick.y );

				if ( Dist ) {
					mMouseMiddleStartClick = Mouse;
					if ( Ang >= 0.0f && Ang <= 180.0f ) {
						mImg.Scale( mImg.Scale() - Dist );
						if ( mImg.Scale() < 0.0125f )
							mImg.Scale( 0.0125f );
					} else {
						mImg.Scale( mImg.Scale() + Dist );
					}
				}
			}
		}

		if ( KM->MouseRightPressed() )
			mImg.Angle( LineAngle( eeVector2f( Mouse.x, Mouse.y ), eeVector2f( HWidth, HHeight ) ) );

		if ( KM->IsKeyUp(KEY_X) ) {
			if ( mImgRT == RN_NORMAL )
				mImgRT = RN_FLIP;
			else if ( mImgRT == RN_MIRROR )
				mImgRT = RN_FLIPMIRROR;
			else if ( mImgRT == RN_FLIPMIRROR )
				mImgRT = RN_MIRROR;
			else
				mImgRT = RN_NORMAL;

			mImg.SetRenderType( mImgRT );
		}

		if ( KM->IsKeyUp(KEY_C) ) {
			if ( mImgRT == RN_NORMAL )
				mImgRT = RN_MIRROR;
			else if ( mImgRT == RN_FLIP )
				mImgRT = RN_FLIPMIRROR;
			else if ( mImgRT == RN_FLIPMIRROR )
				mImgRT = RN_FLIP;
			else
				mImgRT = RN_NORMAL;

			mImg.SetRenderType( mImgRT );
		}

		if ( KM->IsKeyUp(KEY_R) ) {
			mImg.Angle( mImg.Angle() + 90.0f );
			ScaleToScreen();
		}

		if ( KM->IsKeyUp(KEY_A) ) {
			cTexture* Tex = TF->GetTexture( mImg.GetTexture() );
			if ( Tex ) {
				if ( Tex->Filter() == TEX_FILTER_LINEAR )
					Tex->SetTextureFilter( TEX_FILTER_NEAREST );
				else
					Tex->SetTextureFilter( TEX_FILTER_LINEAR );
			}
		}

		if ( KM->IsKeyUp(KEY_T) ) {
			mImg.UpdatePos(0.0f,0.0f);
			mImg.Scale( 1.f );
			mImg.Angle( 0.f );
			ScaleToScreen();
		}
	}
}

void cApp::ScaleToScreen( const bool& force ) {
	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		cTexture* Tex = TF->GetTexture( mFiles[ mCurImg ].Tex );

		if ( NULL == Tex )
			return;

		if ( Tex->ImgWidth() >= Width || Tex->ImgHeight() >= Height ) {
			ZoomImage();
		} else if ( force ) {
			mImg.Scale( 1.f );
		}
	}
}

void cApp::ZoomImage() {
	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		cTexture* Tex = TF->GetTexture( mFiles[ mCurImg ].Tex );

		if ( NULL == Tex )
			return;

		eeFloat OldScale = mImg.Scale();
		mImg.Scale( 1.0f );
		eeAABB mBox = mImg.GetAABB();
		mImg.Scale( OldScale );

		eeFloat iWidth = mBox.Right - mBox.Left;
		eeFloat iHeight = mBox.Bottom - mBox.Top;

		mImg.Scale( Width / iWidth );

		eeFloat mScale2 = 1.f;
		mScale2 = Height / iHeight;

		if ( mScale2 < mImg.Scale() )
			mImg.Scale( mScale2 );
	}
}

void cApp::PrepareFrame() {
	Width = EE->GetWidth();
	Height = EE->GetHeight();
	HWidth = Width * 0.5f;
	HHeight = Height * 0.5f;

	if (eeGetTicks() - mLastTicks >= 100) {
		mLastTicks = eeGetTicks();
		if ( mFiles.size() )
			mInfo = "EEiv - " +  mFiles[ mCurImg ].Path;
		else
			mInfo = "EEiv";

		EE->SetWindowCaption( mInfo );
	}
}

void cApp::Render() {
	PrepareFrame();

	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		DoFade();

		cTexture* Tex = TF->GetTexture( mImg.GetTexture() );

		if ( Tex ) {
			eeFloat X = static_cast<eeFloat> ( static_cast<Int32> ( HWidth - mImg.Width() * 0.5f ) );
			eeFloat Y = static_cast<eeFloat> ( static_cast<Int32> ( HHeight - mImg.Height() * 0.5f ) );

			mImg.OffSetX( X );
			mImg.OffSetY( Y );
			mImg.Alpha( mCurAlpha );
			mImg.Draw();
		}
	}

	if ( mShowInfo )
		Fon->Draw( 0, 0 );

	PrintHelp();

	Con.Draw();
}

void cApp::CreateFade()  {
	if ( mFade ) {
		mAlpha = 0.0f;
		mCurAlpha = 0;
		mFading = true;
		mOldImg = mImg;
	}
}

void cApp::DoFade() {
	if ( mFade && mFading ) {
		mAlpha += ( 255 * RET ) / mConfig.TransitionTime;
		mCurAlpha = static_cast<Uint8> ( mAlpha );

		if ( mAlpha >= 255.0f ) {
			mAlpha = 255.0f;
			mCurAlpha = 255;
			mFading = false;
		}

		cTexture* Tex = TF->GetTexture( mOldImg.GetTexture() );

		if ( Tex ) {
			eeFloat X = static_cast<eeFloat> ( static_cast<Int32> ( HWidth - mOldImg.Width() * 0.5f ) );
			eeFloat Y = static_cast<eeFloat> ( static_cast<Int32> ( HHeight - mOldImg.Height() * 0.5f ) );

			mOldImg.OffSetX( X );
			mOldImg.OffSetY( Y );
			mOldImg.Alpha( 255 - mCurAlpha );
			mOldImg.Draw();
		}
	}
}

void cApp::End() {
	EE->DestroySingleton();
}

void cApp::ResizeTexture( cTexture * pTex, const Uint32& NewWidth, const Uint32& NewHeight, const std::string& SavePath ) {
	std::string Str = SavePath + ".new.png";

	Int32 new_width = (Int32)NewWidth;
	Int32 new_height = (Int32)NewHeight;

	if ( (Int32)pTex->Width() != new_width || (Int32)pTex->Height() != new_height ) {
		pTex->Lock();

		unsigned char * resampled = (unsigned char*)malloc( pTex->Channels() * new_width * new_height );

		up_scale_image( pTex->GetPixelsPtr(), pTex->Width(), pTex->Height(), pTex->Channels(), resampled, new_width, new_height );

		SOIL_save_image( Str.c_str(), SOIL_SAVE_TYPE_PNG, new_width, new_height, pTex->Channels(), resampled );

		eeSAFE_FREE( resampled );

		pTex->Unlock(false, false);
	} else
		pTex->SaveToFile( Str, EE_SAVE_TYPE_PNG );

}

void cApp::ScaleImg( const std::string& Path, const eeFloat& Scale ) {
	if ( IsImage( Path ) && 1.f != Scale ) {
		bool wasLoaded = true;
		cTexture * pTex = TF->GetByName( Path );

		if ( NULL == pTex ) {
			Uint32 Tex = TF->Load( Path );
			pTex = TF->GetTexture( Tex );
			wasLoaded = false;
		}

		Int32 new_width = static_cast<Int32>( pTex->Width() * Scale );
		Int32 new_height = static_cast<Int32>( pTex->Height() * Scale );

		ResizeTexture( pTex, new_width, new_height, Path );

		if ( !wasLoaded )
			TF->Remove( pTex->Id() );
	} else {
		Con.PushText( L"Images does not exists." );
	}
}

void cApp::ResizeImg( const std::string& Path, const Uint32& NewWidth, const Uint32& NewHeight ) {
	if ( IsImage( Path ) ) {
		bool wasLoaded = true;
		cTexture * pTex = TF->GetByName( Path );

		if ( NULL == pTex ) {
			Uint32 Tex = TF->Load( Path );
			pTex = TF->GetTexture( Tex );
			wasLoaded = false;
		}

		ResizeTexture( pTex, NewWidth, NewHeight, Path );

		if ( !wasLoaded )
			TF->Remove( pTex->Id() );
	} else {
		Con.PushText( L"Images does not exists." );
	}
}

void cApp::BatchDir( const std::string& Path, const eeFloat& Scale ) {
	std::string iPath = Path;
	std::vector<std::string> tmpFiles = FilesGetInPath( iPath );

	if ( iPath[ iPath.size() - 1 ] != '/' )
		iPath += "/";

	for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
		std::string fPath = iPath + tmpFiles[i];
		ScaleImg( fPath, Scale );
	}
}

void cApp::CmdImgResize( const std::vector < std::wstring >& params ) {
	std::wstring Error( L"Example of use: imgresize new_width new_height path_to_img" );
	if ( params.size() >= 3 ) {
		try {
			Uint32 nWidth = boost::lexical_cast<Uint32>( wstringTostring( params[1] ) );
			Uint32 nHeight = boost::lexical_cast<Uint32>( wstringTostring( params[2] ) );

			std::string myPath = wstringTostring( params[3] );
			if ( params.size() > 4 ) {
				for ( Uint32 i = 4; i < params.size(); i++ )
					myPath += " " + wstringTostring( params[i] );
			}

			ResizeImg( myPath, nWidth, nHeight );
		} catch (...) {
			Con.PushText( Error );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdImgScale( const std::vector < std::wstring >& params ) {
	std::wstring Error( L"Example of use: imgscale scale path_to_img" );
	if ( params.size() >= 2 ) {
		try {
			eeFloat Scale = boost::lexical_cast<eeFloat>( wstringTostring( params[1] ) );

			std::string myPath = wstringTostring( params[2] );
			if ( params.size() > 3 ) {
				for ( Uint32 i = 3; i < params.size(); i++ )
					myPath += " " + wstringTostring( params[i] );
			}

			ScaleImg( myPath, Scale );
		} catch (...) {
			Con.PushText( Error );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgResize( const std::vector < std::wstring >& params ) {
	std::wstring Error( L"Example of use: batchimgresize scale_value directory_to_resize_img" );
	if ( params.size() >= 3 ) {
		try {
			eeFloat Scale = boost::lexical_cast<eeFloat>( wstringTostring( params[1] ) );

			std::string myPath = wstringTostring( params[2] );
			if ( params.size() > 3 ) {
				for ( Uint32 i = 3; i < params.size(); i++ )
					myPath += " " + wstringTostring( params[i] );
			}

			if ( IsDirectory( myPath ) ) {
				BatchDir( myPath, Scale );
			} else
				Con.PushText( "Second argument is not a directory!" );
		} catch (...) {
			Con.PushText( Error );
		}
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdImgChangeFormat( const std::vector < std::wstring >& params ) {
	std::wstring Error( L"Example of use: imgchangeformat to_format image_to_reformat ( if null will use the current loaded image )" );
	if ( params.size() >= 2 ) {
		try {
			std::string toFormat = wstringTostring( params[1] );
			std::string myPath;

			if ( params.size() >= 3 )
				myPath = wstringTostring( params[2] );
			else
				myPath = mFilePath + mFile;

			if ( params.size() > 3 ) {
				for ( Uint32 i = 4; i < params.size(); i++ )
					myPath += " " + wstringTostring( params[i] );
			}

			std::string fromFormat = FileExtension( myPath );

			Int32 x, y, c;

			if ( stbi_info( myPath.c_str(), &x, &y, &c ) ) {
				std::string fPath 	= myPath;
				std::string fExt	= FileExtension( fPath );

				if ( IsImage( fPath ) && fExt == fromFormat ) {
					std::string fName;

					if ( fExt != toFormat )
						fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
					else
						fName = fPath + "." + toFormat;

					int saveType = -1;

					if ( toFormat == "tga" )
						saveType = SOIL_SAVE_TYPE_TGA;
					else if ( toFormat == "bmp" )
						saveType = SOIL_SAVE_TYPE_BMP;
					else if ( toFormat == "png" )
						saveType = SOIL_SAVE_TYPE_PNG;
					else if ( toFormat == "dds" )
						saveType = SOIL_SAVE_TYPE_DDS;

					if ( -1 != saveType ) {
						int w, h, c;
						unsigned char * img = SOIL_load_image( fPath.c_str(), &w, &h, &c, SOIL_LOAD_AUTO );
						SOIL_save_image( fName.c_str(), saveType, w, h, c, reinterpret_cast<const unsigned char*>( img ) );
						SOIL_free_image_data( img );

						Con.PushText( fName + " created." );
					}
				}
			} else
				Con.PushText( "Third argument is not a directory! Argument: " + myPath );
		} catch (...) {
			Con.PushText( Error );
		}
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgChangeFormat( const std::vector < std::wstring >& params ) {
	std::wstring Error( L"Example of use: batchimgchangeformat from_format to_format directory_to_reformat" );
	if ( params.size() >= 4 ) {
		try {
			std::string fromFormat = wstringTostring( params[1] );
			std::string toFormat = wstringTostring( params[2] );

			std::string myPath = wstringTostring( params[3] );
			if ( params.size() > 3 ) {
				for ( Uint32 i = 4; i < params.size(); i++ )
					myPath += " " + wstringTostring( params[i] );
			}

			if ( IsDirectory( myPath ) ) {
				std::vector<std::string> tmpFiles = FilesGetInPath( myPath );

				if ( myPath[ myPath.size() - 1 ] != '/' )
					myPath += "/";

				for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
					std::string fPath 	= myPath + tmpFiles[i];
					std::string fExt	= FileExtension( fPath );

					if ( IsImage( fPath ) && fExt == fromFormat ) {
						std::string fName;

						if ( fExt != toFormat )
							fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
						else
							fName = fPath + "." + toFormat;

						int saveType = -1;

						if ( toFormat == "tga" )
							saveType = SOIL_SAVE_TYPE_TGA;
						else if ( toFormat == "bmp" )
							saveType = SOIL_SAVE_TYPE_BMP;
						else if ( toFormat == "png" )
							saveType = SOIL_SAVE_TYPE_PNG;
						else if ( toFormat == "dds" )
							saveType = SOIL_SAVE_TYPE_DDS;

						if ( -1 != saveType ) {
							int w, h, c;
							unsigned char * img = SOIL_load_image( fPath.c_str(), &w, &h, &c, SOIL_LOAD_AUTO );
							SOIL_save_image( fName.c_str(), saveType, w, h, c, reinterpret_cast<const unsigned char*>( img ) );
							SOIL_free_image_data( img );

							Con.PushText( fName + " created." );
						}
					}
				}
			} else
				Con.PushText( "Third argument is not a directory! Argument: " + myPath );
		} catch (...) {
			Con.PushText( Error );
		}
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdMoveTo( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );

			if ( tInt )
				tInt--;

			if ( tInt >= 0 && tInt < (Int32)mFiles.size() ) {
				Con.PushText( L"moveto: moving to image number " + toWStr(tInt+1) );
				FastLoadImage( tInt );
			} else
				Con.PushText( L"moveto: image number does not exists" );

		} catch (boost::bad_lexical_cast&) {
			Con.PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	} else
		Con.PushText( L"Expected some parameter" );
}

void cApp::CmdSetBlockWheel( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt == 0 || tInt == 1 ) {
				mBlockWheelSpeed = (bool)tInt;
				Con.PushText( L"setblockwheel " + toWStr(tInt) );
			} else
				Con.PushText( L"Valid parameters are 0 or 1." );
		} catch (boost::bad_lexical_cast&) {
			Con.PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	} else
		Con.PushText( L"Expected some parameter" );
}

void cApp::CmdSetLateLoading( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt == 0 || tInt == 1 ) {
				mLateLoading = (bool)tInt;
				Con.PushText( L"setlateloading " + toWStr(tInt) );
			} else
				Con.PushText( L"Valid parameters are 0 or 1." );
		} catch (boost::bad_lexical_cast&) {
			Con.PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	} else
		Con.PushText( L"Expected some parameter" );
}

void cApp::CmdSetImgFade( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt == 0 || tInt == 1 ) {
				mFade = (bool)tInt;
				Con.PushText( L"setimgfade " + toWStr(tInt) );
			} else
				Con.PushText( L"Valid parameters are 0 or 1." );
		} catch (boost::bad_lexical_cast&) {
			Con.PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	} else
		Con.PushText( L"Expected some parameter" );
}

void cApp::CmdSetBackColor( const std::vector < std::wstring >& params ) {
	std::wstring Error( L"Example of use: setbackcolor 255 255 255 (RGB Color, numbers between 0 and 255)" );
	if ( params.size() >= 2 ) {
		try {
			if ( params.size() == 4 ) {
				Int32 R = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
				Int32 G = boost::lexical_cast<Int32>( wstringTostring( params[2] ) );
				Int32 B = boost::lexical_cast<Int32>( wstringTostring( params[3] ) );

				if ( ( R <= 255 && R >= 0 ) && ( G <= 255 && G >= 0 ) && ( B <= 255 && B >= 0 ) ) {
					EE->SetBackColor( eeColor( R,G,B ) );
					Con.PushText( L"setbackcolor applied" );
					return;
				}
			}
			Con.PushText( Error );
		} catch (...) {
			Con.PushText( Error );
		}
	}
}

void cApp::CmdLoadImg( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			std::string myPath = wstringTostring( params[1] );
			if ( IsImage( myPath ) ) {
				LoadDir( myPath );
			} else
				Con.PushText( "\"" + myPath + "\" is not an image path or the image is not supported." );
		} catch (...) {
			Con.PushText( L"Something goes wrong!!" );
		}
	}
}

void cApp::CmdLoadDir( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			std::string myPath = wstringTostring( params[1] );
			if ( params.size() > 2 ) {
				for ( Uint32 i = 2; i < params.size(); i++ )
					myPath += " " + wstringTostring( params[i] );
			}

			if ( IsDirectory( myPath ) ) {
				LoadDir( myPath );
			} else
				Con.PushText( "If you want to load an image use loadimg. \"" + myPath + "\" is not a directory path." );
		} catch (...) {
			Con.PushText( L"Something goes wrong!!" );
		}
	}
}

void cApp::PrintHelp() {
	if ( mShowHelp ) {
		Uint32 Top = 6;
		Uint32 Left = 6;

		std::wstring prevText = Fon->GetText();

		Fon->Draw( L"Keys List: ", Left, Height - Top - Fon->GetFontSize() * 22 );
		Fon->Draw( L"Escape: Quit from EEiv", Left, Height - Top - Fon->GetFontSize() * 1 );
		Fon->Draw( L"ALT + RETURN or F: Toogle Fullscreen - Windowed", Left, Height - Top - Fon->GetFontSize() * 2 );
		Fon->Draw( L"F3 or º: Toggle Console", Left, Height - Top - Fon->GetFontSize() * 3 );
		Fon->Draw( L"F4 or S: Show/Hide Cursor", Left, Height - Top - Fon->GetFontSize() * 4 );
		Fon->Draw( L"Mouse Wheel Up or PageUp: Go to Previous Image", Left, Height - Top - Fon->GetFontSize() * 5 );
		Fon->Draw( L"Mouse Wheel Down or PageDown: Go to Next Image", Left, Height - Top - Fon->GetFontSize() * 6 );
		Fon->Draw( L"Key I: Show/Hide Image Info", Left, Height - Top - Fon->GetFontSize() * 7 );
		Fon->Draw( L"Key *: Scale image to screen", Left, Height - Top - Fon->GetFontSize() * 8 );
		Fon->Draw( L"Key /: Reset Scale to 1", Left, Height - Top - Fon->GetFontSize() * 9);
		Fon->Draw( L"Key Z: Fit Image to Screen", Left, Height - Top - Fon->GetFontSize() * 10 );
		Fon->Draw( L"Key + and -, or mouse middle press up or down: Zoom in and Zoom out image", Left, Height - Top - Fon->GetFontSize() * 11 );
		Fon->Draw( L"Key X: Flip Image", Left, Height - Top - Fon->GetFontSize() * 12 );
		Fon->Draw( L"Key C: Mirror Image", Left, Height - Top - Fon->GetFontSize() * 13 );
		Fon->Draw( L"Key R: Rotate 90º Image and scale it to screen", Left, Height - Top - Fon->GetFontSize() * 14 );
		Fon->Draw( L"Key T: Reset the position and the scale of the image", Left, Height - Top - Fon->GetFontSize() * 15 );
		Fon->Draw( L"Key A: Change the texture filter", Left, Height - Top - Fon->GetFontSize() * 16 );
		Fon->Draw( L"Key Left - Right - Top - Down or left mouse press: Move the image", Left, Height - Top - Fon->GetFontSize() * 17 );
		Fon->Draw( L"Key F12: Take a screenshot", Left, Height - Top - Fon->GetFontSize() * 18 );
		Fon->Draw( L"Key HOME: Go to the first screenshot on the folder", Left, Height - Top - Fon->GetFontSize() * 19 );
		Fon->Draw( L"Key END: Go to the last screenshot on the folder", Left, Height - Top - Fon->GetFontSize() * 20 );

		Fon->SetText( prevText );
	}
}
