# TailsTalk 2 for PS4 (very early alpha)

A PS4 homebrew build of TailsTalk 2 made with the OpenOrbis toolchain and SDL2.

What it does:

- Shows an email and password form.
- Opens the console's on-screen keyboard for each field.
- Logs in against the TailsTalk 2 API (`https://tails1154.com:9961/api`).
- Shows "Login succeeded" or the reason it failed.

What it does not do yet: channels, messages, voice, sign-up, two-factor login, or saving the session.

## Build

You need the OpenOrbis PS4 toolchain (v0.5.4 release or newer), clang and lld.

```sh
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain
make
```

This produces `IV0000-TTLK00001_00-TAILSTALK2000000.pkg`. Install it on the console with your usual homebrew package installer.

## Controls

| Input | Action |
| --- | --- |
| Up / Down (D-pad) | Move between fields |
| X (Cross) | Edit a field, or log in on the button |
| Circle | Back from the success screen |
| Options | Quit |

On a keyboard, arrow keys move, Enter selects.

## Notes

- Login uses the account **email**, not a username.
- The server certificate is checked by the console's own trust store. If the console does not trust the certificate, login fails rather than sending the password unchecked. This has not been tested on hardware yet.
- The keyboard shows typed text. The form hides the password, but the SDK headers do not expose a masking flag for the keyboard.
- The session token is not stored or shown.
- Title ID `TTLK00001` and the content ID are placeholders.
- The icon is the 512 px web icon from `tailstalk2-forweb/packages/client/public/assets/web/`.

## Layout

```
src/            C++ sources (main screen, login, on-screen keyboard, text rendering)
assets/fonts/   Fonts from the OpenOrbis SDL2 sample
sce_module/     PS4 modules from the OpenOrbis SDL2 sample
sce_sys/        Package metadata and icon
```

Text rendering and the module files come from the OpenOrbis SDL2 sample (GPLv3 licence, see the toolchain repository).
