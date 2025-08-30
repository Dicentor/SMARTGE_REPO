#include "firestore.h"
#include "secrets.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string>
#include <vector>
#include <optional>
#include <ctime>

static const char* TAG = "firestore";

static std::string g_id_token;
static std::string g_refresh_token;
static int g_expires_in = 0; // Sekunden
static int64_t g_token_birth_us = 0;

static bool http_request(const char* url, const char* method, const std::string& body, std::string& out, const char* authBearer=nullptr)
{
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.cert_pem = GOOGLE_ROOT_CA_PEM;
    cfg.timeout_ms = 15000;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, (esp_http_client_method_t)(
        strcmp(method,"GET")==0?HTTP_METHOD_GET:
        strcmp(method,"POST")==0?HTTP_METHOD_POST:
        strcmp(method,"PATCH")==0?HTTP_METHOD_PATCH:HTTP_METHOD_GET));

    if (authBearer) {
        std::string bearer = std::string("Bearer ") + authBearer;
        esp_http_client_set_header(client, "Authorization", bearer.c_str());
    }
    esp_http_client_set_header(client, "Content-Type", "application/json");

    if (!body.empty()) esp_http_client_set_post_field(client, body.c_str(), body.size());

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP %s error: %s", method, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }
    int status = esp_http_client_get_status_code(client);
    if (status < 200 || status >= 300) {
        ESP_LOGE(TAG, "HTTP status %d", status);
    }

    // Robust für chunked/unknown-length
    out.clear();
    char buf[512];
    while (true) {
        int r = esp_http_client_read(client, buf, sizeof(buf));
        if (r <= 0) break;
        out.append(buf, r);
    }

    esp_http_client_cleanup(client);
    return status >= 200 && status < 300;
}

static bool token_valid()
{
    if (g_id_token.empty()) return false;
    int64_t age_s = (esp_timer_get_time() - g_token_birth_us)/1000000;
    return age_s < (g_expires_in - 60); // 60s Puffer
}

bool firebase_auth_init()
{
    if (token_valid()) return true;

    // Anonyme Anmeldung bei Firebase Auth
    std::string url = std::string("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=") + FIREBASE_API_KEY;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "returnSecureToken", true);
    char *payload = cJSON_PrintUnformatted(root);

    std::string resp;
    bool ok = http_request(url.c_str(), "POST", payload, resp, nullptr);
    cJSON_Delete(root);
    free(payload);

    if (!ok) { ESP_LOGE(TAG, "Auth request failed"); return false; }

    cJSON *r = cJSON_ParseWithLength(resp.c_str(), resp.size());
    if (!r) { ESP_LOGE(TAG, "Auth JSON parse failed"); return false; }

    cJSON *idToken = cJSON_GetObjectItemCaseSensitive(r, "idToken");
    cJSON *refreshToken = cJSON_GetObjectItemCaseSensitive(r, "refreshToken");
    cJSON *expiresIn = cJSON_GetObjectItemCaseSensitive(r, "expiresIn");

    if (cJSON_IsString(idToken) && cJSON_IsString(refreshToken) && cJSON_IsString(expiresIn)) {
        g_id_token = idToken->valuestring;
        g_refresh_token = refreshToken->valuestring;
        g_expires_in = atoi(expiresIn->valuestring);
        g_token_birth_us = esp_timer_get_time();
        ESP_LOGI(TAG, "Firebase anonymous auth OK");
        cJSON_Delete(r);
        return true;
    }

    ESP_LOGE(TAG, "Auth response missing fields");
    cJSON_Delete(r);
    return false;
}

static std::string doc_url(const char* collection)
{
    std::string base = "https://firestore.googleapis.com/v1/projects/";
    base += FIREBASE_PROJECT_ID;
    base += "/databases/(default)/documents/";
    base += collection;
    base += "/";
    base += DEVICE_ID;
    return base;
}

