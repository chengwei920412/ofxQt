#include "ofAppQtWindow.h"

//----------------------------------------------------------
ofAppQtWindow::ofAppQtWindow(QWidget *parent){
	ofLogVerbose() << "ofAppQtWindow Ctor";

	bShouldClose = false;
	bIsClosed = false;

	bEnableSetupScreen = true;
	buttonPressed = false;
	bWindowNeedsShowing = true;
	buttonInUse = 0;
	iconSet = false;
	orientation = OF_ORIENTATION_DEFAULT;
	windowMode = OF_WINDOW;
	pixelScreenCoordScale = 1;
	nFramesSinceWindowResized = 0;
	windowW = 0;
	windowH = 0;
	currentW = 0;
	currentH = 0;

	hasQtApp = false;
	bIsWindow = false;
	bSetupSucceded = false;

	if (parent == 0 ||
		parent == NULL ||
		parent == nullptr) {
		parentWidget = nullptr;
		bHasParent = false;
	}
	else {
		parentWidget = parent;
		bHasParent = true;
	}
	ofAppPtr = nullptr;
	qtWidgetPtr = nullptr;
}

//----------------------------------------------------------
ofAppQtWindow::~ofAppQtWindow() {
	ofLogVerbose() << "ofAppQtWindow Dtor";
	bIsClosed = true;
}

void ofAppQtWindow::close()
{
	ofLogVerbose() << "close";
//	qtWidgetPtr->makeCurrent();
//	events().disable();
	bWindowNeedsShowing = true;
}

void ofAppQtWindow::createQtApp()
{
	int argc = 1;
	char *argv = "openframeworks";
	char **vptr = &argv;
	qtAppPtr = new QApplication(argc, vptr);
	hasQtApp = true;
}

void ofAppQtWindow::setQtAppPointer(QApplication * qtApp)
{
	qtAppPtr = qtApp;
	hasQtApp = true;
}

void ofAppQtWindow::setIsWindow(bool value)
{
	bIsWindow = value;

	if (qtWidgetPtr == 0 || qtWidgetPtr == nullptr) return;

	qtWidgetPtr->setAttribute(Qt::WA_AlwaysStackOnTop, value); // very important. fixes transparency bug
}

void ofAppQtWindow::paint()
{
//	ofLogVerbose() << "ofAppQtWindow paint";
	if (this == nullptr) {
		ofLogError() << "ofAppQtWindow has not been initialized!";
		return;
	} 
	if (bIsClosed) return;

	// draw or close
	if (getWindowShouldClose()) {
		close();
	}
	else {
		update();
		draw();
	}
}

