#include <Renderer/Renderer.h>
#include <Renderer/LineRendererImp.h>
#include <Renderer/TriangleRendererImp.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <Core/Reference.h>

class Application
{
private:
	// Debug renderer modules
	LineRendererImp *			mLineRenderer;
	TriangleRendererImp *		mTriangleRenderer;

	// Camera state
	CameraState					mLocalCamera;
	CameraState					mWorldCamera;

protected:
	// Render module
	Renderer *					mRenderer;

	// Input
	Keyboard *					mKeyboard;
	Mouse *						mMouse;

public:
	// Constructor / destructor
								Application();
	virtual						~Application();

	// Enter the main loop
	void						Run();

protected:
	// Callback to render a frame
	virtual bool				RenderFrame(float inDeltaTime)					{ return false; }

	// Will restore camera position to that returned by GetInitialCamera
	void						ResetCamera();

	// Override to specify the initial camera state (world space)
	virtual void				GetInitialCamera(CameraState &ioState) const	{ }

	// Override to specify a camera pivot point and orientation (world space)
	virtual Mat44				GetCameraPivot() const							{ return Mat44::sIdentity(); }

	// Get scale factor for this world, used to boost camera speed and to scale detail of the shadows
	virtual float				GetWorldScale() const							{ return 1.0f; }

	// Get current state of the camera (world space)
	const CameraState &			GetCamera() const								{ return mWorldCamera; }

private:
	void						UpdateCamera(float inDeltaTime);

	uint64						mLastUpdateTicks;
};