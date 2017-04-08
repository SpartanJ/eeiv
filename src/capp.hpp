#ifndef CAPP_H
#define CAPP_H

#include <eepp/ee.hpp>

class App {
	public:
		App( int argc, char *argv[] );

		~App();

		void process();
		void loadDir( const std::string& path, const bool& getimages = true );
	private:
		bool init();
		void input();
		void render();
		void end();
		void prepareFrame();
		void scaleToScreen( const bool& force = false );
		void getImages();
		Uint32 curImagePos( const std::string& path );
		Uint32 loadImage( const std::string& path, const bool& SetAsCurrent = false );
		void loadNextImage();
		void loadPrevImage();
		void zoomImage();
		void unloadImage( const Uint32& img );
		void updateImages();
		void setImage( const Uint32& Tex, const std::string& path );
		void fastLoadImage( const Uint32& ImgNum );
		void doFade();
		void createFade();
		void optUpdate();
		void printHelp();
		void loadFirstImage();
		void loadLastImage();
		void getConfig();
		void loadConfig();
		void clearTempDir();
		void videoResize();
		void restoreMouse();
		void batchImgScale(const std::string& Path, const Float& Scale , const bool & overridePath);
		void scaleImg(const std::string& Path, const Float& Scale, const bool & overridePath, EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN );
		void resizeImg(const std::string& Path, const std::string & outputPath, const Uint32& NewWidth, const Uint32& NewHeight, EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN );
		void thumgnailImg( const std::string& Path, const Uint32& MaxWidth, const Uint32& MaxHeight, EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN );
		void centerCropImg( const std::string& Path, const Uint32& Width, const Uint32& Height, EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN );
		void switchFade();
		void doSlideShow();
		void createSlideShow( Uint32 time );
		void disableSlideShow();
		void batchImgThumbnail( Sizei size, std::string dir, bool recursive );

		void cmdLoadDir( const std::vector < String >& params );
		void cmdLoadImg( const std::vector < String >& params );
		void cmdSetBackColor( const std::vector < String >& params );
		void cmdSetImgFade( const std::vector < String >& params );
		void cmdSetLateLoading( const std::vector < String >& params );
		void cmdSetBlockWheel( const std::vector < String >& params );
		void cmdMoveTo( const std::vector < String >& params );
		void cmdBatchImgScale( const std::vector < String >& params );
		void cmdBatchImgChangeFormat( const std::vector < String >& params );
		void cmdBatchImgThumbnail( const std::vector < String >& params );
		void cmdImgChangeFormat( const std::vector < String >& params );
		void cmdImgResize( const std::vector < String >& params );
		void cmdImgScale( const std::vector < String >& params );
		void cmdImgThumbnail( const std::vector < String >& params );
		void cmdImgCenterCrop( const std::vector < String >& params );
		void cmdSlideShow( const std::vector < String >& params );
		void cmdSetZoom( const std::vector < String >& params );

		std::string createSavePath( const std::string& oriPath, Uint32 width, Uint32 height , EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN );

		EE_SAVE_TYPE getPathSaveType( const std::string& path );

		Engine * EE;
		TextureFactory * TF;
		EE::Window::Window * mWindow;
		System::Log * Log;
		EE::Window::Input * KM;

		std::string MyPath;

		Font * Fon; //! Default App Font
		Font * Mon; //! Console App Font
		Text FonCache;

		FontTrueType * TTF, * TTFMon;

		Console Con; //! Console Instance

		Vector2i Mouse; //! Mouse Position on Screen
		double ET;	//! Elapsed Time Between Frames
		double RET;	//! Relative Elapsed Time ( skip time in Input() )

		Float Width, Height;		//! Width and Height of the Window
		Float HWidth, HHeight;	//! Half Width and Height of the Window

		std::string SLASH; //! Default SLASH string

		Int32 mLastTicks, mZoomTicks;
		std::string mInfo; //! Window caption string

		typedef struct {
			std::string Path;
			Uint32 Tex;
		} mImage;
		std::vector<mImage> mFiles; //! Directory image file list

		std::string mFilePath;	//! Directory path of files
		std::string mFile;		//! Last used file name

		Int32 mCurImg;	//! Current Image Selected from mFile

		bool mFading;
		Float mAlpha;
		Uint8 mCurAlpha;

		bool mLaterLoad;
		Int32 mLastLaterTick;

		Clock TEP;

		bool mCursor;

		Sprite mImg, mOldImg;

		bool mMouseLeftPressing;
		Vector2i mMouseLeftStartClick, mMouseLeftClick;

		bool mMouseMiddlePressing;
		Vector2i mMouseMiddleStartClick, mMouseMiddleClick;

		EE_RENDER_MODE mImgRT;
		EE_TEX_FILTER mFilter;

		Int32 mLastWheelUse;
		bool mShowHelp;

		typedef struct {
			Uint32 Width;
			Uint32 Height;
			Uint8 BitColor;
			bool Windowed;
			bool Resizeable;
			bool VSync;
			bool DoubleBuffering;
			bool UseDesktopResolution;
			bool NoFrame;
			bool MaximizeAtStart;
			bool Fade;
			bool LateLoading;
			bool BlockWheelSpeed;
			bool ShowInfo;
			Uint32 FrameLimit;
			float TransitionTime;
			int ConsoleFontSize;
			int AppFontSize;
			float DefaultImageZoom;
			Uint32 WheelBlockTime;
		} sConfig;
		sConfig mConfig;

		bool mFirstLoad;

		std::string mStorePath;
		std::string mTmpPath;
		bool mUsedTempDir;
		bool mLockZoomAndPosition;

		Clock TE;

		Text * mHelpCache;

		bool	mSlideShow;
		Uint32	mSlideTime;
		Uint32	mSlideTicks;

		IniFile Ini;
};

#endif