//------------------------------------------------------------
#ifdef TARGET_OPENGLES
void ofAppGLFWWindow::setup(const ofGLESWindowSettings & settings) {
#else
void ofAppQtWindow::setup(const ofGLWindowSettings & settings) {
#endif
	const ofQtGLWindowSettings * glSettings = dynamic_cast<const ofQtGLWindowSettings*>(&settings);
	if (glSettings) {
		setup(*glSettings);
	}
	else {
		setup(ofQtGLWindowSettings(settings));
	}
}

//------------------------------------------------------------
void ofAppQtWindow::setup(const ofQtGLWindowSettings & _settings) {
	ofLogVerbose() << "setup ofAppQtWindow";

	if (qtWidgetPtr) {
		ofLogError() << "window already setup, probably you are mixing old and new style setup";
		ofLogError() << "call only ofCreateWindow(settings) or ofSetupOpenGL(...)";
		ofLogError() << "calling window->setup() after ofCreateWindow() is not necesary and won't do anything";
		return;
	}
	settings = _settings;


	//////////////////////////////////////
	// setup OpenGL
	//////////////////////////////////////
	QSurfaceFormat format;
	format.setVersion(settings.glVersionMajor, settings.glVersionMinor);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setAlphaBufferSize(settings.alphaBits);
	format.setDepthBufferSize(settings.depthBits);
	format.setStencilBufferSize(settings.stencilBits);
	format.setStereo(settings.stereo);
	format.setSamples(settings.numSamples);
	if (settings.doubleBuffering) {
		format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	}
	else {
		format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
	}
	QSurfaceFormat::setDefaultFormat(format);

	//////////////////////////////////////
	// create renderer
	//////////////////////////////////////
	if (settings.glVersionMajor >= 3) {
		currentRenderer = shared_ptr<ofBaseRenderer>(new ofGLProgrammableRenderer(this));
	}
	else {
		currentRenderer = shared_ptr<ofBaseRenderer>(new ofGLRenderer(this));
	}

	//////////////////////////////////////
	// create Qt window
	//////////////////////////////////////
	qtWidgetPtr = new QtGLWidget(*this, parentWidget);
	setIsWindow(bIsWindow);

	qtWidgetPtr->resize(settings.getWidth(), settings.getHeight());
//	qtWidgetPtr->setFormat(format);
	qtWidgetPtr->setWindowTitle(settings.title);
	//	currentW = qtWidgetPtr->size().width();
	//	currentH = qtWidgetPtr->size().height();
	windowW = settings.getWidth();
	windowH = settings.getHeight();
	bWindowNeedsShowing = settings.visible;
//	qtWidgetPtr->setAlphabits(settings.alphaBits);
//	qtWidgetPtr->setNumSamples(settings.numSamples);

	qtWidgetPtr->makeCurrent();
	qtWidgetPtr->show();

//	int framebufferW, framebufferH;
//	glfwGetFramebufferSize(qtWidgetPtr, &framebufferW, &framebufferH);

	//this lets us detect if the window is running in a retina mode
	//if (framebufferW != windowW) {
	//	pixelScreenCoordScale = framebufferW / windowW;

	//	auto position = getWindowPosition();
	//	setWindowShape(windowW, windowH);
	//	setWindowPosition(position.x, position.y);
	//}


#ifndef TARGET_OPENGLES
	static bool inited = false;
	if (!inited) {
		glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			/* Problem: glewInit failed, something is seriously wrong. */
			ofLogError("ofAppRunner") << "couldn't init GLEW: " << glewGetErrorString(err);
			return;
		}
		inited = true;
	}
#endif

//	ofLogVerbose() << "GL Version:" << glGetString(GL_VERSION);

	//////////////////////////////////////
	// setup renderer
	//////////////////////////////////////
	if (currentRenderer->getType() == ofGLProgrammableRenderer::TYPE) {
#ifndef TARGET_OPENGLES
		static_cast<ofGLProgrammableRenderer*>(currentRenderer.get())->setup(settings.glVersionMajor, settings.glVersionMinor);
#else
		static_cast<ofGLProgrammableRenderer*>(currentRenderer.get())->setup(settings.glesVersion, 0);
#endif
	}
	else {
		static_cast<ofGLRenderer*>(currentRenderer.get())->setup();
	}

	//events().notifySetup();

	//////////////////////////////////////
	// notes
	//////////////////////////////////////
	// this call goes to an endless loop 
	// which causes no OF calls
//	qtAppPtr->exec();
	// we will use 
//	qtAppPtr->processEvents();
	// so that we can call qt inside the OF loop

	bSetupSucceded = true;
}
//------------------------------------------------------------
void ofAppQtWindow::update() {
///	ofLogVerbose() << "update ofAppQtWindow";

	// this line is very important. 
	// it forces the of loop to draw FBOs in the correct window.
	ofGetMainLoop()->setCurrentWindow(this);

	//////////////////////////////////////
	// process oF events
	//////////////////////////////////////
	qtWidgetPtr->makeCurrent();
	events().notifyUpdate();
	//////////////////////////////////////
	// process Qt events
	//////////////////////////////////////
	qtWidgetPtr->makeCurrent();
	qtWidgetPtr->update();
	//////////////////////////////////////

	//show the window right before the first draw call.
	if (bWindowNeedsShowing && qtWidgetPtr) {

		// GLFW update was here
		bWindowNeedsShowing = false;
		if (settings.windowMode == OF_FULLSCREEN) {
			setFullscreen(true);
		}
	}
}
//------------------------------------------------------------
void ofAppQtWindow::draw() {
///	ofLogVerbose() << "draw ofAppQtWindow";
	currentRenderer->startRender();

	if (bEnableSetupScreen) currentRenderer->setupScreen();

	events().notifyDraw();

#ifdef TARGET_WIN32
	if (currentRenderer->getBackgroundAuto() == false) {
		// on a PC resizing a window with this method of accumulation (essentially single buffering)
		// is BAD, so we clear on resize events.
		if (nFramesSinceWindowResized < 3) {
			currentRenderer->clear();
		}
		else {
			if ((events().getFrameNum() < 3 || nFramesSinceWindowResized < 3) && settings.doubleBuffering) {

				// needed if we want events from Of to Qt
				// currently crashes on closing window
				// it slows down framerate quite a lot!
				if (hasQtApp) {
					qtAppPtr->processEvents();
				}
			}
			else {
				glFlush();
			}
		}
	}
	else {
		if (settings.doubleBuffering) {

			// needed if we want events from Of to Qt
			// currently crashes on closing window
			// it slows down framerate quite a lot!
			if (hasQtApp) {
				qtAppPtr->processEvents();
			}
		}
		else {
			glFlush();
		}
	}
#else
	if (currentRenderer->getBackgroundAuto() == false) {
		// in accum mode resizing a window is BAD, so we clear on resize events.
		if (nFramesSinceWindowResized < 3) {
			currentRenderer->clear();
		}
	}
	if (settings.doubleBuffering) {
		glfwSwapBuffers(windowP);
	}
	else {
		glFlush();
	}
#endif
	

	currentRenderer->finishRender();

	nFramesSinceWindowResized++;
}

