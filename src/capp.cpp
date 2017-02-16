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
		return String( Buffer ).toUtf8();
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

	if ( !FileSystem::isDirectory(mPath) && FileSystem::fileSize( mPath ) ) {
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		String::toLowerInPlace( Ext );

		if ( Ext == "png" || Ext == "tga" || Ext == "bmp" || Ext == "jpg" || Ext == "gif" || Ext == "jpeg" || Ext == "dds" || Ext == "psd" || Ext == "hdr" || Ext == "pic" || Ext == "pvr" || Ext == "pkm" )
			return true;
		else {
			return Image::isImage( mPath );
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
	mSlideTicks(Sys::getTicks())
{
	mStorePath	= Sys::getConfigPath( "eeiv" ) + FileSystem::getOSlash();
	mTmpPath	= mStorePath + "tmp" + FileSystem::getOSlash();

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

void cApp::GetConfig() {
	mConfig.Width = Ini.getValueI( "Window", "Width", 1024 );
	mConfig.Height = Ini.getValueI( "Window", "Height", 768 );
	mConfig.BitColor = Ini.getValueI( "Window", "BitColor", 32 );
	mConfig.Windowed = Ini.getValueB( "Window", "Windowed", true );
	mConfig.Resizeable = Ini.getValueB( "Window", "Resizeable", true );
	mConfig.VSync = Ini.getValueI( "Window", "VSync", true );
	mConfig.DoubleBuffering = Ini.getValueB( "Window", "DoubleBuffering", true );
	mConfig.UseDesktopResolution = Ini.getValueB( "Window", "UseDesktopResolution", false );
	mConfig.NoFrame = Ini.getValueB( "Window", "NoFrame", false );
	mConfig.MaximizeAtStart = Ini.getValueB( "Window", "MaximizeAtStart", true );
	mConfig.FrameLimit = Ini.getValueI( "Window", "FrameLimit", 0 );
	mConfig.Fade = Ini.getValueB( "Viewer", "Fade", true );
	mConfig.LateLoading = Ini.getValueB( "Viewer", "LateLoading", true );
	mConfig.BlockWheelSpeed = Ini.getValueB( "Viewer", "BlockWheelSpeed", true );
	mConfig.ShowInfo = Ini.getValueB( "Viewer", "ShowInfo", true );
	mConfig.TransitionTime = Ini.getValueI( "Viewer", "TransitionTime", 200 );
	mConfig.ConsoleFontSize = Ini.getValueI( "Viewer", "ConsoleFontSize", 12 );
	mConfig.AppFontSize = Ini.getValueI( "Viewer", "AppFontSize", 12 );
	mConfig.DefaultImageZoom = Ini.getValueF( "Viewer", "DefaultImageZoom", 1 );
	mConfig.WheelBlockTime = Ini.getValueI( "Viewer", "WheelBlockTime", 200 );
}

void cApp::LoadConfig() {
	std::string tPath = mStorePath + "eeiv.ini";
	Ini.loadFromFile( tPath );

	if ( FileSystem::fileExists( tPath ) ) {
		Ini.readFile();
		GetConfig();
	} else {
		Ini.setValueI( "Window", "Width", 1024 );
		Ini.setValueI( "Window", "Height", 768 );
		Ini.setValueI( "Window", "BitColor", 32);
		Ini.setValueI( "Window", "Windowed", 1 );
		Ini.setValueI( "Window", "Resizeable", 1 );
		Ini.setValueI( "Window", "VSync", 1 );
		Ini.setValueI( "Window", "DoubleBuffering", 1 );
		Ini.setValueI( "Window", "UseDesktopResolution", 0 );
		Ini.setValueI( "Window", "NoFrame", 0 );
		Ini.setValueI( "Window", "MaximizeAtStart", 1 );
		Ini.setValueI( "Window", "FrameLimit", 0 );
		Ini.setValueI( "Viewer", "Fade", 1 );
		Ini.setValueI( "Viewer", "LateLoading", 1 );
		Ini.setValueI( "Viewer", "BlockWheelSpeed", 1 );
		Ini.setValueI( "Viewer", "ShowInfo", 1 );
		Ini.setValueI( "Viewer", "TransitionTime", 200 );
		Ini.setValueI( "Viewer", "ConsoleFontSize", 12 );
		Ini.setValueI( "Viewer", "AppFontSize", 12 );
		Ini.setValueI( "Viewer", "DefaultImageZoom", 1 );
		Ini.setValueI( "Viewer", "WheelBlockTime", 200 );

		if ( !FileSystem::isDirectory( mStorePath ) )
			FileSystem::makeDir( mStorePath );

		Ini.writeFile();
		GetConfig();
	}
}

bool cApp::Init() {
	LoadConfig();

	EE 		= Engine::instance();
	MyPath 	= Sys::getProcessPath();

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

	if ( !FileSystem::fileExists( iconp ) ) {
		iconp = MyPath + "assets/icon/ee.png";
	}

	WindowSettings WinSettings	= EE->createWindowSettings( &Ini, "Window" );
	ContextSettings ConSettings	= EE->createContextSettings( &Ini, "Window" );

	WinSettings.Icon = iconp;
	WinSettings.Caption = "eeiv";

	mWindow = EE->createWindow( WinSettings, ConSettings );

	if ( mWindow->created() ) {
		if ( mConfig.FrameLimit )
			mWindow->frameRateLimit(60);

		TF 		= TextureFactory::instance();
		Log 	= Log::instance();
		KM 		= mWindow->getInput();

		if ( mConfig.MaximizeAtStart )
			mWindow->maximize();

		Clock TE;

		std::string MyFontPath = MyPath + "assets/fonts" + FileSystem::getOSlash();

		if ( FileSystem::fileExists( MyFontPath + "DejaVuSans.dds" ) && FileSystem::fileExists( MyFontPath + "DejaVuSans.dat" ) && FileSystem::fileExists( MyFontPath + "DejaVuSansMono.dds" ) && FileSystem::fileExists( MyFontPath + "DejaVuSansMono.dat" ) ) {
			TexF 	= TextureFont::New( "DejaVuSans" );
			TexFMon = TextureFont::New( "DejaVuSansMono" );

			TexF->load( TF->load( MyFontPath + "DejaVuSans.dds" ), MyFontPath + "DejaVuSans.dat" );
			TexFMon->load( TF->load( MyFontPath + "DejaVuSansMono.dds" ), MyFontPath + "DejaVuSansMono.dat" );

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

			if ( FileSystem::fileExists( fontsPath + "DejaVuSans.ttf" ) && FileSystem::fileExists( fontsPath + "DejaVuSansMono.ttf" ) ) {
				TTF->load( fontsPath + "DejaVuSans.ttf", mConfig.AppFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
				TTFMon->load( fontsPath + "DejaVuSansMono.ttf", mConfig.ConsoleFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
			} else if ( FileSystem::fileExists( MyFontPath + "DejaVuSans.ttf" ) && FileSystem::fileExists( MyFontPath + "DejaVuSansMono.ttf" ) ) {
				TTF->load( MyFontPath + "DejaVuSans.ttf", mConfig.AppFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
				TTFMon->load( MyFontPath + "DejaVuSansMono.ttf", mConfig.ConsoleFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
			} else if ( FileSystem::fileExists( fontsPath + "Arial.ttf" ) && FileSystem::fileExists( fontsPath + "cour.ttf" ) ) {
				TTF->load( fontsPath + "Arial.ttf", mConfig.AppFontSize, TTF_STYLE_NORMAL, 512, RGB(), 1, RGB(0,0,0) );
				TTFMon->load( fontsPath + "cour.ttf", mConfig.ConsoleFontSize, TTF_STYLE_NORMAL, 512, RGB(), 0, RGB(0,0,0) );
			} else {
				Log::instance()->writef( "Fonts not found... closing." );
				return false;
			}

			Fon = reinterpret_cast<Font*> ( TTF );
			Mon = reinterpret_cast<Font*> ( TTFMon );
		}

		Log::instance()->writef( "Fonts loading time: %f ms", TE.elapsed().asMilliseconds() );

		if ( !Fon && !Mon )
			return false;

		Con.create( Mon, true, true, 1024000 );
		Con.ignoreCharOnPrompt( 186 );

		Con.addCommand( "loaddir", cb::Make1( this, &cApp::CmdLoadDir ) );
		Con.addCommand( "loadimg", cb::Make1( this, &cApp::CmdLoadImg ) );
		Con.addCommand( "setbackcolor", cb::Make1( this, &cApp::CmdSetBackColor ) );
		Con.addCommand( "setimgfade", cb::Make1( this, &cApp::CmdSetImgFade ) );
		Con.addCommand( "setlateloading", cb::Make1( this, &cApp::CmdSetLateLoading ) );
		Con.addCommand( "setblockwheel", cb::Make1( this, &cApp::CmdSetBlockWheel ) );
		Con.addCommand( "moveto", cb::Make1( this, &cApp::CmdMoveTo ) );
		Con.addCommand( "batchimgscale", cb::Make1( this, &cApp::CmdBatchImgScale ) );
		Con.addCommand( "batchimgchangeformat", cb::Make1( this, &cApp::CmdBatchImgChangeFormat ) );
		Con.addCommand( "batchimgthumbnail", cb::Make1( this, &cApp::CmdBatchImgThumbnail ) );
		Con.addCommand( "imgchangeformat", cb::Make1( this, &cApp::CmdImgChangeFormat ) );
		Con.addCommand( "imgresize", cb::Make1( this, &cApp::CmdImgResize ) );
		Con.addCommand( "imgscale", cb::Make1( this, &cApp::CmdImgScale ) );
		Con.addCommand( "imgthumbnail", cb::Make1( this, &cApp::CmdImgThumbnail ) );
		Con.addCommand( "imgcentercrop", cb::Make1( this, &cApp::CmdImgCenterCrop) );
		Con.addCommand( "slideshow", cb::Make1( this, &cApp::CmdSlideShow ) );
		Con.addCommand( "setzoom", cb::Make1( this, &cApp::CmdSetZoom ) );

		PrepareFrame();
		GetImages();

		if ( mFile != "" ) {
			FastLoadImage( CurImagePos( mFile ) );
		} else {
			if ( mFiles.size() )
				FastLoadImage( 0 );
		}

		if ( 0 == mFiles.size() && 0 == mFile.length() ) {
			Con.toggle();
		}

		return true;
	}

	return false;
}

void cApp::Process() {
	if ( Init() ) {
		do {
			ET = mWindow->elapsed().asMilliseconds();

			Input();

			TEP.restart();

			if ( mWindow->visible() ) {
				Render();

				if ( KM->isKeyUp(KEY_F12) ) mWindow->takeScreenshot();

				mWindow->display(true);
			} else {
				Sys::sleep( 16 );
			}

			RET = TEP.elapsed().asMilliseconds();

			if ( mConfig.LateLoading && mLaterLoad ) {
				if ( Sys::getTicks() - mLastLaterTick > mConfig.TransitionTime ) {
					UpdateImages();
					mLaterLoad = false;
				}
			}

			if ( mFirstLoad ) {
				UpdateImages();
				mFirstLoad = false;
			}
		} while( mWindow->isRunning() );
	}

	End();
}

void cApp::LoadDir( const std::string& path, const bool& getimages ) {
	std::string tmpFile;

	if ( !FileSystem::isDirectory( path ) ) {
		if ( path.substr(0,7) == "file://" ) {
			mFilePath = path.substr( 7 );
			mFilePath = mFilePath.substr( 0, mFilePath.find_last_of( FileSystem::getOSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::getOSlash() ) + 1 );
		} else if ( IsHttpUrl( path ) ) {
			mUsedTempDir = true;

			if ( !FileSystem::isDirectory( mTmpPath ) )
				FileSystem::makeDir( mTmpPath );

			URI uri( path );
			Http http( uri.getHost(), uri.getPort() );
			Http::Request request( uri.getPathAndQuery() );
			Http::Response response = http.sendRequest(request);

			if ( response.getStatus() == Http::Response::Ok ) {
				if ( !FileSystem::fileWrite( mTmpPath + "tmpfile", reinterpret_cast<const Uint8*>( &response.getBody()[0] ), response.getBody().size() ) ) {
					Con.pushText( "Couldn't write the downloaded image to disk." );

					return;
				}
			} else {
				Con.pushText( "Couldn't download the image from network." );

				return;
			}

			mFilePath = mTmpPath;
			tmpFile = "tmpfile";
		} else {
			mFilePath = path.substr( 0, path.find_last_of( FileSystem::getOSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::getOSlash() ) + 1 );
		}

		String::replaceAll( mFilePath, "%20", " " );

		if ( mFilePath == "" ) {
			#if EE_PLATFORM == EE_PLATFORM_WIN
				mFilePath = "C:\\";
			#else
				mFilePath = FileSystem::getOSlash();
			#endif
		}

		FileSystem::dirPathAddSlashAtEnd( mFilePath );

		if ( IsImage( mFilePath + tmpFile ) )
			mFile = tmpFile;
		else
			return;
	} else {
		mFilePath = path;

		FileSystem::dirPathAddSlashAtEnd( mFilePath );
	}

	mCurImg = 0;

	if ( getimages )
		GetImages();

	if ( mFiles.size() ) {
		if ( mFile.size() )
			mCurImg = CurImagePos( mFile );

		if ( mWindow->isRunning() )
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

	std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( mFilePath );
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

	Con.pushText( "Image list loaded in %f ms.", TE.elapsed().asMilliseconds() );

	Con.pushText( "Directory: \"" + String::fromUtf8( mFilePath ) + "\"" );
	for ( Uint32 i = 0; i < mFiles.size(); i++ )
		Con.pushText( "	" + String::fromUtf8( mFiles[i].Path ) );
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

		mImg.createStatic( Tex );
		mImg.renderMode( mImgRT );
		mImg.scale( mConfig.DefaultImageZoom );
		mImg.position( 0.0f, 0.0f );

		if ( path != mFiles[ mCurImg ].Path )
			mCurImg = CurImagePos( path );

		mFile = mFiles[ mCurImg ].Path;

		ScaleToScreen();

		Texture * pTex = TF->getTexture( Tex );

		if ( NULL != pTex ) {
			Fon->setText(
				"File: " + String::fromUtf8( mFile ) +
				"\nWidth: " + String::toStr( pTex->width() ) +
				"\nHeight: " + String::toStr( pTex->height() ) +
				"\n" + String::toStr( mCurImg+1 ) + "/" + String::toStr( mFiles.size() )
			);
		}
	} else {
		Fon->setText( "File: " + String::fromUtf8( path ) + " failed to load. \nReason: " + Image::getLastFailureReason() );
	}
}

Uint32 cApp::LoadImage( const std::string& path, const bool& SetAsCurrent ) {
	Uint32 TexId 		= 0;

	TexId = TF->load( mFilePath + path );

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
		TF->remove( mFiles[ img ].Tex );
		mFiles[ img ].Tex = 0;
	}
}

void cApp::OptUpdate() {
	mImg.createStatic( mFiles [ mCurImg ].Tex );
	mImg.scale( mConfig.DefaultImageZoom );
	mImg.position( 0.0f, 0.0f );

	ScaleToScreen();

	if ( mConfig.LateLoading ) {
		mLaterLoad = true;
		mLastLaterTick = Sys::getTicks();

		Texture * Tex = TF->getTexture( mFiles [ mCurImg ].Tex );

		if ( Tex ) {
			Fon->setText(
				"File: " + String::fromUtf8( mFiles [ mCurImg ].Path ) +
				"\nWidth: " + String::toStr( Tex->width() ) +
				"\nHeight: " + String::toStr( Tex->height() ) +
				"\n" + String::toStr( mCurImg + 1 ) + "/" + String::toStr( mFiles.size() )
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
	KM->update();
	Mouse = KM->getMousePos();

	if ( KM->isKeyDown(KEY_TAB) && KM->altPressed() ) {
		mWindow->minimize();
	}

	if ( KM->isKeyDown(KEY_ESCAPE) || ( KM->isKeyDown(KEY_Q) && !Con.active() ) ) {
		mWindow->close();
	}

	if ( ( KM->altPressed() && KM->isKeyUp(KEY_RETURN) ) || ( KM->isKeyUp(KEY_F) && !Con.active() ) ) {
		if ( mWindow->isWindowed() )
			mWindow->size( mWindow->getDesktopResolution().width(), mWindow->getDesktopResolution().height(), false );
		else
			mWindow->toggleFullscreen();

		PrepareFrame();
		ScaleToScreen();
	}

	if ( KM->isKeyUp(KEY_F5) ) {
		SwitchFade();
	}

	if ( KM->isKeyUp(KEY_F3) || KM->isKeyUp(KEY_WORLD_26) ) {
		Con.toggle();
	}

	if ( ( KM->isKeyUp(KEY_S) && !Con.active() ) || KM->isKeyUp(KEY_F4) ) {
		mCursor = !mCursor;
		mWindow->getCursorManager()->visible( mCursor );
	}

	if ( KM->isKeyUp(KEY_H) && !Con.active() ) {
		mShowHelp = !mShowHelp;
	}

	if ( ( ( KM->isKeyUp(KEY_V) && KM->controlPressed() ) || ( KM->isKeyUp(KEY_INSERT) && KM->shiftPressed() ) ) && !Con.active() ) {
		std::string tPath = mWindow->getClipboard()->getText();

		if ( ( tPath.size() && IsImage( tPath ) ) || FileSystem::isDirectory( tPath ) ) {
			LoadDir( tPath );
		}
	}

	if ( !Con.active() ) {
		if ( KM->mouseWheelUp() || KM->isKeyUp(KEY_PAGEUP) ) {
			if ( !mConfig.BlockWheelSpeed || Sys::getTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::getTicks();
				LoadPrevImage();
				DisableSlideShow();
			}
		}

		if ( KM->mouseWheelDown() || KM->isKeyUp(KEY_PAGEDOWN) ) {
			if ( !mConfig.BlockWheelSpeed || Sys::getTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::getTicks();
				LoadNextImage();
				DisableSlideShow();
			}
		}

		if ( KM->isKeyUp(KEY_I) ) {
			mConfig.ShowInfo = !mConfig.ShowInfo;
		}
	}

	if ( mFiles.size() && mFiles[ mCurImg ].Tex && !Con.active() ) {
		if ( KM->isKeyUp(KEY_HOME) ) {
			LoadFirstImage();
			DisableSlideShow();
		}

		if ( KM->isKeyUp(KEY_END) ) {
			LoadLastImage();
			DisableSlideShow();
		}

		if ( KM->isKeyUp(KEY_KP_MULTIPLY) ) {
			ScaleToScreen();
		}

		if ( KM->isKeyUp(KEY_KP_DIVIDE) ) {
			mImg.scale( mConfig.DefaultImageZoom );
		}

		if ( KM->isKeyUp(KEY_Z) ) {
			ZoomImage();
		}

		if ( KM->isKeyUp( KEY_N ) ) {
			if ( mWindow->size().width() != (Int32)mImg.size().width() || mWindow->size().height() != (Int32)mImg.size().height() ) {
				mWindow->size( mImg.size().width(), mImg.size().height() );
			}
		}

		if ( Sys::getTicks() - mZoomTicks >= 15 ) {
			mZoomTicks = Sys::getTicks();

			if ( KM->isKeyDown(KEY_KP_MINUS) )
				mImg.scale( mImg.scale() - 0.02f );


			if ( KM->isKeyDown(KEY_KP_PLUS) )
				mImg.scale( mImg.scale() + 0.02f );

			if ( mImg.scale().x < 0.0125f )
				mImg.scale( 0.0125f );

			if ( mImg.scale().x > 50.0f )
				mImg.scale( 50.0f );
		}

		if ( KM->isKeyDown(KEY_LEFT) ) {
			mImg.x( ( mImg.x() + ( (mWindow->elapsed().asMilliseconds() * 0.4f) ) ) );
			mImg.x( static_cast<Float> ( static_cast<Int32> ( mImg.x() ) ) );
		}

		if ( KM->isKeyDown(KEY_RIGHT) ) {
			mImg.x( ( mImg.x() + ( -(mWindow->elapsed().asMilliseconds() * 0.4f) ) ) );
			mImg.x( static_cast<Float> ( static_cast<Int32> ( mImg.x() ) ) );
		}

		if ( KM->isKeyDown(KEY_UP) ) {
			mImg.y( ( mImg.y() + ( (mWindow->elapsed().asMilliseconds() * 0.4f) ) ) );
			mImg.y( static_cast<Float> ( static_cast<Int32> ( mImg.y() ) ) );
		}

		if ( KM->isKeyDown(KEY_DOWN) ) {
			mImg.y( ( mImg.y() + ( -(mWindow->elapsed().asMilliseconds() * 0.4f) ) ) );
			mImg.y( static_cast<Float> ( static_cast<Int32> ( mImg.y() ) ) );
		}

		if ( KM->mouseLeftClick() ) {
			mMouseLeftPressing = false;
		}

		if ( KM->mouseLeftPressed() ) {
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
					mImg.x( mImg.x() + mNewPos.x );
					mImg.y( mImg.y() + mNewPos.y );
				}
			}
		}

		if ( KM->mouseMiddleClick() ) {
			mMouseMiddlePressing = false;
		}

		if ( KM->mouseMiddlePressed() ) {
			if ( !mMouseMiddlePressing ) {
				mMouseMiddleStartClick = Mouse;
				mMouseMiddlePressing = true;
			} else {
				mMouseMiddleClick = Mouse;

				Vector2f v1( (Float)mMouseMiddleStartClick.x, (Float)mMouseMiddleStartClick.y );
				Vector2f v2( Vector2f( (Float)mMouseMiddleClick.x, (Float)mMouseMiddleClick.y ) );
				Line2f l1( v1, v2 );
				Float Dist = v1.distance( v2 ) * 0.01f;
				Float Ang = l1.getAngle();

				if ( Dist ) {
					mMouseMiddleStartClick = Mouse;
					if ( Ang >= 0.0f && Ang <= 180.0f ) {
						mImg.scale( mImg.scale() - Dist );
						if ( mImg.scale().x < 0.0125f )
							mImg.scale( 0.0125f );
					} else {
						mImg.scale( mImg.scale() + Dist );
					}
				}
			}
		}

		if ( KM->mouseRightPressed() ) {
			Line2f line( Vector2f( Mouse.x, Mouse.y ), Vector2f( HWidth, HHeight ) );
			mImg.angle( line.getAngle() );
		}

		if ( KM->isKeyUp(KEY_X) ) {
			if ( mImgRT == RN_NORMAL )
				mImgRT = RN_FLIP;
			else if ( mImgRT == RN_MIRROR )
				mImgRT = RN_FLIPMIRROR;
			else if ( mImgRT == RN_FLIPMIRROR )
				mImgRT = RN_MIRROR;
			else
				mImgRT = RN_NORMAL;

			mImg.renderMode( mImgRT );
		}

		if ( KM->isKeyUp(KEY_C) ) {
			if ( mImgRT == RN_NORMAL )
				mImgRT = RN_MIRROR;
			else if ( mImgRT == RN_FLIP )
				mImgRT = RN_FLIPMIRROR;
			else if ( mImgRT == RN_FLIPMIRROR )
				mImgRT = RN_FLIP;
			else
				mImgRT = RN_NORMAL;

			mImg.renderMode( mImgRT );
		}

		if ( KM->isKeyUp(KEY_R) ) {
			mImg.angle( mImg.angle() + 90.0f );
			ScaleToScreen();
		}

		if ( KM->isKeyUp(KEY_A) ) {
			Texture * Tex = mImg.getCurrentSubTexture()->getTexture();

			if ( Tex ) {
				if ( Tex->filter() == TEX_FILTER_LINEAR )
					Tex->filter( TEX_FILTER_NEAREST );
				else
					Tex->filter( TEX_FILTER_LINEAR );
			}
		}

		if ( KM->isKeyUp(KEY_M) ) {
			mImg.position( 0.0f,0.0f );
			mImg.scale( mConfig.DefaultImageZoom );
			mImg.angle( 0.f );
			ScaleToScreen();

			if ( EE->getCurrentWindow()->isMaximized() ) {
				EE->getCurrentWindow()->size( mImg.size().width(), mImg.size().height() );
			}
		}

		if ( KM->isKeyUp(KEY_T) ) {
			mImg.position( 0.0f,0.0f );
			mImg.scale( mConfig.DefaultImageZoom );
			mImg.angle( 0.f );
			ScaleToScreen();
		}

		if ( KM->isKeyUp(KEY_E) ) {
			CreateSlideShow( mSlideTime );
		}

		if ( KM->isKeyUp(KEY_D) ) {
			DisableSlideShow();
		}

		if ( KM->isKeyUp(KEY_K) ) {
			Texture * curTex;

			if ( NULL != mImg.getCurrentSubTexture() && NULL != ( curTex = mImg.getCurrentSubTexture()->getTexture() ) ) {
				curTex->mipmap( !curTex->mipmap() );
				curTex->reload();
			}
		}
	}
}

void cApp::CreateSlideShow( Uint32 time ) {
	if ( time < 250 )
		time = 250;

	mSlideShow	= true;
	mSlideTime	= time;
	mSlideTicks	= Sys::getTicks();
}

void cApp::DisableSlideShow() {
	mSlideShow = false;
}

void cApp::DoSlideShow() {
	if ( mSlideShow ) {
		if ( Sys::getTicks() - mSlideTicks >= mSlideTime ) {
			mSlideTicks = Sys::getTicks();

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
		Texture* Tex = TF->getTexture( mFiles[ mCurImg ].Tex );

		if ( NULL == Tex )
			return;

		if ( Tex->imgWidth() * mConfig.DefaultImageZoom >= Width || Tex->imgHeight() * mConfig.DefaultImageZoom >= Height ) {
			ZoomImage();
		} else if ( force ) {
			mImg.scale( mConfig.DefaultImageZoom );
		}
	}
}

void cApp::ZoomImage() {
	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		Texture* Tex = TF->getTexture( mFiles[ mCurImg ].Tex );

		if ( NULL == Tex )
			return;

		Sizef boxSize = mImg.size();

		mImg.scale( eemin( Width / boxSize.width(), Height / boxSize.height() ) );
	}
}

void cApp::PrepareFrame() {
	Width = mWindow->getWidth();
	Height = mWindow->getHeight();
	HWidth = Width * 0.5f;
	HHeight = Height * 0.5f;

	if (Sys::getTicks() - mLastTicks >= 100) {
		mLastTicks = Sys::getTicks();
		if ( mFiles.size() )
			mInfo = "EEiv - " +  mFiles[ mCurImg ].Path;
		else
			mInfo = "EEiv";

		mWindow->caption( mInfo );
	}
}

void cApp::Render() {
	PrepareFrame();

	DoSlideShow();

	if ( mFiles.size() && mFiles[ mCurImg ].Tex ) {
		DoFade();

		Texture * Tex = mImg.getCurrentSubTexture()->getTexture();

		if ( Tex ) {
			Float X = static_cast<Float> ( static_cast<Int32> ( HWidth - mImg.size().width() * 0.5f ) );
			Float Y = static_cast<Float> ( static_cast<Int32> ( HHeight - mImg.size().height() * 0.5f ) );

			mImg.offset( Vector2i( X, Y ) );
			mImg.alpha( mCurAlpha );
			mImg.draw();
		}
	}

	if ( mConfig.ShowInfo )
		Fon->draw( 0, 0 );

	PrintHelp();

	Con.draw();
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

		if ( NULL != mOldImg.getCurrentSubTexture() && ( Tex = mOldImg.getCurrentSubTexture()->getTexture() ) ) {
			Float X = static_cast<Float> ( static_cast<Int32> ( HWidth - mOldImg.size().width() * 0.5f ) );
			Float Y = static_cast<Float> ( static_cast<Int32> ( HHeight - mOldImg.size().height() * 0.5f ) );

			mOldImg.offset( Vector2i( X, Y ) );
			mOldImg.alpha( 255 - mCurAlpha );
			mOldImg.draw();
		}
	}
}

void cApp::End() {
	mConfig.Width			= EE->getWidth();
	mConfig.Height			= EE->getHeight();
	mConfig.MaximizeAtStart	= EE->getCurrentWindow()->isMaximized();

	Ini.setValueI( "Window", "Width", mConfig.Width );
	Ini.setValueI( "Window", "Height", mConfig.Height );
	Ini.setValueI( "Window", "BitColor", mConfig.BitColor );
	Ini.setValueI( "Window", "Windowed", mConfig.Windowed );
	Ini.setValueI( "Window", "Resizeable", mConfig.Resizeable );
	Ini.setValueI( "Window", "VSync", mConfig.VSync );
	Ini.setValueI( "Window", "DoubleBuffering", mConfig.DoubleBuffering );
	Ini.setValueI( "Window", "UseDesktopResolution", mConfig.UseDesktopResolution );
	Ini.setValueI( "Window", "NoFrame", mConfig.NoFrame );
	Ini.setValueI( "Window", "MaximizeAtStart", mConfig.MaximizeAtStart );
	Ini.setValueI( "Window", "FrameLimit", mConfig.FrameLimit );
	Ini.setValueI( "Viewer", "Fade", mConfig.Fade );
	Ini.setValueI( "Viewer", "LateLoading", mConfig.LateLoading );
	Ini.setValueI( "Viewer", "BlockWheelSpeed", mConfig.BlockWheelSpeed );
	Ini.setValueI( "Viewer", "ShowInfo", mConfig.ShowInfo );
	Ini.setValueI( "Viewer", "TransitionTime", mConfig.TransitionTime );
	Ini.setValueI( "Viewer", "ConsoleFontSize", mConfig.ConsoleFontSize );
	Ini.setValueI( "Viewer", "AppFontSize", mConfig.AppFontSize );
	Ini.setValueF( "Viewer", "DefaultImageZoom", mConfig.DefaultImageZoom );
	Ini.setValueI( "Viewer", "WheelBlockTime", mConfig.WheelBlockTime );

	Ini.writeFile();
	Engine::destroySingleton();
}

std::string cApp::CreateSavePath( const std::string & oriPath, Uint32 width, Uint32 height, EE_SAVE_TYPE saveType ) {
	EE_SAVE_TYPE type = saveType == SAVE_TYPE_UNKNOWN ? Image::extensionToSaveType( FileSystem::fileExtension( oriPath ) ) : saveType;

	if ( SAVE_TYPE_UNKNOWN == type ) {
		type = SAVE_TYPE_PNG;
	}

	return FileSystem::fileRemoveExtension( oriPath ) + "-" + String::toStr( width ) + "x" + String::toStr( height ) + "." + Image::saveTypeToExtension( type );
}

EE_SAVE_TYPE cApp::GetPathSaveType( const std::string& path ) {
	return Image::extensionToSaveType( FileSystem::fileExtension( path ) );
}

void cApp::ScaleImg( const std::string& Path, const Float& Scale , EE_SAVE_TYPE saveType ) {
	int w, h, c;

	if ( Image::getInfo( Path, &w, &h, &c ) && Scale > 0.f ) {
		Int32 new_width		= static_cast<Int32>( w * Scale );
		Int32 new_height	= static_cast<Int32>( h * Scale );

		ResizeImg( Path, new_width, new_height, saveType );
	} else {
		Con.pushText( "Images does not exists." );
	}
}

void cApp::ResizeImg( const std::string& Path, const Uint32& NewWidth, const Uint32& NewHeight, EE_SAVE_TYPE saveType ) {
	if ( IsImage( Path ) ) {
		std::string newPath( CreateSavePath( Path, NewWidth, NewHeight, saveType ) );
		EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

		Image img( Path );

		img.resize( NewWidth, NewHeight );

		img.saveToFile( newPath, type );
	} else {
		Con.pushText( "Images does not exists." );
	}
}

void cApp::ThumgnailImg( const std::string& Path, const Uint32& MaxWidth, const Uint32& MaxHeight, EE_SAVE_TYPE saveType ) {
	if ( IsImage( Path ) ) {
		Image img( Path );

		Image * thumb = img.thumbnail( MaxWidth, MaxHeight );

		if ( NULL != thumb ) {
			std::string newPath( CreateSavePath( Path, thumb->width(), thumb->height(), saveType ) );
			EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

			thumb->saveToFile( newPath, type );

			eeSAFE_DELETE( thumb );
		}
	} else {
		Con.pushText( "Images does not exists." );
	}
}

void cApp::CenterCropImg( const std::string& Path, const Uint32& Width, const Uint32& Height, EE_SAVE_TYPE saveType ) {
	if ( IsImage( Path ) ) {
		Image img( Path );

		Sizei nSize;

		double scale = 1.f;

		scale = eemax( (double)Width / (double)img.width(), (double)Height / (double)img.height() );

		nSize.x = Math::round( img.width() * scale );
		nSize.y = Math::round( img.height() * scale );

		if ( nSize.width() == (int)Width - 1 || nSize.width() == (int)Width + 1 ) {
			nSize.x = (int)Width;
		}

		if ( nSize.height() == (int)Height - 1 || nSize.height() == (int)Height + 1 ) {
			nSize.y = (int)Height;
		}

		img.resize( nSize.width(), nSize.height() );

		Image * croppedImg  = NULL;
		Recti rect;

		if ( img.width() > Width ) {
			rect.Left = ( img.width() - Width ) / 2;
			rect.Right = rect.Left + Width;
			rect.Top = 0;
			rect.Bottom = Height;
		} else {
			rect.Top = ( img.height() - Height ) / 2;
			rect.Bottom = rect.Top + Height;
			rect.Left = 0;
			rect.Right = Width;
		}

		croppedImg = img.crop( rect );

		if ( NULL != croppedImg ) {
			std::string newPath( CreateSavePath( Path, croppedImg->width(), croppedImg->height(), saveType ) );
			EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

			croppedImg->saveToFile( newPath, type );

			eeSAFE_DELETE( croppedImg );
		} else {
			std::string newPath( CreateSavePath( Path, img.width(), img.height(), saveType ) );
			EE_SAVE_TYPE type = SAVE_TYPE_UNKNOWN != saveType ? saveType : GetPathSaveType( newPath );

			img.saveToFile( newPath, type );
		}
	}
}

void cApp::BatchImgScale( const std::string& Path, const Float& Scale ) {
	std::string iPath = Path;
	std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( iPath );

	if ( iPath[ iPath.size() - 1 ] != '/' )
		iPath += "/";

	for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
		std::string fPath = iPath + tmpFiles[i];
		ScaleImg( fPath, Scale );
	}
}

void cApp::BatchImgThumbnail( Sizei size, std::string dir, bool recursive ) {
	FileSystem::dirPathAddSlashAtEnd( dir );

	std::vector<std::string> files = FileSystem::filesGetInPath( dir );

	for ( size_t i = 0; i < files.size(); i++ ) {
		std::string fpath( dir + files[i] );

		if ( FileSystem::isDirectory( fpath ) ) {
			if ( recursive ) {
				BatchImgThumbnail( size, fpath, recursive );
			}
		} else {
			int w, h, c;
			if ( Image::getInfo( fpath, &w, &h, &c ) ) {
				if ( w > size.width() || h > size.height() ) {
					Image img( fpath );

					Image * thumb = img.thumbnail( size.width(), size.height() );

					if ( NULL != thumb ) {
						thumb->saveToFile( fpath, Image::extensionToSaveType( FileSystem::fileExtension( fpath ) ) );

						Con.pushText( "Thumbnail created for '%s'. Old size %dx%d. New size %dx%d.", fpath.c_str(), img.width(), img.height(), thumb->width(), thumb->height() );

						eeSAFE_DELETE( thumb );
					} else {
						Con.pushText( "Thumbnail %s failed to create.", fpath.c_str() );
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

		bool Res = String::fromString<Uint32> ( time, params[1] );

		if ( Res ) {
			if ( !mSlideShow ) {
				CreateSlideShow( time );
			} else {
				if ( 0 == time ) {
					mSlideShow = false;
				}
			}
		} else {
			Con.pushText( Error );
		}
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdImgResize( const std::vector < String >& params ) {
	String Error( "Usage example: imgresize new_width new_height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res1 = String::fromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::fromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 ) {
			ResizeImg( myPath, nWidth, nHeight, saveType );
		} else {
			Con.pushText( Error );
		}
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdImgThumbnail( const std::vector < String >& params ) {
	String Error( "Usage example: imgthumbnail max_width max_height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res1 = String::fromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::fromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 ) {
			ThumgnailImg( myPath, nWidth, nHeight, saveType );
		} else {
			Con.pushText( Error );
		}
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdImgCenterCrop( const std::vector < String >& params ) {
	String Error( "Usage example: imgcentercrop width height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res1 = String::fromString<Uint32> ( nWidth, params[1] );
		bool Res2 = String::fromString<Uint32> ( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 )
			CenterCropImg( myPath, nWidth, nHeight, saveType );
		else
			Con.pushText( Error );
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdImgScale( const std::vector < String >& params ) {
	String Error( "Usage example: imgscale scale path_to_img format" );
	if ( params.size() >= 2 ) {
		Float Scale = 0;
		EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

		bool Res = String::fromString<Float>( Scale, params[1] );

		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].toUtf8();

			if ( params.size() > 3 ) {
				saveType = Image::extensionToSaveType( params[3] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res )
			ScaleImg( myPath, Scale, saveType );
		else
			Con.pushText( Error );
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdBatchImgScale( const std::vector < String >& params ) {
	String Error( "Usage example: batchimgscale scale_value directory_to_resize_img ( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 3 ) {
		Float Scale = 0;

		bool Res = String::fromString<Float>( Scale, params[1] );

		std::string myPath = params.size() >= 3 ? params[2].toUtf8() : mFilePath;

		if ( Res ) {
			if ( FileSystem::isDirectory( myPath ) ) {
				BatchImgScale( myPath, Scale );
			} else {
				Con.pushText( "Second argument is not a directory!" );
			}
		} else {
			Con.pushText( Error );
		}
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdBatchImgThumbnail( const std::vector < String >& params ) {
	String Error( "Usage example: batchimgthumbnail max_width max_height directory_to_create_thumbs recursive ( if no dir is passed, it will use the current dir opened )" );

	if ( params.size() >= 3 ) {
		Uint32 max_width = 0, max_height = 0;
		bool recursive = false;

		bool Res1 = String::fromString<Uint32>( max_width, params[1] );
		bool Res2 = String::fromString<Uint32>( max_height, params[2] );

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mFilePath;

		if ( params.size() > 4 && params[4] == "recursive" ) {
			recursive = true;
		}

		if ( Res1 && Res2 ) {
			if ( FileSystem::isDirectory( myPath ) ) {
				BatchImgThumbnail( Sizei( max_width, max_height ), myPath, recursive );
			} else {
				Con.pushText( "Third argument is not a directory!" );
			}
		} else {
			Con.pushText( Error );
		}
	}
	else
	{
		Con.pushText( Error );
	}
}

void cApp::CmdImgChangeFormat( const std::vector < String >& params ) {
	String Error( "Usage example: imgchangeformat to_format image_to_reformat ( if null will use the current loaded image )" );
	if ( params.size() >= 2 ) {
		std::string toFormat = params[1].toUtf8();
		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].toUtf8();
		} else {
			myPath = mFilePath + mFile;
		}

		std::string fromFormat = FileSystem::fileExtension( myPath );

		if ( Image::isImage( myPath ) ) {
			std::string fPath 	= myPath;
			std::string fExt	= FileSystem::fileExtension( fPath );

			if ( fExt == fromFormat ) {
				std::string fName;

				if ( fExt != toFormat )
					fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
				else
					fName = fPath + "." + toFormat;

				EE_SAVE_TYPE saveType = Image::extensionToSaveType( toFormat );

				if ( SAVE_TYPE_UNKNOWN != saveType ) {
					Image * img = eeNew( Image, ( fPath ) );
					img->saveToFile( fName, saveType );
					eeSAFE_DELETE( img );

					Con.pushText( fName + " created." );
				}
			}
		} else {
			Con.pushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdBatchImgChangeFormat( const std::vector < String >& params ) {
	String Error( "Usage example: batchimgchangeformat from_format to_format directory_to_reformat ( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 4 ) {
		std::string fromFormat = params[1].toUtf8();
		std::string toFormat = params[2].toUtf8();

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mFilePath;

		if ( FileSystem::isDirectory( myPath ) ) {
			std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( myPath );

			if ( myPath[ myPath.size() - 1 ] != '/' )
				myPath += "/";

			for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
				std::string fPath 	= myPath + tmpFiles[i];
				std::string fExt	= FileSystem::fileExtension( fPath );

				if ( IsImage( fPath ) && fExt == fromFormat ) {
					std::string fName;

					if ( fExt != toFormat )
						fName = fPath.substr( 0, fPath.find_last_of(".") + 1 ) + toFormat;
					else
						fName = fPath + "." + toFormat;

					EE_SAVE_TYPE saveType = Image::extensionToSaveType( toFormat );

					if ( SAVE_TYPE_UNKNOWN != saveType ) {
						Image * img = eeNew( Image, ( fPath ) );
						img->saveToFile( fPath, saveType );
						eeSAFE_DELETE( img );

						Con.pushText( fName + " created." );
					}
				}
			}
		} else {
			Con.pushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con.pushText( Error );
	}
}

void cApp::CmdMoveTo( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( tInt )
			tInt--;

		if ( Res && tInt >= 0 && tInt < (Int32)mFiles.size() ) {
			Con.pushText( "moveto: moving to image number " + String::toStr( tInt + 1 ) );
			FastLoadImage( tInt );
		} else {
			Con.pushText( "moveto: image number does not exists" );
		}
	} else {
		Con.pushText( "Expected some parameter" );
	}
}

void cApp::CmdSetBlockWheel( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.BlockWheelSpeed = (bool)tInt;
			Con.pushText( "setblockwheel " + String::toStr(tInt) );
		} else
			Con.pushText( "Valid parameters are 0 or 1." );
	} else
		Con.pushText( "Expected some parameter" );
}

void cApp::CmdSetLateLoading( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.LateLoading = (bool)tInt;
			Con.pushText( "setlateloading " + String::toStr(tInt) );
		} else
			Con.pushText( "Valid parameters are 0 or 1." );
	} else
		Con.pushText( "Expected some parameter" );
}

void cApp::CmdSetImgFade( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.Fade = (bool)tInt;
			Con.pushText( "setimgfade " + String::toStr(tInt) );
		} else
			Con.pushText( "Valid parameters are 0 or 1." );
	} else
		Con.pushText( "Expected some parameter" );
}

void cApp::CmdSetBackColor( const std::vector < String >& params ) {
	String Error( "Usage example: setbackcolor 255 255 255 (RGB Color, numbers between 0 and 255)" );

	if ( params.size() >= 2 ) {
		if ( params.size() == 4 ) {
			Int32 R = 0;
			bool Res1 = String::fromString<Int32>( R, params[1] );
			Int32 G = 0;
			bool Res2 = String::fromString<Int32>( G, params[2] );
			Int32 B = 0;
			bool Res3 = String::fromString<Int32>( B, params[3] );

			if ( Res1 && Res2 && Res3 && ( R <= 255 && R >= 0 ) && ( G <= 255 && G >= 0 ) && ( B <= 255 && B >= 0 ) ) {
				mWindow->backColor( RGB( R,G,B ) );
				Con.pushText( "setbackcolor applied" );
				return;
			}
		}

		Con.pushText( Error );
	}
}

void cApp::CmdLoadImg( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].toUtf8();

		if ( IsImage( myPath ) || IsHttpUrl( myPath ) ) {
			LoadDir( myPath );
		} else
			Con.pushText( "\"" + myPath + "\" is not an image path or the image is not supported." );
	}
}

void cApp::CmdLoadDir( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].toUtf8();
		if ( params.size() > 2 ) {
			for ( Uint32 i = 2; i < params.size(); i++ )
				myPath += " " + params[i].toUtf8();
		}

		if ( FileSystem::isDirectory( myPath ) ) {
			LoadDir( myPath );
		} else
			Con.pushText( "If you want to load an image use loadimg. \"" + myPath + "\" is not a directory path." );
	}
}

void cApp::CmdSetZoom( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0;

		bool Res = String::fromString<Float>( tFloat, params[1] );

		if ( Res && tFloat >= 0 && tFloat <= 10 ) {
			Con.pushText( "setzoom: zoom level " + String::toStr( tFloat ) );
			mImg.scale( tFloat );
		} else
			Con.pushText( "setzoom: value out of range" );
	} else
		Con.pushText( "Expected some parameter" );
}

void cApp::PrintHelp() {
	if ( mShowHelp ) {
		Uint32 Top = 6;
		Uint32 Left = 6;

		if ( NULL == mHelpCache ) {
			String HT = "Keys List:\n";
			HT += "Escape: Quit from EEiv\n";
			HT += "ALT + RETURN or F: Toogle Fullscreen - Windowed\n";
			HT += String::fromUtf8( "F3 or º: Toggle Console\n" );
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

		mHelpCache->draw( Left, Height - Top - mHelpCache->getTextHeight() );
	}
}
