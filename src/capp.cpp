#include "capp.hpp"
#include <algorithm>

#if EE_PLATFORM == EE_PLATFORM_WIN
static std::string GetWindowsPath() {
	#ifdef UNICODE
		wchar_t Buffer[1024];
		GetWindowsDirectory( Buffer, 1024 );
		return String( Buffer ).ToUtf8();
	#else
		char Buffer[1024];
		GetWindowsDirectory( Buffer, 1024 );
		return std::string( Buffer );
	#endif
}
#endif

static bool IsImage( const std::string& path ) {
	std::string mPath = path;

	if ( path.size() >= 7 && path.substr(0,7) == "file://" )
		mPath = path.substr( 7 );

	if ( !FileSystem::IsDirectory(mPath) && FileSystem::FileSize( mPath ) ) {
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		String::ToLower( Ext );

		if ( Ext == "png" || Ext == "tga" || Ext == "bmp" || Ext == "jpg" || Ext == "gif" || Ext == "jpeg" || Ext == "dds" || Ext == "psd" || Ext == "hdr" || Ext == "pic" )
			return true;
		else {
			int x,y,c;
			return cImage::GetInfo( mPath, &x, &y, &c );
		}
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
	mUsedTempDir(false),
	mHelpCache( NULL ),
	mSlideShow(false),
	mSlideTime(4000),
	mSlideTicks(Sys::GetTicks())
{
	mStorePath = Sys::GetConfigPath( "eeiv" ) + FileSystem::GetOSlash();
	mTmpPath = mStorePath + "tmp" + FileSystem::GetOSlash();

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

	if ( FileSystem::FileExists( tPath ) ) {
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

		if ( !FileSystem::IsDirectory( mStorePath ) )
			FileSystem::MakeDir( mStorePath );

		Ini.WriteFile();
		LoadConfig();
	}
}

bool cApp::Init() {
	LoadConfig();

	EE 		= cEngine::instance();
	MyPath 	= Sys::GetProcessPath();

	Uint32 Style = WindowStyle::Titlebar;

	if ( mConfig.NoFrame )
		Style = WindowStyle::NoBorder;

	if ( !mConfig.Windowed )
		Style |= WindowStyle::Fullscreen;

	if ( mConfig.Resizeable )
		Style |= WindowStyle::Resize;

	if ( mConfig.UseDesktopResolution )
		Style |= WindowStyle::UseDesktopResolution;

	mWindow = EE->CreateWindow( WindowSettings( mConfig.Width, mConfig.Height, "EEiv", Style, WindowBackend::Default, mConfig.BitColor, MyPath + "fonts/" + "eeiv.png" ), ContextSettings( mConfig.VSync, GLv_default, mConfig.DoubleBuffering, 0, 0 ) );

	if ( mWindow->Created() ) {
		mFade = mConfig.Fade;
		mLateLoading = mConfig.LateLoading;
		mBlockWheelSpeed = mConfig.BlockWheelSpeed;
		mShowInfo = mConfig.ShowInfo;

		if ( mConfig.FrameLimit )
			mWindow->FrameRateLimit(60);

		TF 		= cTextureFactory::instance();
		Log 	= cLog::instance();
		KM 		= mWindow->GetInput();

		if ( mConfig.MaximizeAtStart )
			mWindow->Maximize();

		cTimeElapsed TE;

		std::string MyFontPath = MyPath + "fonts" + FileSystem::GetOSlash();

		if ( FileSystem::FileExists( MyFontPath + "DejaVuSans.dds" ) && FileSystem::FileExists( MyFontPath + "DejaVuSans.dat" ) && FileSystem::FileExists( MyFontPath + "DejaVuSansMono.dds" ) && FileSystem::FileExists( MyFontPath + "DejaVuSansMono.dat" ) ) {
			TexF 	= eeNew( cTextureFont, ( "DejaVuSans" ) );
			TexFMon = eeNew( cTextureFont, ( "DejaVuSansMono" ) );

			TexF->Load( TF->Load( MyFontPath + "DejaVuSans.dds" ), MyFontPath + "DejaVuSans.dat" );
			TexFMon->Load( TF->Load( MyFontPath + "DejaVuSansMono.dds" ), MyFontPath + "DejaVuSansMono.dat" );

			Fon = reinterpret_cast<cFont*> ( TexF );
			Mon = reinterpret_cast<cFont*> ( TexFMon );
		} else {
			TTF 	= eeNew( cTTFFont, ( "DejaVuSans" ) );
			TTFMon 	= eeNew( cTTFFont, ( "DejaVuSansMono" ) );

			#if EE_PLATFORM == EE_PLATFORM_WIN
			std::string WinPath( GetWindowsPath() + "\\Fonts\\" );
			if ( FileExists( WinPath + "DejaVuSans.ttf" ) && FileExists( WinPath + "DejaVuSansMono.ttf" ) ) {
				TTF->Load( WinPath + "DejaVuSans.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
				TTFMon->Load( WinPath + "DejaVuSansMono.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
			#else
			if ( FileSystem::FileExists( "/usr/share/fonts/truetype/DejaVuSans.ttf" ) && FileSystem::FileExists( "/usr/share/fonts/truetype/DejaVuSansMono.ttf" ) ) {
				TTF->Load( "/usr/share/fonts/truetype/DejaVuSans.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
				TTFMon->Load( "/usr/share/fonts/truetype/DejaVuSansMono.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
			#endif
			} else if ( FileSystem::FileExists( MyFontPath + "DejaVuSans.ttf" ) && FileSystem::FileExists( MyFontPath + "DejaVuSansMono.ttf" ) ) {
				TTF->Load( MyFontPath + "DejaVuSans.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
				TTFMon->Load( MyFontPath + "DejaVuSansMono.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(), 1, eeColor(0,0,0) );
			} else {
				return false;
			}

			Fon = reinterpret_cast<cFont*> ( TTF );
			Mon = reinterpret_cast<cFont*> ( TTFMon );
		}

		cLog::instance()->Writef( "Fonts loading time: %f ms", TE.ElapsedSinceStart() );

		if ( !Fon && !Mon )
			return false;

		Con.Create( Mon, true, 1024000 );
		Con.IgnoreCharOnPrompt( 186 );

		Con.AddCommand( "loaddir", cb::Make1( this, &cApp::CmdLoadDir ) );
		Con.AddCommand( "loadimg", cb::Make1( this, &cApp::CmdLoadImg ) );
		Con.AddCommand( "setbackcolor", cb::Make1( this, &cApp::CmdSetBackColor ) );
		Con.AddCommand( "setimgfade", cb::Make1( this, &cApp::CmdSetImgFade ) );
		Con.AddCommand( "setlateloading", cb::Make1( this, &cApp::CmdSetLateLoading ) );
		Con.AddCommand( "setblockwheel", cb::Make1( this, &cApp::CmdSetBlockWheel ) );
		Con.AddCommand( "moveto", cb::Make1( this, &cApp::CmdMoveTo ) );
		Con.AddCommand( "batchimgresize", cb::Make1( this, &cApp::CmdBatchImgResize ) );
		Con.AddCommand( "batchimgchangeformat", cb::Make1( this, &cApp::CmdBatchImgChangeFormat ) );
		Con.AddCommand( "imgchangeformat", cb::Make1( this, &cApp::CmdImgChangeFormat ) );
		Con.AddCommand( "imgresize", cb::Make1( this, &cApp::CmdImgResize ) );
		Con.AddCommand( "imgscale", cb::Make1( this, &cApp::CmdImgScale ) );
		Con.AddCommand( "slideshow", cb::Make1( this, &cApp::CmdSlideShow ) );

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

void cApp::Process() {
	if ( Init() ) {
		do {
			ET = mWindow->Elapsed();

			Input();

			TEP.Reset();

			Render();

			if ( KM->IsKeyUp(KEY_F12) ) mWindow->TakeScreenshot();

			mWindow->Display();

			RET = TEP.ElapsedSinceStart();

			if ( mLateLoading && mLaterLoad ) {
				if ( Sys::GetTicks() - mLastLaterTick > mConfig.TransitionTime ) {
					UpdateImages();
					mLaterLoad = false;
				}
			}

			if ( mFirstLoad ) {
				UpdateImages();
				mFirstLoad = false;
			}
		} while( mWindow->Running() );
	}
	End();
}

void cApp::LoadDir( const std::string& path, const bool& getimages ) {
	if ( !FileSystem::IsDirectory( path ) ) {
		if ( path.substr(0,7) == "file://" ) {
			mFilePath = path.substr( 7 );
			mFilePath = mFilePath.substr( 0, mFilePath.find_last_of( FileSystem::GetOSlash() ) );
		} else if ( path.substr(0,7) == "http://" || path.substr(0,6) == "ftp://" || path.substr(0,8) == "https://" ) {
			mUsedTempDir = true;

			if ( !FileSystem::IsDirectory( mTmpPath ) )
				FileSystem::MakeDir( mTmpPath );

			#if defined( EE_PLATFORM_POSIX )
				std::string cmd = "wget -q -P \"" + mTmpPath + "\" \"" + path + "\"";
				system( cmd.c_str() );
			#elif EE_PLATFORM == EE_PLATFORM_WIN
				return;
			#endif

			mFilePath = mTmpPath;
		} else {
			mFilePath = path.substr( 0, path.find_last_of( FileSystem::GetOSlash() ) );
		}

		size_t res;
		do {
			res = mFilePath.find( "%20" );
			if ( res != std::string::npos )
				mFilePath.replace( res, 3, " " );
		} while ( res != std::string::npos );

		if ( mFilePath == "" ) {
			#if EE_PLATFORM == EE_PLATFORM_WIN
				mFilePath = "C:\\";
			#else
				mFilePath = FileSystem::GetOSlash();
			#endif
		}

		FileSystem::DirPathAddSlashAtEnd( mFilePath );

		std::string tmpFile = path.substr( path.find_last_of( FileSystem::GetOSlash() ) + 1 );

		if ( IsImage( mFilePath + tmpFile ) )
			mFile = tmpFile;
		else
			return;
	} else {
		mFilePath = path;

		FileSystem::DirPathAddSlashAtEnd( mFilePath );
	}

	mCurImg = 0;

	if ( getimages )
		GetImages();

	if ( mFiles.size() ) {
		if ( mFile.size() )
			mCurImg = CurImagePos( mFile );

		if ( mWindow->Running() )
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

	std::vector<std::string> tmpFiles = FileSystem::FilesGetInPath( mFilePath );
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

	Con.PushText( "Directory: \"" + String::FromUtf8( mFilePath ) + "\"" );
	for ( Uint32 i = 0; i < mFiles.size(); i++ )
		Con.PushText( "	" + String::FromUtf8( mFiles[i].Path ) );
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
		mImg.RenderType( mImgRT );
		mImg.Scale( 1.f );
		mImg.Position( 0.0f, 0.0f );

		if ( path != mFiles[ mCurImg ].Path )
			mCurImg = CurImagePos( path );

		mFile = mFiles[ mCurImg ].Path;

		ScaleToScreen();

		cTexture * pTex = TF->GetTexture( Tex );

		if ( NULL != pTex ) {
			Fon->SetText(
				"File: " + String::FromUtf8( mFile ) +
				"\nWidth: " + String::ToStr( pTex->Width() ) +
				"\nHeight: " + String::ToStr( pTex->Height() ) +
				"\n" + String::ToStr( mCurImg+1 ) + "/" + String::ToStr( mFiles.size() )
			);
		}
	} else {
		Fon->SetText( "File: " + String::FromUtf8( path ) + " failed to load. \nReason: " + cImage::GetLastFailureReason() );
	}
}

Uint32 cApp::LoadImage( const std::string& path, const bool& SetAsCurrent ) {
	Uint32 TexId 		= 0;

	TE.Reset();

	TexId = TF->Load( mFilePath + path );

	Con.PushText( String::FromUtf8( path ) + " loaded in " + String::ToStr( TE.ElapsedSinceStart() ) + " ms." );

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
	mImg.Position( 0.0f, 0.0f );
	mImg.ScaleCentered(true);

	ScaleToScreen();

	if ( mLateLoading ) {
		mLaterLoad = true;
		mLastLaterTick = Sys::GetTicks();

		cTexture * Tex = TF->GetTexture( mFiles [ mCurImg ].Tex );

		if ( Tex ) {
			Fon->SetText(
				"File: " + String::FromUtf8( mFiles [ mCurImg ].Path ) +
				"\nWidth: " + String::ToStr( Tex->Width() ) +
				"\nHeight: " + String::ToStr( Tex->Height() ) +
				"\n" + String::ToStr( mCurImg + 1 ) + "/" + String::ToStr( mFiles.size() )
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

	if ( KM->IsKeyDown(KEY_TAB) && KM->AltPressed() ) {
		mWindow->Minimize();
	}

	if ( KM->IsKeyDown(KEY_ESCAPE) || ( KM->IsKeyDown(KEY_Q) && !Con.Active() ) ) {
		mWindow->Close();
	}

	if ( ( KM->AltPressed() && KM->IsKeyUp(KEY_RETURN) ) || ( KM->IsKeyUp(KEY_F) && !Con.Active() ) ) {
		if ( mWindow->Windowed() )
			mWindow->Size( mWindow->GetDesktopResolution().Width(), mWindow->GetDesktopResolution().Height(), false );
		else
			mWindow->ToggleFullscreen();

		PrepareFrame();
		ScaleToScreen();
	}

	if ( KM->IsKeyUp(KEY_F5) ) {
		SwitchFade();
	}

	if ( KM->IsKeyUp(KEY_F3) || KM->IsKeyUp(KEY_WORLD_26) ) {
		Con.Toggle();
	}

	if ( ( KM->IsKeyUp(KEY_S) && !Con.Active() ) || KM->IsKeyUp(KEY_F4) ) {
		mCursor = !mCursor;
		mWindow->GetCursorManager()->Visible( mCursor );
	}

	if ( KM->IsKeyUp(KEY_H) && !Con.Active() ) {
		mShowHelp = !mShowHelp;
	}

	if ( ( ( KM->IsKeyUp(KEY_V) && KM->ControlPressed() ) || ( KM->IsKeyUp(KEY_INSERT) && KM->ShiftPressed() ) ) && !Con.Active() ) {
		std::string tPath = mWindow->GetClipboard()->GetText();

		if ( ( tPath.size() && IsImage( tPath ) ) || FileSystem::IsDirectory( tPath ) ) {
			LoadDir( tPath );
		}
	}

	if ( !Con.Active() ) {
		if ( KM->MouseWheelUp() || KM->IsKeyUp(KEY_PAGEUP) ) {
			if ( !mBlockWheelSpeed || Sys::GetTicks() - mLastWheelUse > mWheelBlockTime ) {
				mLastWheelUse = Sys::GetTicks();
				LoadPrevImage();
				DisableSlideShow();
			}
		}

		if ( KM->MouseWheelDown() || KM->IsKeyUp(KEY_PAGEDOWN) ) {
			if ( !mBlockWheelSpeed || Sys::GetTicks() - mLastWheelUse > mWheelBlockTime ) {
				mLastWheelUse = Sys::GetTicks();
				LoadNextImage();
				DisableSlideShow();
			}
		}

		if ( KM->IsKeyUp(KEY_I) ) {
			mShowInfo = !mShowInfo;
		}
	}

	if ( mFiles.size() && mFiles[ mCurImg ].Tex && !Con.Active() ) {
		if ( KM->IsKeyUp(KEY_HOME) ) {
			LoadFirstImage();
			DisableSlideShow();
		}

		if ( KM->IsKeyUp(KEY_END) ) {
			LoadLastImage();
			DisableSlideShow();
		}

		if ( KM->IsKeyUp(KEY_KP_MULTIPLY) ) {
			ScaleToScreen();
		}

		if ( KM->IsKeyUp(KEY_KP_DIVIDE) ) {
			mImg.Scale( 1.f );
		}

		if ( KM->IsKeyUp(KEY_Z) ) {
			ZoomImage();
		}

		if ( KM->IsKeyUp( KEY_N ) ) {
			if ( mWindow->Size().Width() != (Int32)mImg.Width() || mWindow->Size().Height() != (Int32)mImg.Height() ) {
				mWindow->Size( mImg.Width(), mImg.Height() );
			}
		}

		if ( Sys::GetTicks() - mZoomTicks >= 15 ) {
			mZoomTicks = Sys::GetTicks();

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
			mImg.X( ( mImg.X() + ( (mWindow->Elapsed() * 0.4f) ) ) );
			mImg.X( static_cast<eeFloat> ( static_cast<Int32> ( mImg.X() ) ) );
		}

		if ( KM->IsKeyDown(KEY_RIGHT) ) {
			mImg.X( ( mImg.X() + ( -(mWindow->Elapsed() * 0.4f) ) ) );
			mImg.X( static_cast<eeFloat> ( static_cast<Int32> ( mImg.X() ) ) );
		}

		if ( KM->IsKeyDown(KEY_UP) ) {
			mImg.Y( ( mImg.Y() + ( (mWindow->Elapsed() * 0.4f) ) ) );
			mImg.Y( static_cast<eeFloat> ( static_cast<Int32> ( mImg.Y() ) ) );
		}

		if ( KM->IsKeyDown(KEY_DOWN) ) {
			mImg.Y( ( mImg.Y() + ( -(mWindow->Elapsed() * 0.4f) ) ) );
			mImg.Y( static_cast<eeFloat> ( static_cast<Int32> ( mImg.Y() ) ) );
		}

		if ( KM->MouseLeftClick() ) {
			mMouseLeftPressing = false;
		}

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

		if ( KM->MouseMiddleClick() ) {
			mMouseMiddlePressing = false;
		}

		if ( KM->MouseMiddlePressed() ) {
			if ( !mMouseMiddlePressing ) {
				mMouseMiddleStartClick = Mouse;
				mMouseMiddlePressing = true;
			} else {
				mMouseMiddleClick = Mouse;

				eeVector2f v1( (eeFloat)mMouseMiddleStartClick.x, (eeFloat)mMouseMiddleStartClick.y );
				eeVector2f v2( eeVector2f( (eeFloat)mMouseMiddleClick.x, (eeFloat)mMouseMiddleClick.y ) );
				eeLine2f l1( v1, v2 );
				eeFloat Dist = v1.Distance( v2 ) * 0.01f;
				eeFloat Ang = l1.GetAngle();

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

		if ( KM->MouseRightPressed() ) {
			eeLine2f line( eeVector2f( Mouse.x, Mouse.y ), eeVector2f( HWidth, HHeight ) );
			mImg.Angle( line.GetAngle() );
		}

		if ( KM->IsKeyUp(KEY_X) ) {
			if ( mImgRT == RN_NORMAL )
				mImgRT = RN_FLIP;
			else if ( mImgRT == RN_MIRROR )
				mImgRT = RN_FLIPMIRROR;
			else if ( mImgRT == RN_FLIPMIRROR )
				mImgRT = RN_MIRROR;
			else
				mImgRT = RN_NORMAL;

			mImg.RenderType( mImgRT );
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

			mImg.RenderType( mImgRT );
		}

		if ( KM->IsKeyUp(KEY_R) ) {
			mImg.Angle( mImg.Angle() + 90.0f );
			ScaleToScreen();
		}

		if ( KM->IsKeyUp(KEY_A) ) {
			cTexture * Tex = mImg.GetCurrentSubTexture()->GetTexture();

			if ( Tex ) {
				if ( Tex->Filter() == TEX_FILTER_LINEAR )
					Tex->TextureFilter( TEX_FILTER_NEAREST );
				else
					Tex->TextureFilter( TEX_FILTER_LINEAR );
			}
		}

		if ( KM->IsKeyUp(KEY_M) ) {
			mImg.Position( 0.0f,0.0f );
			mImg.Scale( 1.f );
			mImg.Angle( 0.f );
			ScaleToScreen();
			EE->GetCurrentWindow()->Size( mImg.Width(), mImg.Height() );
		}

		if ( KM->IsKeyUp(KEY_T) ) {
			mImg.Position( 0.0f,0.0f );
			mImg.Scale( 1.f );
			mImg.Angle( 0.f );
			ScaleToScreen();
		}

		if ( KM->IsKeyUp(KEY_E) ) {
			CreateSlideShow( mSlideTime );
		}

		if ( KM->IsKeyUp(KEY_D) ) {
			DisableSlideShow();
		}
	}
}

void cApp::CreateSlideShow( Uint32 time ) {
	if ( time < 250 )
		time = 250;

	mSlideShow	= true;
	mSlideTime	= time;
	mSlideTicks	= Sys::GetTicks();
}

void cApp::DisableSlideShow() {
	mSlideShow = false;
}

void cApp::DoSlideShow() {
	if ( mSlideShow ) {
		if ( Sys::GetTicks() - mSlideTicks >= mSlideTime ) {
			mSlideTicks = Sys::GetTicks();

			if ( (Uint32)( mCurImg + 1 ) < mFiles.size() ) {
				LoadNextImage();
			} else {
				DisableSlideShow();
			}
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
	Width = mWindow->GetWidth();
	Height = mWindow->GetHeight();
	HWidth = Width * 0.5f;
	HHeight = Height * 0.5f;

	if (Sys::GetTicks() - mLastTicks >= 100) {
		mLastTicks = Sys::GetTicks();
		if ( mFiles.size() )
			mInfo = "EEiv - " +  mFiles[ mCurImg ].Path;
		else
			mInfo = "EEiv";

		mWindow->Caption( mInfo );
	}
}

void cApp::Render() {
	PrepareFrame();

	DoSlideShow();

	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		DoFade();

		cTexture * Tex = mImg.GetCurrentSubTexture()->GetTexture();

		if ( Tex ) {
			eeFloat X = static_cast<eeFloat> ( static_cast<Int32> ( HWidth - mImg.Width() * 0.5f ) );
			eeFloat Y = static_cast<eeFloat> ( static_cast<Int32> ( HHeight - mImg.Height() * 0.5f ) );

			mImg.OffsetX( X );
			mImg.OffsetY( Y );
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

		cTexture * Tex = NULL;

		if ( NULL != mOldImg.GetCurrentSubTexture() && ( Tex = mOldImg.GetCurrentSubTexture()->GetTexture() ) ) {
			eeFloat X = static_cast<eeFloat> ( static_cast<Int32> ( HWidth - mOldImg.Width() * 0.5f ) );
			eeFloat Y = static_cast<eeFloat> ( static_cast<Int32> ( HHeight - mOldImg.Height() * 0.5f ) );

			mOldImg.OffsetX( X );
			mOldImg.OffsetY( Y );
			mOldImg.Alpha( 255 - mCurAlpha );
			mOldImg.Draw();
		}
	}
}

void cApp::End() {
	cEngine::DestroySingleton();
}

void cApp::ResizeTexture( cTexture * pTex, const Uint32& NewWidth, const Uint32& NewHeight, const std::string& SavePath ) {
	std::string Str = SavePath + ".new.png";

	Int32 new_width = (Int32)NewWidth;
	Int32 new_height = (Int32)NewHeight;

	if ( (Int32)pTex->Width() != new_width || (Int32)pTex->Height() != new_height ) {
		pTex->Lock();

		cImage * img = eeNew( cImage, ( pTex->GetPixelsPtr(), pTex->Width(), pTex->Height(), pTex->Channels() ) );

		img->Resize( new_width, new_height );

		img->SaveToFile( Str, EE_SAVE_TYPE_PNG );

		eeSAFE_DELETE( img );

		pTex->Unlock(false, false);
	} else {
		pTex->SaveToFile( Str, EE_SAVE_TYPE_PNG );
	}
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
		Con.PushText( "Images does not exists." );
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
		Con.PushText( "Images does not exists." );
	}
}

void cApp::BatchDir( const std::string& Path, const eeFloat& Scale ) {
	std::string iPath = Path;
	std::vector<std::string> tmpFiles = FileSystem::FilesGetInPath( iPath );

	if ( iPath[ iPath.size() - 1 ] != '/' )
		iPath += "/";

	for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
		std::string fPath = iPath + tmpFiles[i];
		ScaleImg( fPath, Scale );
	}
}

void cApp::CmdSlideShow( const std::vector < String >& params ) {
	String Error( "Example of use: slideshow slide_time_in_ms" );

	if ( params.size() >= 2 ) {
		Uint32 time = 0;

		bool Res = String::FromString<Uint32> ( time, params[1] );

		if ( Res ) {
			if ( !mSlideShow ) {
				CreateSlideShow( time );
			} else {
				if ( 0 == time ) {
					mSlideShow = false;
				}
			}
		} else {
			Con.PushText( Error );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdImgResize( const std::vector < String >& params ) {
	String Error( "Example of use: imgresize new_width new_height path_to_img" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;

		bool Res1 = String::FromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::FromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].ToUtf8();
			if ( params.size() > 4 ) {
				for ( Uint32 i = 4; i < params.size(); i++ )
					myPath += " " + params[i].ToUtf8();
			}
		} else
			myPath = mFilePath + mFile;

		if ( Res1 && Res2 )
			ResizeImg( myPath, nWidth, nHeight );
		else
			Con.PushText( Error );
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdImgScale( const std::vector < String >& params ) {
	String Error( "Example of use: imgscale scale path_to_img" );
	if ( params.size() >= 2 ) {
		eeFloat Scale = 0;

		bool Res = String::FromString<eeFloat>( Scale, params[1] );

		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].ToUtf8();
			if ( params.size() > 3 ) {
				for ( Uint32 i = 3; i < params.size(); i++ )
					myPath += " " + params[i].ToUtf8();
			}
		} else
			myPath = mFilePath + mFile;

		if ( Res )
			ScaleImg( myPath, Scale );
		else
			Con.PushText( Error );
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgResize( const std::vector < String >& params ) {
	String Error( "Example of use: batchimgresize scale_value directory_to_resize_img" );
	if ( params.size() >= 3 ) {
		eeFloat Scale = 0;

		bool Res = String::FromString<eeFloat>( Scale, params[1] );

		std::string myPath = params[2].ToUtf8();
		if ( params.size() > 3 ) {
			for ( Uint32 i = 3; i < params.size(); i++ )
				myPath += " " + params[i].ToUtf8();
		}

		if ( Res ) {
			if ( FileSystem::IsDirectory( myPath ) ) {
				BatchDir( myPath, Scale );
			} else
				Con.PushText( "Second argument is not a directory!" );
		} else
			Con.PushText( Error );
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdImgChangeFormat( const std::vector < String >& params ) {
	String Error( "Example of use: imgchangeformat to_format image_to_reformat ( if null will use the current loaded image )" );
	if ( params.size() >= 2 ) {
		std::string toFormat = params[1].ToUtf8();
		std::string myPath;

		if ( params.size() >= 3 )
			myPath = params[2].ToUtf8();
		else
			myPath = mFilePath + mFile;

		if ( params.size() > 3 ) {
			for ( Uint32 i = 4; i < params.size(); i++ )
				myPath += " " + params[i].ToUtf8();
		}

		std::string fromFormat = FileSystem::FileExtension( myPath );

		Int32 x, y, c;

		if ( cImage::GetInfo( myPath, &x, &y, &c ) ) {
			std::string fPath 	= myPath;
			std::string fExt	= FileSystem::FileExtension( fPath );

			if ( IsImage( fPath ) && fExt == fromFormat ) {
				std::string fName;

				if ( fExt != toFormat )
					fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
				else
					fName = fPath + "." + toFormat;

				EE_SAVE_TYPE saveType = cImage::ExtensionToSaveType( toFormat );

				if ( EE_SAVE_TYPE_UNKNOWN != saveType ) {
					cImage * img = eeNew( cImage, ( fPath ) );
					img->SaveToFile( fName, saveType );
					eeSAFE_DELETE( img );

					Con.PushText( fName + " created." );
				}
			}
		} else
			Con.PushText( "Third argument is not a directory! Argument: " + myPath );
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgChangeFormat( const std::vector < String >& params ) {
	String Error( "Example of use: batchimgchangeformat from_format to_format directory_to_reformat" );
	if ( params.size() >= 4 ) {
		std::string fromFormat = params[1].ToUtf8();
		std::string toFormat = params[2].ToUtf8();

		std::string myPath = params[3].ToUtf8();
		if ( params.size() > 3 ) {
			for ( Uint32 i = 4; i < params.size(); i++ )
				myPath += " " + params[i].ToUtf8();
		}

		if ( FileSystem::IsDirectory( myPath ) ) {
			std::vector<std::string> tmpFiles = FileSystem::FilesGetInPath( myPath );

			if ( myPath[ myPath.size() - 1 ] != '/' )
				myPath += "/";

			for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
				std::string fPath 	= myPath + tmpFiles[i];
				std::string fExt	= FileSystem::FileExtension( fPath );

				if ( IsImage( fPath ) && fExt == fromFormat ) {
					std::string fName;

					if ( fExt != toFormat )
						fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
					else
						fName = fPath + "." + toFormat;

					EE_SAVE_TYPE saveType = cImage::ExtensionToSaveType( toFormat );

					if ( EE_SAVE_TYPE_UNKNOWN != saveType ) {
						cImage * img = eeNew( cImage, ( fPath ) );
						img->SaveToFile( fPath, saveType );
						eeSAFE_DELETE( img );

						Con.PushText( fName + " created." );
					}
				}
			}
		} else
			Con.PushText( "Third argument is not a directory! Argument: " + myPath );
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdMoveTo( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( tInt )
			tInt--;

		if ( Res && tInt >= 0 && tInt < (Int32)mFiles.size() ) {
			Con.PushText( "moveto: moving to image number " + String::ToStr( tInt + 1 ) );
			FastLoadImage( tInt );
		} else
			Con.PushText( "moveto: image number does not exists" );
	} else
		Con.PushText( "Expected some parameter" );
}

void cApp::CmdSetBlockWheel( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mBlockWheelSpeed = (bool)tInt;
			Con.PushText( "setblockwheel " + String::ToStr(tInt) );
		} else
			Con.PushText( "Valid parameters are 0 or 1." );
	} else
		Con.PushText( "Expected some parameter" );
}

void cApp::CmdSetLateLoading( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mLateLoading = (bool)tInt;
			Con.PushText( "setlateloading " + String::ToStr(tInt) );
		} else
			Con.PushText( "Valid parameters are 0 or 1." );
	} else
		Con.PushText( "Expected some parameter" );
}

void cApp::CmdSetImgFade( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mFade = (bool)tInt;
			Con.PushText( "setimgfade " + String::ToStr(tInt) );
		} else
			Con.PushText( "Valid parameters are 0 or 1." );
	} else
		Con.PushText( "Expected some parameter" );
}

void cApp::CmdSetBackColor( const std::vector < String >& params ) {
	String Error( "Example of use: setbackcolor 255 255 255 (RGB Color, numbers between 0 and 255)" );

	if ( params.size() >= 2 ) {
		if ( params.size() == 4 ) {
			Int32 R = 0;
			bool Res1 = String::FromString<Int32>( R, params[1] );
			Int32 G = 0;
			bool Res2 = String::FromString<Int32>( G, params[2] );
			Int32 B = 0;
			bool Res3 = String::FromString<Int32>( B, params[3] );

			if ( Res1 && Res2 && Res3 && ( R <= 255 && R >= 0 ) && ( G <= 255 && G >= 0 ) && ( B <= 255 && B >= 0 ) ) {
				mWindow->BackColor( eeColor( R,G,B ) );
				Con.PushText( "setbackcolor applied" );
				return;
			}
		}

		Con.PushText( Error );
	}
}

void cApp::CmdLoadImg( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].ToUtf8();

		if ( IsImage( myPath ) ) {
			LoadDir( myPath );
		} else
			Con.PushText( "\"" + myPath + "\" is not an image path or the image is not supported." );
	}
}

void cApp::CmdLoadDir( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].ToUtf8();
		if ( params.size() > 2 ) {
			for ( Uint32 i = 2; i < params.size(); i++ )
				myPath += " " + params[i].ToUtf8();
		}

		if ( FileSystem::IsDirectory( myPath ) ) {
			LoadDir( myPath );
		} else
			Con.PushText( "If you want to load an image use loadimg. \"" + myPath + "\" is not a directory path." );
	}
}

void cApp::PrintHelp() {
	if ( mShowHelp ) {
		Uint32 Top = 6;
		Uint32 Left = 6;

		if ( NULL == mHelpCache ) {
			String HT = "Keys List:\n";
			HT += "Escape: Quit from EEiv\n";
			HT += "ALT + RETURN or F: Toogle Fullscreen - Windowed\n";
			HT += String::FromUtf8( "F3 or º: Toggle Console\n" );
			HT += "F4 or S: Show/Hide Cursor\n";
			HT += "Mouse Wheel Up or PageUp: Go to Previous Image\n";
			HT += "Mouse Wheel Down or PageDown: Go to Next Image\n";
			HT += "Key I: Show/Hide Image Info\n";
			HT += "Key *: Scale image to screen\n";
			HT += "Key /: Reset Scale to 1\n";
			HT += "Key Z: Fit Image to Screen\n";
			HT += "Key + and -, or mouse middle press up or down: Zoom in and Zoom out image\n";
			HT += "Key X: Flip Image\n";
			HT += "Key C: Mirror Image\n";
			HT += "Key R: Rotate 90º Image and scale it to screen\n";
			HT += "Key T: Reset the position and the scale of the image\n";
			HT += "Key A: Change the texture filter\n";
			HT += "Key M: Change the screen size to the size of the current image.\n";
			HT += "Key E: Play SlideShow\n";
			HT += "Key D: Pause/Disable SlideShow\n";
			HT += "Key Left - Right - Top - Down or left mouse press: Move the image\n";
			HT += "Key F12: Take a screenshot\n";
			HT += "Key HOME: Go to the first screenshot on the folder\n";
			HT += "Key END: Go to the last screenshot on the folder";

			mHelpCache = eeNew( cTextCache, ( Fon, HT ) );
		}

		mHelpCache->Draw( Left, Height - Top - mHelpCache->GetTextHeight() );
	}
}
