#define ENABLE_CEGUI_CONTEXT
#define ENABLE_CEGUI_CLASS
#define ENABLE_OGRE_CONTEXT
#define ENABLE_OGRE_CLASS

#include "FPSGame.h"
#ifdef ENABLE_CEGUI_CLASS
#include "GUI.h"
#endif // ENABLE_CEGUI_CLASS

// #include <OgreWindowEventUtilities.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <iostream>

#include <cstdlib> // for EXIT_FAILURE and EXIT_SUCCESS

static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;

static int mainBody(int argc, const char *argv[])
{
    // create window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); // TODO explore the usefulness of this option
    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow(
            "FPSGame",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
        ),
        SDL_DestroyWindow
    );
    if (!window)
    {
        fprintf(stderr, "error: SDL_CreateWindow() failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // NOTE: SDL_GLContext is already a pointer, you aren't supposed to use pointers like "SDL_GLContext *", and this is inconsistent with other SDL APIs... we have to remove the pointer aspect to make it play nice with std::unique_ptr<>

    // create an OpenGL context for CEGUI
#ifdef ENABLE_CEGUI_CONTEXT
    std::unique_ptr<std::remove_pointer<SDL_GLContext>::type, decltype(&SDL_GL_DeleteContext)> ceguiContext(SDL_GL_CreateContext(window.get()), SDL_GL_DeleteContext);
    if (!ceguiContext)
    {
        fprintf(stderr, "error: SDL_GL_CreateContext() failed for CEGUI: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
#endif // ENABLE_CEGUI_CONTEXT

    // create an OpenGL context for OGRE
#ifdef ENABLE_OGRE_CONTEXT
    std::unique_ptr<std::remove_pointer<SDL_GLContext>::type, decltype(&SDL_GL_DeleteContext)> ogreContext(SDL_GL_CreateContext(window.get()), SDL_GL_DeleteContext);
    if (!ogreContext)
    {
        fprintf(stderr, "error: SDL_GL_CreateContext() failed for OGRE: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
#endif // ENABLE_OGRE_CONTEXT

#ifdef ENABLE_OGRE_CONTEXT
    SDL_GL_MakeCurrent(window.get(), ogreContext.get());
#endif // ENABLE_OGRE_CONTEXT
#ifdef ENABLE_OGRE_CLASS
    FPSGame game(window.get());
    if (!game.getWindow())
        return EXIT_FAILURE;
#endif // ENABLE_OGRE_CLASS

#ifdef ENABLE_CEGUI_CONTEXT
    SDL_GL_MakeCurrent(window.get(), ceguiContext.get()); // <-- this seems to eventually cause Ogre Next to crash in the game.draw() call!
#endif // ENABLE_CEGUI_CONTEXT
#ifdef ENABLE_CEGUI_CLASS
    GUI gui(window.get());
#endif // ENABLE_CEGUI_CLASS

    Uint64 old_ticks = SDL_GetTicks64();

    bool showingGui = true;
    int guiMouseX = 0, guiMouseY = 0;
    SDL_ShowCursor(SDL_DISABLE); // TODO maybe move this further up?
    while (
#ifdef ENABLE_OGRE_CLASS
        !game.getQuit() &&
#endif // ENABLE_OGRE_CLASS
#ifdef ENABLE_CEGUI_CLASS
        !gui.getQuit() &&
#endif // ENABLE_CEGUI_CLASS
        !SDL_QuitRequested())
    {
        // calculate time delta
        Uint64 new_ticks = SDL_GetTicks64();
        const float seconds_elapsed = static_cast<float>(new_ticks - old_ticks)/1000.0f;
        old_ticks = new_ticks;

        // Ogre::WindowEventUtilities::messagePump();

        // process input / window events
        SDL_Event event;
        while (SDL_PollEvent(&event) &&
#ifdef ENABLE_OGRE_CLASS
            !game.getQuit() &&
#endif // ENABLE_OGRE_CLASS
#ifdef ENABLE_CEGUI_CLASS
            !gui.getQuit() &&
#endif // ENABLE_CEGUI_CLASS
            !SDL_QuitRequested())
        {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB)
            {
                if (showingGui)
                {
                    SDL_GetMouseState(&guiMouseX, &guiMouseY);
                }
                showingGui = !showingGui;
                SDL_SetRelativeMouseMode(!showingGui ? SDL_TRUE : SDL_FALSE);
                if (showingGui)
                {
                    // SDL_ShowCursor(SDL_DISABLE);
                    SDL_WarpMouseInWindow(window.get(), guiMouseX, guiMouseY);
                }
            }
#ifdef ENABLE_CEGUI_CLASS
            else if (showingGui)
            {
                gui.handleEvent(event);
            }
            else
#endif // ENABLE_CEGUI_CLASS
            {
#ifdef ENABLE_OGRE_CLASS
                game.handleEvent(event);
#endif // ENABLE_OGRE_CLASS
            }
        }

#ifdef ENABLE_OGRE_CLASS
        game.advance(seconds_elapsed);
#endif // ENABLE_OGRE_CLASS
#ifdef ENABLE_CEGUI_CLASS
        gui.advance(seconds_elapsed);
#endif // ENABLE_CEGUI_CLASS

        // SDL_GL_MakeCurrent(window.get(), ogreContext.get());
        // glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#ifdef ENABLE_OGRE_CONTEXT
        SDL_GL_MakeCurrent(window.get(), ogreContext.get());
#endif // ENABLE_OGRE_CONTEXT
#ifdef ENABLE_OGRE_CLASS
        game.draw();
#endif // ENABLE_OGRE_CLASS
        if (showingGui)
        {
#ifdef ENABLE_CEGUI_CONTEXT
            SDL_GL_MakeCurrent(window.get(), ceguiContext.get());
#endif // ENABLE_CEGUI_CONTEXT
#ifdef ENABLE_CEGUI_CLASS
            gui.draw();
#endif // ENABLE_CEGUI_CLASS
        }
        SDL_GL_SwapWindow(window.get());
    }

    return EXIT_SUCCESS;
}

int main(int argc, const char *argv[])
{
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "error: SDL_Init() failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // run the rest of the app that relies on SDL
    const int result = mainBody(argc, argv);

    // shut down SDL and exit the application
    SDL_Quit();
    return result;
}
