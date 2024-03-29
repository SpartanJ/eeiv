#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

// This application is not meant to be used as an example of beautiful code,
// it's just old code that works fine and looks ugly. It was made exclusively
// for my personal use, and still manages to satisfy my very basic image viewing necessities.
// Some day i'll make this look good.

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
static std::string getWindowsPath() {
#ifdef UNICODE
	wchar_t Buffer[1024];
	GetWindowsDirectory( Buffer, 1024 );
	return EE::String( Buffer ).toUtf8();
#else
	char Buffer[1024];
	GetWindowsDirectory( Buffer, 1024 );
	return std::string( Buffer );
#endif
}
#undef RGB
#undef KEY_EXECUTE
#endif

#include "capp.hpp"
#include <algorithm>

static bool isRawImage( const std::string& path ) {
	std::string Ext = FileSystem::fileExtension( path );
	return Ext == "uint8" || Ext == "float32";
}

static bool isImage( const std::string& _path ) {
	std::string path = _path;

	if ( path.size() >= 7 && path.substr( 0, 7 ) == "file://" )
		path = path.substr( 7 );

	if ( !FileSystem::isDirectory( path ) && FileSystem::fileSize( path ) ) {
		std::string Ext = FileSystem::fileExtension( path );
		if ( Ext == "png" || Ext == "tga" || Ext == "bmp" || Ext == "jpg" || Ext == "gif" ||
			 Ext == "jpeg" || Ext == "dds" || Ext == "psd" || Ext == "hdr" || Ext == "pic" ||
			 Ext == "pvr" || Ext == "pkm" ) {
			return true;
		} else {
			if ( Ext == "uint8" || Ext == "float32" ) {
				return true;
			}
			return Image::isImage( path );
		}
	}

	return false;
}

static bool isHttpUrl( const std::string& path ) {
	return path.substr( 0, 7 ) == "http://" || path.substr( 0, 8 ) == "https://";
}

App::App( int argc, char* argv[] ) :
	Fon( NULL ),
	Mon( NULL ),
	mCurImg( 0 ),
	mFading( false ),
	mAlpha( 255 ),
	mCurAlpha( 255 ),
	mLaterLoad( false ),
	mCursor( true ),
	mMouseLeftPressing( false ),
	mMouseMiddlePressing( false ),
	mImgRT( RENDER_NORMAL ),
	mFilter( Texture::Filter::Linear ),
	mShowHelp( false ),
	mFirstLoad( false ),
	mUsedTempDir( false ),
	mLockZoomAndPosition( false ),
	mHelpCache( NULL ),
	mSlideShow( false ),
	mSlideTime( 4000 ),
	mSlideTicks( Sys::getTicks() ) {
	mStorePath = Sys::getConfigPath( "eeiv" ) + FileSystem::getOSSlash();
	mTmpPath = mStorePath + "tmp" + FileSystem::getOSSlash();

	std::string nstr;

	if ( argc > 1 )
		nstr.assign( argv[1] );
	else
		nstr.assign( argv[0] );

	loadDir( nstr, false );
}

App::~App() {
	clearTempDir();
}

void App::getConfig() {
	mConfig.Width = Ini.getValueI( "window", "width", 1024 );
	mConfig.Height = Ini.getValueI( "window", "height", 768 );
	mConfig.BitColor = Ini.getValueI( "window", "bitcolor", 32 );
	mConfig.Windowed = Ini.getValueB( "window", "windowed", true );
	mConfig.Resizeable = Ini.getValueB( "window", "resizeable", true );
	mConfig.VSync = Ini.getValueB( "window", "vsync", true );
	mConfig.DoubleBuffering = Ini.getValueB( "window", "doublebuffering", true );
	mConfig.UseDesktopResolution = Ini.getValueB( "window", "usedesktopresolution", false );
	mConfig.NoFrame = Ini.getValueB( "window", "borderless", false );
	mConfig.MaximizeAtStart = Ini.getValueB( "window", "maximizeatstart", true );
	mConfig.FrameLimit = Ini.getValueI( "window", "framelimit", 0 );
	mConfig.Fade = Ini.getValueB( "viewer", "fade", true );
	mConfig.LateLoading = Ini.getValueB( "viewer", "lateloading", true );
	mConfig.BlockWheelSpeed = Ini.getValueB( "viewer", "blockwheelspeed", true );
	mConfig.ShowInfo = Ini.getValueB( "viewer", "showinfo", true );
	mConfig.TransitionTime = Ini.getValueI( "viewer", "transitiontime", 200 );
	mConfig.ConsoleFontSize = Ini.getValueI( "viewer", "consolefontsize", 12 );
	mConfig.AppFontSize = Ini.getValueI( "viewer", "appfontsize", 12 );
	mConfig.DefaultImageZoom = Ini.getValueF( "viewer", "defaultimagezoom", 1 );
	mConfig.WheelBlockTime = Ini.getValueI( "viewer", "wheelblocktime", 200 );
}

void App::loadConfig() {
	std::string tPath = mStorePath + "eeiv.ini";
	Ini.loadFromFile( tPath );

	if ( FileSystem::fileExists( tPath ) ) {
		Ini.readFile();
		getConfig();
	} else {
		Ini.setValueI( "window", "width", 1024 );
		Ini.setValueI( "window", "height", 768 );
		Ini.setValueI( "window", "bitcolor", 32 );
		Ini.setValueI( "window", "windowed", 1 );
		Ini.setValueI( "window", "resizeable", 1 );
		Ini.setValueI( "window", "vsync", 1 );
		Ini.setValueI( "window", "doublebuffering", 1 );
		Ini.setValueI( "window", "usedesktopresolution", 0 );
		Ini.setValueI( "window", "borderless", 0 );
		Ini.setValueI( "window", "maximizeatstart", 1 );
		Ini.setValueI( "window", "framelimit", 0 );
		Ini.setValueI( "viewer", "fade", 1 );
		Ini.setValueI( "viewer", "lateloading", 1 );
		Ini.setValueI( "viewer", "blockwheelspeed", 1 );
		Ini.setValueI( "viewer", "showinfo", 1 );
		Ini.setValueI( "viewer", "transitiontime", 200 );
		Ini.setValueI( "viewer", "consolefontsize", 12 );
		Ini.setValueI( "viewer", "appfontsize", 12 );
		Ini.setValueI( "viewer", "defaultimagezoom", 1 );
		Ini.setValueI( "viewer", "wheelblocktime", 200 );

		if ( !FileSystem::isDirectory( mStorePath ) )
			FileSystem::makeDir( mStorePath );

		Ini.writeFile();
		getConfig();
	}
}

