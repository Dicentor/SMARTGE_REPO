```cpp
cJSON *ts = cJSON_CreateObject(); cJSON_AddStringToObject(ts, "timestampValue", iso);
cJSON_AddItemToObject(fields, "timestamp", ts);


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
cJSON* arr = cJSON_GetObjectItemCaseSensitive(fields, name);
if (!arr) return out;
cJSON* values = cJSON_GetObjectItemCaseSensitive(arr, "values");
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