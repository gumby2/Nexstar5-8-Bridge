#!/usr/bin/env python3
"""Extract the existing streamed Web UI and emit a gzip/PROGMEM asset."""

import ast
import gzip
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "Nexstar_Protocol_Converter_v7.03.ino"
OUTPUT = ROOT / "web_ui_asset.h"


def decode_cpp_string(raw: str) -> str:
    return ast.literal_eval('"' + raw + '"')


def extract_page() -> bytes:
    source = SOURCE.read_text(encoding="utf-8")
    start = source.index("void sendWebPage()")
    end = source.index("void handleLogsPage()", start)
    block = source[start:end]
    chunks = []
    for line in block.splitlines():
        if "server.sendContent(F(\"" in line:
            match = re.search(r'server\.sendContent\(F\("((?:\\.|[^"\\])*)"\)\);', line)
            if not match:
                raise RuntimeError(f"Unable to parse UI chunk: {line[:100]}")
            chunks.append(decode_cpp_string(match.group(1)))
        elif "server.sendContent(FW_NAME);" in line:
            chunks.append(re.search(r'FW_NAME\s*=\s*"([^"]+)"', source).group(1))
        elif "server.sendContent(FW_VERSION);" in line:
            chunks.append(re.search(r'FW_VERSION\s*=\s*"([^"]+)"', source).group(1))
        elif "server.sendContent(htmlEscape(staPass));" in line:
            # Do not embed a saved Wi-Fi password in the static asset. The
            # settings API still accepts updates and the field stays editable.
            continue
    page = "".join(chunks)
    # The dashboard now consumes the main updateNow() snapshot. The original
    # interval would remain registered even after the function was replaced.
    page = page.replace("setInterval(refreshProtocolDash,8000);", "")
    page = page.replace("setInterval(updateNow,8000+refreshJitter)", "setInterval(updateNow,12000+refreshJitter)")
    page = page.replace("setInterval(refreshLogs,5000+refreshJitter)", "setInterval(refreshLogs,10000+refreshJitter)")
    page = page.replace(
        "</select></div><div class='formrow'><label>Sort</label>",
        "</select><button id='catLoadBtn' style='display:none' onclick='loadBSC5FromButton()'>Load Catalog</button></div><div class='formrow'><label>Sort</label>",
        1
    )
    # Keep the large BSC5 catalog across page reloads.  FW_VERSION is used as
    # the cache generation so a firmware release invalidates older catalog
    # data without changing the public endpoint.
    page = page.replace(
        "async function loadBSC5(){if(catBSCLoaded)return true;if(catBSCLoading)return false;catBSCLoading=true;setText('bscCount','loading...');try{let t=await getText('/bsc5_data');let a=catDecodeBscText(t);catBSC.splice(0,catBSC.length,...a);catBSCLoaded=true;setText('bscCount',String(catBSC.length));return true}catch(e){setText('bscCount','load failed');return false}finally{catBSCLoading=false}}",
        "async function loadBSC5(){if(catBSCLoaded)return true;if(catBSCLoading)return false;catBSCLoading=true;setText('bscCount','loading...');try{let t=localStorage.getItem('nsc_bsc5_data'),v=localStorage.getItem('nsc_bsc5_version');if(!t||v!==FW_VERSION){t=await getText('/bsc5_data');try{localStorage.setItem('nsc_bsc5_data',t);localStorage.setItem('nsc_bsc5_version',FW_VERSION)}catch(e){}}let a=catDecodeBscText(t);catBSC.splice(0,catBSC.length,...a);catBSCLoaded=true;setText('bscCount',String(catBSC.length));return true}catch(e){setText('bscCount','load failed');return false}finally{catBSCLoading=false}}"
    )
    page = page.replace(
        "function catGroupSelected(){let s=id('cat_search');if(s&&s.value)s.value='';let g=id('cat_group');if(g&&g.value==='bsc'&&!catBSCLoaded)loadBSC5().then(()=>catPopulate());catPopulate()}",
        "function updateCatLoadButton(){let b=id('catLoadBtn'),g=id('cat_group');if(!b)return;b.style.display=(g&&g.value==='bsc'&&!catBSCLoaded)?'inline-block':'none';b.disabled=!!catBSCLoading}async function loadBSC5FromButton(){if(catBSCLoaded)return true;let b=id('catLoadBtn');if(b)b.disabled=true;let ok=await loadBSC5();updateCatLoadButton();catPopulate();identifyPointingNow();return ok}function catGroupSelected(){let s=id('cat_search');if(s&&s.value)s.value='';updateCatLoadButton();catPopulate()}",
    )
    page = page.replace(
        "function catBaseArrayForGroup(v){let a=[];if(v==='solar')a=catSolar.map(x=>({id:x,n:x,src:'solar'}));else if(v==='stars')a=catStars.map(x=>Object.assign({src:'star'},x));else if(v==='bsc'){if(!catBSCLoaded&&!catBSCLoading)loadBSC5().then(()=>{identifyPointingNow();catPopulate()});a=catBSC.map(x=>Object.assign({src:'bsc'},x));}",
        "function catBaseArrayForGroup(v){let a=[];if(v==='solar')a=catSolar.map(x=>({id:x,n:x,src:'solar'}));else if(v==='stars')a=catStars.map(x=>Object.assign({src:'star'},x));else if(v==='bsc'){a=catBSC.map(x=>Object.assign({src:'bsc'},x));}"
    )
    page = page.replace(
        "if(!catBSCLoaded&&!catBSCLoading){loadBSC5().then(()=>{lastPointingIdKey='';identifyPointingNow();catPopulate()})}",
        "if(catBSCLoaded){}"
    )
    page = page.replace(
        "if(typeof catPopulate==='function')catPopulate();},250);",
        "if(typeof updateCatLoadButton==='function')updateCatLoadButton();if(typeof catPopulate==='function')catPopulate();},250);"
    )
    return page.encode("utf-8")


def emit_header(data: bytes) -> None:
    compressed = gzip.compress(data, compresslevel=9, mtime=0)
    lines = []
    for offset in range(0, len(compressed), 16):
        lines.append("  " + ", ".join(f"0x{value:02x}" for value in compressed[offset:offset + 16]) + ",")
    header = """#pragma once

#include <Arduino.h>

// Generated from Nexstar_Protocol_Converter_v7.03.ino.
// Do not edit manually; rerun tools_generate_web_asset.py.
static const uint8_t WEB_UI_GZIP[] PROGMEM = {{
{bytes}
}};
static const size_t WEB_UI_GZIP_SIZE = sizeof(WEB_UI_GZIP);
""".format(bytes="\n".join(lines))
    OUTPUT.write_text(header, encoding="utf-8", newline="\n")
    print(f"source_bytes={len(data)} gzip_bytes={len(compressed)} output={OUTPUT}")


if __name__ == "__main__":
    emit_header(extract_page())
