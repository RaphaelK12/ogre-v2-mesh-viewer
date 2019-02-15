/****************************************************************************
**
** Copyright (C) 2016 This file is generated by the Magus toolkit
** Copyright (C) 2016-2019 Matt Chiawen Chang
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE go (ODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "stdafx.h"
#include "ogremanager.h"

#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreTimer.h"
#include "OgreTextureManager.h"

#include "OgreConfigFile.h"
#include "OgreArchiveManager.h"

#include "OgreHlms.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsCommon.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "OgreHlmsTextureManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsPbs.h"
#include "Overlay/OgreOverlaySystem.h"

#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QApplication>

#include "ogrewidget.h"
#include "OgreGLTF/Ogre_glTF.hpp"


OgreManager::OgreManager()
{
    mGlContext = 0;

    QDir exePath(QApplication::applicationDirPath());

    mResourcesCfg = exePath.filePath("resources2.cfg").toStdString();
    mPluginsCfg = exePath.filePath("plugins.cfg").toStdString();

    // Create Ogre and initialize it
    mRoot = new Ogre::Root(mPluginsCfg, "ogre.cfg", "Ogre.log");

    if (!mRoot->restoreConfig())
    {
        if (!mRoot->showConfigDialog())
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Abort render system configuration", "OgreManager::OgreManager");
    }
    
    Ogre_glTF::glTFLoaderPlugin* gltfPlugin = OGRE_NEW Ogre_glTF::glTFLoaderPlugin;
    mRoot->installPlugin(gltfPlugin);

    mCurrentRenderSystem = mRoot->getRenderSystem();
    mRoot->initialise(false);

    // Initialize resources
    setupResources();

    // Start timer
    mTimer = new Ogre::Timer();
    mTimer->reset();
}

OgreManager::~OgreManager()
{
    mLoadedV1Meshes.clear();
    mLoadedV2Meshes.clear();

    delete mRoot; mRoot = nullptr;
    delete mTimer; mTimer = nullptr;

    mCurrentRenderSystem = nullptr;
    mGlContext = 0;
}

void OgreManager::initialize()
{
    // Create scene manager
    const size_t numThreads = std::max<int>(1, Ogre::PlatformInformation::getNumLogicalCores());
    Ogre::InstancingThreadedCullingMethod threadedCullingMethod = (numThreads > 1) ? Ogre::INSTANCING_CULLING_THREADED : Ogre::INSTANCING_CULLING_SINGLETHREAD;
    mSceneManager = mRoot->createSceneManager(Ogre::ST_GENERIC, numThreads, threadedCullingMethod);
    mSceneManager->setShadowDirectionalLightExtrusionDistance(500.0f);
    mSceneManager->setShadowFarDistance(500.0f);

    // Overlay system
    mOverlaySystem = OGRE_NEW Ogre::v1::OverlaySystem;
    mSceneManager->addRenderQueueListener(mOverlaySystem);

    // After resources have been setup and renderwindows created (in ogre widget), the Hlms managers are registered
    registerHlms();

    // Initialise, parse scripts etc
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(false);
}

void OgreManager::setupResources()
{
    QDir appDir(QApplication::applicationDirPath());

    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load(mResourcesCfg);

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap* settings = seci.getNext();

        if (secName == "Hlms") continue;

        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            Ogre::String rescType = i->first;
            Ogre::String rescPath = i->second;

            QString s = appDir.filePath(QString::fromStdString(rescPath));
            qDebug() << s;

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(s.toStdString(), rescType, secName);
        }
    }
}

Ogre::SceneNode* OgreManager::meshRootNode() const
{
    return mMeshRootNode;
}

bool OgreManager::isRenderSystemGL() const
{
    if (mCurrentRenderSystem)
        return (mCurrentRenderSystem->getName() == OGRE_RENDERSYSTEM_OPENGL3PLUS);

    return false;
}

HGLRC OgreManager::getGlContext() const
{
    return mGlContext;
}

void OgreManager::setGlContext(HGLRC glContext)
{
    mGlContext = glContext;
}

void OgreManager::clearScene()
{
    //auto v1Mgr = Ogre::v1::MeshManager::getSingletonPtr();
    //auto v2Mgr = Ogre::MeshManager::getSingletonPtr();
    //qDebug() << "v1 before:" << v1Mgr->getMemoryUsage() << "bytes";
    //qDebug() << "v2 before:" << v2Mgr->getMemoryUsage() << "bytes";

    for (int i = 0; i < mMeshRootNode->numChildren(); ++i)
    {
        Ogre::Node* node = mMeshRootNode->getChild(i);
        qDebug() << node->getName().c_str();

        Ogre::SceneNode* sceneNode = dynamic_cast<Ogre::SceneNode*>(node);

        std::vector<Ogre::MovableObject*> objToReleaseVec;
        for (int k = 0; k < sceneNode->numAttachedObjects(); ++k)
        {
            auto attachedObj = sceneNode->getAttachedObject(k);
            objToReleaseVec.push_back(attachedObj);
        }
        sceneNode->detachAllObjects();

        for (Ogre::MovableObject* movableObj : objToReleaseVec)
        {
            Ogre::Item* item = dynamic_cast<Ogre::Item*>(movableObj);
            mSceneManager->destroyItem(item);
        }
    }

    mMeshRootNode->removeAndDestroyAllChildren();

    for (const Ogre::MeshPtr& v2Mesh : mLoadedV2Meshes)
    {
        Ogre::MeshManager::getSingleton().remove(v2Mesh->getHandle());
    }
    mLoadedV2Meshes.clear();

    for (const Ogre::v1::MeshPtr& v1Mesh : mLoadedV1Meshes)
    {
        Ogre::v1::MeshManager::getSingleton().remove(v1Mesh->getHandle());
    }
    mLoadedV1Meshes.clear();

    //qDebug() << "v1 after:" << v1Mgr->getMemoryUsage() << "bytes";
    //qDebug() << "v2 after:" << v2Mgr->getMemoryUsage() << "bytes";
}

Ogre::Mesh* OgreManager::currentMesh(int index)
{
    if (mLoadedV2Meshes.size() > index)
        return mLoadedV2Meshes[index].get();

    //Q_ASSERT(false);
    return nullptr;
}

void OgreManager::registerHlms()
{
    QDir exePath(QApplication::applicationDirPath());
    exePath.cd("../common/");

    qDebug() << "Hlms Path:" << exePath.absolutePath();

    QString dataFolder = exePath.absolutePath();
    if (!dataFolder.endsWith('/'))
        dataFolder.append("/");

    Ogre::String rootHlmsFolder = dataFolder.toStdString();

    Ogre::HlmsUnlit* hlmsUnlit = nullptr;
    Ogre::HlmsPbs* hlmsPbs = nullptr;

    Ogre::ArchiveManager& archMgr = Ogre::ArchiveManager::getSingleton();
    {
        // Create & Register HlmsUnlit
        // Get the path to all the subdirectories used by HlmsUnlit
        Ogre::String mainFolderPath;
        Ogre::StringVector libraryFoldersPaths;
        Ogre::HlmsUnlit::getDefaultPaths(mainFolderPath, libraryFoldersPaths);

        Ogre::ArchiveVec archiveUnlitLibraryFolders;
        for (const Ogre::String& path : libraryFoldersPaths)
        {
            Ogre::Archive* archive = archMgr.load(rootHlmsFolder + path, "FileSystem", true);
            archiveUnlitLibraryFolders.push_back(archive);
        }

        //Create and register the unlit Hlms
        Ogre::Archive* archiveUnlit = archMgr.load(rootHlmsFolder + mainFolderPath, "FileSystem", true);
        hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &archiveUnlitLibraryFolders);
        Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsUnlit);
    }
    {
        // Create & Register HlmsPbs
        // Do the same for HlmsPbs:
        Ogre::String mainFolderPath;
        Ogre::StringVector libraryFoldersPaths;
        Ogre::HlmsPbs::getDefaultPaths(mainFolderPath, libraryFoldersPaths);

        //Get the library archive(s)
        Ogre::ArchiveVec archivePbsLibraryFolders;
        for (const Ogre::String& path : libraryFoldersPaths)
        {
            Ogre::Archive* archive = archMgr.load(rootHlmsFolder + path, "FileSystem", true);
            archivePbsLibraryFolders.push_back(archive);
        }

        //Create and register
        Ogre::Archive* archivePbs = archMgr.load(rootHlmsFolder + mainFolderPath, "FileSystem", true);
        hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, &archivePbsLibraryFolders);
        Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsPbs);
    }

    Ogre::RenderSystem* renderSystem = mRoot->getRenderSystem();
    if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
    {
        //Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
        //and below to avoid saturating AMD's discard limit (8MB) or
        //saturate the PCIE bus in some low end machines.
        bool supportsNoOverwriteOnTextureBuffers;
        renderSystem->getCustomAttribute("MapNoOverwriteOnDynamicBufferSRV",
                                         &supportsNoOverwriteOnTextureBuffers);

        if (!supportsNoOverwriteOnTextureBuffers)
        {
            hlmsPbs->setTextureBufferDefaultSize(512 * 1024);
            hlmsUnlit->setTextureBufferDefaultSize(512 * 1024);
        }
    }
}

void OgreManager::render()
{
    if (mRoot && !mOgreWidgets.empty())
    {
        // Determine time since last frame
        Ogre::Real timeSinceLastFrame = 0.0f;
        unsigned long startTime = 0;

        // Render an one frame
        startTime = mTimer->getMillisecondsCPU();
        mRoot->renderOneFrame();
        timeSinceLastFrame = (mTimer->getMillisecondsCPU() - startTime) / 1000.0f;

        // Update all QOgreWidgets
        for (OgreWidget* widget : mOgreWidgets)
        {
            widget->updateOgre(timeSinceLastFrame);
        }
    }
}

void OgreManager::createScene()
{
    /*
    for (int i = 0; i <= 5; ++i)
    {
        for (int j = 0; j <= 5; ++j)
        {
            createBall( i, j );
        }
    }
    //qDebug() << QDir::currentPath();
    */

    auto hlmsManager = mRoot->getHlmsManager();
    Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));
    hlmsPbs->setShadowSettings(Ogre::HlmsPbs::PCF_4x4);

    mSceneManager->setForwardClustered(true,
                                       16, 8,    // wight & height 
                                       24,       // num of slices
                                       96, 5,    // light per cell, decal per cell
                                       5, 1000); // min & max distance

    Q_ASSERT(mMeshRootNode == nullptr);
    mMeshRootNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    mMeshRootNode->setName("Root");

    emit sceneCreated();
}

