// TailsTalk 2 for PS4 (OpenOrbis), very early alpha.
// Shows a username/password form, signs in against the TailsTalk 2 API and reports the result.

#include <proto-include.h>
#include <SDL2/SDL.h>

#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>

#include <stdio.h>
#include <string>

#include "Color.h"
#include "TTF.h"
#include "ime.h"
#include "login.h"

#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080

// Controller buttons, matching the OpenOrbis SDL2 sample
enum ControllerButton
{
    PAD_BUTTON_CROSS = 0,
    PAD_BUTTON_CIRCLE = 1,
    PAD_BUTTON_OPTIONS = 9,
    PAD_BUTTON_UP = 13,
    PAD_BUTTON_DOWN = 14,
};

enum Screen
{
    SCREEN_FORM,
    SCREEN_SUCCESS,
};

static const Color colBackground = {0x17, 0x18, 0x1F};
static const Color colText = {0xF2, 0xF2, 0xF7};
static const Color colMuted = {0x9A, 0x9C, 0xAD};
static const Color colAccent = {0x7C, 0x6C, 0xF0};
static const Color colSuccess = {0x3B, 0xA5, 0x5C};
static const Color colError = {0xE0, 0x5A, 0x5A};

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static FT_Face fontTitle;
static FT_Face fontBody;

static std::string email;
static std::string password;
static std::string statusMessage;
static bool statusIsError = false;
static int selected = 0; // 0 email, 1 password, 2 log in
static Screen screen = SCREEN_FORM;

static bool SystemInit()
{
    // FreeType has to be loaded explicitly, or the app crashes on unresolved NIDs
    if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL) < 0)
        return false;
    if (FT_Init_FreeType(&ftLib) < 0)
        return false;
    if (!InitFont(&fontTitle, "/app0/assets/fonts/Bebas-Regular.ttf", 96))
        return false;
    if (!InitFont(&fontBody, "/app0/assets/fonts/VeraMono.ttf", 36))
        return false;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
        return false;

    window = SDL_CreateWindow("TailsTalk 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_WIDTH, FRAME_HEIGHT, 0);
    if (!window)
        return false;

    SDL_Surface* surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateSoftwareRenderer(surface);
    if (!renderer)
        return false;

    // Open the first controller. Input still works from the keyboard if none is found.
    if (SDL_NumJoysticks() > 0)
        SDL_JoystickOpen(0);

    return true;
}

