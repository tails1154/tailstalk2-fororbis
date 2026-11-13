#include "login.h"

#include <orbis/Http.h>
#include <orbis/Net.h>
#include <orbis/Ssl.h>
#include <orbis/Sysmodule.h>

#include <stdio.h>
#include <string.h>

// Base URL of the TailsTalk 2 API
#define API_LOGIN_URL "https://tails1154.com:9961/api/auth/session/login"
#define HTTP_USER_AGENT "Mozilla/5.0 (PLAYSTATION 4; 1.00) TailsTalk2-Orbis/0.1"

#define NET_POOLSIZE (4 * 1024)
#define TLS_POOL_BYTES (256 * 1024)
#define HTTP_POOLSIZE (256 * 1024)

static int netMemId = 0;
static int sslCtxId = 0;
static int httpCtxId = 0;
static bool networkReady = false;

// Accept the server certificate only when the console's own trust store verifies it.
// Returning 0 aborts the connection, so passwords are never sent to an unverified host.
static int verifyCertificate(int sslId, unsigned int verifyErr, void* const sslCert[], int certNum, void* userArg)
{
    (void)sslId;
    (void)sslCert;
    (void)certNum;
    (void)userArg;
    if (verifyErr != 0)
    {
        printf("TLS verification failed: 0x%08X\n", verifyErr);
        return 0;
    }
    return 1;
}

bool NetworkInit()
{
    if (networkReady)
        return true;

    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0)
        return false;
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0)
        return false;
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0)
        return false;

    int ret = sceNetInit();
    if (ret < 0)
        return false;

    ret = sceNetPoolCreate("tailstalkNetPool", NET_POOLSIZE, 0);
    if (ret < 0)
        return false;
    netMemId = ret;

    ret = sceSslInit(TLS_POOL_BYTES);
    if (ret < 0)
        return false;
    sslCtxId = ret;

    ret = sceHttpInit(netMemId, sslCtxId, HTTP_POOLSIZE);
    if (ret < 0)
        return false;
    httpCtxId = ret;

    networkReady = true;
    return true;
}

// Escape a string for use inside a JSON string literal
static std::string JsonEscape(const std::string& in)
{
    std::string out;
    out.reserve(in.size() + 8);
    for (char c : in)
    {
        switch (c)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

// Read the value of a top-level JSON string field such as "result":"Success".
// This is a small search, not a parser: it is only used on the server's own responses.
static std::string FindJsonString(const std::string& body, const char* key)
{
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = body.find(needle);
    if (pos == std::string::npos)
        return "";
    pos = body.find(':', pos + needle.size());
    if (pos == std::string::npos)
        return "";
    pos = body.find('"', pos);
    if (pos == std::string::npos)
        return "";
    size_t end = body.find('"', pos + 1);
    if (end == std::string::npos)
        return "";
    return body.substr(pos + 1, end - pos - 1);
}

LoginResult LoginWithEmail(const std::string& email, const std::string& password)
{
    LoginResult fail{false, ""};

    if (!NetworkInit())
    {
        fail.message = "Network could not start. Check the connection.";
        return fail;
    }

    std::string body = "{\"email\":\"" + JsonEscape(email) + "\",\"password\":\"" + JsonEscape(password) + "\",\"friendly_name\":\"TailsTalk 2 (PS4 alpha)\"}";

    int tpl = sceHttpCreateTemplate(httpCtxId, HTTP_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
    if (tpl < 0)
    {
        fail.message = "Could not create the HTTP client.";
        return fail;
    }

    int conn = 0, req = 0;
    std::string response;
    int statusCode = 0;

    do
    {
        int ret = sceHttpsSetSslCallback(tpl, verifyCertificate, NULL);
        if (ret < 0)
        {
            fail.message = "Could not set up TLS.";
            break;
        }

        conn = sceHttpCreateConnectionWithURL(tpl, API_LOGIN_URL, 1);
        if (conn < 0)
        {
            fail.message = "Could not reach the server.";
            break;
        }

        req = sceHttpCreateRequestWithURL(conn, ORBIS_METHOD_POST, API_LOGIN_URL, body.size());
        if (req < 0)
        {
            fail.message = "Could not build the request.";
            break;
        }

        sceHttpAddRequestHeader(req, "Content-Type", "application/json", 1);

        ret = sceHttpSendRequest(req, body.data(), body.size());
        if (ret < 0)
        {
            fail.message = "The server did not respond. Check the connection and certificate.";
            break;
        }

        sceHttpGetStatusCode(req, &statusCode);

        char buf[1024];
        for (;;)
        {
            int read = sceHttpReadData(req, buf, sizeof(buf));
            if (read <= 0)
                break;
            response.append(buf, read);
        }
    } while (false);

    if (req > 0)
        sceHttpDeleteRequest(req);
    if (conn > 0)
        sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tpl);

    if (response.empty() && statusCode == 0)
    {
        fail.message = "No response from the server.";
        return fail;
    }

    std::string result = FindJsonString(response, "result");
    if (statusCode == 200 && result == "Success")
        return LoginResult{true, "Login succeeded"};

    if (result == "Disabled")
        return LoginResult{false, "This account is disabled."};
    if (result == "MFA")
        return LoginResult{false, "This account uses two-factor login, which the alpha does not support yet."};

    std::string type = FindJsonString(response, "type");
    if (type == "InvalidCredentials")
        return LoginResult{false, "Wrong email or password."};
    if (type == "LockedOut")
        return LoginResult{false, "Too many attempts. Try again later."};
    if (type == "Banned")
        return LoginResult{false, "This account is banned."};
    if (type == "Suspended")
        return LoginResult{false, "This account is suspended."};

    char status[96];
    snprintf(status, sizeof(status), "Login failed (HTTP %d).", statusCode);
    return LoginResult{false, status};
}
