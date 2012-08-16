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
		void BatchDir( const std::string& Path, const eeFloat& Scale );
		void ScaleImg( const std::string& Path, const eeFloat& Scale );
		void ResizeImg( const std::string& Path, const Uint32& NewWidth, const Uint32& NewHeight );
		void ResizeTexture( cTexture * pTex, const Uint32& NewWidth, const Uint32& NewHeight, const std::string& SavePath );
		void SwitchFade();
		void DoSlideShow();
		void CreateSlideShow( Uint32 time );
		void DisableSlideShow();

		void CmdLoadDir( const std::vector < String >& params );
		void CmdLoadImg( const std::vector < String >& params );
		void CmdSetBackColor( const std::vector < String >& params );
		void CmdSetImgFade( const std::vector < String >& params );
		void CmdSetLateLoading( const std::vector < String >& params );
		void CmdSetBlockWheel( const std::vector < String >& params );
		void CmdMoveTo( const std::vector < String >& params );
		void CmdBatchImgResize( const std::vector < String >& params );
		void CmdBatchImgChangeFormat( const std::vector < String >& params );
		void CmdImgChangeFormat( const std::vector < String >& params );
		void CmdImgResize( const std::vector < String >& params );
		void CmdImgScale( const std::vector < String >& params );
		void CmdSlideShow( const std::vector < String >& params );

		cEngine * EE;
		cTextureFactory * TF;
		cWindow * mWindow;
		cLog * Log;
		cInput * KM;

		std::string MyPath;

		cFont * Fon; //! Default App Font
		cFont * Mon; //! Console App Font

		cTTFFont * TTF, * TTFMon;
		cTextureFont * TexF, * TexFMon;

		cConsole Con; //! Console Instance

		eeVector2i Mouse; //! Mouse Position on Screen
		eeDouble ET;	//! Elapsed Time Between Frames
		eeDouble RET;	//! Relative Elapsed Time ( skip time in Input() )

		eeFloat Width, Height;		//! Width and Height of the Window
		eeFloat HWidth, HHeight;	//! Half Width and Height of the Window

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

		bool mShowInfo;

		bool mFade, mFading;
		eeFloat mAlpha;
		Uint8 mCurAlpha;

		bool mLateLoading;
		bool mLaterLoad;
		Int32 mLastLaterTick;

		cTimeElapsed TEP;

		bool mCursor;

		cSprite mImg, mOldImg;

		bool mMouseLeftPressing;
		eeVector2i mMouseLeftStartClick, mMouseLeftClick;

		bool mMouseMiddlePressing;
		eeVector2i mMouseMiddleStartClick, mMouseMiddleClick;

		EE_RENDERTYPE mImgRT;

		Int32 mLastWheelUse;
		bool mShowHelp;
		bool mBlockWheelSpeed;
		Uint16 mWheelBlockTime;

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
		} sConfig;
		sConfig mConfig;

		bool mFirstLoad;

		std::string mStorePath;
		std::string mTmpPath;
		bool mUsedTempDir;

		cTimeElapsed TE;

		cTextCache * mHelpCache;

		bool	mSlideShow;
		Uint32	mSlideTime;
		Uint32	mSlideTicks;
};

#endif
