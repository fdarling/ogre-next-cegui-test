#ifndef GUI_H
#define GUI_H

namespace CEGUI {

class OpenGL3Renderer;

} // namespace CEGUI

// forward declaration to avoid including <SDL.h>
typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

class GUI
{
public:
    GUI(SDL_Window *window);
    ~GUI();
    void advance(float seconds_elapsed);
    void handleEvent(const SDL_Event &event);
    void draw();
    bool getQuit() const {return mQuit;}
protected:
    CEGUI::OpenGL3Renderer *mRenderer;
    bool mQuit;
};

#endif // GUI_H