bool App::init() {
	loadConfig();

	EE = Engine::instance();
	MyPath = Sys::getProcessPath();

	std::string iconp( MyPath + "assets/eeiv.png" );

	if ( !FileSystem::fileExists( iconp ) ) {
		iconp = MyPath + "assets/icon/ee.png";
	}

	WindowSettings WinSettings = EE->createWindowSettings( &Ini, "window" );
	ContextSettings ConSettings = EE->createContextSettings( &Ini, "window" );

	WinSettings.Icon = iconp;
	WinSettings.Title = "eeiv";

	mWindow = EE->createWindow( WinSettings, ConSettings );

	if ( mWindow->isOpen() ) {
		Display* currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(
			mWindow->getCurrentDisplayIndex() );

		PixelDensity::setPixelDensity( PixelDensity::toFloat( currentDisplay->getPixelDensity() ) );

		formatConfiguration.svgScale( 2 /*PixelDensity::getPixelDensity()*/ );

		if ( mConfig.FrameLimit )
			mWindow->setFrameRateLimit( 60 );

		mWindow->setCloseRequestCallback( [&]( EE::Window::Window* win ) -> bool {
			updateConfig();
			return true;
		} );

		TF = TextureFactory::instance();
		Log = Log::instance();
		KM = mWindow->getInput();

		if ( mConfig.MaximizeAtStart )
			mWindow->maximize();

		Clock TE;

		std::string MyFontPath = MyPath + "assets/fonts" + FileSystem::getOSSlash();

		TTF = FontTrueType::New( "DejaVuSans" );
		TTFMon = FontTrueType::New( "DejaVuSansMono" );

#if EE_PLATFORM == EE_PLATFORM_WIN
		std::string fontsPath( getWindowsPath() + "\\Fonts\\" );
#else
		std::string fontsPath( "/usr/share/fonts/truetype/" );
#endif

		if ( FileSystem::fileExists( fontsPath + "DejaVuSans.ttf" ) &&
			 FileSystem::fileExists( fontsPath + "DejaVuSansMono.ttf" ) ) {
			TTF->loadFromFile( fontsPath + "DejaVuSans.ttf" );
			TTFMon->loadFromFile( fontsPath + "DejaVuSansMono.ttf" );
		} else if ( FileSystem::fileExists( MyFontPath + "DejaVuSans.ttf" ) &&
					FileSystem::fileExists( MyFontPath + "DejaVuSansMono.ttf" ) ) {
			TTF->loadFromFile( MyFontPath + "DejaVuSans.ttf" );
			TTFMon->loadFromFile( MyFontPath + "DejaVuSansMono.ttf" );
		} else if ( FileSystem::fileExists( fontsPath + "Arial.ttf" ) &&
					FileSystem::fileExists( fontsPath + "cour.ttf" ) ) {
			TTF->loadFromFile( fontsPath + "Arial.ttf" );
			TTFMon->loadFromFile( fontsPath + "cour.ttf" );
		} else {
			Log::instance()->writef( "Fonts not found... closing." );
			return false;
		}

		Fon = reinterpret_cast<Font*>( TTF );
		Mon = reinterpret_cast<Font*>( TTFMon );

		FonCache.setFont( Fon );
		FonCache.setFontSize( mConfig.AppFontSize );
		FonCache.setOutlineThickness( PixelDensity::dpToPxI( 1 ) );

		Log::instance()->writef( "Fonts loading time: %f ms",
								 TE.getElapsedTimeAndReset().asMilliseconds() );

		if ( !Fon && !Mon )
			return false;

		UISceneNode* uiSceneNode = UISceneNode::New();
		uiSceneNode->getUIThemeManager()->setDefaultFont( Fon );
		SceneManager::instance()->add( uiSceneNode );

		Con = UIConsole::NewOpt( Mon, true, true, 1024000 );
		Con->setQuakeMode( true );
		Con->setVisible( false );
		Con->setFontSize( mConfig.ConsoleFontSize );

		Con->addCommand( "loaddir", cb::Make1( this, &App::cmdLoadDir ) );
		Con->addCommand( "loadimg", cb::Make1( this, &App::cmdLoadImg ) );
		Con->addCommand( "setbackcolor", cb::Make1( this, &App::cmdSetBackColor ) );
		Con->addCommand( "setimgfade", cb::Make1( this, &App::cmdSetImgFade ) );
		Con->addCommand( "setlateloading", cb::Make1( this, &App::cmdSetLateLoading ) );
		Con->addCommand( "setblockwheel", cb::Make1( this, &App::cmdSetBlockWheel ) );
		Con->addCommand( "moveto", cb::Make1( this, &App::cmdMoveTo ) );
		Con->addCommand( "batchimgscale", cb::Make1( this, &App::cmdBatchImgScale ) );
		Con->addCommand( "batchimgchangeformat", cb::Make1( this, &App::cmdBatchImgChangeFormat ) );
		Con->addCommand( "batchimgthumbnail", cb::Make1( this, &App::cmdBatchImgThumbnail ) );
		Con->addCommand( "imgchangeformat", cb::Make1( this, &App::cmdImgChangeFormat ) );
		Con->addCommand( "imgresize", cb::Make1( this, &App::cmdImgResize ) );
		Con->addCommand( "imgscale", cb::Make1( this, &App::cmdImgScale ) );
		Con->addCommand( "imgthumbnail", cb::Make1( this, &App::cmdImgThumbnail ) );
		Con->addCommand( "imgcentercrop", cb::Make1( this, &App::cmdImgCenterCrop ) );
		Con->addCommand( "slideshow", cb::Make1( this, &App::cmdSlideShow ) );
		Con->addCommand( "setzoom", cb::Make1( this, &App::cmdSetZoom ) );

		setWindowCaption();

		getImages();

		if ( mFile != "" ) {
			fastLoadImage( curImagePos( mFile ) );
		} else {
			if ( mFiles.size() )
				fastLoadImage( 0 );
		}

		if ( 0 == mFiles.size() && 0 == mFile.length() ) {
			Con->toggle();
		}

		return true;
	}

	return false;
}

void App::process() {
	if ( init() ) {
		do {
			ET = mWindow->getElapsed().asMilliseconds();

			input();

			TEP.restart();

			if ( mWindow->isVisible() ) {
				render();

				if ( KM->isKeyUp( KEY_F12 ) )
					mWindow->takeScreenshot();

				mWindow->display( true );
			} else {
				Sys::sleep( 16 );
			}

			RET = TEP.getElapsedTimeAndReset().asMilliseconds();

			if ( mConfig.LateLoading && mLaterLoad ) {
				if ( Sys::getTicks() - mLastLaterTick > mConfig.TransitionTime ) {
					updateImages();
					mLaterLoad = false;
				}
			}

			if ( mFirstLoad ) {
				updateImages();
				mFirstLoad = false;
			}
		} while ( mWindow->isRunning() );
	}

	end();
}

