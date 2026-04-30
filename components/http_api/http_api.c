#include "http_api.h"
#include "config.h"

#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "can_reader.h"

static const char *TAG = "HTTP_API";

static const char *DASHBOARD_HTML =
"<!doctype html><html><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>CAN Live</title>"
"<style>"
"body{font-family:Arial,sans-serif;background:#0f1115;color:#e7eaf0;margin:0;padding:16px;}"
".wrap{max-width:1200px;margin:0 auto;} .top{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:12px;}"
"h1{margin:0;font-size:clamp(1.2rem,3.8vw,2rem);line-height:1.2;}"
"button{padding:12px 20px;font-size:1rem;font-weight:700;border:1px solid #3a3f4b;border-radius:10px;background:#1a1f2a;color:#e7eaf0;cursor:pointer;min-width:120px;}"
"button:hover{background:#232a38;} .card{background:#151922;border:1px solid #2b3140;border-radius:10px;padding:12px;}"
"table{width:100%;border-collapse:collapse;} th,td{padding:8px;border-bottom:1px solid #252b38;text-align:left;white-space:nowrap;}"
"th{background:#1c2230;} .mono{font-family:monospace;}"
"@media (max-width:700px){body{padding:10px;} .card{padding:8px;overflow-x:auto;} button{padding:10px 14px;min-width:104px;} th,td{padding:7px;}}"
"</style></head><body>"
"<div class=\"wrap\">"
"<div class=\"top\"><h1 id=\"title\">CAN Signals</h1><button id=\"toggle\">Pause</button></div>"
"<div class=\"card\"><table><thead><tr><th>Signal</th><th>Valeur</th><th>Unite</th></tr></thead><tbody id=\"rows\"></tbody></table></div>"
"<script>"
"let paused=false; let signals=[];"
"const toggleBtn=document.getElementById('toggle');"
"toggleBtn.onclick=function(){paused=!paused;toggleBtn.textContent=paused?'Play':'Pause';};"
"function addRow(name,unit){var tr=document.createElement('tr');var td1=document.createElement('td');var td2=document.createElement('td');var td3=document.createElement('td');"
"td1.textContent=name;td2.id='v_'+name;td3.textContent=unit||'';tr.appendChild(td1);tr.appendChild(td2);tr.appendChild(td3);document.getElementById('rows').appendChild(tr);}"
"async function init(){"
"var m=await (await fetch('/api/meta')).json();"
"document.getElementById('title').textContent='CAN Signals ('+m.dbc_name+')';"
"signals=m.signals||[];"
"for(var i=0;i<signals.length;i++){addRow(signals[i].name,signals[i].unit);}"
"addRow('ts_ms','ms');document.getElementById('v_ts_ms').className='mono';"
"}"
"async function tick(){if(paused)return;"
"try{var j=await (await fetch('/api/can/latest')).json();"
"var ts=document.getElementById('v_ts_ms'); if(ts){ts.textContent=j.ts_ms;}"
"var vals=j.values||{};"
"for(var i=0;i<signals.length;i++){var k=signals[i].name;var el=document.getElementById('v_'+k);if(el&&vals[k]!==undefined){el.textContent=vals[k];}}"
"}catch(e){}}"
"(async function(){await init(); setInterval(tick,500); tick();})();"
"</script></div></body></html>";

static esp_err_t dashboard_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, DASHBOARD_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t health_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json alloc failed");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddStringToObject(root, "service", "mod_can_logger");

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json encode failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    cJSON_free(payload);
    return err;
}

static esp_err_t meta_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json alloc failed");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "dbc_name", can_reader_get_dbc_name());
    cJSON *arr = cJSON_AddArrayToObject(root, "signals");
    size_t n = can_reader_get_signal_count();
    can_signal_value_t *tmp = calloc(n, sizeof(can_signal_value_t));
    if (tmp) {
        can_reader_copy_signals(tmp, n, NULL);
        for (size_t i = 0; i < n; i++) {
            cJSON *sig = cJSON_CreateObject();
            if (!sig) {
                continue;
            }
            cJSON_AddStringToObject(sig, "name", tmp[i].name ? tmp[i].name : "");
            cJSON_AddStringToObject(sig, "unit", tmp[i].unit ? tmp[i].unit : "");
            cJSON_AddItemToArray(arr, sig);
        }
        free(tmp);
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json encode failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    cJSON_free(payload);
    return err;
}

static esp_err_t can_latest_handler(httpd_req_t *req)
{
    size_t n = can_reader_get_signal_count();
    can_signal_value_t *vals = calloc(n, sizeof(can_signal_value_t));
    if (!vals) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "alloc failed");
        return ESP_FAIL;
    }

    uint64_t ts_ms = 0;
    can_reader_copy_signals(vals, n, &ts_ms);

    cJSON *root = cJSON_CreateObject();
    cJSON *obj_vals = cJSON_CreateObject();
    cJSON *obj_valid = cJSON_CreateObject();
    if (!root || !obj_vals || !obj_valid) {
        free(vals);
        cJSON_Delete(root);
        cJSON_Delete(obj_vals);
        cJSON_Delete(obj_valid);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json alloc failed");
        return ESP_FAIL;
    }

    cJSON_AddNumberToObject(root, "ts_ms", (double)ts_ms);
    cJSON_AddItemToObject(root, "values", obj_vals);
    cJSON_AddItemToObject(root, "valid", obj_valid);

    for (size_t i = 0; i < n; i++) {
        cJSON_AddNumberToObject(obj_vals, vals[i].name, vals[i].value);
        cJSON_AddBoolToObject(obj_valid, vals[i].name, vals[i].valid);
    }

    free(vals);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json encode failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    cJSON_free(payload);
    return err;
}

void http_api_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = API_HTTP_PORT;
    config.max_open_sockets = 5;
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    static const httpd_uri_t dashboard_uri = { .uri = "/", .method = HTTP_GET, .handler = dashboard_handler, .user_ctx = NULL };
    static const httpd_uri_t health_uri = { .uri = "/api/health", .method = HTTP_GET, .handler = health_handler, .user_ctx = NULL };
    static const httpd_uri_t meta_uri = { .uri = "/api/meta", .method = HTTP_GET, .handler = meta_handler, .user_ctx = NULL };
    static const httpd_uri_t latest_uri = { .uri = "/api/can/latest", .method = HTTP_GET, .handler = can_latest_handler, .user_ctx = NULL };

    httpd_register_uri_handler(server, &dashboard_uri);
    httpd_register_uri_handler(server, &health_uri);
    httpd_register_uri_handler(server, &meta_uri);
    httpd_register_uri_handler(server, &latest_uri);

    ESP_LOGI(TAG, "HTTP API started: GET /, /api/health, /api/meta, /api/can/latest");
}
