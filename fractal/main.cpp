#define WINDOW_TITLE "Blackhole"
#define WINDOW_CLASS_NAME "BLACKHOLE_SIM_WINDOW"
#include "windowSetup.h"

#include "logging/debugOutput.h"

#include "Renderer.h"
#include "Shader.h"
#include "DefaultShader.h"
#include "Camera.h"

#define FOV_VALUE 120

#define LOOK_SENSITIVITY_X 0.01f
#define LOOK_SENSITIVITY_Y 0.01f

#define MOVE_SENSITIVITY 1

namespace keys {
	bool w = false;
	bool a = false;
	bool s = false;
	bool d = false;
	bool space = false;
	bool ctrl = false;
}

unsigned int halfWindowWidth;
unsigned int halfWindowHeight;
int absHalfWindowWidth;
int absHalfWindowHeight;

bool captureKeyboard = false;
bool captureMouse = false;

bool pendingMouseMove = false;
int mouseMoveX = 0;
int mouseMoveY = 0;
int cachedMouseMoveX = 0;
int cachedMouseMoveY = 0;

LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_MOUSEMOVE:
		if (captureMouse) {
			cachedMouseMoveX += LOWORD(lParam) - halfWindowWidth;
			cachedMouseMoveY += HIWORD(lParam) - halfWindowHeight;
			if (!pendingMouseMove) {
				mouseMoveX = cachedMouseMoveX;
				mouseMoveY = cachedMouseMoveY;
				pendingMouseMove = true;
				cachedMouseMoveX = 0;
				cachedMouseMoveY = 0;
				SetCursorPos(absHalfWindowWidth, absHalfWindowHeight);		// Set cursor back to middle of window.
				return 0;
			}
		}
		return 0;
	case WM_KEYDOWN:
		if (captureKeyboard) {
			switch (wParam) {
			case (WPARAM)KeyboardKeys::w: keys::w = true; return 0;
			case (WPARAM)KeyboardKeys::a: keys::a = true; return 0;
			case (WPARAM)KeyboardKeys::s: keys::s = true; return 0;
			case (WPARAM)KeyboardKeys::d: keys::d = true; return 0;
			case (WPARAM)KeyboardKeys::space: keys::space = true; return 0;
			case (WPARAM)KeyboardKeys::ctrl: keys::ctrl = true; return 0;
			case (WPARAM)KeyboardKeys::escape: captureMouse = !captureMouse; return 0;
			}
		}
		return 0;
	case WM_KEYUP:
		if (captureKeyboard) {
			switch (wParam) {
			case (WPARAM)KeyboardKeys::w: keys::w = false; return 0;
			case (WPARAM)KeyboardKeys::a: keys::a = false; return 0;
			case (WPARAM)KeyboardKeys::s: keys::s = false; return 0;
			case (WPARAM)KeyboardKeys::d: keys::d = false; return 0;
			case (WPARAM)KeyboardKeys::space: keys::space = false; return 0;
			case (WPARAM)KeyboardKeys::ctrl: keys::ctrl = false; return 0;
			}
		}
		return 0;
	default:
		if (listenForBoundsChange(uMsg, wParam, lParam)) { return 0; }
		if (listenForExitAttempts(uMsg, wParam, lParam)) { return 0; }
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int windowWidth;
int windowHeight;

int newWindowX;
int newWindowY;
int newWindowWidth;
int newWindowHeight;
bool windowResized = false;

void setWindowSize(unsigned int newWindowWidth, unsigned int newWindowHeight) {
	::newWindowWidth = newWindowWidth;
	::newWindowHeight = newWindowHeight;
	windowResized = true;
	halfWindowWidth = newWindowWidth / 2;
	halfWindowHeight = newWindowHeight / 2;

	// Calculate screen coords of middle of window for cursor repositioning.
	absHalfWindowWidth = newWindowX + halfWindowWidth;
	absHalfWindowHeight = newWindowY + halfWindowHeight;
}

