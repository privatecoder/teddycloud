

#include <errno.h>  // for error_t
#include <stdint.h> // for uint8_t
#include <stdio.h>  // for snprintf
#include <stdlib.h> // for NULL, size_t
#include <string.h> // for strlen, strcpy, strncpy, strchr, strncmp

#include "cloud_request.h"    // for req_cbr_t, cloud_request, cloud_reques...
#include "compiler_port.h"    // for char_t, int_t, uint_t
#include "core/net.h"         // for IpAddr, (anonymous struct)::(anonymous)
#include "debug.h"            // for TRACE_INFO, TRACE_ERROR, TRACE_DEBUG
#include "error.h"            // for error2text, NO_ERROR, ERROR_ADDRESS_NO...
#include "handler.h"          // for cbr_ctx_t
#include "handler_api.h"      // for stats_update
#include "http/http_client.h" // for httpClientAddHeaderField, httpClientDi...
#include "http/http_common.h" // for HTTP_VERSION_1_1
#include "mqtt.h"             // for mqtt_sendEvent
#include "net_config.h"       // for client_ctx_t, TONIE_AUTH_TOKEN_LENGTH
#include "os_port.h"          // for osAllocMem, osFreeMem, FALSE, TRUE
#include "platform.h"         // for resolve_free, resolve_get_ip, resolve_...
#include "rand.h"             // for rand_get_algo, rand_get_context
#include "settings.h"         // for settings_t, get_settings, settings_cert_t
#include "stdbool.h"          // for bool, true, false
#include "tls.h"              // for TlsContext, _TlsContext (ptr only)
#include "tls_adapter.h"      // for tls_context_key_log_init

#define MAX_REDIRECTS 5

static const char BOXINE_CLOUD_TRUST_CA_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEtDCCApygAwIBAgIBIDANBgkqhkiG9w0BAQsFADBaMQswCQYDVQQGEwJERTEL\n"
    "MAkGA1UECAwCTlcxFDASBgNVBAcMC0R1ZXNzZWxkb3JmMRQwEgYDVQQKDAtCb3hp\n"
    "bmUgR21iSDESMBAGA1UEAwwJQm94aW5lIENBMB4XDTE2MDcwODE3MjMyOFoXDTI2\n"
    "MDgyNTE3MjMyOFoweTELMAkGA1UEBhMCREUxCzAJBgNVBAgMAk5XMRQwEgYDVQQH\n"
    "DAtEdWVzc2VsZG9yZjEUMBIGA1UECgwLQm94aW5lIEdtYkgxMTAvBgNVBAMMKEJv\n"
    "eGluZSBEb21haW4gVmFsaWRhdGlvbiBJbnRlcm1lZGlhdGUgQ0EwggEiMA0GCSqG\n"
    "SIb3DQEBAQUAA4IBDwAwggEKAoIBAQDPCiqzvMVhBnZ3p/CQVjS3CkwacABgdJ0m\n"
    "yp8I+9AnAk6kmBYp2yw1g63ubWDlOY06k+wjO0n24BeWMn13YJopdjSI+Kdvpyil\n"
    "ojkAcwzNheezdt5HkwqqsIiQyEFbbT3BiGkYfoCsgiobBswmseNeMugFGgZE5nqy\n"
    "znPMQ8eCnkeOncNM/hjlnIYMZbMVSCopN4C/xKuFTKwKnz1Yhh7PgTZgFdlkG9R8\n"
    "7uFHQg7+uYLfPbxt0Xn+X5Xs5WuCCzQOgRnUkMDXVxIGSCEtECITicvqMIL0tSRa\n"
    "vq1uOEi0UMn9sUuCVGLadR31wiKcbwcX/gLLZTpkJUnKAb6p1dmdAgMBAAGjZjBk\n"
    "MB0GA1UdDgQWBBQKvykXFyDlijxlRibodA4WQptl3DAfBgNVHSMEGDAWgBTV1GV8\n"
    "UZaa2+Q0z4y6BLTQ9qg2xTASBgNVHRMBAf8ECDAGAQH/AgEAMA4GA1UdDwEB/wQE\n"
    "AwIBhjANBgkqhkiG9w0BAQsFAAOCAgEAE38BCUEYgHCKmWhcwNz9sTnBulTBLVk4\n"
    "fyh36B1it0fvVmNgSSgy0qlRmy+OSf7zE/0QB9FB8T7HOSivrnoh6Zpywzl5H92j\n"
    "plwBk1zAVWUjHgholI2SpTGRFZHJsMmWRaASK/QV8RXO99Eg/a4VP5Wp6fwKx2FR\n"
    "jm+di44EMfcyYsmJm15u2d+0Kb0De8hkTBECEUnP7t/vKz0D1Lug6wKmGbGusJY6\n"
    "58PdHPLvUoEl6OfcP80LPejOICEvetBBLpBy63SrgrijPj94Tqs7ZR+XMYCuHGSw\n"
    "Le6sNOKfdjDeiwGTnadfya5FFYqS8DwFqpXKA7BZ2B57AJuBGABanYXR0ntqZDlP\n"
    "nuUxcCY7y3atIxNpVblC0eQkHtrixOu7aHfO+meZVT0ayXLWenJsmvp9+eyCH8ia\n"
    "udrzEvJyUovhRA0ybaF6Hq6UusbFbv/fLuL1NXOPg4jeswG/DlH86tG3TEnSIxhj\n"
    "CeaEsn7YgGzYMsdLOURh15ORPCqo1N3JQJA8PDQ0eBcomlTINKHOZAU1N1kGF3bA\n"
    "OMaOyDbYFeHZhbH+kWvmKD5L46YqjmdTL/etDKhqOyqEDd67Ia1FEFokGXmVclAa\n"
    "ENc6KUROzpzzwH+qW6+GLXNgpgxNxKJZ4atoe6OFiajWnOjpgYkcE57OGWvqVHRu\n"
    "477xbKoQJVw=\n"
    "-----END CERTIFICATE-----\n"
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "oct94CxqWdEU5K9C9WJ/Fk92Z8VR5lMvDm6T\n"
    "-----END CERTIFICATE-----\n";

