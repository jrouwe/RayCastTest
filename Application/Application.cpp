#include <pch.h>

#include <Application/Application.h>
#include <Core/TickCounter.h>

// Constructor
Application::Application() : 
	mLineRenderer(nullptr),
	mTriangleRenderer(nullptr),
	mRenderer(nullptr),
	mKeyboard(nullptr),
	mMouse(nullptr)
{
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		
	// Create renderer
	mRenderer = new Renderer;
	mRenderer->Initialize();

	// Init line renderer
	mLineRenderer = new LineRendererImp(mRenderer);

	// Init triangle renderer
	mTriangleRenderer = new TriangleRendererImp(mRenderer);

	// Init keyboard
	mKeyboard = new Keyboard;
	mKeyboard->Initialize(mRenderer);

	// Init mouse
	mMouse = new Mouse;
	mMouse->Initialize(mRenderer);

	// Get initial time
	mLastUpdateTicks = GetProcessorTickCount();
}

// Destructor
Application::~Application()
{
	delete mMouse;
	delete mKeyboard;
	delete mTriangleRenderer;
	delete mLineRenderer;
	delete mRenderer;
}

// Main loop
void Application::Run()
{
	// Set initial camera position
	ResetCamera();

	// Main message loop
	MSG msg;
	memset(&msg, 0, sizeof(msg));
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Get new input
			mKeyboard->Poll();
			mMouse->Poll();

			// Calculate delta time
			uint64 ticks = GetProcessorTickCount();
			uint64 delta = ticks - mLastUpdateTicks;
			mLastUpdateTicks = ticks;
			float clock_delta_time = float(delta) / float(GetProcessorTicksPerSecond());

			// Clear debug lines if we're going to step
			mLineRenderer->Clear();
			mTriangleRenderer->Clear();

			// Update the camera position
			UpdateCamera(clock_delta_time);

			// Start rendering
			mRenderer->BeginFrame(mWorldCamera, GetWorldScale());

			if (!RenderFrame(clock_delta_time))
				break;

			// Draw coordinate axis
			mLineRenderer->DrawCoordinateSystem(Mat44::sIdentity());

			// Draw lines
			mLineRenderer->Draw();

			// Draw triangles
			mTriangleRenderer->Draw();
			
			// Show the frame
			mRenderer->EndFrame();
		}
	}
}

void Application::ResetCamera()
{
	// Get world space camera state
	mWorldCamera = CameraState();
	GetInitialCamera(mWorldCamera);

	// Convert to local space using pivot
	Mat44 inv_pivot = GetCameraPivot().InversedRotationTranslation();
	mLocalCamera = mWorldCamera;
	mLocalCamera.mPos = inv_pivot * mWorldCamera.mPos;
	mLocalCamera.mForward = inv_pivot.Multiply3x3(mWorldCamera.mForward);
	mLocalCamera.mUp = inv_pivot.Multiply3x3(mWorldCamera.mUp);
}

// Update camera position
void Application::UpdateCamera(float inDeltaTime)
{
	// Determine speed
	float speed = GetWorldScale() * mWorldCamera.mFarPlane / 50.0f * inDeltaTime;
	bool shift = mKeyboard->IsKeyPressed(DIK_LSHIFT) || mKeyboard->IsKeyPressed(DIK_RSHIFT);
	bool control = mKeyboard->IsKeyPressed(DIK_LCONTROL) || mKeyboard->IsKeyPressed(DIK_RCONTROL);
	if (shift)				speed *= 10.0f;
	else if (control)		speed /= 25.0f;

	// Position
	Vec3 right = mLocalCamera.mForward.Cross(mLocalCamera.mUp);
	if (mKeyboard->IsKeyPressed(DIK_A))		mLocalCamera.mPos -= speed * right;
	if (mKeyboard->IsKeyPressed(DIK_D))		mLocalCamera.mPos += speed * right;
	if (mKeyboard->IsKeyPressed(DIK_W))		mLocalCamera.mPos += speed * mLocalCamera.mForward;
	if (mKeyboard->IsKeyPressed(DIK_S))		mLocalCamera.mPos -= speed * mLocalCamera.mForward;

	// Forward
	float heading = atan2(mLocalCamera.mForward.GetZ(), mLocalCamera.mForward.GetX());
	float pitch = atan2(mLocalCamera.mForward.GetY(), Vec3(mLocalCamera.mForward.GetX(), 0, mLocalCamera.mForward.GetZ()).Length());
	heading += DegreesToRadians(mMouse->GetDX() * 0.5f);
	pitch = Clamp(pitch - DegreesToRadians(mMouse->GetDY() * 0.5f), -0.49f * F_PI, 0.49f * F_PI);
	mLocalCamera.mForward = Vec3(cos(pitch) * cos(heading), sin(pitch), cos(pitch) * sin(heading));

	// Convert local to world space using the camera pivot
	Mat44 pivot = GetCameraPivot();
	mWorldCamera.mPos = pivot * mLocalCamera.mPos;
	mWorldCamera.mForward = pivot.Multiply3x3(mLocalCamera.mForward);
	mWorldCamera.mUp = pivot.Multiply3x3(mLocalCamera.mUp);
}
