#include "FPSGame.h"

#include "OgreAbiUtils.h"
#include "OgreArchiveManager.h"
#include "OgreMeshManager.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreEntity.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlit.h"
// #include "OgreHlmsUnlitDatablock.h"

#include "Compositor/OgreCompositorManager2.h"

#include "OgreWindowEventUtilities.h"

#include "SceneLoader.h"

#include <SDL.h>
#include <SDL_syswm.h>

#include <string>
#include <iostream>

#include <unistd.h> // for getuid()
#include <pwd.h> // for getpwuid()

static void registerHlms()
{
    static const Ogre::String resourcePath = "";

    // NOTE: Linux platform specific!
    struct passwd * const pw = getpwuid(getuid());
    static const Ogre::String configFilePath = Ogre::String(pw->pw_dir) + "/apps/ogre-next/share/OGRE-Next/resources2.cfg";

    Ogre::ConfigFile cf;
    cf.load(configFilePath);

    Ogre::String rootHlmsFolder = resourcePath + cf.getSetting("DoNotUseAsResource", "Hlms", "");

    if(rootHlmsFolder.empty())
        rootHlmsFolder = "./";
    else if(*(rootHlmsFolder.end() - 1) != '/')
        rootHlmsFolder += "/";

    // At this point rootHlmsFolder should be a valid path to the Hlms data folder

    Ogre::HlmsUnlit *hlmsUnlit = nullptr;
    Ogre::HlmsPbs *hlmsPbs = nullptr;

    // For retrieval of the paths to the different folders needed
    Ogre::String mainFolderPath;
    Ogre::StringVector libraryFoldersPaths;
    Ogre::StringVector::const_iterator libraryFolderPathIt;
    Ogre::StringVector::const_iterator libraryFolderPathEn;

    Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

    {
        // Create & Register HlmsUnlit
        // Get the path to all the subdirectories used by HlmsUnlit
        Ogre::HlmsUnlit::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
        Ogre::Archive *archiveUnlit = archiveManager.load(rootHlmsFolder + mainFolderPath, "FileSystem", true);
        Ogre::ArchiveVec archiveUnlitLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while(libraryFolderPathIt != libraryFolderPathEn)
        {
            Ogre::Archive *archiveLibrary = archiveManager.load(rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true);
            archiveUnlitLibraryFolders.push_back(archiveLibrary);
            ++libraryFolderPathIt;
        }

        // Create and register the unlit Hlms
        hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &archiveUnlitLibraryFolders);
        Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsUnlit);
    }

    {
        // Create & Register HlmsPbs
        // Do the same for HlmsPbs:
        Ogre::HlmsPbs::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
        Ogre::Archive *archivePbs = archiveManager.load(rootHlmsFolder + mainFolderPath, "FileSystem", true);

        // Get the library archive(s)
        Ogre::ArchiveVec archivePbsLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while(libraryFolderPathIt != libraryFolderPathEn)
        {
            Ogre::Archive *archiveLibrary =
                archiveManager.load(rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true);
            archivePbsLibraryFolders.push_back(archiveLibrary);
            ++libraryFolderPathIt;
        }

        // Create and register
        hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, &archivePbsLibraryFolders);
        Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsPbs);
    }
}