void setWindowPos(int newWindowX, int newWindowY) {
	::newWindowX = newWindowX;
	::newWindowY = newWindowY;

	// Calculate screen coords of middle of window for cursor repositioning.
	absHalfWindowWidth = newWindowX + halfWindowWidth;
	absHalfWindowHeight = newWindowY + halfWindowHeight;
}

void setWindow(int newWindowX, int newWindowY, unsigned int newWindowWidth, unsigned int newWindowHeight) {				// This gets triggered once if the first action of you do is to move the window, for the rest of the moves, it doesn't get triggered.
	setWindowSize(newWindowWidth, newWindowHeight);																		// This is practically unavoidable without a little much effort. It's not really bad as long as it's just one time, so I'm going to leave it.
	setWindowPos(newWindowX, newWindowY);
}

void updateWindowSizeVars() {
	windowWidth = newWindowWidth;
	windowHeight = newWindowHeight;
}

#define EXIT_FROM_THREAD POST_THREAD_EXIT; goto releaseAndReturn;

/*
* 
* SIDE-NOTE: Exception handling is pretty complicated, but the basics of
* what you should know are this:
*	- C++ exceptions on Windows are based on SEH exceptions.
*	- SEH exceptions predate C++ exceptions and are facilitated through
*		Windows syscalls.
*		- This is important: SEH exceptions have nothing to do with the compiler or the code generation or anything,
*			they're just syscalls and the assorted structures and such. So basically you don't have to have any sort
*			of exceptions turned on to write and use SEH exceptions.
*			
*			- They are kind of more low-tech than the exceptions that you're used to, they don't natively support
*				try/except behaviour, since they don't do any stack unwinding or anything.
*			- Basically, what you can do with them is have a linked list of exception handlers that get traversed
*				by the OS (because it goes from handlers corresponding to inner-most scope to handlers for outer scopes
				(like you would for try/except)). OS finds the first exception handler that handles the exception
				and calls it. That handler is your code, so you can do whatever you want.
				The problem is you normally only have two options, either continue the search after doing whatever it is you
				did (that tells OS that the exception isn't handled and it needs to keep going up through handlers) or
				restart execution at the source of the exception (before the instruction that caused the exception).

		- Windows also has compiler extentions for C and C++ that put in the keywords __try, __except, __finally.
			- This actually DOES do stack unwinding via code that is generated on top of the basic win32 API stuff.
			- Here, things basically work as you'd expect since stack unwinding and such is present.
			- How do they implement it? It's actually kind of beautiful, but also complicated, so I'm not going to explain
				all too much here. Basically, instead of telling the OS to either continue search or restart
				(which is handled through the return value of the handler function), the handler function just never returns.
				The compiler generated code in the SEH exception handler unwinds the stack like it should, and then just
				jumps into the code for the except block straight from the handler. I don't remember the exact
				specifics of how this works myself, but AFAIK, the except block then just jumps to the normal code that comes
				after it when it's finished, like an except block would behave normally.
				What this means is that you've sort of made your whole program part of your handler.
				You might think this could cause stack overflow since it's basically recursion, but it doesn't since
				we unwind the stack before we do this, making it all work out.
				Because the stack is unwinded and the stack frame of the handler itself it gone as well, you can't really
				count yourself as in the handler anymore either, but it's sort of fun to think about it that way.


	- C++ exceptions are an abstraction on top of the SEH exceptions, since C++ exceptions are cross-platform.
		This obviously means that the implementation of C++ exceptions changes from platform to platform, only in Windows
		can they be handled through SEH.
		In that case though, they are pretty simple, they basically just do the same stack unwinding stuff that the
		language extentions above did, but with standardized keywords. The other difference is that they
		automatically put in try/finally blocks whenever you initialize local variables, since those definitely need to
		get destructed, even if an exception is thrown. This actually also gets done with the SEH language extentions,
		at least for me in visual studio.


	IMPORTANT: For a lot of new architectures, turning on exceptions has no cost until you put in exception handling code or
		until an exception is thrown. THIS ISN'T TRUE FOR x86-32. Here, every function needs to do some initialization at
		the top of it's code block and there is still overhead, even if you don't actually use exceptions. Since we never use
		exceptions, I've turned them off for this project, even if the overhead is relatively small.

	IMPORTANT: Does the above note mean that system exceptions such as div by zero, nullptr dereference, stack overflow, etc...
	won't reach us anymore? Won't that be bad for debugging, since we won't know why our program failed?
	No, everything will still work fine, the reason is this:
		- Exceptions such as those generate SEH exceptions, just like any other exception on Windows, but just because we
		don't handle them with compiler generated code doesn't mean they are not there. You can't turn them off, they are part
		of the OS.
		- The OS will look for handlers, but since we haven't registered any, the exception won't get handled. Our program will
		crash.
		- UNLESS a debugger is attached. A debugger can register it's own handler and use it to extract data about the exception,
		which it can then show us. This technically has the overhead of exceptions, but that isn't a problem since it happens
		once at the end of our programs lifetime. Considering that, it's actually a splendid use of the system.
* 
*/

