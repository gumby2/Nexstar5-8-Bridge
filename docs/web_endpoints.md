# Web endpoint reference

This is the operator-facing route inventory for the v6.99 Web UI. The route
registration in `firmware/Nexstar_Protocol_Converter/Nexstar_Protocol_Converter.ino`
remains the source of truth; this document groups the routes by purpose.

## Pages and status

| Route | Purpose |
| --- | --- |
| `/` | Main Web UI. |
| `/status` | JSON status used by the live UI and external checks. |
| `/http_health` | Lightweight HTTP health page. |
| `/btstatus` | Bluetooth status page/data. |
| `/bsc5_data` | BSC5 catalog data endpoint. |
| `/status_text` | Human-readable status text. |
| `/sys_text` | Human-readable system-health text. |
| `/logs` | Current diagnostic log text. |
| `/mount_test` | Mount test page and controls. |

## Configuration and maintenance actions

These routes use query parameters supplied by the corresponding Setup page.
They are intentionally listed as actions because they can change persistent
settings, start services, or initiate mount activity.

| Route | Purpose |
| --- | --- |
| `/setwifi`, `/clearwifi` | Save or clear Wi-Fi settings. |
| `/setap` | Save access-point and service-port settings. |
| `/setmode` | Change operating mode. |
| `/reboot` | Reboot the Wi-Fi/device services. |
| `/setbtpoll` | Change Bluetooth polling behavior. |
| `/setup_action` | Run named Setup actions, including catalog/setup operations. |
| `/setlog`, `/clearlogs`, `/clearlogalert` | Configure or clear diagnostics. |
| `/setaltlimits`, `/setdeclimits` | Save altitude/declination limits. |
| `/setrates`, `/setwebrate`, `/resetrates` | Configure or reset motion rates. |
| `/set_site_time` | Save site/time values. |
| `/set_manual_site_time` | Apply browser-provided manual time/site data. |
| `/set_device_site_time` | Apply device-derived time/site data. |
| `/setntp`, `/syncntp` | Configure or trigger NTP synchronization. |
| `/fetch_ip_location`, `/use_ip_location` | Fetch or apply IP-based location. |
| `/clearsettings` | Clear saved settings. |

## Mount controls

| Route | Purpose |
| --- | --- |
| `/webnudge` | Send a bounded directional nudge. |
| `/getradecweb` | Read or display RA/Dec. |
| `/getaltazweb` | Read or display Alt/Az. |
| `/webgoto_radec` | Queue an RA/Dec GOTO. |
| `/webgoto_altaz` | Queue an Alt/Az GOTO. |

Mount actions still obey the single-command NexStar transaction lock. A Web
request is not permission to bypass an active GOTO, poll position during a
GOTO, or write directly to the mount UART.

## HTTPS setup surface

When the HTTPS compatibility service is enabled, the setup server exposes:

| Route | Purpose |
| --- | --- |
| `/` | HTTPS setup page. |
| `/set` | HTTPS setup action endpoint. |
| `/status` | HTTPS status JSON. |
| `/timeloc_status` | HTTPS time/location status. |
| `/start_https` | Start HTTPS setup flow. |
| `/https_setup` | Redirect/setup entry point. |

The HTTPS path uses the ESP-IDF HTTPS server and retains the existing
certificate behavior. It is separate from the regular HTTP compatibility
server and should not be treated as a drop-in replacement for every HTTP
route.

## Alpaca management routes

The Alpaca surface is rooted at `/management` and `/api`. The main discovery
and description routes are:

| Route | Purpose |
| --- | --- |
| `/management/apiversions` | Report supported Alpaca API versions. |
| `/management/v1/description` | Report device/service description. |
| `/management/v1/configureddevices` | Report configured devices. |
| `/api/v1/telescope/0/...` | Alpaca telescope device API. |
| `/api/v1/telescope/-1/...` | Compatibility path for the alternate telescope ID. |

The wildcard telescope suffixes are implemented in the firmware handler and
are not reproduced as a finite list here.