FPSGame::FPSGame(SDL_Window *sdlWindow) :
    mSDLWindow(sdlWindow),
    mWindow(nullptr),
    mSceneManager(nullptr),
    mCamera(nullptr),
    mCaptureMouse(true),
    mQuit(false),
    mPitch(0.0),
    mYaw(0.0),
    mWASD({false, false, false, false}),
    mArrows({false, false, false, false})
{

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(mSDLWindow, &wmInfo);

    // NOTE: Linux platform specific!
    Window x11Window = wmInfo.info.x11.window;
    Display * const x11Display = wmInfo.info.x11.display;
    struct passwd * const pw = getpwuid(getuid());

    static const Ogre::String pluginsFolder = Ogre::String(pw->pw_dir) + "/apps/ogre-next/share/OGRE-Next/";
    static const Ogre::String writeAccessFolder = pluginsFolder;
    static const char pluginsFile[] = "plugins.cfg";

    const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
    mRoot = std::make_unique<Ogre::Root>(&abiCookie, pluginsFolder + pluginsFile,
                                writeAccessFolder + "ogre.cfg",
                                writeAccessFolder + "Ogre.log");

    // TODO handle this gracefully
    if (!mRoot->restoreConfig() && !mRoot->showConfigDialog())
        return;

    // Initialize Root
    mRoot->getRenderSystem()->setConfigOption("sRGB Gamma Conversion", "Yes");
    mRoot->initialise(false, "MyFPSGame");
    // mWindow = mRoot->initialise(true, "MyFPSGame");

    {
        // https://ogrecave.github.io/ogre-next/api/2.3/class_ogre_1_1_render_system.html
        // https://ogrecave.github.io/ogre-next/api/3.0/class_ogre_1_1_root.html
        // https://github.com/OGRECave/ogre-next/blob/3ba612e2511f0919370927edbdde6646ac8351bf/RenderSystems/GL3Plus/src/windowing/GLX/OgreGLXWindow.cpp#L153
        // https://github.com/gazebosim/gz-rendering/blob/414735f8353d3ab8f09b84bf7bfce8c98472b3cb/ogre2/src/Ogre2RenderEngine.cc#L1242C13-L1242C29
        Ogre::NameValuePairList params;
        params["externalWindowHandle"] = std::to_string((unsigned long)(x11Window));
        // params["parentWindowHandle"] = std::to_string((unsigned long)(x11Window)); // optional
        params["externalGLControl"] = std::to_string(true);
        params["currentGLContext"] = std::to_string(true);
        // params["externalGLContext"] = std::to_string((unsigned long)(sdlGLContext)); // TODO use uintptr_t instead?

        int window_width = 0;
        int window_height = 0;
        SDL_GetWindowSize(mSDLWindow, &window_width, &window_height);
        mWindow = mRoot->createRenderWindow("MyOGREWindow", window_width, window_height, false, &params);
    }

    registerHlms();

    // Create SceneManager
    static const size_t numThreads = 1u;
    mSceneManager = mRoot->createSceneManager(Ogre::ST_GENERIC, numThreads, "ExampleSMInstance");
    mSceneManager->setForward3D(true, 4, 4, 3, 128, 3.0f, 2000.0f); // TODO figure out what values to use!
    // mSceneManager->setAmbientLight(Ogre::ColourValue::Black, Ogre::ColourValue::Black, Ogre::Vector3::UNIT_Y);
    // mSceneManager->setLightPowerScale(1.0f); // Default is 1.0, try lowering to 0.01–1.0

    _CreateScene();
    _UpdateMouseCaptured();
}

FPSGame::~FPSGame()
{
}

// TODO look at https://github.com/OGRECave/ogre-next/blob/master/Samples/2.0/Common/src/GraphicsSystem.cpp#L439