void App::loadDir( const std::string& path, const bool& getimages ) {
	std::string tmpFile;

	if ( !FileSystem::isDirectory( path ) ) {
		if ( path.substr( 0, 7 ) == "file://" ) {
			mFilePath = path.substr( 7 );
			mFilePath = mFilePath.substr( 0, mFilePath.find_last_of( FileSystem::getOSSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::getOSSlash() ) + 1 );
		} else if ( isHttpUrl( path ) ) {
			mUsedTempDir = true;

			if ( !FileSystem::isDirectory( mTmpPath ) )
				FileSystem::makeDir( mTmpPath );

			URI uri( path );
			Http http( uri.getHost(), uri.getPort() );
			Http::Request request( uri.getPathAndQuery() );
			Http::Response response = http.sendRequest( request );

			if ( response.getStatus() == Http::Response::Ok ) {
				if ( !FileSystem::fileWrite(
						 mTmpPath + "tmpfile",
						 reinterpret_cast<const Uint8*>( &response.getBody()[0] ),
						 response.getBody().size() ) ) {
					Con->pushText( "Couldn't write the downloaded image to disk." );

					return;
				}
			} else {
				Con->pushText( "Couldn't download the image from network." );

				return;
			}

			mFilePath = mTmpPath;
			tmpFile = "tmpfile";
		} else {
			mFilePath = path.substr( 0, path.find_last_of( FileSystem::getOSSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::getOSSlash() ) + 1 );
		}

		String::replaceAll( mFilePath, "%20", " " );

		if ( mFilePath == "" ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
			mFilePath = "C:\\";
#else
			mFilePath = FileSystem::getOSSlash();
#endif
		}

		FileSystem::dirAddSlashAtEnd( mFilePath );

		if ( isImage( mFilePath + tmpFile ) )
			mFile = tmpFile;
		else
			return;
	} else {
		mFilePath = path;

		FileSystem::dirAddSlashAtEnd( mFilePath );
	}

	mCurImg = 0;

	if ( getimages )
		getImages();

	if ( mFiles.size() ) {
		if ( mFile.size() )
			mCurImg = curImagePos( mFile );

		if ( mWindow->isRunning() )
			updateImages();
	}
}

void App::updateConfig() {
	mConfig.Width = EE->getCurrentWindow()->getWidth();
	mConfig.Height = EE->getCurrentWindow()->getHeight();
	mConfig.MaximizeAtStart = EE->getCurrentWindow()->isMaximized();
}

void App::clearTempDir() {
	if ( mUsedTempDir ) {
		getImages();

		for ( Uint32 i = 0; i < mFiles.size(); i++ ) {
			std::string Delfile = mFilePath + mFiles[i].Path;
			remove( Delfile.c_str() );
		}
	}
}

void App::getImages() {
	Clock TE;

	Uint32 i;
	std::vector<std::string> tStr;
	mFiles.clear();

	std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( mFilePath );
	for ( i = 0; i < tmpFiles.size(); i++ )
		if ( isImage( mFilePath + tmpFiles[i] ) )
			tStr.push_back( tmpFiles[i] );

	std::sort( tStr.begin(), tStr.end() );

	for ( i = 0; i < tStr.size(); i++ ) {
		mImage tmpI;
		tmpI.Path = tStr[i];
		tmpI.Tex = 0;

		mFiles.push_back( tmpI );
	}

	Con->pushText( "Image list loaded in %f ms.", TE.getElapsedTimeAndReset().asMilliseconds() );

	Con->pushText( "Directory: \"" + String::fromUtf8( mFilePath ) + "\"" );
	for ( Uint32 i = 0; i < mFiles.size(); i++ )
		Con->pushText( "	" + String::fromUtf8( mFiles[i].Path ) );
}

Uint32 App::curImagePos( const std::string& path ) {
	for ( Uint32 i = 0; i < mFiles.size(); i++ ) {
		if ( mFiles[i].Path == path )
			return i;
	}
	return 0;
}

void App::fastLoadImage( const Uint32& ImgNum ) {
	mCurImg = ImgNum;
	mFiles[mCurImg].Tex = loadImage( mFiles[mCurImg].Path, true );
	mFirstLoad = true;
}

void App::setImage( const Uint32& Tex, const std::string& path ) {
	if ( Tex ) {
		mFiles[mCurImg].Tex = Tex;

		mImgRT = RENDER_NORMAL;

		Vector2f scale( mImg.getScale() );
		mImg.createStatic( Tex );
		mImg.setRenderMode( mImgRT );
		mImg.setScale( scale );

		if ( !mLockZoomAndPosition ) {
			mImg.setScale( mConfig.DefaultImageZoom );
			mImg.setPosition( Vector2f::Zero );
		}

		if ( path != mFiles[mCurImg].Path )
			mCurImg = curImagePos( path );

		mFile = mFiles[mCurImg].Path;

		if ( !mLockZoomAndPosition )
			scaleToScreen();

		Texture* pTex = TF->getTexture( Tex );

		pTex->setFilter( mFilter );

		if ( NULL != pTex ) {
			FonCache.setString( "File: " + String::fromUtf8( mFile ) +
								"\nWidth: " + String::toString( pTex->getWidth() ) +
								"\nHeight: " + String::toString( pTex->getHeight() ) + "\n" +
								String::toString( mCurImg + 1 ) + "/" +
								String::toString( mFiles.size() ) );
		}
	} else {
		FonCache.setString( "File: " + String::fromUtf8( path ) +
							" failed to load. \nReason: " + Image::getLastFailureReason() );
	}

	setWindowCaption();
}

Sizei imageSizeFromName( const std::string& path ) {
	Sizei size( Sizei::Zero );
	size_t xPos = std::string::npos;
	for ( size_t i = 0; i < path.size(); i++ ) {
		if ( i > 0 && i < path.size() - 1 && path[i] == 'x' && String::isNumber( path[i - 1] ) &&
			 String::isNumber( path[i + 1] ) ) {
			xPos = i;
			break;
		}
	}
	if ( xPos != std::string::npos ) {
		std::string width;
		std::string height;
		for ( size_t i = xPos - 1; i >= 0; i-- ) {
			if ( String::isNumber( path[i] ) ) {
				width = path[i] + width;
			} else {
				break;
			}
		}
		for ( size_t i = xPos + 1; i < path.size(); i++ ) {
			if ( String::isNumber( path[i] ) ) {
				height += path[i];
			} else {
				break;
			}
		}
		int w;
		int h;
		if ( String::fromString<int>( w, width ) && String::fromString<int>( h, height ) ) {
			return Sizei( w, h );
		}
	}
	return size;
}

Uint32 App::loadImage( const std::string& path, const bool& setAsCurrent ) {
	Uint32 TexId = 0;

	if ( isRawImage( mFilePath + path ) ) {
		ScopedBuffer buffer;
		if ( FileSystem::fileGet( mFilePath + path, buffer ) ) {
			Sizei size( imageSizeFromName( path ) );
			int channels = 0;
			for ( size_t c = 1; c <= 4; c++ ) {
				if ( buffer.size() - 8 == size.getWidth() * size.getHeight() * c ) {
					channels = c;
					break;
				}
			}
			if ( channels > 0 ) {
				Texture* tex = TF->loadFromPixels( buffer.get() + 8, size.getWidth(),
												   size.getHeight(), channels );
				if ( tex )
					TexId = tex->getTextureId();
			}
		}
	} else {
		Texture* tex = TF->loadFromFile( mFilePath + path, false, Texture::ClampMode::ClampToEdge,
										 false, false, formatConfiguration );
		if ( tex )
			TexId = tex->getTextureId();
	}

	if ( setAsCurrent )
		setImage( TexId, path );

	return TexId;
}