int OgreManager::registerOgreWidget(OgreWidget* ogreWidget)
{
    int nextId = mIdCount;
    mIdCount += 1;

    ogreWidget->setId(nextId);
    mOgreWidgets.push_back(ogreWidget);

    return nextId;
}

void OgreManager::unregisterOgreWidget(int id)
{
    for (int i = 0; i < mOgreWidgets.size(); ++i)
    {
        if (mOgreWidgets[i]->id() == id)
        {
            // no need to release widget, just remove from list
            mOgreWidgets[i] = mOgreWidgets.back();
            mOgreWidgets.pop_back();
        }
    }
}

void OgreManager::createBall(int x, int y)
{
    Ogre::Item* item = mSceneManager->createItem("Sphere1000.mesh",
                                                 Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::SCENE_DYNAMIC);
    Ogre::HlmsManager* hlmsManager = mRoot->getHlmsManager();

    assert(dynamic_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS)));

    Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

    char buff[128];
    sprintf(buff, "PBS_%d_%d", x, y);
    std::string strBlockName(buff);
    Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(
        hlmsPbs->createDatablock(strBlockName, strBlockName,
                                 Ogre::HlmsMacroblock(),
                                 Ogre::HlmsBlendblock(),
                                 Ogre::HlmsParamVec()));

    //auto tex = Ogre::TextureManager::getSingleton().getByName( "wood.png", "OgreSpooky" );
    auto skybox = Ogre::TextureManager::getSingleton().getByName("env.dds", "OgreSpooky");

    datablock->setWorkflow(Ogre::HlmsPbsDatablock::MetallicWorkflow);
    //datablock->setTexture( Ogre::PBSM_DIFFUSE, 0, tex );
    datablock->setTexture(Ogre::PBSM_REFLECTION, 0, skybox);
    datablock->setDiffuse(Ogre::Vector3(1.0f, 1.0f, 1.0f));
    datablock->setSpecular(Ogre::Vector3(1.0f, 1.0f, 1.0f));
    datablock->setRoughness(x / 4.0f);
    datablock->setMetalness(y / 4.0f);

    item->setDatablock(datablock);

    Ogre::SceneNode* sceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    sceneNode->setScale(30, 30, 30);
    sceneNode->setPosition(x * 40 - 100, y * 40 - 100, 0);
    sceneNode->attachObject(item);
}
