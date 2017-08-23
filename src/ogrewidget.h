/****************************************************************************
**
** Copyright (C) 2016
**
** This file is generated by the Magus toolkit
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#pragma once

#include <QWidget>
#include "OgreString.h"
#include "OgreColourValue.h"
#include "OgreVector2.h"

#define NOMINMAX
#include <windows.h>


namespace Ogre
{
class Root;
class RenderWindow;
class SceneManager;
class Camera;
}

class MyHlmsListener;


class OgreManager;
class CameraManager;


class OgreWidget : public QWidget
{
	Q_OBJECT

public:
	OgreWidget( QWidget* parent = 0 );
	virtual ~OgreWidget();

    QPaintEngine* paintEngine() const override; // Turn off QTs paint engine for the Ogre widget.

	void updateOgre( float timeSinceLastFrame );
	void createRenderWindow( OgreManager* ogreManager );
	void createCompositor();

	Ogre::Root* getRoot() { return mRoot; }
    int id() { return mId; }
    void setId(int id) { mId = id; }

signals:
	void sceneCreated();

protected:
	virtual void paintEvent( QPaintEvent* );
	virtual void resizeEvent( QResizeEvent* );
	virtual void keyPressEvent( QKeyEvent* );
	virtual void keyReleaseEvent( QKeyEvent* );
	virtual void mouseMoveEvent( QMouseEvent* );

	virtual void wheelEvent( QWheelEvent* );
	virtual void mousePressEvent( QMouseEvent* );
	virtual void mouseReleaseEvent( QMouseEvent* );
	HGLRC getCurrentGlContext();

private:
    OgreManager* mOgreManager = nullptr;

	Ogre::Root* mRoot = nullptr;
	Ogre::RenderWindow* mOgreRenderWindow = nullptr;

	Ogre::Camera* mCamera = nullptr;

	Ogre::ColourValue mBackground;
	Ogre::Real mTimeSinceLastFrame = 0.0;

	CameraManager* mCameraManager = nullptr;
	bool mSceneCreated = false;
	
    Ogre::Vector2 mAbsolute;
	Ogre::Vector2 mRelative;
	
    bool mInitialized = false;
    int mId = 0;
};