void App::updateImages() {
	for ( Int32 i = 0; i < (Int32)mFiles.size(); i++ ) {
		if ( !( i == ( mCurImg - 1 ) || i == mCurImg || i == ( mCurImg + 1 ) ) ) {
			unloadImage( i );
		}

		if ( i == ( mCurImg - 1 ) || i == ( mCurImg + 1 ) ) {
			if ( mFiles[i].Tex == 0 ) {
				mFiles[i].Tex = loadImage( mFiles[i].Path );
			}
		}

		if ( i == mCurImg ) {
			if ( mFiles[i].Tex == 0 ) {
				mFiles[i].Tex = loadImage( mFiles[i].Path, true );
			} else
				setImage( mFiles[i].Tex, mFiles[i].Path );
		}
	}
}

void App::unloadImage( const Uint32& img ) {
	if ( mFiles[img].Tex != 0 ) {
		TF->remove( mFiles[img].Tex );
		mFiles[img].Tex = 0;
	}
}

void App::optUpdate() {
	Vector2f scale( mImg.getScale() );
	mImg.createStatic( mFiles[mCurImg].Tex );
	mImg.setScale( scale );

	if ( !mLockZoomAndPosition ) {
		mImg.setScale( mConfig.DefaultImageZoom );
		mImg.setPosition( Vector2f::Zero );
		scaleToScreen();
	}

	if ( mConfig.LateLoading ) {
		mLaterLoad = true;
		mLastLaterTick = Sys::getTicks();

		Texture* Tex = TF->getTexture( mFiles[mCurImg].Tex );

		if ( Tex ) {
			FonCache.setString( "File: " + String::fromUtf8( mFiles[mCurImg].Path ) +
								"\nWidth: " + String::toString( Tex->getWidth() ) +
								"\nHeight: " + String::toString( Tex->getHeight() ) + "\n" +
								String::toString( mCurImg + 1 ) + "/" +
								String::toString( mFiles.size() ) );
		}
	} else
		updateImages();
}

void App::loadFirstImage() {
	if ( mCurImg != 0 )
		fastLoadImage( 0 );
}

void App::loadLastImage() {
	if ( mCurImg != (Int32)( mFiles.size() - 1 ) )
		fastLoadImage( mFiles.size() - 1 );
}

void App::loadNextImage() {
	if ( ( mCurImg + 1 ) < (Int32)mFiles.size() ) {
		createFade();
		mCurImg++;
		optUpdate();
	}
}

void App::loadPrevImage() {
	if ( ( mCurImg - 1 ) >= 0 ) {
		createFade();
		mCurImg--;
		optUpdate();
	}
}

void App::switchFade() {
	if ( mConfig.Fade ) {
		mAlpha = 255.0f;
		mCurAlpha = 255;
		mFading = false;
	}

	mConfig.Fade = !mConfig.Fade;
	mConfig.LateLoading = !mConfig.LateLoading;
	mConfig.BlockWheelSpeed = !mConfig.BlockWheelSpeed;
}