void FPSGame::handleEvent(const SDL_Event &event)
{
    if (event.type == SDL_KEYDOWN)
    {
        switch (event.key.keysym.sym)
        {
            case SDLK_ESCAPE:
            mQuit = true;
            break;

            case SDL_KeyCode::SDLK_w: mWASD[0] = true; break;
            case SDL_KeyCode::SDLK_a: mWASD[1] = true; break;
            case SDL_KeyCode::SDLK_s: mWASD[2] = true; break;
            case SDL_KeyCode::SDLK_d: mWASD[3] = true; break;

            case SDL_KeyCode::SDLK_UP:    mArrows[0] = true; break;
            case SDL_KeyCode::SDLK_LEFT:  mArrows[1] = true; break;
            case SDL_KeyCode::SDLK_DOWN:  mArrows[2] = true; break;
            case SDL_KeyCode::SDLK_RIGHT: mArrows[3] = true; break;

            case SDL_KeyCode::SDLK_RETURN:
            {
                if (event.key.keysym.mod & KMOD_ALT)
                {
                    // TODO figure out why this effectively crashes the X server on Linux at least... BadRRCrtc error :-(
                    // const bool is_fullsreen = bool(SDL_GetWindowFlags(mSDLWindow) & SDL_WINDOW_FULLSCREEN);
                    // SDL_SetWindowFullscreen(mSDLWindow, !is_fullsreen ? SDL_WINDOW_FULLSCREEN : 0);
                }
            }
            break;

            case SDL_KeyCode::SDLK_TAB:
            {
                // mCaptureMouse = !mCaptureMouse;
                // _UpdateMouseCaptured();
            }
            break;
        }
    }
    else if (event.type == SDL_KEYUP)
    {
        switch (event.key.keysym.sym)
        {
            case SDL_KeyCode::SDLK_w: mWASD[0] = false; break;
            case SDL_KeyCode::SDLK_a: mWASD[1] = false; break;
            case SDL_KeyCode::SDLK_s: mWASD[2] = false; break;
            case SDL_KeyCode::SDLK_d: mWASD[3] = false; break;

            case SDL_KeyCode::SDLK_UP:    mArrows[0] = false; break;
            case SDL_KeyCode::SDLK_LEFT:  mArrows[1] = false; break;
            case SDL_KeyCode::SDLK_DOWN:  mArrows[2] = false; break;
            case SDL_KeyCode::SDLK_RIGHT: mArrows[3] = false; break;
        }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (!mCaptureMouse)
        {
            mCaptureMouse = true;
            _UpdateMouseCaptured();
        }
    }
    else if (event.type == SDL_MOUSEMOTION)
    {
        if (mCaptureMouse)
        {
            mYaw -= event.motion.xrel * 0.1f;
            mPitch -= event.motion.yrel * 0.1f;
            mPitch = std::clamp(mPitch, -89.0f, 89.0f);
            _UpdateCameraRotation();
        }
    }
    else if (event.type == SDL_QUIT)
    {
        mQuit = true;
    }
    else if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(mSDLWindow))
        {
            mQuit = true;
        }
        else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            // std::cout << "SDL_WINDOWEVENT_FOCUS_LOST" << std::endl;
            mCaptureMouse = false;
            _UpdateMouseCaptured();
        }
        else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
        {
            std::cout << "SDL_WINDOWEVENT_MINIMIZED" << std::endl;
            // mCaptureMouse = false;
            // _UpdateMouseCaptured();
        }
        // else if (event.window.event == SDL_WINDOWEVENT_LEAVE)
        // {
            // mCaptureMouse = false;
            // _UpdateMouseCaptured();
        // }
        // else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
        // else if (event.window.event == SDL_WINDOWEVENT_ENTER)
        // {
            // mCaptureMouse = true;
            // _UpdateMouseCaptured();
        // }
        else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            std::cout << "SDL_WINDOWEVENT_FOCUS_GAINED" << std::endl;
            // mCaptureMouse = true;
            // _UpdateMouseCaptured();
        }
    }
}

void FPSGame::advance(float seconds_elapsed)
{
    // handle walking
    {
        Ogre::Vector3 move(Ogre::Vector3::ZERO);
        if (mWASD[0]) move.z -= 1;
        if (mWASD[1]) move.x -= 1;
        if (mWASD[2]) move.z += 1;
        if (mWASD[3]) move.x += 1;

        if (move != Ogre::Vector3::ZERO)
        {
            move.normalise();
            static const float METERS_PER_SECOND = 25.0;
            const Ogre::Vector3 dir = mCamera->getOrientation() * move;
            mCamera->setPosition(mCamera->getPosition() + dir * METERS_PER_SECOND * seconds_elapsed);
        }
    }

    // handle looking
    {
        static const float DEGREES_PER_SECOND = 50.0f;
        float up_down = 0.0;
        float left_right = 0.0;
        if (mArrows[0])
            up_down -= 1.0;
        if (mArrows[2])
            up_down += 1.0;
        if (mArrows[1])
            left_right -= 1.0;
        if (mArrows[3])
            left_right += 1.0;
        if (up_down != 0.0 || left_right != 0.0)
        {
            mYaw -= left_right * DEGREES_PER_SECOND * seconds_elapsed;
            mPitch -= up_down * DEGREES_PER_SECOND * seconds_elapsed;
            mPitch = std::clamp(mPitch, -89.0f, 89.0f);
            _UpdateCameraRotation();
        }
    }
}

void FPSGame::draw()
{
    // Ogre::WindowEventUtilities::messagePump();
    mQuit |= !mRoot->renderOneFrame();
}

void FPSGame::_UpdateMouseCaptured()
{
    return;
    // setWindowGrab(mCaptureMouse);
    SDL_SetRelativeMouseMode(mCaptureMouse ? SDL_TRUE : SDL_FALSE);
    if (mCaptureMouse)
    {
        // mTrayMgr->hideCursor();
    }
    else
    {
        // SDL_ShowCursor(SDL_DISABLE);
        // mTrayMgr->showCursor();
    }
}

void FPSGame::_UpdateCameraRotation()
{
    const Ogre::Quaternion qPitch(Ogre::Degree(mPitch), Ogre::Vector3::UNIT_X);
    const Ogre::Quaternion qYaw(Ogre::Degree(mYaw), Ogre::Vector3::UNIT_Y);
    mCamera->setOrientation(qYaw * qPitch);
}