void ofAppQtWindow::exit()
{
	ofLogVerbose() << "ofAppQtWindow exit";
	if (bIsClosed) return;
	
	// this line is very important. 
	// if we create an ofAppQtWindow from main (e.g. example 3)
	// and then place an ofRunApp(theWindow, theApp);
	// when we close the window clicking on the top right close button
	// then the QtGLWidget will call destructor and call this exit function.
	// in order to tell the main ofLoop to close this renderer 
	// we must set this bShouldClose flag to true
	setWindowShouldClose(true);

	events().notifyExit();
	// wont work:
//	ofGetMainLoop()->removeWindow(this);
// this has to be a shared pointer
	// we have to remove the window from derived class
	// and this is even worst:
//	ofGetMainLoop()->removeWindow(std::shared_ptr<ofAppQtWindow>(this));
}

//------------------------------------------------------------
ofCoreEvents & ofAppQtWindow::events() {
	return coreEvents;
}

//------------------------------------------------------------
shared_ptr<ofBaseRenderer> & ofAppQtWindow::renderer() {
	return currentRenderer;
}

QWidget * ofAppQtWindow::getQWidgetPtr()
{
	return qtWidgetPtr;
}

//------------------------------------------------------------
void ofAppQtWindow::setAppPtr(shared_ptr<ofBaseApp> appPtr){
	ofAppPtr = appPtr;
}

//------------------------------------------------------------
void ofAppQtWindow::setStatusMessage(string s) {
}

//------------------------------------------------------------
void ofAppQtWindow::exitApp() {
	ofLog(OF_LOG_VERBOSE, "QT OF app is being terminated!");
	OF_EXIT_APP(0);
}

////------------------------------------------------------------
//float ofAppQtWindow::getFrameRate() {
//	return qtWidgetPtr->getGlFrameRate();
//}

//------------------------------------------------------------
void ofAppQtWindow::setWindowTitle(string title) {
	settings.title = title;
	qtWidgetPtr->setWindowTitle(title);
}

//------------------------------------------------------------
glm::vec2 ofAppQtWindow::getWindowSize() {
	int width = qtWidgetPtr->width();
	int height = qtWidgetPtr->height();

	return glm::vec2(width, height);
}

//------------------------------------------------------------
glm::vec2 ofAppQtWindow::getWindowPosition() {
	int x = qtWidgetPtr->pos().x();
	int y = qtWidgetPtr->pos().y();

//	cout << "getWindowPosition "<< x <<" "<< y << endl;
	return glm::vec2{ x, y };
}

//------------------------------------------------------------
glm::vec2 ofAppQtWindow::getScreenSize() {
	int width = qtWidgetPtr->size().width();
	int height = qtWidgetPtr->size().height();

//	cout << "getScreenSize " << width << " " << height << endl;
	return glm::vec2{ width, height };
}

//------------------------------------------------------------
void ofAppQtWindow::setWindowPosition(int x, int y) {
//	cout << "setWindowPosition " << x << " " << y << endl;
	qtWidgetPtr->move(QPoint{ x, y });
}

//------------------------------------------------------------
void ofAppQtWindow::setWindowShape(int w, int h) {

	if (windowMode == OF_WINDOW) {
		windowW = w;
		windowH = h;
	}
	currentW = w / pixelScreenCoordScale;
	currentH = h / pixelScreenCoordScale;

#ifdef TARGET_OSX
	auto pos = getWindowPosition();
	windowP->resize(currentW, currentH);
	if (pos != getWindowPosition()) {
		setWindowPosition(pos.x, pos.y);
	}
#else
//	cout << "setWindowShape " << currentW << " " << currentH << endl;
	qtWidgetPtr->resize(currentW, currentH);
#endif
}

//------------------------------------------------------------
void ofAppQtWindow::hideCursor() {
	qtWidgetPtr->unsetCursor();
}

//------------------------------------------------------------
void ofAppQtWindow::showCursor() {
	showCursor();
}

//------------------------------------------------------------
int ofAppQtWindow::getWidth() {
	if (orientation == OF_ORIENTATION_DEFAULT || orientation == OF_ORIENTATION_180) {
		return currentW * pixelScreenCoordScale;
	}
	else {
		return currentH * pixelScreenCoordScale;
	}
}

//------------------------------------------------------------
int ofAppQtWindow::getHeight() {
	if (orientation == OF_ORIENTATION_DEFAULT || orientation == OF_ORIENTATION_180) {
		return currentH * pixelScreenCoordScale;
	}
	else {
		return currentW * pixelScreenCoordScale;
	}
}

//------------------------------------------------------------
ofWindowMode ofAppQtWindow::getWindowMode() {
	return windowMode;
}

//------------------------------------------------------------
void ofAppQtWindow::enableSetupScreen() {
	bEnableSetupScreen = true;
}

//------------------------------------------------------------
void ofAppQtWindow::disableSetupScreen() {
	bEnableSetupScreen = false;
}

void ofAppQtWindow::makeCurrent()
{
	// used
	qtWidgetPtr->makeCurrent();
}

void ofAppQtWindow::swapBuffers()
{
	//unused
//	qtWidgetPtr->swapBuffers();
}

void ofAppQtWindow::startRender()
{
	//unused
//	renderer()->startRender();
}

void ofAppQtWindow::finishRender()
{
	//unused
//	renderer()->finishRender();
}