void App::input() {
	KM->update();
	Mouse = KM->getMousePos();

	if ( KM->isKeyDown( KEY_TAB ) && KM->isAltPressed() ) {
		mWindow->minimize();
	}

	if ( KM->isKeyDown( KEY_ESCAPE ) || ( KM->isKeyDown( KEY_Q ) && !Con->isActive() ) ) {
		updateConfig();
		mWindow->close();
	}

	if ( ( KM->isAltPressed() && KM->isKeyUp( KEY_RETURN ) ) ||
		 ( KM->isKeyUp( KEY_F ) && !Con->isActive() ) ) {
		mWindow->toggleFullscreen();

		prepareFrame();
		scaleToScreen();
	}

	if ( KM->isKeyUp( KEY_F6 ) ) {
		switchFade();
	}

	if ( KM->isKeyUp( KEY_F3 ) ) {
		Con->toggle();
	}

	if ( ( KM->isKeyUp( KEY_S ) && !Con->isActive() ) || KM->isKeyUp( KEY_F4 ) ) {
		mCursor = !mCursor;
		mWindow->getCursorManager()->setVisible( mCursor );
	}

	if ( KM->isKeyUp( KEY_H ) && !Con->isActive() ) {
		mShowHelp = !mShowHelp;
	}

	if ( ( ( KM->isKeyUp( KEY_V ) && KM->isControlPressed() ) ||
		   ( KM->isKeyUp( KEY_INSERT ) && KM->isShiftPressed() ) ) &&
		 !Con->isActive() ) {
		std::string tPath = mWindow->getClipboard()->getText();

		if ( ( tPath.size() && isImage( tPath ) ) || FileSystem::isDirectory( tPath ) ) {
			loadDir( tPath );
		}
	}

	if ( !Con->isActive() ) {
		if ( KM->mouseWheelScrolledUp() || KM->isKeyUp( KEY_PAGEUP ) ) {
			if ( !mConfig.BlockWheelSpeed ||
				 Sys::getTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::getTicks();
				loadPrevImage();
				disableSlideShow();
			}
		}

		if ( KM->mouseWheelScrolledDown() || KM->isKeyUp( KEY_PAGEDOWN ) ) {
			if ( !mConfig.BlockWheelSpeed ||
				 Sys::getTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::getTicks();
				loadNextImage();
				disableSlideShow();
			}
		}

		if ( KM->isKeyUp( KEY_I ) ) {
			mConfig.ShowInfo = !mConfig.ShowInfo;
		}
	}

	if ( mFiles.size() && mFiles[mCurImg].Tex && !Con->isActive() ) {
		if ( KM->isKeyUp( KEY_HOME ) ) {
			loadFirstImage();
			disableSlideShow();
		}

		if ( KM->isKeyUp( KEY_END ) ) {
			loadLastImage();
			disableSlideShow();
		}

		if ( KM->isKeyUp( KEY_KP_MULTIPLY ) ) {
			scaleToScreen();
		}

		if ( KM->isKeyUp( KEY_KP_DIVIDE ) ) {
			mImg.setScale( mConfig.DefaultImageZoom );
		}

		if ( KM->isKeyUp( KEY_Z ) ) {
			zoomImage();
		}

		if ( KM->isKeyUp( KEY_N ) ) {
			if ( mWindow->getSize().getWidth() != (Int32)mImg.getSize().getWidth() ||
				 mWindow->getSize().getHeight() != (Int32)mImg.getSize().getHeight() ) {
				mWindow->setSize( mImg.getSize().getWidth(), mImg.getSize().getHeight() );
			}
		}

		if ( Sys::getTicks() - mZoomTicks >= 15 ) {
			mZoomTicks = Sys::getTicks();

			if ( KM->isKeyDown( KEY_KP_MINUS ) )
				mImg.setScale( mImg.getScale() - 0.02f );

			if ( KM->isKeyDown( KEY_KP_PLUS ) )
				mImg.setScale( mImg.getScale() + 0.02f );

			if ( mImg.getScale().x < 0.0125f )
				mImg.setScale( 0.0125f );

			if ( mImg.getScale().x > 50.0f )
				mImg.setScale( 50.0f );
		}

		if ( KM->isKeyDown( KEY_LEFT ) ) {
			Vector2f nPos(
				(Float)( (Int32)( mImg.getPosition().x +
								  ( ( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ),
				mImg.getPosition().y );
			mImg.setPosition( nPos );
		}

		if ( KM->isKeyDown( KEY_RIGHT ) ) {
			Vector2f nPos(
				(Float)( (Int32)( mImg.getPosition().x +
								  ( -( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ),
				mImg.getPosition().y );
			mImg.setPosition( nPos );
		}

		if ( KM->isKeyDown( KEY_UP ) ) {
			Vector2f nPos(
				mImg.getPosition().x,
				(Float)( (Int32)( mImg.getPosition().y +
								  ( ( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ) );
			mImg.setPosition( nPos );
		}

		if ( KM->isKeyDown( KEY_DOWN ) ) {
			Vector2f nPos(
				mImg.getPosition().x,
				(Float)( (Int32)( mImg.getPosition().y +
								  ( -( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ) );
			mImg.setPosition( nPos );
		}

		if ( KM->mouseLeftClicked() ) {
			mMouseLeftPressing = false;
		}

		if ( KM->isMouseLeftPressed() ) {
			Vector2f mNewPos;
			if ( !mMouseLeftPressing ) {
				mMouseLeftStartClick = Mouse;
				mMouseLeftPressing = true;
			} else {
				mMouseLeftClick = Mouse;

				mNewPos.x = static_cast<Float>( mMouseLeftClick.x ) -
							static_cast<Float>( mMouseLeftStartClick.x );
				mNewPos.y = static_cast<Float>( mMouseLeftClick.y ) -
							static_cast<Float>( mMouseLeftStartClick.y );

				if ( mNewPos.x != 0 || mNewPos.y != 0 ) {
					mMouseLeftStartClick = Mouse;
					mImg.setPosition( Vector2f( mImg.getPosition().x + mNewPos.x,
												mImg.getPosition().y + mNewPos.y ) );
				}
			}
		}

		if ( KM->mouseMiddleClicked() ) {
			mMouseMiddlePressing = false;
		}

		if ( KM->isMouseMiddlePressed() ) {
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
						mImg.setScale( mImg.getScale() - Dist );
						if ( mImg.getScale().x < 0.0125f )
							mImg.setScale( 0.0125f );
					} else {
						mImg.setScale( mImg.getScale() + Dist );
					}
				}
			}
		}

		if ( KM->isMouseRightPressed() ) {
			Line2f line( Vector2f( Mouse.x, Mouse.y ), Vector2f( HWidth, HHeight ) );
			mImg.setRotation( line.getAngle() );
		}

		if ( KM->isKeyUp( KEY_X ) ) {
			if ( mImgRT == RENDER_NORMAL )
				mImgRT = RENDER_FLIPPED;
			else if ( mImgRT == RENDER_MIRROR )
				mImgRT = RENDER_FLIPPED_MIRRORED;
			else if ( mImgRT == RENDER_FLIPPED_MIRRORED )
				mImgRT = RENDER_MIRROR;
			else
				mImgRT = RENDER_NORMAL;

			mImg.setRenderMode( mImgRT );
		}

		if ( KM->isKeyUp( KEY_C ) ) {
			if ( mImgRT == RENDER_NORMAL )
				mImgRT = RENDER_MIRROR;
			else if ( mImgRT == RENDER_FLIPPED )
				mImgRT = RENDER_FLIPPED_MIRRORED;
			else if ( mImgRT == RENDER_FLIPPED_MIRRORED )
				mImgRT = RENDER_FLIPPED;
			else
				mImgRT = RENDER_NORMAL;

			mImg.setRenderMode( mImgRT );
		}

		if ( KM->isKeyUp( KEY_R ) ) {
			mImg.setRotation( mImg.getRotation() + 90.0f );
			scaleToScreen();
		}

		if ( KM->isKeyUp( KEY_A ) ) {
			mFilter = mFilter == Texture::Filter::Linear ? Texture::Filter::Nearest
														 : Texture::Filter::Linear;

			Texture* Tex = mImg.getCurrentTextureRegion()->getTexture();

			if ( Tex ) {
				Tex->setFilter( mFilter );
			}
		}

		if ( KM->isKeyUp( KEY_M ) ) {
			mImg.setPosition( Vector2f::Zero );
			mImg.setScale( mConfig.DefaultImageZoom );
			mImg.setRotation( 0.f );
			scaleToScreen();

			if ( EE->getCurrentWindow()->isMaximized() ) {
				EE->getCurrentWindow()->setSize( mImg.getSize().getWidth(),
												 mImg.getSize().getHeight() );
			}
		}

		if ( KM->isKeyUp( KEY_T ) ) {
			mImg.setPosition( Vector2f::Zero );
			mImg.setScale( mConfig.DefaultImageZoom );
			mImg.setRotation( 0.f );
			scaleToScreen();
		}

		if ( KM->isKeyUp( KEY_E ) ) {
			createSlideShow( mSlideTime );
		}

		if ( KM->isKeyUp( KEY_D ) ) {
			disableSlideShow();
		}

		if ( KM->isKeyUp( KEY_K ) ) {
			Texture* curTex;

			if ( NULL != mImg.getCurrentTextureRegion() &&
				 NULL != ( curTex = mImg.getCurrentTextureRegion()->getTexture() ) ) {
				curTex->setMipmap( !curTex->getMipmap() );
				curTex->reload();
			}
		}

		if ( KM->isKeyUp( KEY_L ) ) {
			mLockZoomAndPosition = !mLockZoomAndPosition;
		}

		if ( KM->isKeyUp( KEY_F5 ) ) {
			Texture* curTex;

			if ( NULL != mImg.getCurrentTextureRegion() &&
				 NULL != ( curTex = mImg.getCurrentTextureRegion()->getTexture() ) ) {
				Image img( curTex->getFilepath(), 0, formatConfiguration );
				curTex->replace( &img );
			}
		}
	}
}

void App::createSlideShow( Uint32 time ) {
	if ( time < 250 )
		time = 250;

	mSlideShow = true;
	mSlideTime = time;
	mSlideTicks = Sys::getTicks();
}

void App::disableSlideShow() {
	mSlideShow = false;
}

void App::doSlideShow() {
	if ( mSlideShow ) {
		if ( Sys::getTicks() - mSlideTicks >= mSlideTime ) {
			mSlideTicks = Sys::getTicks();

			if ( (Uint32)( mCurImg + 1 ) < mFiles.size() ) {
				loadNextImage();
			} else {
				disableSlideShow();
			}
		}
	}
}

void App::scaleToScreen( const bool& force ) {
	if ( mFiles.size() && mFiles[mCurImg].Tex ) {
		Texture* Tex = TF->getTexture( mFiles[mCurImg].Tex );

		if ( NULL == Tex )
			return;

		if ( Tex->getImageWidth() * mConfig.DefaultImageZoom >= Width ||
			 Tex->getImageHeight() * mConfig.DefaultImageZoom >= Height ) {
			zoomImage();
		} else if ( force ) {
			mImg.setScale( mConfig.DefaultImageZoom );
		}
	}
}

void App::zoomImage() {
	if ( mFiles.size() && mFiles[mCurImg].Tex ) {
		Texture* Tex = TF->getTexture( mFiles[mCurImg].Tex );

		if ( NULL == Tex )
			return;

		Sizef boxSize = mImg.getSize();

		mImg.setScale( eemin( Width / boxSize.getWidth(), Height / boxSize.getHeight() ) );
	}
}

void App::setWindowCaption() {
	if ( mFiles.size() )
		mInfo = "EEiv - " + mFiles[mCurImg].Path;
	else
		mInfo = "EEiv";

	if ( mInfo != mWindow->getTitle() )
		mWindow->setTitle( mInfo );
}

void App::prepareFrame() {
	Width = mWindow->getWidth();
	Height = mWindow->getHeight();
	HWidth = Width * 0.5f;
	HHeight = Height * 0.5f;
}

void App::render() {
	prepareFrame();

	doSlideShow();

	if ( mFiles.size() && mFiles[mCurImg].Tex ) {
		doFade();

		Texture* Tex = mImg.getCurrentTextureRegion()->getTexture();

		if ( Tex ) {
			Float X = static_cast<Float>(
				static_cast<Int32>( HWidth - mImg.getSize().getWidth() * 0.5f ) );
			Float Y = static_cast<Float>(
				static_cast<Int32>( HHeight - mImg.getSize().getHeight() * 0.5f ) );

			mImg.setOffset( Vector2i( X, Y ) );
			mImg.setAlpha( mCurAlpha );
			mImg.draw();
		}
	}

	if ( mConfig.ShowInfo )
		FonCache.draw( 0, 0 );

	printHelp();

	SceneManager::instance()->update();
	SceneManager::instance()->draw();
}

void App::createFade() {
	if ( mConfig.Fade ) {
		mAlpha = 0.0f;
		mCurAlpha = 0;
		mFading = true;
		mOldImg = mImg;
	}
}

void App::doFade() {
	if ( mConfig.Fade && mFading ) {
		mAlpha += ( 255 * RET ) / mConfig.TransitionTime;
		mCurAlpha = static_cast<Uint8>( mAlpha );

		if ( mAlpha >= 255.0f ) {
			mAlpha = 255.0f;
			mCurAlpha = 255;
			mFading = false;
		}

		Texture* Tex = NULL;

		if ( NULL != mOldImg.getCurrentTextureRegion() &&
			 ( Tex = mOldImg.getCurrentTextureRegion()->getTexture() ) ) {
			Float X = static_cast<Float>(
				static_cast<Int32>( HWidth - mOldImg.getSize().getWidth() * 0.5f ) );
			Float Y = static_cast<Float>(
				static_cast<Int32>( HHeight - mOldImg.getSize().getHeight() * 0.5f ) );

			mOldImg.setOffset( Vector2i( X, Y ) );
			mOldImg.setAlpha( 255 - mCurAlpha );
			mOldImg.draw();
		}
	}
}

void App::end() {
	Ini.setValueI( "window", "width", mConfig.Width );
	Ini.setValueI( "window", "height", mConfig.Height );
	Ini.setValueI( "window", "bitcolor", mConfig.BitColor );
	Ini.setValueI( "window", "windowed", mConfig.Windowed );
	Ini.setValueI( "window", "resizeable", mConfig.Resizeable );
	Ini.setValueI( "window", "vsync", mConfig.VSync );
	Ini.setValueI( "window", "doublebuffering", mConfig.DoubleBuffering );
	Ini.setValueI( "window", "usedesktopresolution", mConfig.UseDesktopResolution );
	Ini.setValueI( "window", "noframe", mConfig.NoFrame );
	Ini.setValueI( "window", "maximizeatstart", mConfig.MaximizeAtStart );
	Ini.setValueI( "window", "framelimit", mConfig.FrameLimit );
	Ini.setValueI( "viewer", "fade", mConfig.Fade );
	Ini.setValueI( "viewer", "lateloading", mConfig.LateLoading );
	Ini.setValueI( "viewer", "blockwheelspeed", mConfig.BlockWheelSpeed );
	Ini.setValueI( "viewer", "showinfo", mConfig.ShowInfo );
	Ini.setValueI( "viewer", "transitiontime", mConfig.TransitionTime );
	Ini.setValueI( "viewer", "consolefontsize", mConfig.ConsoleFontSize );
	Ini.setValueI( "viewer", "appfontsize", mConfig.AppFontSize );
	Ini.setValueF( "viewer", "defaultimagezoom", mConfig.DefaultImageZoom );
	Ini.setValueI( "viewer", "wheelblocktime", mConfig.WheelBlockTime );

	Ini.writeFile();
	Engine::destroySingleton();
}

std::string App::createSavePath( const std::string& oriPath, Uint32 width, Uint32 height,
								 Image::SaveType saveType ) {
	Image::SaveType type = saveType == Image::SaveType::SAVE_TYPE_UNKNOWN
							   ? Image::extensionToSaveType( FileSystem::fileExtension( oriPath ) )
							   : saveType;

	if ( Image::SaveType::SAVE_TYPE_UNKNOWN == type ) {
		type = Image::SaveType::SAVE_TYPE_PNG;
	}

	return FileSystem::fileRemoveExtension( oriPath ) + "-" + String::toString( width ) + "x" +
		   String::toString( height ) + "." + Image::saveTypeToExtension( type );
}

Image::SaveType App::getPathSaveType( const std::string& path ) {
	return Image::extensionToSaveType( FileSystem::fileExtension( path ) );
}

void App::scaleImg( const std::string& Path, const Float& Scale, const bool& overridePath,
					Image::SaveType saveType ) {
	int w, h, c;

	if ( Image::getInfo( Path, &w, &h, &c ) && Scale > 0.f ) {
		Int32 new_width = static_cast<Int32>( w * Scale );
		Int32 new_height = static_cast<Int32>( h * Scale );
		std::string outputPath( Path );

		if ( !overridePath ) {
			outputPath = createSavePath( Path, new_width, new_height, saveType );
		}

		resizeImg( Path, outputPath, new_width, new_height, saveType );
	} else {
		Con->pushText( "Images does not exists." );
	}
}

void App::resizeImg( const std::string& Path, const std::string& outputPath, const Uint32& NewWidth,
					 const Uint32& NewHeight, Image::SaveType saveType ) {
	if ( isImage( Path ) ) {
		Image::SaveType type = Image::SaveType::SAVE_TYPE_UNKNOWN != saveType
								   ? saveType
								   : getPathSaveType( outputPath );

		Image img( Path );

		img.resize( NewWidth, NewHeight );

		img.saveToFile( outputPath, type );
	} else {
		Con->pushText( "Images does not exists." );
	}
}

void App::thumgnailImg( const std::string& Path, const Uint32& MaxWidth, const Uint32& MaxHeight,
						Image::SaveType saveType ) {
	if ( isImage( Path ) ) {
		Image img( Path );

		Image* thumb = img.thumbnail( MaxWidth, MaxHeight );

		if ( NULL != thumb ) {
			std::string newPath(
				createSavePath( Path, thumb->getWidth(), thumb->getHeight(), saveType ) );
			Image::SaveType type = Image::SaveType::SAVE_TYPE_UNKNOWN != saveType
									   ? saveType
									   : getPathSaveType( newPath );

			thumb->saveToFile( newPath, type );

			eeSAFE_DELETE( thumb );
		}
	} else {
		Con->pushText( "Images does not exists." );
	}
}

void App::centerCropImg( const std::string& Path, const Uint32& Width, const Uint32& Height,
						 Image::SaveType saveType ) {
	if ( isImage( Path ) ) {
		Image img( Path );

		Sizei nSize;

		double scale = 1.f;

		scale = eemax( (double)Width / (double)img.getWidth(),
					   (double)Height / (double)img.getHeight() );

		nSize.x = Math::round( img.getWidth() * scale );
		nSize.y = Math::round( img.getHeight() * scale );

		if ( nSize.getWidth() == (int)Width - 1 || nSize.getWidth() == (int)Width + 1 ) {
			nSize.x = (int)Width;
		}

		if ( nSize.getHeight() == (int)Height - 1 || nSize.getHeight() == (int)Height + 1 ) {
			nSize.y = (int)Height;
		}

		img.resize( nSize.getWidth(), nSize.getHeight() );

		Image* croppedImg = NULL;
		Rect rect;

		if ( img.getWidth() > Width ) {
			rect.Left = ( img.getWidth() - Width ) / 2;
			rect.Right = rect.Left + Width;
			rect.Top = 0;
			rect.Bottom = Height;
		} else {
			rect.Top = ( img.getHeight() - Height ) / 2;
			rect.Bottom = rect.Top + Height;
			rect.Left = 0;
			rect.Right = Width;
		}

		croppedImg = img.crop( rect );

		if ( NULL != croppedImg ) {
			std::string newPath(
				createSavePath( Path, croppedImg->getWidth(), croppedImg->getHeight(), saveType ) );
			Image::SaveType type = Image::SaveType::SAVE_TYPE_UNKNOWN != saveType
									   ? saveType
									   : getPathSaveType( newPath );

			croppedImg->saveToFile( newPath, type );

			eeSAFE_DELETE( croppedImg );
		} else {
			std::string newPath(
				createSavePath( Path, img.getWidth(), img.getHeight(), saveType ) );
			Image::SaveType type = Image::SaveType::SAVE_TYPE_UNKNOWN != saveType
									   ? saveType
									   : getPathSaveType( newPath );

			img.saveToFile( newPath, type );
		}
	}
}

void App::batchImgScale( const std::string& Path, const Float& Scale, const bool& overridePath ) {
	std::string iPath = Path;
	std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( iPath );

	if ( iPath[iPath.size() - 1] != '/' )
		iPath += "/";

	for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
		std::string fPath = iPath + tmpFiles[i];

		scaleImg( fPath, Scale, overridePath );
	}
}

void App::batchImgThumbnail( Sizei size, std::string dir, bool recursive ) {
	FileSystem::dirAddSlashAtEnd( dir );

	std::vector<std::string> files = FileSystem::filesGetInPath( dir );

	for ( size_t i = 0; i < files.size(); i++ ) {
		std::string fpath( dir + files[i] );

		if ( FileSystem::isDirectory( fpath ) ) {
			if ( recursive ) {
				batchImgThumbnail( size, fpath, recursive );
			}
		} else {
			int w, h, c;
			if ( Image::getInfo( fpath, &w, &h, &c ) ) {
				if ( w > size.getWidth() || h > size.getHeight() ) {
					Image img( fpath );

					Image* thumb = img.thumbnail( size.getWidth(), size.getHeight() );

					if ( NULL != thumb ) {
						thumb->saveToFile( fpath, Image::extensionToSaveType(
													  FileSystem::fileExtension( fpath ) ) );

						Con->pushText(
							"Thumbnail created for '%s'. Old size %dx%d. New size %dx%d.",
							fpath.c_str(), img.getWidth(), img.getHeight(), thumb->getWidth(),
							thumb->getHeight() );

						eeSAFE_DELETE( thumb );
					} else {
						Con->pushText( "Thumbnail %s failed to create.", fpath.c_str() );
					}
				}
			}
		}
	}
}

void App::cmdSlideShow( const std::vector<String>& params ) {
	String Error( "Usage example: slideshow slide_time_in_ms" );

	if ( params.size() >= 2 ) {
		Uint32 time = 0;

		bool Res = String::fromString<Uint32>( time, params[1] );

		if ( Res ) {
			if ( !mSlideShow ) {
				createSlideShow( time );
			} else {
				if ( 0 == time ) {
					mSlideShow = false;
				}
			}
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdImgResize( const std::vector<String>& params ) {
	String Error(
		"Usage example: imgresize new_width new_height path_to_img format override_image_path" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		Image::SaveType saveType = Image::SaveType::SAVE_TYPE_UNKNOWN;
		Uint32 override = 0;

		bool Res1 = String::fromString<Uint32>( nWidth, params[1] );
		bool Res2 = String::fromString<Uint32>( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}

			if ( params.size() > 5 ) {
				String::fromString<Uint32>( override, params[5] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res1 && Res2 ) {
			std::string savePath =
				override != 0 ? myPath : createSavePath( myPath, nWidth, nHeight, saveType );

			resizeImg( myPath, savePath, nWidth, nHeight, saveType );
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdImgThumbnail( const std::vector<String>& params ) {
	String Error( "Usage example: imgthumbnail max_width max_height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		Image::SaveType saveType = Image::SaveType::SAVE_TYPE_UNKNOWN;

		bool Res1 = String::fromString<Uint32>( nWidth, params[1] );
		bool Res2 = String::fromString<Uint32>( nHeight, params[2] );

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
			thumgnailImg( myPath, nWidth, nHeight, saveType );
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdImgCenterCrop( const std::vector<String>& params ) {
	String Error( "Usage example: imgcentercrop width height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		Image::SaveType saveType = Image::SaveType::SAVE_TYPE_UNKNOWN;

		bool Res1 = String::fromString<Uint32>( nWidth, params[1] );
		bool Res2 = String::fromString<Uint32>( nHeight, params[2] );

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
			centerCropImg( myPath, nWidth, nHeight, saveType );
		else
			Con->pushText( Error );
	} else {
		Con->pushText( Error );
	}
}

void App::cmdImgScale( const std::vector<String>& params ) {
	String Error( "Usage example: imgscale scale path_to_img format override_path" );
	if ( params.size() >= 2 ) {
		Float Scale = 0;
		Image::SaveType saveType = Image::SaveType::SAVE_TYPE_UNKNOWN;
		Uint32 override = 0;

		bool Res = String::fromString<Float>( Scale, params[1] );

		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].toUtf8();

			if ( params.size() > 3 ) {
				saveType = Image::extensionToSaveType( params[3] );
			}

			if ( params.size() > 4 ) {
				String::fromString<Uint32>( override, params[4] );
			}
		} else {
			myPath = mFilePath + mFile;
		}

		if ( Res )
			scaleImg( myPath, Scale, 0 != override, saveType );
		else
			Con->pushText( Error );
	} else {
		Con->pushText( Error );
	}
}

void App::cmdBatchImgScale( const std::vector<String>& params ) {
	String Error(
		"Usage example: batchimgscale scale_value override_img_path ( default disabled ) "
		"directory_to_resize_img ( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 2 ) {
		Float Scale = 0;
		Uint32 override = 0;

		bool Res = String::fromString<Float>( Scale, params[1] );

		override = String::fromString<Uint32>( override, params[2] );

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mFilePath;

		if ( Res ) {
			if ( FileSystem::isDirectory( myPath ) ) {
				batchImgScale( myPath, Scale, 0 != override );
			} else {
				Con->pushText( "Second argument is not a directory!" );
			}
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdBatchImgThumbnail( const std::vector<String>& params ) {
	String Error(
		"Usage example: batchimgthumbnail max_width max_height directory_to_create_thumbs "
		"recursive ( if no dir is passed, it will use the current dir opened )" );

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
				batchImgThumbnail( Sizei( max_width, max_height ), myPath, recursive );
			} else {
				Con->pushText( "Third argument is not a directory!" );
			}
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdImgChangeFormat( const std::vector<String>& params ) {
	String Error( "Usage example: imgchangeformat to_format image_to_reformat ( if null will use "
				  "the current loaded image )" );
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
			std::string fPath = myPath;
			std::string fExt = FileSystem::fileExtension( fPath );

			if ( fExt == fromFormat ) {
				std::string fName;

				if ( fExt != toFormat )
					fName = fPath.substr( 0, fPath.find_last_of( "." ) + 1 ) + toFormat;
				else
					fName = fPath + "." + toFormat;

				Image::SaveType saveType = Image::extensionToSaveType( toFormat );

				if ( Image::SaveType::SAVE_TYPE_UNKNOWN != saveType ) {
					Image* img = eeNew( Image, ( fPath ) );
					img->saveToFile( fName, saveType );
					eeSAFE_DELETE( img );

					Con->pushText( fName + " created." );
				}
			}
		} else {
			Con->pushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdBatchImgChangeFormat( const std::vector<String>& params ) {
	String Error( "Usage example: batchimgchangeformat from_format to_format directory_to_reformat "
				  "( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 4 ) {
		std::string fromFormat = params[1].toUtf8();
		std::string toFormat = params[2].toUtf8();

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mFilePath;

		if ( FileSystem::isDirectory( myPath ) ) {
			std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( myPath );

			if ( myPath[myPath.size() - 1] != '/' )
				myPath += "/";

			for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
				std::string fPath = myPath + tmpFiles[i];
				std::string fExt = FileSystem::fileExtension( fPath );

				if ( isImage( fPath ) && fExt == fromFormat ) {
					std::string fName;

					if ( fExt != toFormat )
						fName = fPath.substr( 0, fPath.find_last_of( "." ) + 1 ) + toFormat;
					else
						fName = fPath + "." + toFormat;

					Image::SaveType saveType = Image::extensionToSaveType( toFormat );

					if ( Image::SaveType::SAVE_TYPE_UNKNOWN != saveType ) {
						Image* img = eeNew( Image, ( fPath ) );
						img->saveToFile( fPath, saveType );
						eeSAFE_DELETE( img );

						Con->pushText( fName + " created." );
					}
				}
			}
		} else {
			Con->pushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con->pushText( Error );
	}
}

void App::cmdMoveTo( const std::vector<String>& params ) {
	if ( params.size() >= 2 && mFiles.size() > 0 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( tInt )
			tInt--;

		if ( Res && tInt >= 0 && tInt < (Int32)mFiles.size() ) {
			Con->pushText( "moveto: moving to image number " + String::toString( tInt + 1 ) );
			fastLoadImage( tInt );
		} else if ( params[1] == "last" ) {
			Con->pushText( "moveto: moving to last" );
			fastLoadImage( mFiles.size() - 1 );
		} else if ( params[1] == "first" ) {
			Con->pushText( "moveto: moving to first" );
			fastLoadImage( 0 );
		} else {
			Con->pushText( "moveto: image number does not exists" );
		}
	} else {
		Con->pushText( "Expected some parameter" );
	}
}

void App::cmdSetBlockWheel( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.BlockWheelSpeed = tInt ? true : false;
			Con->pushText( "setblockwheel " + String::toString( tInt ) );
		} else
			Con->pushText( "Valid parameters are 0 or 1." );
	} else
		Con->pushText( "Expected some parameter" );
}

void App::cmdSetLateLoading( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.LateLoading = tInt ? true : false;
			Con->pushText( "setlateloading " + String::toString( tInt ) );
		} else
			Con->pushText( "Valid parameters are 0 or 1." );
	} else
		Con->pushText( "Expected some parameter" );
}

void App::cmdSetImgFade( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mConfig.Fade = tInt ? true : false;
			Con->pushText( "setimgfade " + String::toString( tInt ) );
		} else
			Con->pushText( "Valid parameters are 0 or 1." );
	} else
		Con->pushText( "Expected some parameter" );
}

void App::cmdSetBackColor( const std::vector<String>& params ) {
	String Error(
		"Usage example: setbackcolor 255 255 255 (RGB Color, numbers between 0 and 255)" );

	if ( params.size() >= 2 ) {
		if ( params.size() >= 2 ) {
			mWindow->setClearColor( Color::fromString( params[1].toUtf8() ).toRGB() );
			Con->pushText( "setbackcolor applied" );
			return;
		}

		Con->pushText( Error );
	}
}

void App::cmdLoadImg( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].toUtf8();

		if ( isImage( myPath ) || isHttpUrl( myPath ) ) {
			loadDir( myPath );
		} else
			Con->pushText( "\"" + myPath +
						   "\" is not an image path or the image is not supported." );
	}
}

void App::cmdLoadDir( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].toUtf8();
		if ( params.size() > 2 ) {
			for ( Uint32 i = 2; i < params.size(); i++ )
				myPath += " " + params[i].toUtf8();
		}

		if ( FileSystem::isDirectory( myPath ) ) {
			loadDir( myPath );
		} else
			Con->pushText( "If you want to load an image use loadimg. \"" + myPath +
						   "\" is not a directory path." );
	}
}

void App::cmdSetZoom( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0;

		bool Res = String::fromString<Float>( tFloat, params[1] );

		if ( Res && tFloat >= 0 && tFloat <= 10 ) {
			Con->pushText( "setzoom: zoom level " + String::toString( tFloat ) );
			mImg.setScale( tFloat );
		} else
			Con->pushText( "setzoom: value out of range" );
	} else
		Con->pushText( "Expected some parameter" );
}

void App::printHelp() {
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
			HT +=
				"Key K: Reload the image switching the mipmap state ( with or without mipmaps )\n";
			HT += "Key L: Lock zoom and image position when switching images\n";
			HT += "Key Left - Right - Top - Down or left mouse press: Move the image\n";
			HT += "Key F5: Reload the image\n";
			HT += "Key F6: Switch fade\n";
			HT += "Key F12: Take a screenshot\n";
			HT += "Key HOME: Go to the first screenshot on the folder\n";
			HT += "Key END: Go to the last screenshot on the folder";

			mHelpCache = eeNew( Text, ( HT, Fon, mConfig.AppFontSize ) );
		}

		mHelpCache->draw( Left, Height - Top - mHelpCache->getTextHeight() );
	}
}
