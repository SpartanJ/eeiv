#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
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
#undef RGB
#undef CreateWindow
#endif

#include "capp.hpp"
#include <algorithm>

static bool IsImage( const std::string& path ) {
	std::string mPath = path;

	if ( path.size() >= 7 && path.substr(0,7) == "file://" )
		mPath = path.substr( 7 );

	if ( !FileSystem::IsDirectory(mPath) && FileSystem::FileSize( mPath ) ) {
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		String::ToLowerInPlace( Ext );

		if ( Ext == "png" || Ext == "tga" || Ext == "bmp" || Ext == "jpg" || Ext == "gif" || Ext == "jpeg" || Ext == "dds" || Ext == "psd" || Ext == "hdr" || Ext == "pic" || Ext == "pvr" || Ext == "pkm" )
			return true;
		else {
			return Image::IsImage( mPath );
		}
	}

	return false;
}

static bool IsHttpUrl( const std::string& path ) {
	return path.substr(0,7) == "http://" || path.substr(0,8) == "https://";
}

cApp::cApp( int argc, char *argv[] ) :
	Fon(NULL),
	Mon(NULL),
	mCurImg(0),
	mFading(false),
	mAlpha(255),
	mCurAlpha(255),
	mLaterLoad(false),
	mCursor(true),
	mMouseLeftPressing(false),
	mMouseMiddlePressing(false),
	mImgRT(RN_NORMAL),
	mShowHelp(false),
	mFirstLoad(false),
	mUsedTempDir(false),
	mHelpCache( NULL ),
	mSlideShow(false),
	mSlideTime(4000),
	mSlideTicks(Sys::GetTicks())
{
	mStorePath	= Sys::GetConfigPath( "eeiv" ) + FileSystem::GetOSlash();
	mTmpPath	= mStorePath + "tmp" + FileSystem::GetOSlash();

	std::string nstr;

	if ( argc > 1 )
		nstr.assign( argv[1] );
	else
		nstr.assign( argv[0] );

	LoadDir( nstr, false );
}

cApp::~cApp() {
	ClearTempDir();
}

void cApp::LoadConfig() {
	std::string tPath = mStorePath + "eeiv.ini";
	Ini.LoadFromFile( tPath );

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
		mConfig.ConsoleFontSize = Ini.GetValueI( "Viewer", "ConsoleFontSize", 12 );
		mConfig.AppFontSize = Ini.GetValueI( "Viewer", "AppFontSize", 12 );
		mConfig.DefaultImageZoom = Ini.GetValueF( "Viewer", "DefaultImageZoom", 1 );
		mConfig.WheelBlockTime = Ini.GetValueI( "Viewer", "WheelBlockTime", 200 );
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
		Ini.SetValueI( "Viewer", "ConsoleFontSize", 12 );
		Ini.SetValueI( "Viewer", "AppFontSize", 12 );
		Ini.SetValueI( "Viewer", "DefaultImageZoom", 1 );
		Ini.SetValueI( "Viewer", "WheelBlockTime", 200 );

		if ( !FileSystem::IsDirectory( mStorePath ) )
			FileSystem::MakeDir( mStorePath );

		Ini.WriteFile();
		LoadConfig();
	}
}

