#include "GUI.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
#pragma GCC diagnostic pop

#include <SDL.h>
#include <SDL_opengl.h>

#include "sdl_scancode_to_dinput_mappings.h"

#include <iostream>

static CEGUI::Key::Scan toCEGUIKey(SDL_Scancode key)
{
    return static_cast<CEGUI::Key::Scan>(scanCodeToKeyNum[static_cast<int>(key)]);
}

static void injectUTF8Text(const char* utf8str)
{
    static SDL_iconv_t cd = SDL_iconv_t(-1);

    if (cd == SDL_iconv_t(-1))
    {
        // note: just "UTF-32" doesn't work as toFormat, because then you get BOMs, which we don't want.
        const char* toFormat = "UTF-32LE"; // TODO: what does CEGUI expect on big endian machines?
        cd = SDL_iconv_open(toFormat, "UTF-8");
        if (cd == SDL_iconv_t(-1))
        {
            std::cerr << "Couldn't initialize SDL_iconv for UTF-8 to UTF-32!" << std::endl;
            return;
        }
    }

    // utf8str has at most SDL_TEXTINPUTEVENT_TEXT_SIZE (32) chars,
    // so we won't have have more utf32 chars than that
    Uint32 utf32buf[SDL_TEXTINPUTEVENT_TEXT_SIZE] = {0};

    // we'll convert utf8str to a utf32 string, saved in utf32buf.
    // the utf32 chars will be injected into cegui

    size_t len = strlen(utf8str);

    size_t inbytesleft = len;
    size_t outbytesleft = 4 * SDL_TEXTINPUTEVENT_TEXT_SIZE; // *4 because utf-32 needs 4x as much space as utf-8
    char* outbuf = (char*)utf32buf;
    size_t n = SDL_iconv(cd, &utf8str, &inbytesleft, &outbuf, &outbytesleft);

    if (n == size_t(-1)) // some error occured during iconv
    {
        std::cerr << "Converting UTF-8 string " << utf8str << " from SDL_TEXTINPUT to UTF-32 failed!" << std::endl;
    }

    for (int i = 0; i < SDL_TEXTINPUTEVENT_TEXT_SIZE; ++i)
    {
        if (utf32buf[i] == 0)
            break; // end of string

        CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(utf32buf[i]);
    }

    // reset cd so it can be used again
    SDL_iconv(cd, NULL, &inbytesleft, NULL, &outbytesleft);
}

static void initCEGUI(int w, int h)
{
    using namespace CEGUI;

    // create renderer and enable extra states
    OpenGL3Renderer& cegui_renderer = OpenGL3Renderer::create(Sizef(w, h));
    cegui_renderer.enableExtraStateSettings(true);

    // create CEGUI system object
    System::create(cegui_renderer);

    // setup resource directories
    DefaultResourceProvider* rp = static_cast<DefaultResourceProvider*>(System::getSingleton().getResourceProvider());
    static const std::string ceguiDataPath = "/usr/share/cegui-0.8.7";
    rp->setResourceGroupDirectory("schemes", ceguiDataPath + "/schemes/");
    rp->setResourceGroupDirectory("imagesets", ceguiDataPath + "/imagesets/");
    rp->setResourceGroupDirectory("fonts", ceguiDataPath + "/fonts/");
    rp->setResourceGroupDirectory("layouts", ceguiDataPath + "/layouts/");
    rp->setResourceGroupDirectory("looknfeels", ceguiDataPath + "/looknfeel/");


/*    rp->setResourceGroupDirectory("schemes", "datafiles/schemes/");
    rp->setResourceGroupDirectory("imagesets", "datafiles/imagesets/");
    rp->setResourceGroupDirectory("fonts", "datafiles/fonts/");
    rp->setResourceGroupDirectory("layouts", "datafiles/layouts/");
    rp->setResourceGroupDirectory("looknfeels", "datafiles/looknfeel/");
    rp->setResourceGroupDirectory("lua_scripts", "datafiles/lua_scripts/");
    rp->setResourceGroupDirectory("schemas", "datafiles/xml_schemas/");
*/
    // set default resource groups
    ImageManager::setImagesetDefaultResourceGroup("imagesets");
    Font::setDefaultResourceGroup("fonts");
    Scheme::setDefaultResourceGroup("schemes");
    WidgetLookManager::setDefaultResourceGroup("looknfeels");
    WindowManager::setDefaultResourceGroup("layouts");
    ScriptModule::setDefaultResourceGroup("lua_scripts");

    XMLParser* parser = System::getSingleton().getXMLParser();
    if (parser->isPropertyPresent("SchemaDefaultResourceGroup"))
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");

    // load TaharezLook scheme and DejaVuSans-10 font
    SchemeManager::getSingleton().createFromFile("TaharezLook.scheme", "schemes");
    FontManager::getSingleton().createFromFile("DejaVuSans-10.font");

    // set default font and cursor image and tooltip type
    System::getSingleton().getDefaultGUIContext().setDefaultFont("DejaVuSans-10");
    System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");
    System::getSingleton().getDefaultGUIContext().setDefaultTooltipType("TaharezLook/Tooltip");
}