void graphicsLoop() {
	updateWindowSizeVars();
	windowResized = false;

	debuglogger::out << alignof(uint64_t) << '\n';

	DefaultShader mainShader;
	ErrorCode err = Renderer::init(&mainShader, 2, windowWidth, windowHeight, ImageChannelOrderType::RGBA);
	debuglogger::out << (int16_t)err << '\n';
	Camera camera({ 0, 0, 0 }, { 0, 0, 0 }, 90);
	Renderer::loadCamera(camera);
	Renderer::transferCameraPosition();
	Renderer::transferCameraRotation();
	Renderer::transferCameraFOV();

	Scene mainScene(50, 5);
	/*for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int x = 0; x < 4; x++) {
				Entity entity;
				entity.position = nmath::Vector3f(i * 40, j * 40, x * 40);
				entity.scale = nmath::Vector3f(10, 0, 0);
				mainScene.entityHeap[i * 16 + j * 4 + x] = entity;
			}
		}
	}*/
	for (int i = 0; i < mainScene.entityHeapLength; i++) {
				Entity entity;
				entity.position = nmath::Vector3f(rand() % 1000, 0, rand() % 1000);
				entity.scale = nmath::Vector3f(10, 0, 0);
				mainScene.entityHeap[i] = entity;
	}
	for (int i = 0; i < 5; i++) {
		Light light;
		light.position = nmath::Vector3f(rand() % 100, 0, rand() % 100);
		light.color = nmath::Vector3f(1, 1, 1);
		mainScene.lightHeap[i] = light;
	}
	mainScene.entityHeap[1].position = nmath::Vector3f(500, -1000, 500);
	mainScene.entityHeap[1].scale = nmath::Vector3f(1000, 0, 0);

	mainScene.generateKDTree();

	Renderer::loadScene(std::move(mainScene));

	ResourceHeap resources(0, 1);
	Renderer::loadResources(std::move(resources));

	err = Renderer::transferResources();
	debuglogger::out << "tran res err: " << (int16_t)err << '\n';
	err = Renderer::transferScene();
	debuglogger::out << "tran scene err: " << (int16_t)err << '\n';





	HDC finalG = GetDC(hWnd);
	HBITMAP bmp = CreateCompatibleBitmap(finalG, windowWidth, windowHeight);
	size_t outputFrame_size = windowWidth * windowHeight * 4;
	HDC g = CreateCompatibleDC(finalG);
	HBITMAP defaultBmp = (HBITMAP)SelectObject(g, bmp);

	captureMouse = true;
	captureKeyboard = true;

	while (isAlive) {

		err = Renderer::render();
		if (err != ErrorCode::SUCCESS) {
			debuglogger::out << (int16_t)err << '\n';
		}

		if (!SetBitmapBits(bmp, outputFrame_size, Renderer::frame)) {			// TODO: Replace this copy (which is unnecessary), with a direct access to the bitmap bits.
			debuglogger::out << debuglogger::error << "failed to set bmp bits\n";
			EXIT_FROM_THREAD;
		}
		if (!BitBlt(finalG, 0, 0, windowWidth, windowHeight, g, 0, 0, SRCCOPY)) {
			debuglogger::out << debuglogger::error << "failed to copy g into finalG\n";
			EXIT_FROM_THREAD;
		}

		if (windowResized) {
			windowResized = false;			// Doing this at beginning leaves space for size event handler to set it to true again while we're recallibrating, which minimizes the chance that the window gets stuck with a drawing surface that doesn't match it's size.
			updateWindowSizeVars();			// NOTE: The chance that something goes wrong with the above is astronomically low and basically zero because size events get fired after resizing is done and user can't start and stop another size move fast enough to trip us up.

			err = Renderer::resizeFrame(windowWidth, windowHeight);
			debuglogger::out << (int16_t)err << '\n';

			// Resize GDI stuff.
			SelectObject(g, defaultBmp);			// Deselect our bmp by replacing it with the defaultBmp that we got from above.
			DeleteObject(bmp);
			bmp = CreateCompatibleBitmap(finalG, windowWidth, windowHeight);
			outputFrame_size = windowWidth * windowHeight * 4;
			SelectObject(g, bmp);

			continue;
		}

		if (pendingMouseMove) {
			Renderer::camera.rotation.x += mouseMoveX * LOOK_SENSITIVITY_X;
			Renderer::camera.rotation.y -= mouseMoveY * LOOK_SENSITIVITY_Y;
			Renderer::transferCameraRotation();
			pendingMouseMove = false;
		}

		nmath::Vector3f moveVector = { };
		if (keys::w) { moveVector.z -= MOVE_SENSITIVITY; }
		if (keys::a) { moveVector.x -= MOVE_SENSITIVITY; }
		if (keys::s) { moveVector.z += MOVE_SENSITIVITY; }
		if (keys::d) { moveVector.x += MOVE_SENSITIVITY; }
		if (keys::space) { moveVector.y += MOVE_SENSITIVITY; }
		if (keys::ctrl) { moveVector.y -= MOVE_SENSITIVITY; }
		Renderer::camera.position += moveVector.rotate(Renderer::camera.rotation);
		Renderer::transferCameraPosition();
		//camera.move(moveVector);			// TODO: Is there really a reason to use custom vector rotation code when you can just pipe the vec through cameraRotMat? Do that.

		//if (!renderer.loadCameraPos(&camera.pos)) { debuglogger::out << debuglogger::error << "failed to load new camera position" << debuglogger::endl; EXIT_FROM_THREAD; }
		//cameraRotMat = Matrix4f::createRotationMat(camera.rot);
		//if (!renderer.loadCameraRotMat(&cameraRotMat)) { debuglogger::out << debuglogger::error << "failed to load new camera rotation" << debuglogger::endl; EXIT_FROM_THREAD; }*/
	}

	releaseAndReturn:
		if (!DeleteDC(g)) { debuglogger::out << debuglogger::error << "failed to delete memory DC (g)\n"; }
		if (!DeleteObject(bmp)) { debuglogger::out << debuglogger::error << "failed to delete bmp\n"; }	// This needs to be deleted after it is no longer selected by any DC.
		if (!ReleaseDC(hWnd, finalG)) { debuglogger::out << debuglogger::error << "failed to release window DC (finalG)\n"; }

		Renderer::release();

		// TODO: If this were perfect, this thread would exit with EXIT_FAILURE as well as the main thread if something bad happened, it doesn't yet.
}