static void DrawText(const std::string& text, FT_Face face, int x, int y, Color color)
{
    if (text.empty())
        return;
    std::string copy = text; // CreateText takes a mutable buffer
    SDL_Texture* tex = CreateText(renderer, &copy[0], face, color, Color{0, 0, 0});
    if (!tex)
        return;
    int w = 0, h = 0;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static void DrawBox(int x, int y, int w, int h, Color fill, bool outline)
{
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, 0xFF);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
    if (outline)
    {
        SDL_SetRenderDrawColor(renderer, colAccent.r, colAccent.g, colAccent.b, 0xFF);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

// Passwords are shown as dots, never as text
static std::string Masked(const std::string& s)
{
    return std::string(s.size(), '*');
}

static void DrawFormRow(int row, const char* label, const std::string& value, const char* placeholder)
{
    const int rowY = 380 + row * 130;
    const bool isSelected = (selected == row);

    DrawBox(360, rowY, 1200, 100, Color{0x25, 0x27, 0x30}, isSelected);
    DrawText(label, fontBody, 400, rowY + 30, colMuted);
    DrawText(value.empty() ? placeholder : value, fontBody, 700, rowY + 30, value.empty() ? colMuted : colText);
}

static void DrawForm()
{
    DrawText("TailsTalk 2", fontTitle, 360, 120, colText);
    DrawText("Very early alpha. Log in with your email and password.", fontBody, 360, 250, colMuted);

    DrawFormRow(0, "Email", email, "Press X to enter your email");
    DrawFormRow(1, "Password", Masked(password), "Press X to enter your password");

    // Log in button
    const int buttonY = 380 + 2 * 130;
    const bool buttonSelected = (selected == 2);
    DrawBox(360, buttonY, 420, 100, colAccent, buttonSelected);
    DrawText("Log in", fontBody, 500, buttonY + 30, colText);

    if (!statusMessage.empty())
        DrawText(statusMessage, fontBody, 360, 900, statusIsError ? colError : colMuted);

    DrawText("Up/Down to move   X to select   Options to quit", fontBody, 360, 1000, colMuted);
}

static void DrawSuccess()
{
    DrawBox(360, 300, 1200, 480, colSuccess, false);
    DrawText("Login succeeded", fontTitle, 420, 380, colText);
    DrawText("Signed in as " + email, fontBody, 420, 540, colText);
    DrawText("Circle to go back", fontBody, 420, 680, colText);
}

static void Render()
{
    SDL_SetRenderDrawColor(renderer, colBackground.r, colBackground.g, colBackground.b, 0xFF);
    SDL_RenderClear(renderer);

    if (screen == SCREEN_FORM)
        DrawForm();
    else
        DrawSuccess();

    SDL_UpdateWindowSurface(window);
}

static void TryLogin()
{
    if (email.empty() || password.empty())
    {
        statusMessage = "Enter both an email and a password.";
        statusIsError = true;
        return;
    }

    statusMessage = "Logging in...";
    statusIsError = false;
    Render(); // show the message before the request blocks

    LoginResult result = LoginWithEmail(email, password);
    statusMessage = result.message;
    statusIsError = !result.ok;
    if (result.ok)
    {
        password.clear(); // do not keep the password after a successful login
        screen = SCREEN_SUCCESS;
    }
}

static void Activate()
{
    if (selected == 0)
        EditTextWithKeyboard(L"Email", false, email, 254);
    else if (selected == 1)
        EditTextWithKeyboard(L"Password", true, password, 128);
    else
        TryLogin();
}

// Returns false when the app should quit
static bool HandleButton(int button)
{
    switch (button)
    {
    case PAD_BUTTON_CROSS:
        if (screen == SCREEN_FORM)
            Activate();
        break;
    case PAD_BUTTON_CIRCLE:
        if (screen == SCREEN_SUCCESS)
            screen = SCREEN_FORM;
        break;
    case PAD_BUTTON_UP:
        if (screen == SCREEN_FORM && selected > 0)
            selected--;
        break;
    case PAD_BUTTON_DOWN:
        if (screen == SCREEN_FORM && selected < 2)
            selected++;
        break;
    case PAD_BUTTON_OPTIONS:
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (!SystemInit())
    {
        // Nothing to show on screen if startup fails; the log has the reason.
        printf("TailsTalk 2: startup failed\n");
        return 1;
    }

    bool running = true;
    bool dirty = true;
    while (running)
    {
        SDL_Event ev;
        // Wait briefly for input so the loop does not spin
        if (SDL_WaitEventTimeout(&ev, 250))
        {
            do
            {
                switch (ev.type)
                {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_JOYBUTTONDOWN:
                    running = HandleButton(ev.jbutton.button) && running;
                    dirty = true;
                    break;
                case SDL_JOYHATMOTION:
                    if (ev.jhat.value & SDL_HAT_UP)
                        running = HandleButton(PAD_BUTTON_UP) && running;
                    if (ev.jhat.value & SDL_HAT_DOWN)
                        running = HandleButton(PAD_BUTTON_DOWN) && running;
                    dirty = true;
                    break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_UP)
                        running = HandleButton(PAD_BUTTON_UP) && running;
                    else if (ev.key.keysym.sym == SDLK_DOWN)
                        running = HandleButton(PAD_BUTTON_DOWN) && running;
                    else if (ev.key.keysym.sym == SDLK_RETURN)
                        running = HandleButton(PAD_BUTTON_CROSS) && running;
                    dirty = true;
                    break;
                }
            } while (SDL_PollEvent(&ev));
        }

        if (dirty)
        {
            Render();
            dirty = false;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
