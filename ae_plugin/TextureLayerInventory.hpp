#pragma once
#include "TransformNullUI.hpp"
#include <algorithm>
#include <limits>
#include <new>
#include <set>
#include <string_view>

namespace starfield::adapter {
inline constexpr std::size_t kTextureInventoryMaxBytes=8'400'000;
inline std::string texture_layer_inventory_script(A_long comp_id,AEGP_LayerIDVal owner_id) {
    // IDs alone enter the script; names are returned as UTF16 hex, never code.
    return "(function(){var cid="+std::to_string(comp_id)+",oid="+std::to_string(owner_id)+
        ";if(!app.project||app.project.numItems>65536)throw Error('Texture project inventory unavailable');"
        "var c=null;for(var i=1;i<=app.project.numItems;i++){var item=app.project.item(i);"
        "if(item.id===cid){if(!(item instanceof CompItem))throw Error('Texture owner is not a composition');c=item;break;}}"
        "if(!c||c.numLayers>4096)throw Error('Texture owner composition unavailable');"
        "var r='SF_TEXTURE_LAYERS_V1|'+cid+'|'+oid+'\\n';"
        "for(var j=1;j<=c.numLayers;j++){var l=c.layer(j);if(l.id===oid||l.nullLayer||!l.source||"
        "(!(l.source instanceof CompItem)&&!l.hasVideo))continue;"
        "var n=String(l.name),end=Math.min(n.length,512);"
        "if(end<n.length&&n.charCodeAt(end-1)>=55296&&n.charCodeAt(end-1)<=56319)end--;"
        "r+=l.id+'|';for(var k=0;k<end;k++){var h=n.charCodeAt(k).toString(16);r+=('0000'+h).slice(-4);}r+='\\n';}return r;}())";
}
inline PF_Err parse_texture_layer_inventory(std::string_view text,A_long comp_id,
    AEGP_LayerIDVal owner_id,std::vector<TransformLayerChoice>& choices) noexcept try {
    if(text.size()>kTextureInventoryMaxBytes || comp_id<=0 || owner_id<=0)return PF_Err_BAD_CALLBACK_PARAM;
    const auto prefix="SF_TEXTURE_LAYERS_V1|"+std::to_string(comp_id)+"|"+std::to_string(owner_id)+"\n";
    if(!text.starts_with(prefix))return PF_Err_BAD_CALLBACK_PARAM;
    std::vector<TransformLayerChoice> parsed{{0,u"None"}};std::set<AEGP_LayerIDVal> ids;
    text.remove_prefix(prefix.size());
    while(!text.empty()) {
        const auto newline=text.find('\n');if(newline==std::string_view::npos || parsed.size()>4096)return PF_Err_BAD_CALLBACK_PARAM;
        const auto row=text.substr(0,newline);const auto bar=row.find('|');
        if(bar==std::string_view::npos || !bar || bar>10)return PF_Err_BAD_CALLBACK_PARAM;
        std::uint64_t id=0;
        for(char c:row.substr(0,bar)){if(c<'0'||c>'9')return PF_Err_BAD_CALLBACK_PARAM;id=id*10+std::uint64_t(c-'0');}
        if(!id || id>std::uint64_t((std::numeric_limits<AEGP_LayerIDVal>::max)()) || id==std::uint64_t(owner_id))return PF_Err_BAD_CALLBACK_PARAM;
        const auto value=static_cast<AEGP_LayerIDVal>(id);if(!ids.insert(value).second)return PF_Err_BAD_CALLBACK_PARAM;
        const auto hex=row.substr(bar+1);if(hex.size()>2048 || hex.size()%4)return PF_Err_BAD_CALLBACK_PARAM;
        std::u16string name;name.reserve(hex.size()/4);
        for(std::size_t at=0;at<hex.size();at+=4) {
            unsigned unit=0;for(unsigned j=0;j<4;++j){const char c=hex[at+j];
                const unsigned digit=c>='0'&&c<='9'?unsigned(c-'0'):c>='a'&&c<='f'?unsigned(c-'a'+10):16;
                if(digit>15)return PF_Err_BAD_CALLBACK_PARAM;unit=unit*16+digit;}
            if(!unit)return PF_Err_BAD_CALLBACK_PARAM;name+=static_cast<char16_t>(unit);
        }
        for(std::size_t at=0;at<name.size();++at) {
            if(name[at]>=0xd800 && name[at]<=0xdbff) {
                if(++at==name.size() || name[at]<0xdc00 || name[at]>0xdfff)return PF_Err_BAD_CALLBACK_PARAM;
            } else if(name[at]>=0xdc00 && name[at]<=0xdfff)return PF_Err_BAD_CALLBACK_PARAM;
        }
        parsed.push_back({value,std::move(name)});text.remove_prefix(newline+1);
    }
    choices=std::move(parsed);return PF_Err_NONE;
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