bool firestore_push(const AllReadings& r)
{
    if (!token_valid()) {
        if (!firebase_auth_init()) return false;
    }

    // Firestore Dokument Payload aufbauen
    cJSON *fields = cJSON_CreateObject();

    // Luftwerte
    { cJSON *o = cJSON_CreateObject(); cJSON_AddNumberToObject(o, "doubleValue", r.air.humidity);     cJSON_AddItemToObject(fields, "airHumidity", o); }
    { cJSON *o = cJSON_CreateObject(); cJSON_AddNumberToObject(o, "doubleValue", r.air.temperatureC); cJSON_AddItemToObject(fields, "airTemperature", o); }

    // Bodenfeuchte als Firestore-Array: arrayValue.values[...]
    cJSON *soil_field = cJSON_CreateObject();
    cJSON *arrayValue = cJSON_CreateObject();
    cJSON *values = cJSON_CreateArray();
    for (int i=0;i<3;i++) {
        cJSON *dv = cJSON_CreateObject();
        cJSON_AddNumberToObject(dv, "doubleValue", r.soil[i].fraction);
        cJSON_AddItemToArray(values, dv);
    }
    cJSON_AddItemToObject(arrayValue, "values", values);
    cJSON_AddItemToObject(soil_field, "arrayValue", arrayValue);
    cJSON_AddItemToObject(fields, "soil", soil_field);

    // NPN Sensor
    { cJSON *o = cJSON_CreateObject(); cJSON_AddBoolToObject(o, "booleanValue", r.npnActive); cJSON_AddItemToObject(fields, "npn", o); }

    // Timestamp (RFC3339 UTC)
    char iso[64];
    time_t now = 0; time(&now);
    struct tm tm_utc; gmtime_r(&now, &tm_utc);
    strftime(iso, sizeof(iso), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    { cJSON *o = cJSON_CreateObject(); cJSON_AddStringToObject(o, "timestampValue", iso); cJSON_AddItemToObject(fields, "timestamp", o); }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "fields", fields);
    char *payload = cJSON_PrintUnformatted(root);

    std::string resp;
    std::string url = doc_url("telemetry");
    bool ok = http_request(url.c_str(), "PATCH", payload, resp, g_id_token.c_str());

    free(payload);
    cJSON_Delete(root);

    if (!ok) {
        ESP_LOGE(TAG, "Firestore push failed");
        return false;
    }
    ESP_LOGI(TAG, "Telemetry pushed: %s", url.c_str());
    return true;
}

static std::optional<double> get_double_field(cJSON* fields, const char* name)
{
    cJSON* f = cJSON_GetObjectItemCaseSensitive(fields, name);
    if (!f) return std::nullopt;
    cJSON* dv = cJSON_GetObjectItemCaseSensitive(f, "doubleValue");
    if (cJSON_IsNumber(dv)) return dv->valuedouble;
    cJSON* iv = cJSON_GetObjectItemCaseSensitive(f, "integerValue");
    if (cJSON_IsString(iv)) return (double)atoll(iv->valuestring);
    return std::nullopt;
}

static std::vector<double> get_double_array(cJSON* fields, const char* name)
{
    std::vector<double> out;
    cJSON* field = cJSON_GetObjectItemCaseSensitive(fields, name);
    if (!field) return out;
    cJSON* arrayVal = cJSON_GetObjectItemCaseSensitive(field, "arrayValue");
    if (!arrayVal) return out;
    cJSON* values = cJSON_GetObjectItemCaseSensitive(arrayVal, "values");
    if (!cJSON_IsArray(values)) return out;
    cJSON* it = nullptr;
    cJSON_ArrayForEach(it, values) {
        cJSON* dv = cJSON_GetObjectItemCaseSensitive(it, "doubleValue");
        if (cJSON_IsNumber(dv)) out.push_back(dv->valuedouble);
        else {
            cJSON* iv = cJSON_GetObjectItemCaseSensitive(it, "integerValue");
            if (cJSON_IsString(iv)) out.push_back((double)atoll(iv->valuestring));
        }
    }
    return out;
}

static std::optional<bool> get_bool_field(cJSON* fields, const char* name)
{
    cJSON* f = cJSON_GetObjectItemCaseSensitive(fields, name);
    if (!f) return std::nullopt;
    cJSON* bv = cJSON_GetObjectItemCaseSensitive(f, "booleanValue");
    if (cJSON_IsBool(bv)) return cJSON_IsTrue(bv);
    return std::nullopt;
}

bool firestore_pull(DeviceConfig& cfg)
{
    if (!token_valid()) {
        if (!firebase_auth_init()) return false;
    }

    std::string url = doc_url("configs");
    std::string resp;
    bool ok = http_request(url.c_str(), "GET", "", resp, g_id_token.c_str());
    if (!ok) { ESP_LOGE(TAG, "Firestore pull failed"); return false; }

    cJSON *root = cJSON_ParseWithLength(resp.c_str(), resp.size());
    if (!root) { ESP_LOGE(TAG, "JSON parse error"); return false; }

    cJSON *fields = cJSON_GetObjectItemCaseSensitive(root, "fields");
    if (!cJSON_IsObject(fields)) { cJSON_Delete(root); return false; }

    auto th = get_double_array(fields, "soilThreshold");
    for (int i=0;i<3 && i<(int)th.size(); ++i) cfg.soilThreshold[i] = (float)th[i];

    auto wm = get_double_array(fields, "waterMs");
    for (int i=0;i<3 && i<(int)wm.size(); ++i) cfg.waterMs[i] = (int32_t)wm[i];

    if (auto b = get_bool_field(fields, "fanManual")) cfg.fanManual = *b;
    if (auto b = get_bool_field(fields, "fanOn")) cfg.fanOn = *b;

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Pulled config: T=[%.2f,%.2f,%.2f] W=[%d,%d,%d] fanManual=%d fanOn=%d",
        cfg.soilThreshold[0], cfg.soilThreshold[1], cfg.soilThreshold[2],
        cfg.waterMs[0], cfg.waterMs[1], cfg.waterMs[2],
        (int)cfg.fanManual, (int)cfg.fanOn);
    return true;
}
