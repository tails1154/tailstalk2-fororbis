#pragma once

#include <string>

// Network setup for the HTTPS API client. Call once before LoginWithEmail.
bool NetworkInit();

// Result of a login attempt. `message` is safe to show to the user.
struct LoginResult
{
    bool ok;
    std::string message;
};

// POST /api/auth/session/login with an email and password.
// The returned session token is never stored or shown.
LoginResult LoginWithEmail(const std::string& email, const std::string& password);