bool cApp::Init() {
	LoadConfig();

	EE 		= Engine::instance();
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

	std::string iconp( MyPath + "assets/eeiv.png" );

	if ( !FileSystem::FileExists( iconp ) ) {
		iconp = MyPath + "assets/icon/ee.png";
	}

	WindowSettings WinSettings	= EE->CreateWindowSettings( &Ini, "Window" );
	ContextSettings ConSettings	= EE->CreateContextSettings( &Ini, "Window" );

	WinSettings.Icon = iconp;
	WinSettings.Caption = "eeiv";

	mWindow = EE->CreateWindow( WinSettings, ConSettings );

	if ( mWindow->Created() ) {
		if ( mConfig.FrameLimit )
			mWindow->FrameRateLimit(60);

		TF 		= TextureFactory::instance();
		Log 	= Log::instance();
		KM 		= mWindow->GetInput();

		if ( mConfig.MaximizeAtStart )
			mWindow->Maximize();

		Clock TE;

		std::string MyFontPath = MyPath + "assets/fonts" + FileSystem::GetOSlash();

		if ( FileSystem::FileExists( MyFontPath + "DejaVuSans.dds" ) && FileSystem::FileExists( MyFontPath + "DejaVuSans.dat" ) && FileSystem::FileExists( MyFontPath + "DejaVuSansMono.dds" ) && FileSystem::FileExists( MyFontPath + "DejaVuSansMono.dat" ) ) {
			TexF 	= TextureFont::New( "DejaVuSans" );
			TexFMon = TextureFont::New( "DejaVuSansMono" );

			TexF->Load( TF->Load( MyFontPath + "DejaVuSans.dds" ), MyFontPath + "DejaVuSans.dat" );
			TexFMon->Load( TF->Load( MyFontPath + "DejaVuSansMono.dds" ), MyFontPath + "DejaVuSansMono.dat" );

			Fon = reinterpret_cast<Font*> ( TexF );
			Mon = reinterpret_cast<Font*> ( TexFMon );
		} else {
			TTF 	= TTFFont::New( "DejaVuSans" );
			TTFMon 	= TTFFont::New( "DejaVuSansMono" );

			#if EE_PLATFORM == EE_PLATFORM_WIN
			std::string fontsPath( GetWindowsPath() + "\\Fonts\\" );
			#else
			std::string fontsPath( "/usr/share/fonts/truetype/" );
			#endif

			if ( FileSystem::FileExists( fontsPath + "DejaVuSans.ttf" ) && FileSystem::FileExists( fontsPath + "DejaVuSansMono.ttf" ) ) {
				TTF->Load( fontsPath + "DejaVuSans.ttf", mConfig.AppFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
				TTFMon->Load( fontsPath + "DejaVuSansMono.ttf", mConfig.ConsoleFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
			} else if ( FileSystem::FileExists( MyFontPath + "DejaVuSans.ttf" ) && FileSystem::FileExists( MyFontPath + "DejaVuSansMono.ttf" ) ) {
				TTF->Load( MyFontPath + "DejaVuSans.ttf", mConfig.AppFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
				TTFMon->Load( MyFontPath + "DejaVuSansMono.ttf", mConfig.ConsoleFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
			} else if ( FileSystem::FileExists( fontsPath + "Arial.ttf" ) && FileSystem::FileExists( fontsPath + "cour.ttf" ) ) {
				TTF->Load( fontsPath + "Arial.ttf", mConfig.AppFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
				TTFMon->Load( fontsPath + "cour.ttf", mConfig.ConsoleFontSize, TTF_STYLE_NORMAL, 512, RGB(), 0, RGB(0,0,0) );
			} else {
				Log::instance()->Writef( "Fonts not found... closing." );
				return false;
			}

			Fon = reinterpret_cast<Font*> ( TTF );
			Mon = reinterpret_cast<Font*> ( TTFMon );
		}

		Log::instance()->Writef( "Fonts loading time: %f ms", TE.Elapsed().AsMilliseconds() );

		if ( !Fon && !Mon )
			return false;

		Con.Create( Mon, true, true, 1024000 );
		Con.IgnoreCharOnPrompt( 186 );

		Con.AddCommand( "loaddir", cb::Make1( this, &cApp::CmdLoadDir ) );
		Con.AddCommand( "loadimg", cb::Make1( this, &cApp::CmdLoadImg ) );
		Con.AddCommand( "setbackcolor", cb::Make1( this, &cApp::CmdSetBackColor ) );
		Con.AddCommand( "setimgfade", cb::Make1( this, &cApp::CmdSetImgFade ) );
		Con.AddCommand( "setlateloading", cb::Make1( this, &cApp::CmdSetLateLoading ) );
		Con.AddCommand( "setblockwheel", cb::Make1( this, &cApp::CmdSetBlockWheel ) );
		Con.AddCommand( "moveto", cb::Make1( this, &cApp::CmdMoveTo ) );
		Con.AddCommand( "batchimgscale", cb::Make1( this, &cApp::CmdBatchImgScale ) );
		Con.AddCommand( "batchimgchangeformat", cb::Make1( this, &cApp::CmdBatchImgChangeFormat ) );
		Con.AddCommand( "batchimgthumbnail", cb::Make1( this, &cApp::CmdBatchImgThumbnail ) );
		Con.AddCommand( "imgchangeformat", cb::Make1( this, &cApp::CmdImgChangeFormat ) );
		Con.AddCommand( "imgresize", cb::Make1( this, &cApp::CmdImgResize ) );
		Con.AddCommand( "imgscale", cb::Make1( this, &cApp::CmdImgScale ) );
		Con.AddCommand( "imgthumbnail", cb::Make1( this, &cApp::CmdImgThumbnail ) );
		Con.AddCommand( "imgcentercrop", cb::Make1( this, &cApp::CmdImgCenterCrop) );
		Con.AddCommand( "slideshow", cb::Make1( this, &cApp::CmdSlideShow ) );
		Con.AddCommand( "setzoom", cb::Make1( this, &cApp::CmdSetZoom ) );

		PrepareFrame();
		GetImages();

		if ( mFile != "" ) {
			FastLoadImage( CurImagePos( mFile ) );
		} else {
			if ( mFiles.size() )
				FastLoadImage( 0 );
		}

		if ( 0 == mFiles.size() && 0 == mFile.length() ) {
			Con.Toggle();
		}

		return true;
	}

	return false;
}

void cApp::Process() {
	if ( Init() ) {
		do {
			ET = mWindow->Elapsed().AsMilliseconds();

			Input();

			TEP.Restart();

			if ( mWindow->Visible() ) {
				Render();

				if ( KM->IsKeyUp(KEY_F12) ) mWindow->TakeScreenshot();

				mWindow->Display(true);
			} else {
				Sys::Sleep( 16 );
			}

			RET = TEP.Elapsed().AsMilliseconds();

			if ( mConfig.LateLoading && mLaterLoad ) {
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
	std::string tmpFile;

	if ( !FileSystem::IsDirectory( path ) ) {
		if ( path.substr(0,7) == "file://" ) {
			mFilePath = path.substr( 7 );
			mFilePath = mFilePath.substr( 0, mFilePath.find_last_of( FileSystem::GetOSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::GetOSlash() ) + 1 );
		} else if ( IsHttpUrl( path ) ) {
			mUsedTempDir = true;

			if ( !FileSystem::IsDirectory( mTmpPath ) )
				FileSystem::MakeDir( mTmpPath );

			URI uri( path );
			Http http( uri.GetHost(), uri.GetPort() );
			Http::Request request( uri.GetPathAndQuery() );
			Http::Response response = http.SendRequest(request);

			if ( response.GetStatus() == Http::Response::Ok ) {
				if ( !FileSystem::FileWrite( mTmpPath + "tmpfile", reinterpret_cast<const Uint8*>( &response.GetBody()[0] ), response.GetBody().size() ) ) {
					Con.PushText( "Couldn't write the downloaded image to disk." );

					return;
				}
			} else {
				Con.PushText( "Couldn't download the image from network." );

				return;
			}

			mFilePath = mTmpPath;
			tmpFile = "tmpfile";
		} else {
			mFilePath = path.substr( 0, path.find_last_of( FileSystem::GetOSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::GetOSlash() ) + 1 );
		}

		String::ReplaceAll( mFilePath, "%20", " " );

		if ( mFilePath == "" ) {
			#if EE_PLATFORM == EE_PLATFORM_WIN
				mFilePath = "C:\\";
			#else
				mFilePath = FileSystem::GetOSlash();
			#endif
		}

		FileSystem::DirPathAddSlashAtEnd( mFilePath );

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
	Clock TE;

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

	Con.PushText( "Image list loaded in %f ms.", TE.Elapsed().AsMilliseconds() );

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
		mImg.RenderMode( mImgRT );
		mImg.Scale( mConfig.DefaultImageZoom );
		mImg.Position( 0.0f, 0.0f );

		if ( path != mFiles[ mCurImg ].Path )
			mCurImg = CurImagePos( path );

		mFile = mFiles[ mCurImg ].Path;

		ScaleToScreen();

		Texture * pTex = TF->GetTexture( Tex );

		if ( NULL != pTex ) {
			Fon->SetText(
				"File: " + String::FromUtf8( mFile ) +
				"\nWidth: " + String::ToStr( pTex->Width() ) +
				"\nHeight: " + String::ToStr( pTex->Height() ) +
				"\n" + String::ToStr( mCurImg+1 ) + "/" + String::ToStr( mFiles.size() )
			);
		}
	} else {
		Fon->SetText( "File: " + String::FromUtf8( path ) + " failed to load. \nReason: " + Image::GetLastFailureReason() );
	}
}

Uint32 cApp::LoadImage( const std::string& path, const bool& SetAsCurrent ) {
	Uint32 TexId 		= 0;

	TexId = TF->Load( mFilePath + path );

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
	mImg.Scale( mConfig.DefaultImageZoom );
	mImg.Position( 0.0f, 0.0f );

	ScaleToScreen();

	if ( mConfig.LateLoading ) {
		mLaterLoad = true;
		mLastLaterTick = Sys::GetTicks();

		Texture * Tex = TF->GetTexture( mFiles [ mCurImg ].Tex );

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
	if ( mConfig.Fade ) {
		mAlpha = 255.0f;
		mCurAlpha = 255;
		mFading = false;
	}

	mConfig.Fade = !mConfig.Fade;
	mConfig.LateLoading = !mConfig.LateLoading;
	mConfig.BlockWheelSpeed = !mConfig.BlockWheelSpeed;
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
			if ( !mConfig.BlockWheelSpeed || Sys::GetTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::GetTicks();
				LoadPrevImage();
				DisableSlideShow();
			}
		}

		if ( KM->MouseWheelDown() || KM->IsKeyUp(KEY_PAGEDOWN) ) {
			if ( !mConfig.BlockWheelSpeed || Sys::GetTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::GetTicks();
				LoadNextImage();
				DisableSlideShow();
			}
		}

		if ( KM->IsKeyUp(KEY_I) ) {
			mConfig.ShowInfo = !mConfig.ShowInfo;
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
			mImg.Scale( mConfig.DefaultImageZoom );
		}

		if ( KM->IsKeyUp(KEY_Z) ) {
			ZoomImage();
		}

		if ( KM->IsKeyUp( KEY_N ) ) {
			if ( mWindow->Size().Width() != (Int32)mImg.Size().Width() || mWindow->Size().Height() != (Int32)mImg.Size().Height() ) {
				mWindow->Size( mImg.Size().Width(), mImg.Size().Height() );
			}
		}

		if ( Sys::GetTicks() - mZoomTicks >= 15 ) {
			mZoomTicks = Sys::GetTicks();

			if ( KM->IsKeyDown(KEY_KP_MINUS) )
				mImg.Scale( mImg.Scale() - 0.02f );


			if ( KM->IsKeyDown(KEY_KP_PLUS) )
				mImg.Scale( mImg.Scale() + 0.02f );

			if ( mImg.Scale().x < 0.0125f )
				mImg.Scale( 0.0125f );

			if ( mImg.Scale().x > 50.0f )
				mImg.Scale( 50.0f );
		}

		if ( KM->IsKeyDown(KEY_LEFT) ) {
			mImg.X( ( mImg.X() + ( (mWindow->Elapsed().AsMilliseconds() * 0.4f) ) ) );
			mImg.X( static_cast<Float> ( static_cast<Int32> ( mImg.X() ) ) );
		}

		if ( KM->IsKeyDown(KEY_RIGHT) ) {
			mImg.X( ( mImg.X() + ( -(mWindow->Elapsed().AsMilliseconds() * 0.4f) ) ) );
			mImg.X( static_cast<Float> ( static_cast<Int32> ( mImg.X() ) ) );
		}

		if ( KM->IsKeyDown(KEY_UP) ) {
			mImg.Y( ( mImg.Y() + ( (mWindow->Elapsed().AsMilliseconds() * 0.4f) ) ) );
			mImg.Y( static_cast<Float> ( static_cast<Int32> ( mImg.Y() ) ) );
		}

		if ( KM->IsKeyDown(KEY_DOWN) ) {
			mImg.Y( ( mImg.Y() + ( -(mWindow->Elapsed().AsMilliseconds() * 0.4f) ) ) );
			mImg.Y( static_cast<Float> ( static_cast<Int32> ( mImg.Y() ) ) );
		}

		if ( KM->MouseLeftClick() ) {
			mMouseLeftPressing = false;
		}

		if ( KM->MouseLeftPressed() ) {
			Vector2f mNewPos;
			if ( !mMouseLeftPressing ) {
				mMouseLeftStartClick = Mouse;
				mMouseLeftPressing = true;
			} else {
				mMouseLeftClick = Mouse;

				mNewPos.x = static_cast<Float>(mMouseLeftClick.x) - static_cast<Float>(mMouseLeftStartClick.x);
				mNewPos.y = static_cast<Float>(mMouseLeftClick.y) - static_cast<Float>(mMouseLeftStartClick.y);

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

				Vector2f v1( (Float)mMouseMiddleStartClick.x, (Float)mMouseMiddleStartClick.y );
				Vector2f v2( Vector2f( (Float)mMouseMiddleClick.x, (Float)mMouseMiddleClick.y ) );
				Line2f l1( v1, v2 );
				Float Dist = v1.Distance( v2 ) * 0.01f;
				Float Ang = l1.GetAngle();

				if ( Dist ) {
					mMouseMiddleStartClick = Mouse;
					if ( Ang >= 0.0f && Ang <= 180.0f ) {
						mImg.Scale( mImg.Scale() - Dist );
						if ( mImg.Scale().x < 0.0125f )
							mImg.Scale( 0.0125f );
					} else {
						mImg.Scale( mImg.Scale() + Dist );
					}
				}
			}
		}

		if ( KM->MouseRightPressed() ) {
			Line2f line( Vector2f( Mouse.x, Mouse.y ), Vector2f( HWidth, HHeight ) );
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

			mImg.RenderMode( mImgRT );
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

			mImg.RenderMode( mImgRT );
		}

		if ( KM->IsKeyUp(KEY_R) ) {
			mImg.Angle( mImg.Angle() + 90.0f );
			ScaleToScreen();
		}

		if ( KM->IsKeyUp(KEY_A) ) {
			Texture * Tex = mImg.GetCurrentSubTexture()->GetTexture();

			if ( Tex ) {
				if ( Tex->Filter() == TEX_FILTER_LINEAR )
					Tex->Filter( TEX_FILTER_NEAREST );
				else
					Tex->Filter( TEX_FILTER_LINEAR );
			}
		}

		if ( KM->IsKeyUp(KEY_M) ) {
			mImg.Position( 0.0f,0.0f );
			mImg.Scale( mConfig.DefaultImageZoom );
			mImg.Angle( 0.f );
			ScaleToScreen();

			if ( EE->GetCurrentWindow()->IsMaximized() ) {
				EE->GetCurrentWindow()->Size( mImg.Size().Width(), mImg.Size().Height() );
			}
		}

		if ( KM->IsKeyUp(KEY_T) ) {
			mImg.Position( 0.0f,0.0f );
			mImg.Scale( mConfig.DefaultImageZoom );
			mImg.Angle( 0.f );
			ScaleToScreen();
		}

		if ( KM->IsKeyUp(KEY_E) ) {
			CreateSlideShow( mSlideTime );
		}

		if ( KM->IsKeyUp(KEY_D) ) {
			DisableSlideShow();
		}

		if ( KM->IsKeyUp(KEY_K) ) {
			Texture * curTex;

			if ( NULL != mImg.GetCurrentSubTexture() && NULL != ( curTex = mImg.GetCurrentSubTexture()->GetTexture() ) ) {
				curTex->Mipmap( !curTex->Mipmap() );
				curTex->Reload();
			}
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
		Texture* Tex = TF->GetTexture( mFiles[ mCurImg ].Tex );

		if ( NULL == Tex )
			return;

		if ( Tex->ImgWidth() * mConfig.DefaultImageZoom >= Width || Tex->ImgHeight() * mConfig.DefaultImageZoom >= Height ) {
			ZoomImage();
		} else if ( force ) {
			mImg.Scale( mConfig.DefaultImageZoom );
		}
	}
}

void cApp::ZoomImage() {
	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		Texture* Tex = TF->GetTexture( mFiles[ mCurImg ].Tex );

		if ( NULL == Tex )
			return;

		Sizef boxSize = mImg.Size();

		mImg.Scale( eemin( Width / boxSize.Width(), Height / boxSize.Height() ) );
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

		Texture * Tex = mImg.GetCurrentSubTexture()->GetTexture();

		if ( Tex ) {
			Float X = static_cast<Float> ( static_cast<Int32> ( HWidth - mImg.Size().Width() * 0.5f ) );
			Float Y = static_cast<Float> ( static_cast<Int32> ( HHeight - mImg.Size().Height() * 0.5f ) );

			mImg.Offset( Vector2i( X, Y ) );
			mImg.Alpha( mCurAlpha );
			mImg.Draw();
		}
	}

	if ( mConfig.ShowInfo )
		Fon->Draw( 0, 0 );

	PrintHelp();

	Con.Draw();
}

void cApp::CreateFade()  {
	if ( mConfig.Fade ) {
		mAlpha = 0.0f;
		mCurAlpha = 0;
		mFading = true;
		mOldImg = mImg;
	}
}

void cApp::DoFade() {
	if ( mConfig.Fade && mFading ) {
		mAlpha += ( 255 * RET ) / mConfig.TransitionTime;
		mCurAlpha = static_cast<Uint8> ( mAlpha );

		if ( mAlpha >= 255.0f ) {
			mAlpha = 255.0f;
			mCurAlpha = 255;
			mFading = false;
		}

		Texture * Tex = NULL;

		if ( NULL != mOldImg.GetCurrentSubTexture() && ( Tex = mOldImg.GetCurrentSubTexture()->GetTexture() ) ) {
			Float X = static_cast<Float> ( static_cast<Int32> ( HWidth - mOldImg.Size().Width() * 0.5f ) );
			Float Y = static_cast<Float> ( static_cast<Int32> ( HHeight - mOldImg.Size().Height() * 0.5f ) );

			mOldImg.Offset( Vector2i( X, Y ) );
			mOldImg.Alpha( 255 - mCurAlpha );
			mOldImg.Draw();
		}
	}
}

void cApp::End() {
	mConfig.Width			= EE->GetWidth();
	mConfig.Height			= EE->GetHeight();
	mConfig.MaximizeAtStart	= EE->GetCurrentWindow()->IsMaximized();

	Ini.SetValueI( "Window", "Width", mConfig.Width );
	Ini.SetValueI( "Window", "Height", mConfig.Height );
	Ini.SetValueI( "Window", "BitColor", mConfig.BitColor );
	Ini.SetValueI( "Window", "Windowed", mConfig.Windowed );
	Ini.SetValueI( "Window", "Resizeable", mConfig.Resizeable );
	Ini.SetValueI( "Window", "VSync", mConfig.VSync );
	Ini.SetValueI( "Window", "DoubleBuffering", mConfig.DoubleBuffering );
	Ini.SetValueI( "Window", "UseDesktopResolution", mConfig.UseDesktopResolution );
	Ini.SetValueI( "Window", "NoFrame", mConfig.NoFrame );
	Ini.SetValueI( "Window", "MaximizeAtStart", mConfig.MaximizeAtStart );
	Ini.SetValueI( "Window", "FrameLimit", mConfig.FrameLimit );
	Ini.SetValueI( "Viewer", "Fade", mConfig.Fade );
	Ini.SetValueI( "Viewer", "LateLoading", mConfig.LateLoading );
	Ini.SetValueI( "Viewer", "BlockWheelSpeed", mConfig.BlockWheelSpeed );
	Ini.SetValueI( "Viewer", "ShowInfo", mConfig.ShowInfo );
	Ini.SetValueI( "Viewer", "TransitionTime", mConfig.TransitionTime );
	Ini.SetValueI( "Viewer", "ConsoleFontSize", mConfig.ConsoleFontSize );
	Ini.SetValueI( "Viewer", "AppFontSize", mConfig.AppFontSize );
	Ini.SetValueF( "Viewer", "DefaultImageZoom", mConfig.DefaultImageZoom );
	Ini.SetValueI( "Viewer", "WheelBlockTime", mConfig.WheelBlockTime );

	Ini.WriteFile();
	Engine::DestroySingleton();
}

std::string cApp::CreateSavePath( const std::string & oriPath, Uint32 width, Uint32 height, EE_SAVE_TYPE saveType ) {
	EE_SAVE_TYPE type = saveType == SAVE_TYPE_UNKNOWN ? Image::ExtensionToSaveType( FileSystem::FileExtension( oriPath ) ) : saveType;

	if ( SAVE_TYPE_UNKNOWN == type ) {
		type = SAVE_TYPE_PNG;
	}

	return FileSystem::FileRemoveExtension( oriPath ) + "-" + String::ToStr( width ) + "x" + String::ToStr( height ) + "." + Image::SaveTypeToExtension( type );
}

EE_SAVE_TYPE cApp::GetPathSaveType( const std::string& path ) {
	return Image::ExtensionToSaveType( FileSystem::FileExtension( path ) );
}

void cApp::ScaleImg( const std::string& Path, const Float& Scale , EE_SAVE_TYPE saveType ) {
	int w, h, c;

	if ( Image::GetInfo( Path, &w, &h, &c ) && Scale > 0.f ) {
		Int32 new_width		= static_cast<Int32>( w * Scale );
		Int32 new_height	= static_cast<Int32>( h * Scale );

		ResizeImg( Path, new_width, new_height, saveType );
	} else {
		Con.PushText( "Images does not exists." );
	}
}

void cApp::ResizeImg( const std::string& Path, const Uint32& NewWidth, const Uint32& NewHeight, EE_SAVE_TYPE saveType ) {
	if ( IsImage( Path ) ) {
		std::string newPath( CreateSavePath( Path, NewWidth, NewHeight, saveType ) );
		EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

		Image img( Path );

		img.Resize( NewWidth, NewHeight );

		img.SaveToFile( newPath, type );
	} else {
		Con.PushText( "Images does not exists." );
	}
}

void cApp::ThumgnailImg( const std::string& Path, const Uint32& MaxWidth, const Uint32& MaxHeight, EE_SAVE_TYPE saveType ) {
	if ( IsImage( Path ) ) {
		Image img( Path );

		Image * thumb = img.Thumbnail( MaxWidth, MaxHeight );

		if ( NULL != thumb ) {
			std::string newPath( CreateSavePath( Path, thumb->Width(), thumb->Height(), saveType ) );
			EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

			thumb->SaveToFile( newPath, type );

			eeSAFE_DELETE( thumb );
		}
	} else {
		Con.PushText( "Images does not exists." );
	}
}

void cApp::CenterCropImg( const std::string& Path, const Uint32& Width, const Uint32& Height, EE_SAVE_TYPE saveType ) {
	if ( IsImage( Path ) ) {
		Image img( Path );

		Sizei nSize;

		double scale = 1.f;

		if ( Width*Height < img.Width()*img.Height() ) {
			scale = eemax( (double)Width / (double)img.Width(), (double)Height / (double)img.Height() );
		} else {
			scale = eemax( (double)img.Width() / (double)Width, (double)img.Height() / (double)Height );
		}

		nSize.x = Math::Round( img.Width() * scale );
		nSize.y = Math::Round( img.Height() * scale );

		if ( nSize.Width() == (int)Width - 1 || nSize.Width() == (int)Width + 1 ) {
			nSize.x = (int)Width;
		}

		if ( nSize.Height() == (int)Height - 1 || nSize.Height() == (int)Height + 1 ) {
			nSize.y = (int)Height;
		}

		img.Resize( nSize.Width(), nSize.Height() );

		Image * croppedImg  = NULL;
		Recti rect;

		if ( img.Width() > Width ) {
			rect.Left = ( img.Width() - Width ) / 2;
			rect.Right = rect.Left + Width;
			rect.Top = 0;
			rect.Bottom = Height;
		} else {
			rect.Top = ( img.Height() - Height ) / 2;
			rect.Bottom = rect.Top + Height;
			rect.Left = 0;
			rect.Right = Width;
		}

		croppedImg = img.Crop( rect );

		if ( NULL != croppedImg ) {
			std::string newPath( CreateSavePath( Path, croppedImg->Width(), croppedImg->Height(), saveType ) );
			EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

			croppedImg->SaveToFile( newPath, type );

			eeSAFE_DELETE( croppedImg );
		} else {
			std::string newPath( CreateSavePath( Path, img.Width(), img.Height(), saveType ) );
			EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

			img.SaveToFile( newPath, type );
		}
	}
}

void cApp::BatchImgScale( const std::string& Path, const Float& Scale ) {
	std::string iPath = Path;
	std::vector<std::string> tmpFiles = FileSystem::FilesGetInPath( iPath );

	if ( iPath[ iPath.size() - 1 ] != '/' )
		iPath += "/";

	for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
		std::string fPath = iPath + tmpFiles[i];
		ScaleImg( fPath, Scale );
	}
}

void cApp::BatchImgThumbnail( Sizei size, std::string dir, bool recursive ) {
	FileSystem::DirPathAddSlashAtEnd( dir );

	std::vector<std::string> files = FileSystem::FilesGetInPath( dir );

	for ( size_t i = 0; i < files.size(); i++ ) {
		std::string fpath( dir + files[i] );

		if ( FileSystem::IsDirectory( fpath ) ) {
			if ( recursive ) {
				BatchImgThumbnail( size, fpath, recursive );
			}
		} else {
			int w, h, c;
			if ( Image::GetInfo( fpath, &w, &h, &c ) ) {
				if ( w > size.Width() || h > size.Height() ) {
					Image img( fpath );

					Image * thumb = img.Thumbnail( size.Width(), size.Height() );

					if ( NULL != thumb ) {
						thumb->SaveToFile( fpath, Image::ExtensionToSaveType( FileSystem::FileExtension( fpath ) ) );

						Con.PushText( "Thumbnail created for '%s'. Old size %dx%d. New size %dx%d.", fpath.c_str(), img.Width(), img.Height(), thumb->Width(), thumb->Height() );

						eeSAFE_DELETE( thumb );
					} else {
						Con.PushText( "Thumbnail %s failed to create.", fpath.c_str() );
					}
				}
			}
		}
	}
}

void cApp::CmdSlideShow( const std::vector < String >& params ) {
	String Error( "Usage example: slideshow slide_time_in_ms" );

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
	String Error( "Usage example: imgresize new_width new_height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res1 = String::FromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::FromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].ToUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::ExtensionToSaveType( params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 ) {
			ResizeImg( myPath, nWidth, nHeight, saveType );
		} else {
			Con.PushText( Error );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdImgThumbnail( const std::vector < String >& params ) {
	String Error( "Usage example: imgthumbnail max_width max_height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res1 = String::FromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::FromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].ToUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::ExtensionToSaveType( params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 ) {
			ThumgnailImg( myPath, nWidth, nHeight, saveType );
		} else {
			Con.PushText( Error );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdImgCenterCrop( const std::vector < String >& params ) {
	String Error( "Usage example: imgcentercrop width height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res1 = String::FromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::FromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].ToUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::ExtensionToSaveType( params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 )
			CenterCropImg( myPath, nWidth, nHeight, saveType );
		else
			Con.PushText( Error );
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdImgScale( const std::vector < String >& params ) {
	String Error( "Usage example: imgscale scale path_to_img format" );
	if ( params.size() >= 2 ) {
		Float Scale = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res = String::FromString<Float>( Scale, params[1] );

		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].ToUtf8();

			if ( params.size() > 3 ) {
				saveType = Image::ExtensionToSaveType( params[3] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res )
			ScaleImg( myPath, Scale, saveType );
		else
			Con.PushText( Error );
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgScale( const std::vector < String >& params ) {
	String Error( "Usage example: batchimgscale scale_value directory_to_resize_img ( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 3 ) {
		Float Scale = 0;

		bool Res = String::FromString<Float>( Scale, params[1] );

		std::string myPath = params.size() >= 3 ? params[2].ToUtf8() : mFilePath;

		if ( Res ) {
			if ( FileSystem::IsDirectory( myPath ) ) {
				BatchImgScale( myPath, Scale );
			} else {
				Con.PushText( "Second argument is not a directory!" );
			}
		} else {
			Con.PushText( Error );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgThumbnail( const std::vector < String >& params ) {
	String Error( "Usage example: batchimgthumbnail max_width max_height directory_to_create_thumbs recursive ( if no dir is passed, it will use the current dir opened )" );

	if ( params.size() >= 3 ) {
		Uint32 max_width = 0, max_height = 0;
		bool recursive = false;

		bool Res1 = String::FromString<Uint32>( max_width, params[1] );
		bool Res2 = String::FromString<Uint32>( max_height, params[2] );

		std::string myPath = params.size() >= 4 ? params[3].ToUtf8() : mFilePath;

		if ( params.size() > 4 && params[4] == "recursive" ) {
			recursive = true;
		}

		if ( Res1 && Res2 ) {
			if ( FileSystem::IsDirectory( myPath ) ) {
				BatchImgThumbnail( Sizei( max_width, max_height ), myPath, recursive );
			} else {
				Con.PushText( "Third argument is not a directory!" );
			}
		} else {
			Con.PushText( Error );
		}
	}
	else
	{
		Con.PushText( Error );
	}
}

void cApp::CmdImgChangeFormat( const std::vector < String >& params ) {
	String Error( "Usage example: imgchangeformat to_format image_to_reformat ( if null will use the current loaded image )" );
	if ( params.size() >= 2 ) {
		std::string toFormat = params[1].ToUtf8();
		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].ToUtf8();
		} else {
			myPath = mFilePath + mFile;
		}

		std::string fromFormat = FileSystem::FileExtension( myPath );

		if ( Image::IsImage( myPath ) ) {
			std::string fPath 	= myPath;
			std::string fExt	= FileSystem::FileExtension( fPath );

			if ( fExt == fromFormat ) {
				std::string fName;

				if ( fExt != toFormat )
					fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
				else
					fName = fPath + "." + toFormat;

				EE_SAVE_TYPE saveType = Image::ExtensionToSaveType( toFormat );

				if ( SAVE_TYPE_UNKNOWN != saveType ) {
					Image * img = eeNew( Image, ( fPath ) );
					img->SaveToFile( fName, saveType );
					eeSAFE_DELETE( img );

					Con.PushText( fName + " created." );
				}
			}
		} else {
			Con.PushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con.PushText( Error );
	}
}

void cApp::CmdBatchImgChangeFormat( const std::vector < String >& params ) {
	String Error( "Usage example: batchimgchangeformat from_format to_format directory_to_reformat ( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 4 ) {
		std::string fromFormat = params[1].ToUtf8();
		std::string toFormat = params[2].ToUtf8();

		std::string myPath = params.size() >= 4 ? params[3].ToUtf8() : mFilePath;

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

					EE_SAVE_TYPE saveType = Image::ExtensionToSaveType( toFormat );

					if ( SAVE_TYPE_UNKNOWN != saveType ) {
						Image * img = eeNew( Image, ( fPath ) );
						img->SaveToFile( fPath, saveType );
						eeSAFE_DELETE( img );

						Con.PushText( fName + " created." );
					}
				}
			}
		} else {
			Con.PushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
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
		} else {
			Con.PushText( "moveto: image number does not exists" );
		}
	} else {
		Con.PushText( "Expected some parameter" );
	}
}

void cApp::CmdSetBlockWheel( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.BlockWheelSpeed = (bool)tInt;
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
			mConfig.LateLoading = (bool)tInt;
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
			mConfig.Fade = (bool)tInt;
			Con.PushText( "setimgfade " + String::ToStr(tInt) );
		} else
			Con.PushText( "Valid parameters are 0 or 1." );
	} else
		Con.PushText( "Expected some parameter" );
}

void cApp::CmdSetBackColor( const std::vector < String >& params ) {
	String Error( "Usage example: setbackcolor 255 255 255 (RGB Color, numbers between 0 and 255)" );

	if ( params.size() >= 2 ) {
		if ( params.size() == 4 ) {
			Int32 R = 0;
			bool Res1 = String::FromString<Int32>( R, params[1] );
			Int32 G = 0;
			bool Res2 = String::FromString<Int32>( G, params[2] );
			Int32 B = 0;
			bool Res3 = String::FromString<Int32>( B, params[3] );

			if ( Res1 && Res2 && Res3 && ( R <= 255 && R >= 0 ) && ( G <= 255 && G >= 0 ) && ( B <= 255 && B >= 0 ) ) {
				mWindow->BackColor( RGB( R,G,B ) );
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

		if ( IsImage( myPath ) || IsHttpUrl( myPath ) ) {
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

void cApp::CmdSetZoom( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0;

		bool Res = String::FromString<Float>( tFloat, params[1] );

		if ( Res && tFloat >= 0 && tFloat <= 10 ) {
			Con.PushText( "setzoom: zoom level " + String::ToStr( tFloat ) );
			mImg.Scale( tFloat );
		} else
			Con.PushText( "setzoom: value out of range" );
	} else
		Con.PushText( "Expected some parameter" );
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
			HT += "Key K: Reload the image switching the mipmap state ( with or without mipmaps )\n";
			HT += "Key Left - Right - Top - Down or left mouse press: Move the image\n";
			HT += "Key F12: Take a screenshot\n";
			HT += "Key HOME: Go to the first screenshot on the folder\n";
			HT += "Key END: Go to the last screenshot on the folder";

			mHelpCache = eeNew( TextCache, ( Fon, HT ) );
		}

		mHelpCache->Draw( Left, Height - Top - mHelpCache->GetTextHeight() );
	}
}