error_t httpClientTlsInitCallbackBase(HttpClientContext *context,
                                      TlsContext *tlsContext, const char *trusted_ca, const char *client_crt, const char *client_key)
{
    TRACE_DEBUG("Initializing TLS...\r\n");
    error_t error;

    // Select client operation mode
    error = tlsSetConnectionEnd(tlsContext, TLS_CONNECTION_END_CLIENT);
    // Any error to report?
    if (error)
        return error;

    // Set the PRNG algorithm to be used
    error = tlsSetPrng(tlsContext, rand_get_algo(), rand_get_context());
    // Any error to report?
    if (error)
        return error;

    if (trusted_ca != NULL)
    {
        // Import the list of trusted CA certificates
        error = tlsSetTrustedCaList(tlsContext, trusted_ca, strlen(trusted_ca));
        // Any error to report?
        if (error)
            return error;
    }

    if (client_crt != NULL && client_key != NULL)
    {
        // Import the client's certificate
        error = tlsAddCertificate(tlsContext, client_crt, strlen(client_crt), client_key, strlen(client_key));
        // Any error to report?
        if (error)
            return error;
    }

    tls_context_key_log_init(tlsContext);

    tlsContext->serverName = (char *)strdup(context->serverName);

    TRACE_DEBUG("Initializing TLS done\r\n");

    // Successful processing
    return NO_ERROR;
}
error_t httpClientTlsInitCallbackNoCA(HttpClientContext *context,
                                      TlsContext *tlsContext)
{
    return httpClientTlsInitCallbackBase(context, tlsContext, NULL, NULL, NULL);
}
error_t httpClientTlsInitCallbackClientAuthTonies(HttpClientContext *context,
                                                  TlsContext *tlsContext)
{
    req_cbr_t *cbr_ctx = context->sourceCtx;
    client_ctx_t *client_ctx = ((cbr_ctx_t *)cbr_ctx->ctx)->client_ctx;
    settings_t *settings = client_ctx->settings;

    const char *client_crt = settings->internal.client.crt;
    const char *client_key = settings->internal.client.key;
    const char *trusted_ca = BOXINE_CLOUD_TRUST_CA_PEM;

    bool crt_missing = (client_crt == NULL || osStrlen(client_crt) == 0);
    bool key_missing = (client_key == NULL || osStrlen(client_key) == 0);

    if (settings->internal.overlayNumber != 0 && (crt_missing || key_missing))
    {
        TRACE_WARNING("Missing certificates for overlay %s, fallback to global certificates\r\n", settings->internal.overlayUniqueId);
        if (crt_missing)
        {
            TRACE_WARNING(" client.der (%s) missing\r\n", settings->core.client_cert.file.crt);
        }
        if (key_missing)
        {
            TRACE_WARNING(" private.der (%s) missing\r\n", settings->core.client_cert.file.key);
        }

        settings = get_settings();
        client_crt = settings->internal.client.crt;
        client_key = settings->internal.client.key;

        crt_missing = (client_crt == NULL || osStrlen(client_crt) == 0);
        key_missing = (client_key == NULL || osStrlen(client_key) == 0);
    }

    if (crt_missing || key_missing)
    {
        TRACE_ERROR("Failed to get certificates:\r\n");
        if (crt_missing)
        {
            TRACE_ERROR(" client.der (%s) missing\r\n", settings->core.client_cert.file.crt);
        }
        if (key_missing)
        {
            TRACE_ERROR(" private.der (%s) missing\r\n", settings->core.client_cert.file.key);
        }
        return ERROR_FAILURE;
    }

    return httpClientTlsInitCallbackBase(context, tlsContext, trusted_ca, client_crt, client_key);
}