static void initWindows()
{
    using namespace CEGUI;

    /////////////////////////////////////////////////////////////
    // Add your gui initialisation code in here.
    // You can use the following code as an inspiration for
    // creating your own windows.
    // But you should preferably use layout loading because you won't
    // have to recompile everytime you change the layout.
    /////////////////////////////////////////////////////////////

    // load layout
    Window* root = WindowManager::getSingleton().loadLayoutFromFile("application_templates.layout");
    System::getSingleton().getDefaultGUIContext().setRootWindow(root);
}

// convert SDL mouse button to CEGUI mouse button
static CEGUI::MouseButton SDLtoCEGUIMouseButton(const Uint8& button)
{
    using namespace CEGUI;

    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return LeftButton;

    case SDL_BUTTON_MIDDLE:
        return MiddleButton;

    case SDL_BUTTON_RIGHT:
        return RightButton;

    case SDL_BUTTON_X1:
        return X1Button;

    case SDL_BUTTON_X2:
        return X2Button;

    default:
        return NoButton;
    }
}

GUI::GUI(SDL_Window *window) : mRenderer(nullptr), mQuit(false)
{
    using namespace CEGUI;

    // SDL_GL_MakeCurrent(window, ceguiContext);
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);

    // init cegui
    initCEGUI(w, h);

    // notify system of the window size
    System::getSingleton().notifyDisplaySizeChanged(Sizef(w, h));

    // initialise windows and setup layout
    initWindows();

    // glClearColor(0, 0, 0, 255);
    mRenderer = static_cast<OpenGL3Renderer*>(System::getSingleton().getRenderer());
}

GUI::~GUI()
{
    CEGUI::System::destroy();
    CEGUI::OpenGL3Renderer::destroy(*mRenderer);
}

void GUI::advance(float seconds_elapsed)
{
    CEGUI::System::getSingleton().injectTimePulse(seconds_elapsed);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(seconds_elapsed);
}

void GUI::handleEvent(const SDL_Event &event)
{
    switch (event.type)
    {
        case SDL_QUIT:
        mQuit = true;
        break;

        case SDL_MOUSEMOTION:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
        break;

        case SDL_MOUSEBUTTONDOWN:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(SDLtoCEGUIMouseButton(event.button.button));
        break;

        case SDL_MOUSEBUTTONUP:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(SDLtoCEGUIMouseButton(event.button.button));
        break;

        case SDL_MOUSEWHEEL:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(static_cast<float>(event.wheel.y));
        break;

        case SDL_KEYDOWN:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(toCEGUIKey(event.key.keysym.scancode));
        break;

        case SDL_KEYUP:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(toCEGUIKey(event.key.keysym.scancode));
        break;

        case SDL_TEXTINPUT:
        injectUTF8Text(event.text.text);
        break;

        case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(static_cast<float>(event.window.data1), static_cast<float>(event.window.data2)));
            glViewport(0, 0, event.window.data1, event.window.data2);
        }
        else if (event.window.event == SDL_WINDOWEVENT_LEAVE)
        {
            CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseLeaves();
        }
        break;

        default:
        break;
    }
}

void GUI::draw()
{
    mRenderer->beginRendering();
    CEGUI::System::getSingleton().renderAllGUIContexts();
    mRenderer->endRendering();
}
