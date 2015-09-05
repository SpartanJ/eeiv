#ifndef CAPP_H
#define CAPP_H

#include <eepp/ee.hpp>

class cApp {
	public:
		cApp( int argc, char *argv[] );

		~cApp();

		void Process();
		void LoadDir( const std::string& path, const bool& getimages = true );
	private:
		bool Init();
		void Input();
		void Render();
		void End();
		void PrepareFrame();
		void ScaleToScreen( const bool& force = false );
		void GetImages();
		Uint32 CurImagePos( const std::string& path );
		Uint32 LoadImage( const std::string& path, const bool& SetAsCurrent = false );
		void LoadNextImage();
		void LoadPrevImage();
		void ZoomImage();
		void UnloadImage( const Uint32& img );
		void UpdateImages();
		void SetImage( const Uint32& Tex, const std::string& path );
		void FastLoadImage( const Uint32& ImgNum );
		void DoFade();
		void CreateFade();
		void OptUpdate();
		void PrintHelp();
		void LoadFirstImage();
		void LoadLastImage();
		void LoadConfig();
		void ClearTempDir();
		void VideoResize();
		void RestoreMouse();
		void BatchDir( const std::string& Path, const Float& Scale );
		void ScaleImg( const std::string& Path, const Float& Scale );
		void ResizeImg( const std::string& Path, const Uint32& NewWidth, const Uint32& NewHeight );
		void ThumgnailImg( const std::string& Path, const Uint32& MaxWidth, const Uint32& MaxHeight );
		void CenterCropImg( const std::string& Path, const Uint32& Width, const Uint32& Height );
		void SwitchFade();
		void DoSlideShow();
		void CreateSlideShow( Uint32 time );
		void DisableSlideShow();
		void BatchImgThumbnail( Sizei size, std::string dir, bool recursive );

		void CmdLoadDir( const std::vector < String >& params );
		void CmdLoadImg( const std::vector < String >& params );
		void CmdSetBackColor( const std::vector < String >& params );
		void CmdSetImgFade( const std::vector < String >& params );
		void CmdSetLateLoading( const std::vector < String >& params );
		void CmdSetBlockWheel( const std::vector < String >& params );
		void CmdMoveTo( const std::vector < String >& params );
		void CmdBatchImgResize( const std::vector < String >& params );
		void CmdBatchImgChangeFormat( const std::vector < String >& params );
		void CmdBatchImgThumbnail( const std::vector < String >& params );
		void CmdImgChangeFormat( const std::vector < String >& params );
		void CmdImgResize( const std::vector < String >& params );
		void CmdImgScale( const std::vector < String >& params );
		void CmdImgThumbnail( const std::vector < String >& params );
		void CmdImgCenterCrop( const std::vector < String >& params );
		void CmdSlideShow( const std::vector < String >& params );
		void CmdSetZoom( const std::vector < String >& params );

		std::string CreateSavePath( const std::string& oriPath, Uint32 width, Uint32 height );

		EE_SAVE_TYPE GetPathSaveType( const std::string& path );

		Engine * EE;
		TextureFactory * TF;
		Window::Window * mWindow;
		System::Log * Log;
		Window::Input * KM;

		std::string MyPath;

		Font * Fon; //! Default App Font
		Font * Mon; //! Console App Font

		TTFFont * TTF, * TTFMon;
		TextureFont * TexF, * TexFMon;

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

		Clock TE;

		TextCache * mHelpCache;

		bool	mSlideShow;
		Uint32	mSlideTime;
		Uint32	mSlideTicks;

		IniFile Ini;
};

#endif