int_t cloud_request_get(const char *server, int port, const char *uri, const char *queryString, const uint8_t *hash, req_cbr_t *cbr)
{
    return cloud_request(server, port, true, uri, queryString, "GET", NULL, 0, hash, cbr);
}

int_t cloud_request_post(const char *server, int port, const char *uri, const char *queryString, const uint8_t *body, size_t bodyLen, const uint8_t *hash, req_cbr_t *cbr)
{
    return cloud_request(server, port, true, uri, queryString, "POST", body, bodyLen, hash, cbr);
}

char_t *ipv4AddrToString(Ipv4Addr ipAddr, char_t *str);

int_t cloud_request(const char *server, int port, bool https, const char *uri, const char *queryString, const char *method, const uint8_t *body, size_t bodyLen, const uint8_t *hash, req_cbr_t *cbr)
{
    return web_request(server, port, https, uri, queryString, method, body, bodyLen, hash, cbr, true, true, NULL);
}

static void build_relative_redirect_path(const char *base_uri, const char *location, char *uri_path, char *query_str, size_t buf_size)
{
    const char *location_query = strchr(location, '?');
    size_t location_path_len = location_query ? (size_t)(location_query - location) : strlen(location);

    if (location_query)
    {
        strncpy(query_str, location_query + 1, buf_size - 1);
        query_str[buf_size - 1] = '\0';
    }
    else
    {
        query_str[0] = '\0';
    }

    if (location_path_len == 0)
    {
        const char *base_query = strchr(base_uri, '?');
        size_t base_path_len = base_query ? (size_t)(base_query - base_uri) : strlen(base_uri);
        if (base_path_len >= buf_size)
        {
            base_path_len = buf_size - 1;
        }
        strncpy(uri_path, base_uri, base_path_len);
        uri_path[base_path_len] = '\0';
        return;
    }

    if (location[0] == '/')
    {
        if (location_path_len >= buf_size)
        {
            location_path_len = buf_size - 1;
        }
        strncpy(uri_path, location, location_path_len);
        uri_path[location_path_len] = '\0';
        return;
    }

    char base_path[256];
    strncpy(base_path, base_uri, sizeof(base_path) - 1);
    base_path[sizeof(base_path) - 1] = '\0';

    char *base_query = strchr(base_path, '?');
    if (base_query)
    {
        *base_query = '\0';
    }

    char *last_slash = strrchr(base_path, '/');
    if (last_slash != NULL)
    {
        last_slash[1] = '\0';
    }
    else
    {
        strncpy(base_path, "/", sizeof(base_path) - 1);
        base_path[sizeof(base_path) - 1] = '\0';
    }

    size_t base_len = strlen(base_path);
    if (base_len >= buf_size)
    {
        base_len = buf_size - 1;
    }
    memcpy(uri_path, base_path, base_len);
    size_t copy_len = location_path_len;
    if (base_len + copy_len >= buf_size)
    {
        copy_len = buf_size - base_len - 1;
    }
    memcpy(uri_path + base_len, location, copy_len);
    uri_path[base_len + copy_len] = '\0';

    char normalized[256];
    size_t normalized_len = 0;
    size_t pos = 0;

    while (uri_path[pos] != '\0' && normalized_len < sizeof(normalized) - 1)
    {
        if (uri_path[pos] == '/')
        {
            normalized[normalized_len++] = uri_path[pos++];
            continue;
        }

        size_t segment_start = pos;
        while (uri_path[pos] != '\0' && uri_path[pos] != '/')
        {
            pos++;
        }
        size_t segment_len = pos - segment_start;

        if (segment_len == 1 && strncmp(&uri_path[segment_start], ".", 1) == 0)
        {
            continue;
        }

        if (segment_len == 2 && strncmp(&uri_path[segment_start], "..", 2) == 0)
        {
            if (normalized_len > 1)
            {
                normalized_len--;
                while (normalized_len > 0 && normalized[normalized_len - 1] != '/')
                {
                    normalized_len--;
                }
            }
            continue;
        }

        if (normalized_len > 0 && normalized[normalized_len - 1] != '/')
        {
            normalized[normalized_len++] = '/';
        }

        if (normalized_len + segment_len >= sizeof(normalized))
        {
            segment_len = sizeof(normalized) - normalized_len - 1;
        }
        memcpy(&normalized[normalized_len], &uri_path[segment_start], segment_len);
        normalized_len += segment_len;
    }

    if (normalized_len == 0)
    {
        normalized[normalized_len++] = '/';
    }
    normalized[normalized_len] = '\0';

    size_t normalized_copy_len = strlen(normalized);
    if (normalized_copy_len >= buf_size)
    {
        normalized_copy_len = buf_size - 1;
    }
    memcpy(uri_path, normalized, normalized_copy_len);
    uri_path[normalized_copy_len] = '\0';
}

