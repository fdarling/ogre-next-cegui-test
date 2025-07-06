#ifndef FPSGAME_H
#define FPSGAME_H

#include <array>
#include <memory>

namespace Ogre {

class Root;
class Window;
class SceneManager;
class Camera;

} // namespace Ogre

// forward declaration to avoid including <SDL.h>
typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

class FPSGame
{
public:
    FPSGame(SDL_Window *sdlWindow);
    ~FPSGame();
    Ogre::Window * getWindow() {return mWindow;}
    void handleEvent(const SDL_Event &event);
    void advance(float seconds_elapsed);
    void draw();
    bool getQuit() const { return mQuit; }
protected:
    void _UpdateMouseCaptured();
    void _UpdateCameraRotation();
    void _CreateScene();
protected:
    std::unique_ptr<Ogre::Root> mRoot;
    SDL_Window *mSDLWindow;
    Ogre::Window *mWindow;
    Ogre::SceneManager *mSceneManager;
    Ogre::Camera *mCamera;
    bool mCaptureMouse;
    bool mQuit;
    float mPitch, mYaw;
    std::array<bool, 4> mWASD;
    std::array<bool, 4> mArrows;
};

#endif // FPSGAME_H