void FPSGame::_CreateScene()
{
    // Create & setup camera
    mCamera = mSceneManager->createCamera("Main Camera");
    mCamera->setPosition(Ogre::Vector3(0, 5, 15)); // Position it at 500 in Z direction (? 500 ?)
    // mCamera->lookAt(Ogre::Vector3(0, 0, 0)); // Look back along -Z
    _UpdateCameraRotation();
    mCamera->setNearClipDistance(0.2f);
    mCamera->setFarClipDistance(1000.0f);
    mCamera->setAutoAspectRatio(true);

    // Setup a basic compositor with a blue clear colour
    Ogre::CompositorManager2 * const compositorManager = mRoot->getCompositorManager2();
    static const Ogre::String workspaceName("Demo Workspace");
    static const Ogre::ColourValue backgroundColour(0.2f, 0.4f, 0.6f);
    compositorManager->createBasicWorkspaceDef(workspaceName, backgroundColour, Ogre::IdString());
    compositorManager->addWorkspace(mSceneManager, mWindow->getTexture(), mCamera, workspaceName, true);



#if 0
    // Ogre::HlmsUnlit * const hlmsUnlit = static_cast<Ogre::HlmsUnlit *>(mRoot->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT));
    Ogre::HlmsPbs * const hlmsUnlit = static_cast<Ogre::HlmsPbs *>(mRoot->getHlmsManager()->getHlms(Ogre::HLMS_PBS));

    // Create an unlit material with a solid color
    // Ogre::HlmsUnlitDatablock * const datablock = reinterpret_cast<Ogre::HlmsUnlitDatablock *>(hlmsUnlit->createDatablock(
    Ogre::HlmsPbsDatablock * const datablock = reinterpret_cast<Ogre::HlmsPbsDatablock *>(hlmsUnlit->createDatablock(
        "SolidGreen",
        "SolidGreen",
        Ogre::HlmsMacroblock(),
        Ogre::HlmsBlendblock(),
        Ogre::HlmsParamVec()
    ));

    // datablock->setTexture(0, nullptr); // No texture, solid color
    // datablock->setUseColour(true);
    // datablock->setColour(Ogre::ColourValue(0.2f, 0.8f, 0.2f, 1.0f));
    // datablock->setColour(Ogre::ColourValue(0.9f, 0.8f, 0.2f, 1.0f));
    // datablock->setTexture( Ogre::PBSM_REFLECTION, nullptr );
    datablock->setDiffuse( Ogre::Vector3( 0.0f, 1.0f, 0.0f ) );







    Ogre::Plane plane(Ogre::Vector3::UNIT_Y, 1.0f);
    // Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
        // "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        // Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ),
        // 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
        // Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
        // Ogre::v1::HardwareBuffer::HBU_STATIC );
    Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
        "ground",                      // name
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        plane,                         // plane definition
        50, 50,                    // width, height
        20, 20,                        // x- and y-segments
        true,                          // normals
        1,                             // numTexCoordSets
        4.0f, 4.0f,
        Ogre::Vector3::UNIT_Z         // up vector (used for UVs)
    );

    Ogre::v1::Entity * const groundEntity = mSceneManager->createEntity("ground");
    groundEntity->setDatablock(datablock);
    groundEntity->setCastShadows(false);  // optional
    mSceneManager->getRootSceneNode()->createChildSceneNode()->attachObject(groundEntity);

    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "../data",
        "FileSystem",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        true
    );
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(false); // TODO is this really necessary?




    // Assuming "Cube" is already a mesh (you might need to create your own cube.mesh or use ManualObject)
    Ogre::v1::Entity * const cubeEntity = mSceneManager->createEntity("cube.mesh");
    Ogre::SceneNode * const cubeNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    cubeNode->attachObject(cubeEntity);
    cubeNode->setPosition(0, 10, 0);  // put it above the plane
#endif


#if 0
    Ogre::Light * const light = mSceneManager->createLight();
    Ogre::SceneNode * const lightNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    lightNode->attachObject(light);
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    // light->setType(Ogre::Light::LT_POINT);
    light->setDirection(Ogre::Vector3(-1, -1, -1).normalisedCopy());
    lightNode->setPosition(10, 10, 10);
#endif

    // loadSceneWithAssimp("../data/test_scene.dae", mSceneManager, mSceneManager->getRootSceneNode());
    loadSceneWithAssimp("../data/test_scene.glb", mSceneManager, mSceneManager->getRootSceneNode());
}