static error_t web_request_impl(const char *server, int port, bool https, const char *uri, const char *queryString, const char *method, const uint8_t *body, size_t bodyLen, const uint8_t *hash, req_cbr_t *cbr, bool isCloud, bool printTextData, uint32_t *statusCode, int redirect_depth)
{
    cbr_ctx_t *cbr_ctx = cbr ? (cbr_ctx_t *)cbr->ctx : NULL;
    client_ctx_t *client_ctx = (cbr_ctx != NULL) ? cbr_ctx->client_ctx : NULL;
    settings_t *settings;
    error_t error = NO_ERROR;

    if (client_ctx == NULL)
    {
        settings = get_settings();
    }
    else
    {
        settings = client_ctx->settings;
    }

    if (isCloud)
    {
        if (!settings->cloud.enabled)
        {
            TRACE_INFO("Cloud requests generally blocked in settings\r\n");
            stats_update("cloud_blocked", 1);
            return ERROR_ADDRESS_NOT_FOUND;
        }

        mqtt_sendEvent("CloudRequest", uri, client_ctx);
    }
    HttpClientContext httpClientContext;

    if (isCloud)
    {
        if (!server)
        {
            server = settings->cloud.remote_hostname;
        }
        if (port <= 0)
        {
            port = settings->cloud.remote_port;
        }

        stats_update("cloud_requests", 1);
    }
    TRACE_INFO("Connecting to HTTP server %s:%d...\r\n",
               server, port);

    httpClientInit(&httpClientContext);
    httpClientContext.sourceCtx = cbr;
    httpClientContext.serverName = server;

    if (https)
    {
        HttpClientTlsInitCallback callback = httpClientTlsInitCallbackNoCA;
        if (isCloud)
            callback = httpClientTlsInitCallbackClientAuthTonies;
        error = httpClientRegisterTlsInitCallback(&httpClientContext, callback);
        if (error)
        {
            httpClientDeinit(&httpClientContext);
            return error;
        }
    }

    error = httpClientSetVersion(&httpClientContext, HTTP_VERSION_1_1);
    if (error)
    {
        httpClientDeinit(&httpClientContext);
        return error;
    }
    error = httpClientSetTimeout(&httpClientContext, settings->core.http_client_timeout);
    if (error)
    {
        httpClientDeinit(&httpClientContext);
        return error;
    }

    void *resolve_ctx = resolve_host(server);
    if (!resolve_ctx)
    {
        TRACE_ERROR("Failed to resolve ipv4 address!\r\n");
        if (isCloud)
            stats_update("cloud_failed", 1);
        httpClientDeinit(&httpClientContext);
        return ERROR_ADDRESS_NOT_FOUND;
    }

    int pos = 0;
    do
    {
        IpAddr ipAddr;
        if (!resolve_get_ip(resolve_ctx, pos, &ipAddr))
        {
            break;
        }
        bool success = FALSE;

        char_t host[129];

        ipv4AddrToString(ipAddr.ipv4Addr, host);
        TRACE_INFO("  trying IP: %s\n", host);

        do
        {
            error = httpClientConnect(&httpClientContext, &ipAddr,
                                      port);
            // Any error to report?
            if (error)
            {
                // Debug message
                TRACE_ERROR("Failed to connect to HTTP server! HTTP=%s error=%s\r\n", httpstatus2text(error), error2text(error));
                if (isCloud)
                    stats_update("cloud_failed", 1);
                break;
            }

            // Create an HTTP request
            httpClientCreateRequest(&httpClientContext);
            if (isCloud)
            {
                // Disable keep-alive for cloud requests.
                // Quick and dirty solution for slow cloud communication https://github.com/toniebox-reverse-engineering/teddycloud/issues/310
                httpClientContext.keepAlive = false;
            }

            httpClientSetMethod(&httpClientContext, method);
            httpClientSetUri(&httpClientContext, uri);
            httpClientSetQueryString(&httpClientContext, queryString);
            if (body && bodyLen > 0)
            {
                error = httpClientSetContentLength(&httpClientContext, bodyLen);
                if (error)
                {
                    // Debug message
                    TRACE_ERROR("Failed to set content length! Error=%s\r\n", error2text(error));
                    if (isCloud)
                        stats_update("cloud_failed", 1);
                    break;
                }
            }

            // Add HTTP header fields
            char host_line[128];
            snprintf(host_line, sizeof(host_line), "%s:%d", server, port);
            httpClientAddHeaderField(&httpClientContext, "Host", host_line);

            if (hash)
            {
                char tmp[3];
                char auth_line[128];

                osStrcpy(auth_line, "BD ");

                for (int token_pos = 0; token_pos < TONIE_AUTH_TOKEN_LENGTH; token_pos++)
                {
                    osSprintf(tmp, "%02X", hash[token_pos]);
                    osStrcat(auth_line, tmp);
                }
                httpClientAddHeaderField(&httpClientContext, "Authorization", auth_line);
            }

            if (cbr_ctx && cbr_ctx->user_agent)
            {
                httpClientAddHeaderField(&httpClientContext, "User-Agent", cbr_ctx->user_agent);
            }

            // Send HTTP request header
            error = httpClientWriteHeader(&httpClientContext);
            // Any error to report?
            if (error)
            {
                // Debug message
                TRACE_ERROR("Failed to write HTTP request header, error=%s!\r\n", error2text(error));
                if (isCloud)
                    stats_update("cloud_failed", 1);
                break;
            }
            // Send HTTP request body
            if (body && bodyLen > 0)
            {
                size_t n;
                error = httpClientWriteBody(&httpClientContext, body, bodyLen, &n, 0);
                // Any error to report?
                if (error)
                {
                    // Debug message
                    TRACE_ERROR("Failed to write HTTP request body, error=%s!\r\n", error2text(error));
                    if (isCloud)
                        stats_update("cloud_failed", 1);
                    break;
                }
            }

            // Receive HTTP response header
            error = httpClientReadHeader(&httpClientContext);
            // Any error to report?
            if (error)
            {
                // Debug message
                TRACE_ERROR("Failed to read HTTP response header, error=%s!\r\n", error2text(error));
                if (isCloud)
                    stats_update("cloud_failed", 1);
                break;
            }

            success = TRUE;

            // Retrieve HTTP status code
            uint_t status = httpClientGetStatus(&httpClientContext);

            if (status)
            {
                if (status == 200 || status == 302)
                {
                    TRACE_DEBUG("HTTP code: %u\r\n", status);
                }
                else
                {
                    TRACE_INFO("HTTP code: %u\r\n", status);
                }

                if (statusCode)
                {
                    *statusCode = status;
                }

                if (status == 302 && redirect_depth < MAX_REDIRECTS)
                {
                    // Extract location from response header
                    const char *location = httpClientGetHeaderField(&httpClientContext, "Location");
                    if (!location)
                    {
                        TRACE_ERROR("302 Found but no Location header present.\r\n");
                        error = ERROR_INVALID_RESPONSE;
                        break;
                    }

                    TRACE_INFO("Redirecting to: %s\r\n", location);

                    // Disconnect HTTP client
                    httpClientDisconnect(&httpClientContext);

                    char uri_base[256], uri_path[256], query_str[256];
                    int redirect_port;
                    bool redirect_https;

                    if (strstr(location, "://"))
                    {
                        // Absolute URL
                        split_url(location, uri_base, uri_path, query_str, sizeof(uri_base));

                        // Extract port from uri_base if present (host:port)
                        redirect_port = 443;
                        redirect_https = true;
                        char *port_sep = strchr(uri_base, ':');
                        if (port_sep)
                        {
                            redirect_port = atoi(port_sep + 1);
                            *port_sep = '\0';
                        }
                        if (strncmp(location, "http://", 7) == 0)
                        {
                            redirect_https = false;
                            if (!port_sep) redirect_port = 80;
                        }
                    }
                    else
                    {
                        // Relative URL — reuse current server/port/scheme
                        strncpy(uri_base, server, sizeof(uri_base) - 1);
                        uri_base[sizeof(uri_base) - 1] = '\0';
                        redirect_port = port;
                        redirect_https = https;
                        build_relative_redirect_path(uri, location, uri_path, query_str, sizeof(uri_path));
                    }

                    TRACE_DEBUG("URI Base: %s\r\n", uri_base);
                    TRACE_DEBUG("URI Path: %s\r\n", uri_path);
                    TRACE_DEBUG("Query String: %s\r\n", query_str);

                    error = web_request_impl(uri_base, redirect_port, redirect_https, uri_path, query_str, "GET", NULL, 0, NULL, cbr, isCloud, false, NULL, redirect_depth + 1);
                    break;
                }
            }

            if (cbr && cbr->response)
            {
                error_t cbr_error = cbr->response(cbr->ctx, &httpClientContext);
                if (cbr_error)
                {
                    TRACE_WARNING("Response callback detected downstream failure: %s\r\n", error2text(cbr_error));
                    error = cbr_error;
                    break;
                }
            }

            char content_type[64];

            strcpy(content_type, "");

            do
            {
                const char *header_name = NULL;
                const char *header_value = NULL;
                error_t ret = httpClientGetNextHeaderField(&httpClientContext, &header_name, &header_value);

                if (cbr && cbr->header)
                {
                    error_t cbr_error = cbr->header(cbr->ctx, &httpClientContext, header_name, header_value);
                    if (cbr_error)
                    {
                        TRACE_WARNING("Header callback detected downstream failure: %s\r\n", error2text(cbr_error));
                        error = cbr_error;
                        break;
                    }
                }

                if (ret != NO_ERROR)
                {
                    break;
                }

                if (!osStrcmp(header_name, "Content-Type"))
                {
                    osStrncpy(content_type, header_value, sizeof(content_type) - 1);
                    TRACE_DEBUG("Content-Type is %s\r\n", content_type);
                }
            } while (1);

            // Abort if response or header callback detected downstream failure
            if (error)
                break;

            // Header field found?
            if (strlen(content_type) == 0)
            {
                TRACE_INFO("Content-Type header field not found!\r\n");
            }

            bool binary = true;
            if (!strncmp(content_type, "text", 4))
            {
                binary = false;
            }
            else if (!strncmp(content_type, "application/json", 16))
            {
                binary = false;
            }
            else
            {
                TRACE_INFO("Binary data, not dumping body\r\n");
            }

            size_t maxSize = 4096;
            uint8_t *buffer = osAllocMem(maxSize + 1);
            if (buffer == NULL)
            {
                TRACE_ERROR("Failed to allocate response buffer (%" PRIuSIZE " bytes)\r\n", maxSize + 1);
                error = ERROR_OUT_OF_MEMORY;
                break;
            }
            // Receive HTTP response body
            while (!error)
            {
                // Read data
                size_t length = 0;

                error = httpClientReadBody(&httpClientContext, buffer, maxSize, &length, 0);

                if (cbr && cbr->body)
                {
                    error_t cbr_error = cbr->body(cbr->ctx, &httpClientContext, (const char *)buffer, length, error);
                    if (cbr_error && cbr_error != ERROR_END_OF_STREAM)
                    {
                        TRACE_WARNING("Body callback aborted download: %s\r\n", error2text(cbr_error));
                        error = cbr_error;
                        break;
                    }
                }

                // Check status code
                if (!error)
                {
                    if (printTextData && !binary)
                    {
                        // Properly terminate the string with a NULL character
                        buffer[length] = '\0';
                        // Dump HTTP response body
                        TRACE_INFO("Response: '%s'\r\n", buffer);
                    }
                }
            }

            osFreeMem(buffer);

            // Any error to report?
            if (error != ERROR_END_OF_STREAM)
                break;

            // Close HTTP response body
            error = httpClientCloseBody(&httpClientContext);
            // Any error to report?
            if (error)
            {
                // Debug message
                TRACE_INFO("Failed to read HTTP response trailer!\r\n");
                if (isCloud)
                    stats_update("cloud_failed", 1);
                break;
            }

            // Gracefully disconnect from the HTTP server
            httpClientDisconnect(&httpClientContext);

            // Debug message
            TRACE_DEBUG("Connection closed\r\n");
        } while (0);

        if (success)
        {
            break;
        }
        pos++;
    } while (1);

    resolve_free(resolve_ctx);
    // Release HTTP client context
    httpClientDeinit(&httpClientContext);

    return error;
}

error_t web_request(const char *server, int port, bool https, const char *uri, const char *queryString, const char *method, const uint8_t *body, size_t bodyLen, const uint8_t *hash, req_cbr_t *cbr, bool isCloud, bool printTextData, uint32_t *statusCode)
{
    error_t error = web_request_impl(server, port, https, uri, queryString, method, body, bodyLen, hash, cbr, isCloud, printTextData, statusCode, 0);

    /* Lifecycle: always invoke disconnect callback so callers waiting on
     * PROX_STATUS_DONE (handler_reverse.c) are unblocked, regardless of
     * whether the request completed normally, was aborted, or failed. */
    if (cbr && cbr->disconnect)
    {
        cbr->disconnect(cbr->ctx, NULL);
    }

    return error;
}

void split_url(const char *location, char *uri_base, char *uri_path, char *query_string, size_t buf_size)
{
    uri_base[0] = '\0';
    uri_path[0] = '\0';
    query_string[0] = '\0';

    const char *scheme_end = strstr(location, "://");
    if (!scheme_end)
    {
        TRACE_ERROR("Invalid URL: Scheme not found\n");
        return;
    }
    // Move pointer to start after "://"
    scheme_end += 3;

    const char *path_start = strchr(scheme_end, '/');
    if (!path_start)
    {
        // URL like "https://example.com" or "https://example.com?x=1" — no path separator
        const char *query_in_host = strchr(scheme_end, '?');
        size_t base_len = query_in_host ? (size_t)(query_in_host - scheme_end) : strlen(scheme_end);
        if (base_len >= buf_size) base_len = buf_size - 1;
        strncpy(uri_base, scheme_end, base_len);
        uri_base[base_len] = '\0';
        strncpy(uri_path, "/", buf_size - 1);
        uri_path[buf_size - 1] = '\0';
        if (query_in_host)
        {
            strncpy(query_string, query_in_host + 1, buf_size - 1);
            query_string[buf_size - 1] = '\0';
        }
        return;
    }
    const char *query_start = strchr(path_start, '?');

    // Copy base URI without scheme
    size_t base_len = path_start - scheme_end;
    if (base_len >= buf_size) base_len = buf_size - 1;
    strncpy(uri_base, scheme_end, base_len);
    uri_base[base_len] = '\0';

    if (query_start)
    {
        // Copy path
        size_t path_len = query_start - path_start;
        if (path_len >= buf_size) path_len = buf_size - 1;
        strncpy(uri_path, path_start, path_len);
        uri_path[path_len] = '\0';

        // Copy query string
        strncpy(query_string, query_start + 1, buf_size - 1);
        query_string[buf_size - 1] = '\0';
    }
    else
    {
        // Copy path
        strncpy(uri_path, path_start, buf_size - 1);
        uri_path[buf_size - 1] = '\0';
    }
}
